class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Slot data storage - uses ID-based system where IDs are generated in AddSlot
	protected ref map<int, ref CRF_SlotData> m_mSlotsMap = new map<int, ref CRF_SlotData>;
	
	// Latest Slot ID used
	protected int m_iLatestSlotID;
	
	// Invoker for slot updates (batch/structural: used by all consumers including AAR & spectator)
	protected ref ScriptInvoker m_OnSlottingUpdate = new ScriptInvoker;
	
	// Invoker for surgical per-slot player-ID changes (avoids full list rebuild on every click)
	protected int m_iLastChangedSlotId = -1;
	protected ref ScriptInvoker m_OnSlotChanged = new ScriptInvoker;
	
	// References to other managers
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		
		// Need to call next frame due to race conditions if the faction manager hasn't fully initilized.
		GetGame().GetCallqueue().Call(InitilizeSlots);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitilizeSlots()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		InitilizeSlotsForFaction("BLUFOR", gamemode.m_BluforSlots);
		InitilizeSlotsForFaction("OPFOR", gamemode.m_OpforSlots);
		InitilizeSlotsForFaction("INDFOR", gamemode.m_IndforSlots);
		InitilizeSlotsForFaction("CIV", gamemode.m_CivSlots);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SLOTTING UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentCharacter(charId);
			m_RplBroadcastManager.UpdateSlotCharacterDelta(slotId, charId);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotRole(int slotId, CRF_EGearRole role)
	{
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotRole(role);
			m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId group)
	{
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(group);
			m_RplBroadcastManager.UpdateSlotGroupDelta(slotId, group);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId = -1)
	{	
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			// Server-side first-come-first-served guard:
			// If a player (playerId > 0) is trying to claim a slot that is already occupied
			// by a DIFFERENT player, reject the request silently.
			// playerId == 0 (kick/vacate) always proceeds; own-slot re-claim always proceeds.
			int currentOccupant = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0 && currentOccupant > 0 && currentOccupant != playerId)
				return;
			
			slotData.SetSlotCurrentPlayerId(playerId);
			m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, playerId);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force-assigns a player to a slot for reconnect restoration.
	//! Bypasses the first-come-first-served guard so a reconnecting player (who may have a
	//! new numeric ID on a dedicated server) can reclaim the slot they held before disconnecting.
	//! Safe to call with newPlayerId > 0 — never triggers CleanupCharacterFromSlot.
	//! \param[in] slotId      Slot to restore
	//! \param[in] newPlayerId The reconnecting player's current numeric player ID
	void ForceUpdateSlotPlayerID(int slotId, int newPlayerId)
	{
		if (newPlayerId <= 0)
			return;
		
		CRF_SlotData slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// SetSlotCurrentPlayerId only triggers CleanupCharacterFromSlot when playerId <= 0,
		// so calling it with newPlayerId > 0 is safe and will not delete the character entity.
		slotData.SetSlotCurrentPlayerId(newPlayerId);
		m_RplBroadcastManager.UpdateSlotPlayerIdDelta(slotId, newPlayerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool isLocked = false)
	{
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsLockedSlot(isLocked);
			if (isLocked)
				slotData.SetSlotCurrentPlayerId(0);
			
			m_RplBroadcastManager.UpdateSlotLockedDelta(slotId, isLocked);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotData slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsDeadSlot(input);
			m_RplBroadcastManager.UpdateSlotDeathDelta(slotId, input);
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
	//! Returns the invoker that fires when a single slot's player ID changes (surgical update path).
	ScriptInvoker GetOnSlotChanged()
	{
		return m_OnSlotChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the slot ID that was most recently changed via NotifySlotPlayerIdChanged().
	int GetLastChangedSlotId()
	{
		return m_iLastChangedSlotId;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Called by RpcDo_UpdateSlotPlayerIdDelta to fire the targeted slot-change invoker.
	//! Stores the slot ID so subscribers can retrieve it without invoice arguments.
	void NotifySlotPlayerIdChanged(int slotId)
	{
		m_iLastChangedSlotId = slotId;
		if (m_OnSlotChanged)
			m_OnSlotChanged.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotData GetSlotData(int slotId)
	{
		return m_mSlotsMap.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	map<int, ref CRF_SlotData> GetSlotMap()
	{
		return m_mSlotsMap;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIds()
	{
		array<int> slotIds = {};
		
		foreach (int slotId, CRF_SlotData slotData : m_mSlotsMap)
		{
			slotIds.Insert(slotId);
		}
		
		return slotIds;
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
		foreach (int slotId, CRF_SlotData slotData : m_mSlotsMap)
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
	protected bool IsValidGroupInSlot(CRF_SlotData slotData)
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
		array<int> outputArray = {};
		
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
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
	CRF_SlotData GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
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
		
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
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
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotData GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId groupId = slotData.GetSlotCurrentGroup();
		return CRF_EntityHelper.GetGroupFromRplId(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_PlayerCharacter GetPlayerSlotCharacter(int playerId)
	{
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return null;
			
		RplId charId = slotData.GetSlotCurrentCharacter();
		return CRF_PlayerCharacter.Cast(CRF_EntityHelper.GetCharacterFromRplId(charId));
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId, bool returnNull = false)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
		
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
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return ResourceName.Empty;
			
		return slotData.GetSlotResource();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATE CHECKING
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Count how many players are slotted in a specific faction
	//! This counts SLOTTED players, not spawned players
	//! \param[in] factionKey The faction key to count (e.g. "BLUFOR", "OPFOR", "INDFOR", "CIV")
	//! \return Number of players slotted in that faction
	int GetSlottedPlayerCountByFaction(FactionKey factionKey)
	{
		int count = 0;
		
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			// Check if slot has a player and matches the faction
			if (slotData.GetSlotCurrentPlayerId() > 0 && slotData.GetSlotFactionKey() == factionKey)
				count++;
		}
		
		return count;
	}
	
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotFactionKey() == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentPlayerId() == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
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
		
		foreach (int slotId, CRF_SlotData slotData : m_mSlotsMap)
		{
			if (slotData.GetSlotCurrentGroup() != groupId)
				continue;
				
			if (slotData.GetSlotCurrentPlayerId() > 0)
				return false;
		}
		
		return true;
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
				CRF_RolesConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig();
				CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				
				if (!roleConfig || !rolesConfig)
					return;
					
				// Create and configure new slot data
				CRF_SlotData slotData = new CRF_SlotData;
				
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
	void LockAllOpenSlots()
	{
		// Lock all empty slots
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
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
	void CleanupCharacterFromSlot(CRF_SlotData slotData)
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
//	 CLIENT SIDE REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Update single slot from RPC (called by CRF_RplBroadcastManager)
	//! Only updates if data actually changed (prevents unnecessary UI rebuilds)
	void UpdateSlotDataClient(CRF_SlotData slotData)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		int slotId = slotData.GetSlotId();
		CRF_SlotData oldSlotData = m_mSlotsMap.Get(slotId);

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
		foreach (int slotId, CRF_SlotData slotData : m_mSlotsMap)
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
			CRF_SlotData slotData = new CRF_SlotData();
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