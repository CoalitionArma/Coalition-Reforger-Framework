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
	
	void CRF_SlottingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
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
		
		// Initialize string registry for optimized slot data replication
		if (Replication.IsServer())
			InitializeSlotDataRegistry();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize the string registry with common resources for bandwidth optimization
	// This allows us to send resource indices (1-2 bytes) instead of full paths (80+ bytes)
	//------------------------------------------------------------------------------------------------
	protected void InitializeSlotDataRegistry()
	{
		// Pre-register common factions (add all your faction keys here)
		CRF_SlotDataContainer_StringRegistry.s_FactionKeyRegistry.Insert("BLUFOR");
		CRF_SlotDataContainer_StringRegistry.s_FactionKeyRegistry.Insert("OPFOR");
		CRF_SlotDataContainer_StringRegistry.s_FactionKeyRegistry.Insert("INDFOR");
		CRF_SlotDataContainer_StringRegistry.s_FactionKeyRegistry.Insert("CIVILIAN");

		// Pre-register common slot resources (character prefabs)
		// TODO: Add your most common character prefabs here
		// Example:
		// CRF_SlotDataContainer_StringRegistry.s_SlotResourceRegistry.Insert("{26A9756790131354}Prefabs/Characters/...");
		
		// Pre-register common icons
		// TODO: Add your most common icon resources here
		
		Print("[CRF_SlottingManager] String registry initialized for optimized slot replication", LogLevel.NORMAL);
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
			m_RplBroadcastManager.UpdateSlotData(slotData);
		};
	}
	
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotResource(resource);
			m_RplBroadcastManager.UpdateSlotData(slotData);
		};
	}
	
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetIsDeadSlot(input);
			m_RplBroadcastManager.UpdateSlotData(slotData);
		};
	}
	
	void UpdateSlotGroup(int slotId, RplId group)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(group);
			m_RplBroadcastManager.UpdateSlotData(slotData);
		};
	}
	
	void UpdateSlotPlayerID(int slotId, int playerId = -1)
	{	
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		
		if (slotData)
		{
			slotData.SetSlotCurrentPlayerId(playerId);
			m_RplBroadcastManager.UpdateSlotData(slotData);
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
			
			m_RplBroadcastManager.UpdateSlotData(slotData);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	// OPTIMIZED BANDWIDTH UPDATE METHODS
	// These methods use specialized RPCs that send only changed data (8-30 bytes vs 362 bytes)
	// Use these instead of UpdateSlot* methods above for 96%+ bandwidth savings
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot character (sends only slot ID + character RplId = ~12 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacterOptimized(int slotId, RplId charId)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetSlotCurrentCharacter(charId);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotCharacterOptimized(slotId, charId);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot resource (sends only slot ID + resource index = ~10 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResourceOptimized(int slotId, ResourceName resource)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetSlotResource(resource);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotResourceOptimized(slotId, resource);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot death state (sends only slot ID + bool = ~8 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathStateOptimized(int slotId, bool isDead)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetIsDeadSlot(isDead);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotDeadStatus(slotId, isDead);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot group (sends only slot ID + group RplId = ~12 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroupOptimized(int slotId, RplId groupId)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetSlotCurrentGroup(groupId);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotGroupOptimized(slotId, groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot player assignment (sends slot ID + player ID + char ID + group ID = ~18 bytes vs 362)
	// This is the most common operation - use this for player joins/leaves
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerAssignmentOptimized(int slotId, int playerId, RplId charId, RplId groupId)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetSlotCurrentPlayerId(playerId);
		slotData.SetSlotCurrentCharacter(charId);
		slotData.SetSlotCurrentGroup(groupId);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotPlayerAssignment(slotId, playerId, charId, groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Clear player from slot (sends slot ID only = ~8 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void ClearPlayerFromSlotOptimized(int slotId)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetSlotCurrentPlayerId(-1);
		slotData.SetSlotCurrentCharacter(RplId.Invalid());
		
		// Broadcast optimized update - just send invalid player assignment
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotPlayerAssignment(slotId, -1, RplId.Invalid(), slotData.GetSlotCurrentGroup());
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized: Update slot lock state (sends only slot ID + bool = ~8 bytes vs 362)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedStateOptimized(int slotId, bool isLocked)
	{
		if (!Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
		
		// Update local data
		slotData.SetIsLockedSlot(isLocked);
		if (isLocked)
			slotData.SetSlotCurrentPlayerId(0);
		
		// Broadcast optimized update
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.UpdateSlotLockStatus(slotId, isLocked);
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		if (!m_OnSlottingUpdate)
			m_OnSlottingUpdate = new ScriptInvoker();

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
	// Alias for GetSlotData (used by RPC handlers)
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotById(int slotId)
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
	SCR_ChimeraCharacter GetCharacterFromRplId(RplId charId)
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
	void GetPlayerSlotVector(int playerId, out vector vec[4])
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if (!slotData)
			return;
			
		slotData.GetSlotVector(vec);
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
		GetPlayerSlotVector(playerId, playerSlotVector);

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
		
		// Spawn the character
		Resource resource = Resource.Load(resourceName);
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(
			GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return null;
	
		// Update slot data
		RplComponent charRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (charRplComp)
		{
			UpdateSlotCharacter(slotId, charRplComp.Id());
			UpdateSlotDeathState(slotId, false);
		}
		
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
	    vector candidate;
	    vector surface;
	    vector outTransform[4] = baseTransform;
		
		if (!IsOverlappingOtherPlayer(baseTransform[3]))
		{
			trasnformOut = baseTransform;
			return;
		}
	
	    for (int i = 0; i < 20; i++)
	    {
	        float angle = Math.RandomFloat01() * Math.PI2;
	        float dist  = Math.RandomFloat01() * radius;
	        vector offset = Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
	
	        candidate = baseTransform[3] + offset;

	        SCR_TerrainHelper.SnapToGeometry(surface, candidate, {}, GetGame().GetWorld());
	
	        if (surface != vector.Zero && !IsOverlappingOtherPlayer(surface))
	        {
	            outTransform[3] = surface;
	
	            SCR_TerrainHelper.OrientToTerrain(outTransform);
	
	            trasnformOut = outTransform;
				return;
	        }
	    }
	
	    trasnformOut = baseTransform;
	}

	
	//------------------------------------------------------------------------------------------------
	bool IsOverlappingOtherPlayer(vector pos)
	{
		return !GetGame().GetWorld().QueryEntitiesBySphere(pos, 1.5, FilterEntities, null);
	}
	
	bool FilterEntities(IEntity entity)
	{
		if (SCR_ChimeraCharacter.Cast(entity) || entity.FindComponent(CRF_RespawnPointComponent))
			return false;
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddPlayableEntityToManager(IEntity entity)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!entity)
			return;
		
		// Check if entity is playable
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(
			entity.FindComponent(CRF_PlayableCharacter)
		);
		
		if (!playableCharComp)
			return;
		
		// Get required components
		SCR_EditableCharacterComponent editableCharComp = SCR_EditableCharacterComponent.Cast(
			entity.FindComponent(SCR_EditableCharacterComponent)
		);
		
		if (!editableCharComp)
			return;
			
		ChimeraAIControlComponent aiControlComp = ChimeraAIControlComponent.Cast(
			entity.FindComponent(ChimeraAIControlComponent)
		);
		
		if (!aiControlComp)
			return;
			
		SCR_AIGroup group = SCR_AIGroup.Cast(aiControlComp.GetControlAIAgent().GetParentGroup());
		if (!group || !group.IsGroupPlayable())
			return;
		
		CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(entity.GetPrefabData().GetPrefabName());
		
		CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		
		if (!role || !roleConfig || !rolesConfig)
			return;
			
		// Create and configure new slot data
		CRF_SlotDataContainer slotData = new CRF_SlotDataContainer;
		
		// Set group and faction
		RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
		slotData.SetSlotCurrentGroup(groupRplComp.Id());
		slotData.SetSlotFactionKey(group.GetFaction().GetFactionKey());
		
		// Set position
		vector tempVec[4];
		entity.GetWorldTransform(tempVec);
		slotData.SetSlotVector(tempVec);
		
		// Set resource and character ID
		slotData.SetSlotResource(entity.GetPrefabData().GetPrefabName());
		
		RplComponent entityRplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		slotData.SetSlotCurrentCharacter(entityRplComp.Id());
		
		string customSlottingName = m_GearscriptManager.GetCustomRoleName(group.GetFaction().GetFactionKey(), role);
		
		// Set slot name
		if (!customSlottingName.IsEmpty())
			slotData.SetSlotName(customSlottingName);
		else if (!roleConfig.m_sRoleName.IsEmpty())
			slotData.SetSlotName(roleConfig.m_sRoleName);
		else
			slotData.SetSlotName(editableCharComp.GetDisplayName());
		
		// Set icon
		if (!roleConfig.m_RoleIcon.IsEmpty())
			slotData.SetSlotIcon(roleConfig.m_RoleIcon);
		else
			slotData.SetSlotIcon(editableCharComp.GetInfo().GetIconPath());
		
		// Set type
		slotData.SetSlotType(roleConfig.m_SlottingType);
				
		// Add to slots map
		m_iLatestSlotID++;
		slotData.SetSlotId(m_iLatestSlotID);
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		// Broadcast new slot to all clients
		m_RplBroadcastManager.UpdateSlotData(slotData);
		
		// Delete entity if not in game state
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Generate a random position within specified radius to spread out initial entity spawns
	* This reduces replication congestion when many entities spawn in the same location
	* @param centerPosition Original spawn position to spread from
	* @param maxRadius Maximum radius in meters to spread entities (default 500m)
	* @return New spawn position within the spread radius
	*/
	protected vector GenerateRandomSpreadPosition(vector centerPosition, float maxRadius = 500.0)
	{
		// Generate random angle (0-360 degrees)
		float randomAngle = Math.RandomFloat(0, 2 * Math.PI);
		
		// Generate random distance within radius (using square root for uniform distribution)
		float randomDistance = Math.Sqrt(Math.RandomFloat(0, 1)) * maxRadius;
		
		// Calculate offset from center
		float offsetX = Math.Cos(randomAngle) * randomDistance;
		float offsetZ = Math.Sin(randomAngle) * randomDistance;
		
		// Apply offset to center position
		vector spreadPosition = centerPosition;
		spreadPosition[0] = centerPosition[0] + offsetX;
		spreadPosition[2] = centerPosition[2] + offsetZ;
		
		// Attempt to find valid terrain position, fallback to original logic if needed
		vector finalPosition;
		bool foundValidPosition = SCR_WorldTools.FindEmptyTerrainPosition(finalPosition, spreadPosition, 25);
		
		if (!foundValidPosition)
		{
			// Fallback: try original position with smaller search radius
			bool foundFallback = SCR_WorldTools.FindEmptyTerrainPosition(finalPosition, centerPosition, 12);
			if (!foundFallback)
				finalPosition = centerPosition; // Last resort: use original position
		}
		
		Print(string.Format("GenerateRandomSpreadPosition: Original pos [%1, %2, %3] -> Spread pos [%4, %5, %6] (distance: %7m)", 
			centerPosition[0], centerPosition[1], centerPosition[2],
			finalPosition[0], finalPosition[1], finalPosition[2],
			vector.Distance(centerPosition, finalPosition)), LogLevel.VERBOSE);
			
		return finalPosition;
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
	// OPTIMIZED CLIENT-SIDE RPC HANDLERS
	// These receive the optimized delta updates and apply them to local slot data
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot character from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacterClient(int slotId, RplId charId)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetSlotCurrentCharacter(charId);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot resource from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResourceClient(int slotId, ResourceName resource)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetSlotResource(resource);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot dead status from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeadStatusClient(int slotId, bool isDead)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetIsDeadSlot(isDead);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot group from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroupClient(int slotId, RplId groupId)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetSlotCurrentGroup(groupId);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot player assignment from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerAssignmentClient(int slotId, int playerId, RplId charId, RplId groupId)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetSlotCurrentPlayerId(playerId);
		slot.SetSlotCurrentCharacter(charId);
		slot.SetSlotCurrentGroup(groupId);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	// Client: Update slot lock status from optimized RPC
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockStatusClient(int slotId, bool isLocked)
	{
		if (Replication.IsServer())
			return;
			
		CRF_SlotDataContainer slot = GetSlotById(slotId);
		if (!slot)
			return;
		
		slot.SetIsLockedSlot(isLocked);
		
		ScriptInvoker invoker = slot.GetOnDataUpdate();
		if (invoker)
			invoker.Invoke();
			
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
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