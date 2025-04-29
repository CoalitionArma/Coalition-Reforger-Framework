class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
	// INT in this map works on a "ID" based system where a ID is generated for every slot that is created in the AddSlot function bellow.
	// CRF_SlotDataContainer is then stored in this map for further use by the relevant systems or to be updated later when applicable.
	protected ref map<int, ref CRF_SlotDataContainer> m_mSlotsMap = new map<int, ref CRF_SlotDataContainer>;
	
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
	
	[RplProp(onRplName: "UpdateClientSlotsMap")]
	protected int m_SlottingUpdate;
	
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
	ScriptInvoker GetOnSlottingUpdate()
	{
		if (!m_OnSlottingUpdate)
			m_OnSlottingUpdate = new ScriptInvoker();

		return m_OnSlottingUpdate;
	}
	
	//Updates all players the slotting information has changed
	//------------------------------------------------------------------------------------------------
	void SlottingChangesUpdate()
	{
		// Create a temp array so we arent broadcasting for each change to m_aPlayerArray.
		protected ref array<int> tempSlotsKey = {};
		protected ref array<ref CRF_SlotDataContainer> tempSlotsData = {};

		// Fill tempSlotsKey/tempSlotsData with all keys and values in m_mSlotsMap.
		for (int i = 0; i < m_mSlotsMap.Count(); i++)
		{
			int key = m_mSlotsMap.GetKey(i);
			ref CRF_SlotDataContainer value = m_mSlotsMap.Get(key);
			
			tempSlotsKey.Insert(key);
			tempSlotsData.Insert(value);
		};

		// Replicate m_aSlotsKey/m_aSlotsData to all clients.
		m_aSlotsKey = tempSlotsKey;
		m_aSlotsData = tempSlotsData;
		m_SlottingUpdate++;
		Replication.BumpMe();
		
		#ifdef WORKBENCH
			UpdateClientSlotsMap()
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateClientSlotsMap()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		foreach (int i, int slotID : m_aSlotsKey)
			m_mSlotsMap.Set(slotID, m_aSlotsData.Get(i));
		
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	ref CRF_SlotDataContainer GetSlotData(int slotId)
	{
		return m_mSlotsMap.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	map<int, ref CRF_SlotDataContainer> GetSlotMap()
	{
		return m_mSlotsMap;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> tempArray = {};
		
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentGroup == rplId)
				tempArray.Insert(slotID);
		}
		
		return tempArray;
	}
	
	//------------------------------------------------------------------------------------------------
	ref CRF_SlotDataContainer GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentCharacter == rplId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	ref CRF_SlotDataContainer GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		ref CRF_SlotDataContainer data = GetPlayerSlotData(playerId);
		
		if(!data || !data.m_iSlotCurrentGroup || !data.m_iSlotCurrentGroup.IsValid() || !Replication.FindItem(data.m_iSlotCurrentGroup))
			return null;
		
		return SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(data.m_iSlotCurrentGroup)).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter GetPlayerSlotCharacter(int playerId)
	{
		ref CRF_SlotDataContainer data = GetPlayerSlotData(playerId);
		
		if(!data || !data.m_iSlotCurrentCharacter || !data.m_iSlotCurrentCharacter.IsValid() || !Replication.FindItem(data.m_iSlotCurrentCharacter))
			return null;
		
		return SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(data.m_iSlotCurrentCharacter)).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId)
	{
		if(GetPlayerSlotData(playerId) && (!(GetPlayerSlotData(playerId).m_SlotFactionKey).IsEmpty()))
			return GetGame().GetFactionManager().GetFactionByKey(GetPlayerSlotData(playerId).m_SlotFactionKey);
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		return GetPlayerSlotData(playerId).m_rSlotResource;
	}
	
	//------------------------------------------------------------------------------------------------
	void GetPlayerSlotVector(int playerId, out vector vec[4])
	{
		vec[0] = GetPlayerSlotData(playerId).m_vSlotVectorOne;
		vec[1] = GetPlayerSlotData(playerId).m_vSlotVectorTwo;
		vec[2] = GetPlayerSlotData(playerId).m_vSlotVectorThree;
		vec[3] = GetPlayerSlotData(playerId).m_vSlotVectorFour;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		RplId rplId = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentCharacter == rplId)
				return slotID;
		}
		
		return 0;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_SlotFactionKey == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		ref CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if(slotData)
		{
			return slotData.m_bIsDeadSlot;
		} else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_bIsLockedSlot = input;
		
		SlottingChangesUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_bIsDeadSlot = input;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		if(playerId <= 0)
		{
			ref CRF_SlotDataContainer data = GetSlotData(slotId);
			
			if(data && data.m_iSlotCurrentCharacter && data.m_iSlotCurrentCharacter.IsValid() && Replication.FindItem(data.m_iSlotCurrentCharacter))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(data.m_iSlotCurrentCharacter)).GetEntity()));
				UpdateSlotCharacter(slotId, RplId.Invalid());
			};
		}
	
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentPlayerId = playerId;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupId)
	{
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentGroup = groupId;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_rSlotResource = resource;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		ref CRF_SlotDataContainer slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentCharacter = charId;
		
		SlottingChangesUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		foreach (int slotID, ref CRF_SlotDataContainer slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId > 0)
				continue;
			else
				slotData.m_bIsLockedSlot = true;
		}
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void AddPlayableEntityToManager(IEntity entity)
	{
		if(RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter));
		
		if(!playableCharComp || !playableCharComp.IsPlayable())
			return;
		
		SCR_EditableCharacterComponent editableCharComp = SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent));
		SCR_AIGroup group = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
		
		ref CRF_SlotDataContainer slotData = new CRF_SlotDataContainer;
		
		if(group)
		{
			slotData.m_iSlotCurrentGroup = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			slotData.m_SlotFactionKey = group.GetFaction().GetFactionKey();
		} else 
			slotData.m_SlotFactionKey = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent)).GetAffiliatedFactionKey();
		
		vector tempVec[4];
		entity.GetWorldTransform(tempVec);
		
		slotData.m_vSlotVectorOne = tempVec[0];
		slotData.m_vSlotVectorTwo = tempVec[1];
		slotData.m_vSlotVectorThree = tempVec[2];
		slotData.m_vSlotVectorFour = tempVec[3];
		
		slotData.m_rSlotResource = entity.GetPrefabData().GetPrefabName();
		slotData.m_iSlotCurrentCharacter = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		if (!playableCharComp.GetName().IsEmpty())
			slotData.m_sSlotName = playableCharComp.GetName();
		else
			slotData.m_sSlotName = editableCharComp.GetDisplayName();	
		
		slotData.m_rSlotIconResource = editableCharComp.GetInfo().GetIconPath();
		slotData.m_iSlotType = playableCharComp.GetSlottingRole();
				
		m_iLatestSlotID++;
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		SlottingChangesUpdate();
		
		if(CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}