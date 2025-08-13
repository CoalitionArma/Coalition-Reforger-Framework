class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
	// Slot data storage - uses ID-based system where IDs are generated in AddSlot
	protected ref map<int, CRF_SlotDataContainer> m_mSlotsMap = new map<int, CRF_SlotDataContainer>;
	
	// Replication arrays (maps cannot be directly replicated)
	[RplProp()]
	protected ref array<int> m_aSlotsKey = {}; 
	
	[RplProp()]
	protected ref array<ref CRF_SlotDataContainer> m_aSlotsData = {}; 
	
	// Latest Slot ID used
	protected int m_iLatestSlotID;
	
	// Invoker for slot updates
	protected ref ScriptInvoker m_OnSlottingUpdate;
	
	// Replication property for slotting updates
	[RplProp(onRplName: "SlottingUpdate")]
	protected int m_SlottingUpdate;
	
	// References to other managers
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	
	//------------------------------------------------------------------------------------------------
	// INITIALIZATION
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
			
		return CRF_SlottingManager.Cast(gameMode.FindComponent(CRF_SlottingManager));
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	// SLOTTING UPDATE METHODS
	//------------------------------------------------------------------------------------------------
	void RequestSlottingUpdate()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Create temporary arrays to avoid multiple broadcasts
		array<int> tempSlotsKey = {};
		array<ref CRF_SlotDataContainer> tempSlotsData = {};

		// Fill temporary arrays with all map data
		foreach (int slotId, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			tempSlotsKey.Insert(slotId);
			tempSlotsData.Insert(slotData);
		}

		// Update replication properties
		m_aSlotsKey = tempSlotsKey;
		m_aSlotsData = tempSlotsData;
		m_SlottingUpdate++;
		Replication.BumpMe();
		
		#ifdef WORKBENCH
			SlottingUpdate();
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SlottingUpdate()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Update local map from replicated arrays
		for (int i = 0; i < m_aSlotsKey.Count(); i++)
		{
			int slotID = m_aSlotsKey.Get(i);
			m_mSlotsMap.Set(slotID, m_aSlotsData.Get(i));
		}
		
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
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
	map<int, CRF_SlotDataContainer> GetSlotMap()
	{
		return m_mSlotsMap;
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
	protected SCR_AIGroup GetGroupFromRplId(RplId groupId)
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
	protected SCR_ChimeraCharacter GetCharacterFromRplId(RplId charId)
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
	Faction GetPlayerSlotFaction(int playerId)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		
		if (!slotData)
			return factionManager.GetFactionByKey("CIV");
			
		FactionKey factionKey = slotData.GetSlotFactionKey();
		if (factionKey.IsEmpty())
			return factionManager.GetFactionByKey("CIV");
			
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
	// SLOT UPDATE METHODS
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetIsLockedSlot(input);
		RequestSlottingUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetIsDeadSlot(input);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotCurrentPlayerId(playerId);
		
		// If player is removed from slot, clean up character
		if (playerId <= 0)
		{
			CleanupCharacterFromSlot(slotData);
		}
		
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to clean up character from slot
	protected void CleanupCharacterFromSlot(CRF_SlotDataContainer slotData)
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
	void UpdateSlotGroup(int slotId, RplId groupId)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotCurrentGroup(groupId);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotResource(resource);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotIcon(int slotId, ResourceName icon)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotIcon(icon);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotType(int slotId, CRF_ESlotType type)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotType(type);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotName(int slotId, string name)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotName(name);
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return;
			
		slotData.SetSlotCurrentCharacter(charId);
		RequestSlottingUpdate();
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

		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		if (overrideLocation[3] != vector.Zero)
		{
			spawnParams.Transform = overrideLocation;
		}
		else
		{
			GetPlayerSlotVector(playerId, spawnParams.Transform);
		}
		
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
		
		// Set slot name
		if (!roleConfig.m_sRoleName.IsEmpty())
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
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		RequestSlottingUpdate();
		
		// Delete entity if not in game state
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}