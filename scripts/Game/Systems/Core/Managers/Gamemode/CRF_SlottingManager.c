class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
	// Slot data storage - uses ID-based system where IDs are generated in AddSlot
	protected ref map<int, ref CRF_SlotDataContainer> m_mSlotsMap = new map<int, ref CRF_SlotDataContainer>;
	
	// Latest Slot ID used
	protected int m_iLatestSlotID;
	
	// Invoker for slot updates
	protected ref ScriptInvoker m_OnSlottingUpdate;
	
	// References to other managers
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	
	protected static CRF_SlottingManager m_sInstance;
	
	// Resource caching for optimized spawning
	protected ref map<ResourceName, Resource> m_mCachedResources = new map<ResourceName, Resource>();
	
	// Mass initialization flag for optimizing collision checks
	protected bool m_bMassInitializationInProgress = false;
	
	void CRF_SlottingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
		// Initialize ScriptInvoker to avoid null checks - PERFORMANCE OPTIMIZATION
		m_OnSlottingUpdate = new ScriptInvoker();
	}
	
	//------------------------------------------------------------------------------------------------
	// INITIALIZATION
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		
		// Need to call next frame due to race conditions if the faction manager hasn't fully initilized.
		GetGame().GetCallqueue().Call(InitilizeSlots);
	}
	
	protected void InitilizeSlots()
	{
		InitilizeSlotsForFaction("BLUFOR", m_Gamemode.m_BluforSlots);
		InitilizeSlotsForFaction("OPFOR", m_Gamemode.m_OpforSlots);
		InitilizeSlotsForFaction("INDFOR", m_Gamemode.m_IndforSlots);
		InitilizeSlotsForFaction("CIV", m_Gamemode.m_CivSlots);
	}
	
	//------------------------------------------------------------------------------------------------
	// SLOTTING UPDATE METHODS
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
	
	void UpdateSlotRole(int slotId, CRF_EGearRole role)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotRole(role);
			m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		};
	}
	
	void UpdateSlotGroup(int slotId, RplId group)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(group);
			m_RplBroadcastManager.UpdateSlotGroupDelta(slotId, group);
		};
	}
	
	void UpdateSlotPlayerID(int slotId, int playerId = -1)
	{	
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentPlayerId(playerId);
			m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, playerId);
		};
	}
	
	void UpdateSlotLockedState(int slotId, bool isLocked = false)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsLockedSlot(isLocked);
			if (isLocked)
				slotData.SetSlotCurrentPlayerId(0);
			
			m_RplBroadcastManager.UpdateSlotLockedDelta(slotId, isLocked);
		};
	}
	
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsDeadSlot(input);
			m_RplBroadcastManager.UpdateSlotDeathDelta(slotId, input);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		return m_OnSlottingUpdate;
	}
	
	//------------------------------------------------------------------------------------------------
	// SLOT DATA ACCESS METHODS
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
	// Get all slot IDs (useful for JIP sync and iteration)
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
			
			SCR_AIGroup group = GetGroupFromRplId(slotData.GetSlotCurrentGroup());
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
	// Helper method to get group from RplId
	SCR_AIGroup GetGroupFromRplId(RplId groupId)
	{
		if (groupId == RplId.Invalid())
			return null;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(groupId));
		if (!rplComp)
			return null;
			
		return SCR_AIGroup.Cast(rplComp.GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to get character from RplId
	static SCR_ChimeraCharacter GetCharacterFromRplId(RplId charId)
	{
		if (charId == RplId.Invalid())
			return null;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(charId));
		if (!rplComp)
			return null;
			
		return SCR_ChimeraCharacter.Cast(rplComp.GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> outputArray = {};
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() == rplId)
				outputArray.Insert(slotID);
		}
		
		outputArray.Sort();
		
		return outputArray;
	}
	
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
	// PLAYER SLOT METHODS
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
		return GetGroupFromRplId(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter GetPlayerSlotCharacter(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		return GetCharacterFromRplId(charId);
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
	
	//------------------------------------------------------------------------------------------------
	// STATE CHECKING METHODS
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
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return false;
			
		return slotData.GetIsDeadSlot();
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to clean up character from slot
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

	//------------------------------------------------------------------------------------------------
	// GAME MANAGEMENT METHODS
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
	
	//------------------------------------------------------------------------------------------------
	// Helper method to check if a group is empty
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
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter SpawnPlayableEntity(int playerId, vector overrideLocation[4])
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
		
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(
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
		
		// Set playable flag if component exists
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(
			playerCharacter.FindComponent(CRF_PlayableCharacter)
		);
		
		if (playableCharComp)
			playableCharComp.SetIsSlotSpawned();
		
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
	
	//------------------------------------------------------------------------------------------------
	// NEW CLIENT-SIDE METHODS: Receive targeted RPC slot updates
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// Client-side: Update single slot from RPC (called by CRF_RplBroadcastManager)
	// Only updates if data actually changed (prevents unnecessary UI rebuilds)
	//------------------------------------------------------------------------------------------------
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
	// Client-side: Remove slot from RPC (called by CRF_RplBroadcastManager)
	//------------------------------------------------------------------------------------------------
	void RemoveSlotClient(int slotId)
	{
		if (Replication.IsServer())
			return;
		
		m_mSlotsMap.Remove(slotId);
		
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		Print(string.Format("[CRF_SlottingManager] Client removed slot %1", slotId), LogLevel.VERBOSE);
	}
	
	//------------------------------------------------------------------------------------------------
	// RESOURCE CACHING SYSTEM
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Get a cached resource or load and cache it if not already cached
	 * Reduces repeated Resource.Load() calls during mass spawning
	 * @param resourceName The resource path to load
	 * @return The loaded resource or null if invalid
	 */
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
	
	/**
	 * Clear all cached resources
	 * Call this when unloading mission or changing scenarios
	 */
	void ClearResourceCache()
	{
		m_mCachedResources.Clear();
		Print("[CRF_SlottingManager] Resource cache cleared", LogLevel.VERBOSE);
	}
	
	//------------------------------------------------------------------------------------------------
	// MASS INITIALIZATION FLAG
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Set the mass initialization flag
	 * Used to optimize collision checks during batch player spawning
	 * @param inProgress True when batch spawning is active
	 */
	void SetMassInitializationInProgress(bool inProgress)
	{
		m_bMassInitializationInProgress = inProgress;
	}
	
	/**
	 * Check if mass initialization is currently in progress
	 * @return True if batch spawning is active
	 */
	bool IsMassInitializationInProgress()
	{
		return m_bMassInitializationInProgress;
	}
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION
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
}