class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
// INT in this map works on a "ID" based system where a ID is generated for every slot that is created in the AddSlot function bellow.
	// CRF_SlotDataContainer is then stored in this map for further use by the relevant systems or to be updated later when applicable.
	protected ref map<int, CRF_SlotDataContainer> m_mSlotsMap = new map<int, CRF_SlotDataContainer>;
	
	// Cannot replicate maps, so we use this array to replicate all map keys (in correllation with the map data array bellow).
	// Is also a really easy way to update clients the slots map has changed.
	[RplProp()]
	protected ref array<int> m_aSlotsKey = {}; 
	
	// Cannot replicate maps, so we use this array to replicate all map data (in correllation with the map key array above).
	[RplProp()]
	protected ref array<ref CRF_SlotDataContainer> m_aSlotsData = {}; 
	
	// Latest Slot ID that was used to create a slot
	protected int m_iLatestSlotID;
	
	// Script Invoker for all your invoker needs
	protected ref ScriptInvoker m_OnSlottingUpdate;
	
	// How we propagate slotting updates
	[RplProp(onRplName: "SlottingUpdate")]
	protected int m_SlottingUpdate;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SlottingManager.Cast(gameMode.FindComponent(CRF_SlottingManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	// Init method
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
	};
	
	//Updates all players the slotting information has changed
	//------------------------------------------------------------------------------------------------
	void RequestSlottingUpdate()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Create a temp array so we arent broadcasting for each change to m_aPlayerArray.
		array<int> tempSlotsKey = {};
		array<ref CRF_SlotDataContainer> tempSlotsData = {};

		// Fill tempSlotsKey/tempSlotsData with all keys and values in m_mSlotsMap.
		foreach (int slotId, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			tempSlotsKey.Insert(slotId);
			tempSlotsData.Insert(slotData);
		};

		// Replicate m_aSlotsKey/m_aSlotsData to all clients.
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
		
		foreach (int i, int slotID : m_aSlotsKey)
			m_mSlotsMap.Set(slotID, m_aSlotsData.Get(i));
		
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
		map<int, SCR_AIGroup> tempHoldermap = new map<int, SCR_AIGroup>;
		array<SCR_AIGroup> outputArray = {};
		array<int> sortingArray = {};
		
		foreach (int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(!slotData || !slotData.GetSlotCurrentGroup() || slotData.GetSlotCurrentGroup() == RplId.Invalid() || !Replication.FindItem(slotData.GetSlotCurrentGroup()))
				continue;
			
			if(factionKey.IsEmpty() || (!factionKey.IsEmpty() && slotData.GetSlotFactionKey() == factionKey))
			{
				SCR_AIGroup tempGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentGroup())).GetEntity());
				
				if (tempGroup && !tempHoldermap.Get(tempGroup.GetGroupID()))
					tempHoldermap.Set(tempGroup.GetGroupID(), tempGroup);
			}
		}
		
		foreach (int Id, SCR_AIGroup groupToSort : tempHoldermap)
			sortingArray.Insert(Id);
		
		sortingArray.Sort(false);
		
		foreach (int Id : sortingArray)
		{
			SCR_AIGroup group = tempHoldermap.Get(Id);
			outputArray.Insert(group);
		}
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> outputArray = {};
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentGroup() == rplId)
				outputArray.Insert(slotID);
		}
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentCharacter() == rplId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		
		if(!slotData || !slotData.GetSlotCurrentGroup() || slotData.GetSlotCurrentGroup() == RplId.Invalid() || !Replication.FindItem(slotData.GetSlotCurrentGroup()))
			return null;
		
		return SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentGroup())).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter GetPlayerSlotCharacter(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		
		if(!slotData || !slotData.GetSlotCurrentCharacter() || slotData.GetSlotCurrentCharacter() == RplId.Invalid() || !Replication.FindItem(slotData.GetSlotCurrentCharacter()))
			return null;
		
		return SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter())).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId)
	{
		if(GetPlayerSlotData(playerId) && (!(GetPlayerSlotData(playerId).GetSlotFactionKey()).IsEmpty()))
			return GetGame().GetFactionManager().GetFactionByKey(GetPlayerSlotData(playerId).GetSlotFactionKey());
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		return GetPlayerSlotData(playerId).GetSlotResource();
	}
	
	//------------------------------------------------------------------------------------------------
	void GetPlayerSlotVector(int playerId, out vector vec[4])
	{
		GetPlayerSlotData(playerId).GetSlotVector(vec);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		RplId rplId = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentCharacter() == rplId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotFactionKey() == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if(slotData)
		{
			return slotData.GetIsDeadSlot();
		} else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.SetIsLockedSlot(input);
		
		RequestSlottingUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.SetIsDeadSlot(input);
		
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		CRF_SlotDataContainer slotData = GetSlotData(slotId);
		slotData.SetSlotCurrentPlayerId(playerId);
		
		if(playerId <= 0)
		{	
			if(slotData && slotData.GetSlotCurrentCharacter() && slotData.GetSlotCurrentCharacter() != RplId.Invalid() && Replication.FindItem(slotData.GetSlotCurrentCharacter()))
			{
				
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter())).GetEntity());
				 if(character)
					SCR_EntityHelper.DeleteEntityAndChildren(character);
				
				UpdateSlotCharacter(slotId, RplId.Invalid());
			};
		}
		
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupId)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.SetSlotCurrentGroup(groupId);
		
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.SetSlotResource(resource);
		
		RequestSlottingUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.SetSlotCurrentCharacter(charId);
		
		RequestSlottingUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.GetSlotCurrentPlayerId() <= 0)
				UpdateSlotLockedState(slotID, true);
		}
		
		array<SCR_AIGroup> allGroups = GetAllGroups();
		
		// Process each group and its players
		foreach(SCR_AIGroup group : allGroups)
		{	
			// Skip already private groups
			if(group.IsPrivate())
				continue;
			
			int playersInGroup = 0;
			
			// Get group ID
			RplId groupId = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			
			// Process all slots in this group
			foreach(int slotId, CRF_SlotDataContainer slotData : m_mSlotsMap)
			{
				if(slotData.GetSlotCurrentGroup() != groupId)
					continue;
				
				// Count slots
				if (slotData.GetSlotCurrentPlayerId() > 0)
					playersInGroup++;
			};
							
			// lock empty groups
			if (playersInGroup == 0)
				group.SetPrivate(true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter SpawnPlayableEntity(int playerId, vector overrideLocation)
	{
		int slotId = GetPlayerSlotID(playerId);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		if (overrideLocation != vector.Zero)
			spawnParams.Transform[3] = overrideLocation;
		else
			GetPlayerSlotVector(playerId, spawnParams.Transform);
		
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load(GetPlayerSlotResource(playerId)), GetGame().GetWorld(), spawnParams));
	
		UpdateSlotCharacter(slotId, RplComponent.Cast(playerCharacter.FindComponent(RplComponent)).Id());
		UpdateSlotDeathState(slotId, false);
		
		CRF_PlayableCharacter playabeCharComp = CRF_PlayableCharacter.Cast(playerCharacter.FindComponent(CRF_PlayableCharacter));
		if (playabeCharComp)
			playabeCharComp.SetIsSlotSpawned();
		
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddPlayableEntityToManager(IEntity entity)
	{
		if(RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter));
		
		if(!playableCharComp || !playableCharComp.m_bIsPlayable)
			return;
		
		SCR_EditableCharacterComponent editableCharComp = SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent));
		SCR_AIGroup group = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
		
		if(!group.IsGroupPlayable())
			return;
		
		CRF_SlotDataContainer slotData = new CRF_SlotDataContainer;
		
		if(group)
		{
			slotData.SetSlotCurrentGroup(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
			slotData.SetSlotFactionKey(group.GetFaction().GetFactionKey());
		} else 
			slotData.SetSlotFactionKey(SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent)).GetAffiliatedFactionKey());
		
		vector tempVec[4];
		entity.GetWorldTransform(tempVec);
		slotData.SetSlotVector(tempVec);
		
		slotData.SetSlotResource(entity.GetPrefabData().GetPrefabName());
		slotData.SetSlotCurrentCharacter(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
		
		if (!playableCharComp.m_sName.IsEmpty())
			slotData.SetSlotName(playableCharComp.m_sName);
		else
			slotData.SetSlotName(editableCharComp.GetDisplayName());	
		
		slotData.SetSlotIcon(editableCharComp.GetInfo().GetIconPath());
		slotData.SetSlotType(playableCharComp.m_SlottingRole);
				
		m_iLatestSlotID++;
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		RequestSlottingUpdate();
		
		if(m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}