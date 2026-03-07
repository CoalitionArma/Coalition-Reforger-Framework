//=============================================================================================================================================================================================================================================================================================================================================================
// Supporting classes for efficient slot management
//=============================================================================================================================================================================================================================================================================================================================================================

// Statistics for a faction's slots
class CRF_FactionSlotStats
{
	int m_iTotalSlots = 0;
	int m_iTakenSlots = 0;
	int m_iLockedSlots = 0;
	int m_iDeadSlots = 0;
}

// Typed invoker classes for specific slot events
// Note: Using ScriptInvoker directly since Enforce doesn't support typed func pointers well
class ScriptInvoker_SlotPlayerChanged : ScriptInvoker {}
class ScriptInvoker_SlotLockedChanged : ScriptInvoker {}
class ScriptInvoker_SlotDeathChanged : ScriptInvoker {}
class ScriptInvoker_FactionStatsChanged : ScriptInvoker {}

//=============================================================================================================================================================================================================================================================================================================================================================
// Main SlottingManager class
//=============================================================================================================================================================================================================================================================================================================================================================

class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Slot data storage - uses ID-based system where IDs are generated in AddSlot
	protected ref map<int, ref CRF_SlotDataContainer> m_mSlotsMap = new map<int, ref CRF_SlotDataContainer>;
	
	// Indexed lookups for O(1) access
	protected ref map<int, ref array<int>> m_mGroupToSlots = new map<int, ref array<int>>;
	protected ref map<string, ref array<int>> m_mFactionToSlots = new map<string, ref array<int>>;
	protected ref map<int, int> m_mPlayerToSlot = new map<int, int>;
	
	// Cached faction statistics
	protected ref map<string, ref CRF_FactionSlotStats> m_mFactionStats = new map<string, ref CRF_FactionSlotStats>;
	
	// Batch update system
	protected ref array<int> m_aPendingSlotUpdates = new array<int>;
	protected bool m_bHasPendingUpdates = false;
	protected bool m_bFlushTimerActive = false;  // Track if timer is running
	
	// Latest Slot ID used
	protected int m_iLatestSlotID;
	
	// Invokers for slot updates - more granular events
	protected ref ScriptInvoker m_OnSlottingUpdate = new ScriptInvoker;
	protected ref ScriptInvoker_SlotPlayerChanged m_OnSlotPlayerChanged;
	protected ref ScriptInvoker_SlotLockedChanged m_OnSlotLockedChanged;
	protected ref ScriptInvoker_SlotDeathChanged m_OnSlotDeathChanged;
	protected ref ScriptInvoker_FactionStatsChanged m_OnFactionStatsChanged;
	
	// References to other managers
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	
	// Resource caching for optimized spawning
	protected ref map<ResourceName, Resource> m_mCachedResources = new map<ResourceName, Resource>();
	
	// Mass initialization flag for optimizing collision checks
	protected bool m_bMassInitializationInProgress = false;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		
		// Initialize faction stats
		InitializeFactionStats();
		
		// Need to call next frame due to race conditions if the faction manager hasn't fully initilized.
		GetGame().GetCallqueue().Call(InitilizeSlots);
		
		// Batch timer is created on-demand in MarkSlotDirty() instead of running constantly
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize faction statistics tracking
	protected void InitializeFactionStats()
	{
		array<string> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string factionKey : factionKeys)
		{
			if (!m_mFactionStats.Contains(factionKey))
				m_mFactionStats.Insert(factionKey, new CRF_FactionSlotStats());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitilizeSlots()
	{
		InitilizeSlotsForFaction("BLUFOR", m_Gamemode.m_BluforSlots);
		InitilizeSlotsForFaction("OPFOR", m_Gamemode.m_OpforSlots);
		InitilizeSlotsForFaction("INDFOR", m_Gamemode.m_IndforSlots);
		InitilizeSlotsForFaction("CIV", m_Gamemode.m_CivSlots);
		
		// Build indices after all slots are initialized
		RebuildAllIndices();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SLOTTING UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentCharacter(charId);
			m_RplBroadcastManager.UpdateSlotCharacterDelta(slotId, charId);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotRole(int slotId, CRF_EGearRole role)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotRole(role);
			m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId group)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(group);
			m_RplBroadcastManager.UpdateSlotGroupDelta(slotId, group);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId = -1)
	{	
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			// Track old value for delta updates and stats
			int oldPlayerId = slotData.GetSlotCurrentPlayerId();
			string factionKey = slotData.GetSlotFactionKey();
			
			// Update the slot data
			slotData.SetSlotCurrentPlayerId(playerId);
			
			// Update player-to-slot index
			if (oldPlayerId > 0)
				m_mPlayerToSlot.Remove(oldPlayerId);
			if (playerId > 0)
				m_mPlayerToSlot.Set(playerId, slotId);
			
			// Update faction statistics incrementally
			CRF_FactionSlotStats stats = m_mFactionStats.Get(factionKey);
			if (stats)
			{
				if (oldPlayerId > 0)
					stats.m_iTakenSlots--;
				if (playerId > 0)
					stats.m_iTakenSlots++;
				
				// Notify faction stats listeners
				if (m_OnFactionStatsChanged)
					m_OnFactionStatsChanged.Invoke(factionKey, stats);
			}
			
			// Mark for batched update instead of immediate RPC
			MarkSlotDirty(slotId);
			
			// Fire specific event for player change
			if (m_OnSlotPlayerChanged)
				m_OnSlotPlayerChanged.Invoke(slotId, oldPlayerId, playerId);
			
			// Legacy broad update invoker
			m_OnSlottingUpdate.Invoke();
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool isLocked = false)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			// Track old state for stats
			bool wasLocked = slotData.GetIsLockedSlot();
			string factionKey = slotData.GetSlotFactionKey();
			
			slotData.SetIsLockedSlot(isLocked);
			if (isLocked)
				slotData.SetSlotCurrentPlayerId(0);
			
			// Update faction statistics
			CRF_FactionSlotStats stats = m_mFactionStats.Get(factionKey);
			if (stats)
			{
				if (wasLocked && !isLocked)
					stats.m_iLockedSlots--;
				else if (!wasLocked && isLocked)
					stats.m_iLockedSlots++;
				
				if (m_OnFactionStatsChanged)
					m_OnFactionStatsChanged.Invoke(factionKey, stats);
			}
			
			// Mark for batched update
			MarkSlotDirty(slotId);
			
			// Fire specific event
			if (m_OnSlotLockedChanged)
				m_OnSlotLockedChanged.Invoke(slotId, isLocked);
			
			m_OnSlottingUpdate.Invoke();
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			// Track old state for stats
			bool wasDead = slotData.GetIsDeadSlot();
			string factionKey = slotData.GetSlotFactionKey();
			
			slotData.SetIsDeadSlot(input);
			
			// Update faction statistics
			CRF_FactionSlotStats stats = m_mFactionStats.Get(factionKey);
			if (stats)
			{
				if (wasDead && !input)
					stats.m_iDeadSlots--;
				else if (!wasDead && input)
					stats.m_iDeadSlots++;
				
				if (m_OnFactionStatsChanged)
					m_OnFactionStatsChanged.Invoke(factionKey, stats);
			}
			
			// Mark for batched update
			MarkSlotDirty(slotId);
			
			// Fire specific event
			if (m_OnSlotDeathChanged)
				m_OnSlotDeathChanged.Invoke(slotId, input);
			
			m_OnSlottingUpdate.Invoke();
		};
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISC GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		return m_OnSlottingUpdate;
	}
	
	//------------------------------------------------------------------------------------------------
	// Specific event invokers
	ScriptInvoker_SlotPlayerChanged GetOnSlotPlayerChanged()
	{
		if (!m_OnSlotPlayerChanged)
			m_OnSlotPlayerChanged = new ScriptInvoker_SlotPlayerChanged();
		return m_OnSlotPlayerChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker_SlotLockedChanged GetOnSlotLockedChanged()
	{
		if (!m_OnSlotLockedChanged)
			m_OnSlotLockedChanged = new ScriptInvoker_SlotLockedChanged();
		return m_OnSlotLockedChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker_SlotDeathChanged GetOnSlotDeathChanged()
	{
		if (!m_OnSlotDeathChanged)
			m_OnSlotDeathChanged = new ScriptInvoker_SlotDeathChanged();
		return m_OnSlotDeathChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker_FactionStatsChanged GetOnFactionStatsChanged()
	{
		if (!m_OnFactionStatsChanged)
			m_OnFactionStatsChanged = new ScriptInvoker_FactionStatsChanged();
		return m_OnFactionStatsChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotData(int slotId)
	{
		return m_mSlotsMap.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	map<int, ref CRF_SlotDataContainer> GetSlotMap()
	{
		return m_mSlotsMap;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIds()
	{
		array<int> slotIds = {};
		
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			slotIds.Insert(slotId);
		}
		
		return slotIds;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get faction statistics - O(1) instead of O(n)
	CRF_FactionSlotStats GetFactionStats(string factionKey)
	{
		return m_mFactionStats.Get(factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get slots for specific faction - O(1) lookup
	array<int> GetSlotsForFaction(string factionKey)
	{
		array<int> slots = m_mFactionToSlots.Get(factionKey);
		if (!slots)
			return new array<int>();
		return slots;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get slot ID for player - O(1) lookup
	int GetSlotForPlayer(int playerId)
	{
		if (m_mPlayerToSlot.Contains(playerId))
			return m_mPlayerToSlot.Get(playerId);
		return -1;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 Index Management
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// Rebuilds all indices - should only be called during initialization or after mass changes
	protected void RebuildAllIndices()
	{
		// Clear existing indices
		m_mGroupToSlots.Clear();
		m_mFactionToSlots.Clear();
		m_mPlayerToSlot.Clear();
		
		// Reset faction stats
		foreach (string key, CRF_FactionSlotStats stats : m_mFactionStats)
		{
			stats.m_iTotalSlots = 0;
			stats.m_iTakenSlots = 0;
			stats.m_iLockedSlots = 0;
			stats.m_iDeadSlots = 0;
		}
		
		// Build indices from current slot data
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			// Add to faction index
			string factionKey = slotData.GetSlotFactionKey();
			if (!m_mFactionToSlots.Contains(factionKey))
				m_mFactionToSlots.Insert(factionKey, new array<int>());
			m_mFactionToSlots.Get(factionKey).Insert(slotId);
			
			// Add to group index
			int groupId = slotData.GetSlotCurrentGroup();
			if (groupId != RplId.Invalid())
			{
				if (!m_mGroupToSlots.Contains(groupId))
					m_mGroupToSlots.Insert(groupId, new array<int>());
				m_mGroupToSlots.Get(groupId).Insert(slotId);
			}
			
			// Add to player index and update stats
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0)
				m_mPlayerToSlot.Set(playerId, slotId);
			
			// Update faction stats
			CRF_FactionSlotStats stats = m_mFactionStats.Get(factionKey);
			if (stats)
			{
				if (!slotData.GetIsLockedSlot())
					stats.m_iTotalSlots++;
				if (playerId > 0)
					stats.m_iTakenSlots++;
				if (slotData.GetIsLockedSlot())
					stats.m_iLockedSlots++;
				if (slotData.GetIsDeadSlot())
					stats.m_iDeadSlots++;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Mark slot as needing update for batch processing
	// Creates timer on-demand instead of running constantly
	protected void MarkSlotDirty(int slotId)
	{
		if (!m_aPendingSlotUpdates.Contains(slotId))
		{
			m_aPendingSlotUpdates.Insert(slotId);
			m_bHasPendingUpdates = true;
			
			// Only start timer if not already running
			if (!m_bFlushTimerActive)
			{
				m_bFlushTimerActive = true;
				GetGame().GetCallqueue().CallLater(FlushPendingUpdates, 100, false);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Flush all pending slot updates in a single batch RPC
	// Timer automatically stops after flush (on-demand pattern)
	protected void FlushPendingUpdates()
	{
		// Reset timer flag
		m_bFlushTimerActive = false;
		
		if (!m_bHasPendingUpdates || m_aPendingSlotUpdates.IsEmpty())
			return;
		
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		// Send batch update to clients
		SendBatchedSlotUpdate(m_aPendingSlotUpdates);
		
		// Clear pending updates
		m_aPendingSlotUpdates.Clear();
		m_bHasPendingUpdates = false;
	}
	
	//------------------------------------------------------------------------------------------------
	// Send multiple slot updates using the RplBroadcastManager's batching system
	// The RplBroadcastManager will automatically queue these updates and send them
	// in an optimized batch when its flush timer triggers
	protected void SendBatchedSlotUpdate(array<int> slotIds)
	{
		foreach (int slotId : slotIds)
		{
			CRF_SlotDataContainer slotData = GetSlotData(slotId);
			if (!slotData)
				continue;
			
			// Queue all properties that might have changed through the broadcast manager
			// The broadcast manager handles batching and network serialization
			m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, slotData.GetSlotCurrentPlayerId());
			m_RplBroadcastManager.UpdateSlotLockedDelta(slotId, slotData.GetIsLockedSlot());
			m_RplBroadcastManager.UpdateSlotDeathDelta(slotId, slotData.GetIsDeadSlot());
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GROUP GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	array<SCR_AIGroup> GetAllGroups(FactionKey factionKey = "")
	{
		map<int, SCR_AIGroup> groupMap = new map<int, SCR_AIGroup>;
		array<SCR_AIGroup> outputArray = {};
		array<int> sortingArray = {};
		
		// Pre-allocate based on slot count (worst case: every slot has unique group)
		int slotCount = m_mSlotsMap.Count();
		sortingArray.Reserve(slotCount);
		outputArray.Reserve(slotCount);
		
		// Collect all relevant groups
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (!IsValidGroupInSlot(slotData))
				continue;
			
			// Check faction filter if provided
			if (!factionKey.IsEmpty() && slotData.GetSlotFactionKey() != factionKey)
				continue;
			
			SCR_AIGroup group = CRF_EntityHelper.GetGroupFromRplId(slotData.GetSlotCurrentGroup());
			if (!group)
				continue;
				
			if (!groupMap.Contains(group.GetGroupID()))
				groupMap.Set(group.GetGroupID(), group);
		}
		
		// Sort groups by ID
		foreach (int groupId, SCR_AIGroup group : groupMap)
			sortingArray.Insert(groupId);
		
		sortingArray.Sort(false);
		
		// Create output array in sorted order
		foreach (int groupId : sortingArray)
			outputArray.Insert(groupMap.Get(groupId));
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to check if group in slot is valid
	protected bool IsValidGroupInSlot(CRF_SlotDataContainer slotData)
	{
		if (!slotData)
			return false;
			
		RplId groupId = slotData.GetSlotCurrentGroup();
		if (groupId == RplId.Invalid())
			return false;
			
		if (!Replication.FindItem(groupId))
			return false;
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		// Use index for O(1) lookup instead of O(n) iteration
		array<int> slots = m_mGroupToSlots.Get(rplId);
		if (slots)
		{
			array<int> sortedSlots = {};
			sortedSlots.Copy(slots);
			sortedSlots.Sort();
			return sortedSlots;
		}
		
		// Fallback to old method if index not available
		array<int> outputArray = {};
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() == rplId)
				outputArray.Insert(slotID);
		}
		outputArray.Sort();
		return outputArray;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHRARACTER GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentCharacter() == rplId)
				return slotData;
		}
		
		return null;
	}

	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		if (!entity)
			return -1;
			
		RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rplComp)
			return -1;
			
		RplId rplId = rplComp.Id();
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentCharacter() == rplId)
				return slotID;
		}
		
		return -1;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId groupId = slotData.GetSlotCurrentGroup();
		return CRF_EntityHelper.GetGroupFromRplId(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_PlayerCharacter GetPlayerSlotCharacter(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		return CRF_PlayerCharacter.Cast(CRF_EntityHelper.GetCharacterFromRplId(charId));
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId, bool returnNull = false)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		
		if (!slotData && !returnNull)
			return factionManager.GetFactionByKey("CIV");
		else if (!slotData)
			return null;
			
		FactionKey factionKey = slotData.GetSlotFactionKey();
		
		if (factionKey.IsEmpty() && !returnNull)
			return factionManager.GetFactionByKey("CIV");
		else if (factionKey.IsEmpty())
			return null;
			
		return factionManager.GetFactionByKey(factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return ResourceName.Empty;
			
		return slotData.GetSlotResource();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATE CHECKING
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotFactionKey() == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Count how many players are slotted in a specific faction
	//! This counts SLOTTED players, not spawned players
	//! \param[in] factionKey The faction key to count (e.g. "BLUFOR", "OPFOR", "INDFOR", "CIV")
	//! \return Number of players slotted in that faction
	int GetSlottedPlayerCountByFaction(FactionKey factionKey)
	{
		int count = 0;
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			// Check if slot has a player and matches the faction
			if (slotData.GetSlotCurrentPlayerId() > 0 && slotData.GetSlotFactionKey() == factionKey)
				count++;
		}
		
		return count;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return false;
			
		return slotData.GetIsDeadSlot();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper method to check if a group is empty
	protected bool IsEmptyGroup(SCR_AIGroup group)
	{
		if (!group)
			return true;
			
		RplComponent rplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		if (!rplComp)
			return true;
			
		RplId groupId = rplComp.Id();
		
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() != groupId)
				continue;
				
			if (slotData.GetSlotCurrentPlayerId() > 0)
				return false;
		}
		
		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHARACTER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	CRF_PlayerCharacter SpawnPlayableEntity(int playerId, vector overrideLocation[4])
	{
		int slotId = GetPlayerSlotID(playerId);
		if (slotId < 0)
			return null;
			
		ResourceName resourceName = GetPlayerSlotResource(playerId);
		if (resourceName.IsEmpty())
			return null;
		
		vector playerSlotVector[4];
		CRF_RespawnManager.GetInstance().FindInitalSpawnLocation(GetPlayerSlotFaction(playerId).GetFactionKey(), GetPlayerSlotGroup(playerId), playerSlotVector);

		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		if (overrideLocation[3] != vector.Zero)
		{	
			foreach (int i, vector vec : overrideLocation)
			{
				if (overrideLocation[i] == vector.Zero)
					overrideLocation[i] = playerSlotVector[i];
			}
			
			spawnParams.Transform[3] = overrideLocation[3];
		} else {
			spawnParams.Transform = playerSlotVector;
		}

		spawnParams.Transform[3][1] + spawnParams.Transform[3][1] + 0.5; //Go up 1 incase theres some weird slope, floor issue
		vector surface;
		SCR_TerrainHelper.SnapToGeometry(surface, spawnParams.Transform[3], {}, GetGame().GetWorld());
		spawnParams.Transform[3] = surface;
		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);
		
		GetSafeSpawnTransform(spawnParams.Transform, 12, spawnParams.Transform);
		
		// Spawn the character using cached resource
		Resource resource = GetCachedResource(resourceName);
		if (!resource)
		{
			Print(string.Format("[CRF_SlottingManager] Failed to load resource: %1", resourceName), LogLevel.ERROR);
			return null;
		}
		
		CRF_PlayerCharacter playerCharacter = CRF_PlayerCharacter.Cast(
			GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return null;
		
		// Update character faction
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		facComp.SetAffiliatedFaction(GetPlayerSlotFaction(playerId));
	
		// Update slot data
		RplComponent charRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (charRplComp)
			UpdateSlotCharacter(slotId, charRplComp.Id());
		
		// Route entity assignment through the base game SCR_SpawnRequestComponent pipeline so that
		// all data components (SCR_RespawnSystemComponent, SCR_DataCollectorComponent,
		// SCR_SpawnLockComponent, PreparePlayerEntity_S on all SCR_BaseGameModeComponents, etc.)
		// are properly notified — identical to how the Editor's SpawnEntityResource assigns a
		// player character via SCR_PossessSpawnData.
		SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(
			GetGame().GetPlayerManager().GetPlayerRespawnComponent(playerId)
		);
		
		if (respawnComponent)
		{
			SCR_PossessSpawnData spawnData = SCR_PossessSpawnData.FromEntity(playerCharacter);
			
			// Verify the possess spawn handler exists before attempting RequestSpawn
			// This prevents "GameMode does not support this method of spawning" errors
			// during early initialization when handler components may not be fully registered
			bool canUseRequestSpawn = false;
			
			// Try to get the request component for PossessSpawnData type
			array<GenericComponent> components = {};
			respawnComponent.FindComponents(SCR_SpawnRequestComponent, components);
			
			foreach (GenericComponent comp : components)
			{
				SCR_SpawnRequestComponent requestComp = SCR_SpawnRequestComponent.Cast(comp);
				if (requestComp && requestComp.GetDataType() == SCR_PossessSpawnData && requestComp.GetHandlerComponent())
				{
					canUseRequestSpawn = true;
					break;
				}
			}
			
			if (canUseRequestSpawn)
			{
				if (!respawnComponent.RequestSpawn(spawnData))
					Print(string.Format("[CRF_SlottingManager] WARNING: RequestSpawn failed for player %1, entity %2", playerId, playerCharacter), LogLevel.WARNING);
			}
			else
			{
				// Handler not ready yet - use direct assignment as fallback
				Print(string.Format("[CRF_SlottingManager] INFO: SCR_PossessSpawnHandlerComponent not ready for player %1 — using direct assignment", playerId), LogLevel.NORMAL);
				SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
				
				if (playerController)
					playerController.SetInitialMainEntity(playerCharacter);
			}
		}
		else
		{
			// Fallback: SCR_RespawnComponent not yet available (e.g. very early init), assign directly
			Print(string.Format("[CRF_SlottingManager] WARNING: No SCR_RespawnComponent for player %1 — falling back to SetInitialMainEntity", playerId), LogLevel.WARNING);
			SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			
			if (playerController)
				playerController.SetInitialMainEntity(playerCharacter);
		}
		
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	void GetSafeSpawnTransform(vector baseTransform[4], float radius, out vector trasnformOut[4])
	{
		// Base Enfusion spawn already handles position validation
		// Simply apply a small random offset for player spacing during mass spawns
		vector outTransform[4] = baseTransform;
		
		// Add random offset to prevent exact position overlap
		float angle = Math.RandomFloat01() * Math.PI2;
		float dist = Math.RandomFloat01() * radius;
		vector offset = Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
		
		outTransform[3] = baseTransform[3] + offset;
		
		// Snap to terrain geometry
		vector surface;
		SCR_TerrainHelper.SnapToGeometry(surface, outTransform[3], {}, GetGame().GetWorld());
		if (surface != vector.Zero)
		{
			outTransform[3] = surface;
			SCR_TerrainHelper.OrientToTerrain(outTransform);
		}
		
		trasnformOut = outTransform;
	}

	//------------------------------------------------------------------------------------------------
	void CleanupCharacterFromSlot(CRF_SlotDataContainer slotData)
	{
		if (!slotData)
			return;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		if (charId == RplId.Invalid())
			return;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(charId));
		if (!rplComp)
			return;
			
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(rplComp.GetEntity());
		if (character)
			SCR_EntityHelper.DeleteEntityAndChildren(character);
			
		slotData.SetSlotCurrentCharacter(RplId.Invalid());
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FACTION INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void InitilizeSlotsForFaction(FactionKey factionKey, array <ref CRF_SlottingGroup> factionSlots)
	{
		if (factionKey.IsEmpty() || factionSlots.IsEmpty())
			return;
		
		InitilizeGroupCallsignsForFaction(factionKey, factionSlots);
		
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		
		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
		{	
			CRF_EFlagType flagType = slotGroup.m_FlagType;
			
			if(scrFaction && scrFaction.GetFlagName(0))
			{
				TStringArray flagArray = {};
				scrFaction.GetFlagNames(flagArray);
				if((flagArray.Count() - 1) < flagType)
					flagType = CRF_EFlagType.INFANTRY;
			};
			
			SCR_AIGroup group = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(scrFaction);
			group.SetFaction(scrFaction);
			group.SetGroupFlag(flagType, true);
			group.SetCanDeleteIfNoPlayer(false);
			group.SetDeleteWhenEmpty(false);
			group.SetMaxMembers(16);
			
			foreach(CRF_EGearRole role : slotGroup.m_aSlots)
			{
				CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();
				CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				
				if (!roleConfig || !rolesConfig)
					return;
					
				// Create and configure new slot data
				CRF_SlotDataContainer slotData = new CRF_SlotDataContainer;
				
				// Set group and faction
				RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
				slotData.SetSlotCurrentGroup(groupRplComp.Id());
				slotData.SetSlotFactionKey(factionKey);
				
				// Set resource and character ID
				slotData.SetSlotRole(role);
						
				// Add to slots map
				m_iLatestSlotID++;
				slotData.SetSlotId(m_iLatestSlotID);
				m_mSlotsMap.Set(m_iLatestSlotID, slotData);
				
				// Broadcast new slot to all clients
				m_RplBroadcastManager.UpdateSlotData(slotData);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitilizeGroupCallsignsForFaction(FactionKey factionKey, array <ref CRF_SlottingGroup> factionSlots)
	{
		array<ref SCR_CallsignInfo> callsignArray = new array<ref SCR_CallsignInfo>;
		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
		{
			ref SCR_CallsignInfo callsignInfo = new SCR_CallsignInfo;
			callsignInfo.SetCallsign(slotGroup.m_sCallsign);
			callsignArray.Insert(callsignInfo);
		}
		
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		
		scrFaction.GetCallsignInfo().SetSquadArray(callsignArray);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Assign player to their slotted group
	//! \param[in] playerId ID of the player to assign
	void AssignPlayerToGroup(int playerId)
	{
		SCR_AIGroup group = GetPlayerSlotGroup(playerId);
		if (!group)
			return;
			
		int groupId = group.GetGroupID();
		if (groupId == -1)
			return;
			
		SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
		
		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RequestJoinGroup(groupId);
	}

	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		// Lock all empty slots
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() <= 0)
				UpdateSlotLockedState(slotID, true);
		}
		
		// Process groups
		array<SCR_AIGroup> allGroups = GetAllGroups();
		foreach (SCR_AIGroup group : allGroups)
		{    
			// Skip already private groups
			if (group.IsPrivate())
				continue;
			
			if (IsEmptyGroup(group))
				group.SetPrivate(true);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESOURCE CACHING
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Get a cached resource or load and cache it if not already cached
	//! Reduces repeated Resource.Load() calls during mass spawning
	//! \param[in] resourceName The resource path to load
	//! \return The loaded resource or null if invalid
	Resource GetCachedResource(ResourceName resourceName)
	{
		if (resourceName.IsEmpty())
			return null;
		
		Resource res = m_mCachedResources.Get(resourceName);
		if (!res)
		{
			res = Resource.Load(resourceName);
			if (res)
			{
				m_mCachedResources.Set(resourceName, res);
				Print(string.Format("[CRF_SlottingManager] Cached resource: %1", resourceName), LogLevel.VERBOSE);
			}
		}
		return res;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all cached resources
	//! Call this when unloading mission or changing scenarios
	void ClearResourceCache()
	{
		m_mCachedResources.Clear();
		Print("[CRF_SlottingManager] Resource cache cleared", LogLevel.VERBOSE);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MASS INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Set the mass initialization flag
	//! Used to optimize collision checks during batch player spawning
	//! \param[in] inProgress True when batch spawning is active
	void SetMassInitializationInProgress(bool inProgress)
	{
		m_bMassInitializationInProgress = inProgress;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if mass initialization is currently in progress
	//! \return True if batch spawning is active
	bool IsMassInitializationInProgress()
	{
		return m_bMassInitializationInProgress;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT SIDE REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Update single slot from RPC (called by CRF_RplBroadcastManager)
	//! Only updates if data actually changed to prevent unnecessary UI rebuilds
	void UpdateSlotDataClient(CRF_SlotDataContainer slotData)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		int slotId = slotData.GetSlotId();
		CRF_SlotDataContainer oldSlotData = m_mSlotsMap.Get(slotId);

		if(!oldSlotData)
			m_mSlotsMap.Set(slotId, slotData);
		else
			oldSlotData.DataUpdate(slotData);
				
		// Trigger UI update
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		Print(string.Format("[CRF_SlottingManager] Client received slot %1 update", slotId), LogLevel.VERBOSE);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Remove slot from RPC (called by CRF_RplBroadcastManager)
	void RemoveSlotClient(int slotId)
	{
		if (Replication.IsServer())
			return;
		
		m_mSlotsMap.Remove(slotId);
		
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		Print(string.Format("[CRF_SlottingManager] Client removed slot %1", slotId), LogLevel.VERBOSE);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save slotData
		int slotsCount = m_mSlotsMap.Count();
		writer.WriteInt(slotsCount);
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			slotData.Save(writer);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load slotData
		int slotsCount;
		reader.ReadInt(slotsCount);
		for (int i = 0; i < slotsCount; i++)
		{
			CRF_SlotDataContainer slotData = new CRF_SlotDataContainer();
			slotData.Load(reader);
			UpdateSlotDataClient(slotData);
		}

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_SlottingManager m_sInstance;
	void CRF_SlottingManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		return m_sInstance;
	}
}