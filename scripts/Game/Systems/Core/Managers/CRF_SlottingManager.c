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
	protected CRF_GearscriptManager m_GearscriptManager;
	
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
	// Optimized batch update method that minimizes replication calls
	bool BatchUpdateSlot(int slotId, int playerId = -1, RplId groupId = RplId.Invalid(), RplId charId = RplId.Invalid(), 
	                    ResourceName resource = "", string name = "", bool isLocked = false, bool isDead = false)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		if (!slotData)
			return false;
		
		// Use the optimized batch update that only triggers one InvokeDataUpdate()
		bool updated = slotData.BatchUpdateSlotData(playerId, groupId, charId, resource, name, isLocked, isDead);
		
		// Handle special cleanup logic for player removal
		if (updated && playerId == 0)
		{
			CleanupCharacterFromSlot(slotData);
		}
		
		// Only trigger global replication if something actually changed
		if (updated)
		{
			RequestSlottingUpdate();
		}
		
		return updated;
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
	// SLOT UPDATE METHODS
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		// Use optimized batch update method
		BatchUpdateSlot(slotId, -1, RplId.Invalid(), RplId.Invalid(), "", "", input, false);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		// Use optimized batch update method  
		BatchUpdateSlot(slotId, -1, RplId.Invalid(), RplId.Invalid(), "", "", false, input);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		// Use optimized batch update method (includes automatic cleanup for player removal)
		BatchUpdateSlot(slotId, playerId, RplId.Invalid(), RplId.Invalid(), "", "", false, false);
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
		// Use optimized batch update method
		BatchUpdateSlot(slotId, -1, groupId, RplId.Invalid(), "", "", false, false);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		// Use optimized batch update method
		BatchUpdateSlot(slotId, -1, RplId.Invalid(), RplId.Invalid(), resource, "", false, false);
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
		// Use optimized batch update method
		BatchUpdateSlot(slotId, -1, RplId.Invalid(), RplId.Invalid(), "", name, false, false);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		// Use optimized batch update method
		BatchUpdateSlot(slotId, -1, RplId.Invalid(), charId, "", "", false, false);
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
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		RequestSlottingUpdate();
		
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
}