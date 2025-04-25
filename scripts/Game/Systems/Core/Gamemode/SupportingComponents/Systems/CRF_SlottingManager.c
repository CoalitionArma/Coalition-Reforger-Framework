enum CRF_ESlotType
{
	LEADERORMEDIC = 0,
	SPECIALTY,
}

class CRF_SlotData
{
	vector m_vSlotVector;
	
	ResourceName m_rSlotResource;
	
	int m_iSlotCurrentPlayerId;
	
	RplId m_iSlotCurrentGroup;
	
	RplId m_iSlotCurrentCharacter;
	
	ref CRF_SlotUIData m_UIData;
}

class CRF_SlotUIData
{
	string m_sSlotName;

	ResourceName m_rSlotIconResource;
	
	CRF_ESlotType m_iSlotType;
	
	bool m_bIsDeadSlot;
}


class CRF_SlottinManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_SlottingManager : SCR_BaseGameModeComponent
{
	
	// INT in this map works on a "ID" based system where a ID is generated for every slot that is created in the AddSlot function bellow.
	// CRF_SlotData is then stored in this map for further use by the relevant systems or to be updated later when applicable.
	ref map<int, CRF_SlotData> m_mSlotsMap = new map<int, CRF_SlotData>;
	
	// Cannot replicate maps, so we use this array to replicate all map keys (in correllation with the map data array bellow).
	// Is also a really easy way to update clients the slots map has changed.
	[RplProp(onRplName: "UpdateClientSlotsMap")]
	ref array<int> m_aSlotsKey = {}; 
	
	// Cannot replicate maps, so we use this array to replicate all map data (in correllation with the map key array above).
	[RplProp()]
	ref array<CRF_SlotData> m_aSlotsData = {}; 
	
	//Updates all players the slotting information has changed
	//------------------------------------------------------------------------------------------------
	void SlottingChangesUpdate()
	{
		// Create a temp array so we arent broadcasting for each change to m_aPlayerArray.
		protected ref array<int> tempSlotsKey = {};
		protected ref array<CRF_SlotData> tempSlotsData = {};

		// Fill tempSlotsKey/tempSlotsData with all keys and values in m_mSlotsMap.
		for (int i = 0; i < m_mSlotsMap.Count(); i++)
		{
			int key = m_mSlotsMap.GetKey(i);
			CRF_SlotData value = m_mSlotsMap.Get(key);
			
			tempSlotsKey.Insert(key);
			tempSlotsData.Insert(value);
		};

		// Replicate m_aSlotsKey/m_aSlotsData to all clients.
		m_aSlotsKey = tempSlotsKey;
		m_aSlotsData = tempSlotsData;
		Replication.BumpMe();
	}

	//Initializes group into the replicated arrays
	//------------------------------------------------------------------------------------------------
	void AddGroup(SCR_AIGroup group)
	{
		m_aGroupRplIDs.Insert(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
		SCR_AIGroup newGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(group.GetFaction());
		newGroup.SetCanDeleteIfNoPlayer(false);
		newGroup.SetMaxMembers(12);
		m_aActivePlayerGroupsIDs.Insert(RplComponent.Cast(newGroup.FindComponent(RplComponent)).Id());
		Replication.BumpMe();
	}
	//Sets slot to player or removes him from it
	//------------------------------------------------------------------------------------------------
	void SetSlot(int index, int playerId)
	{
		if (playerId > 0)
		{
			SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(FactionAffiliationComponent.Cast(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(index))).GetEntity().FindComponent(FactionAffiliationComponent)).GetAffiliatedFaction());
			m_aSlotPlayerNames.Set(index, GetGame().GetPlayerManager().GetPlayerName(playerId));
		}
		else
		{
			if (m_aSlots.Get(index) > 0)
			{
				SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(m_aSlots.Get(index)).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));
				m_aSlotPlayerNames.Set(index, "");
			}
		}
		m_aSlots.Set(index, playerId);
		SlottingChangesUpdate();
	}

	//Sets if an entity is dead or not in the array
	//------------------------------------------------------------------------------------------------
	void SetDeathState(IEntity entity, bool input)
	{
		if (entity.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			m_aEntityDeathStatus.Set(m_aEntitySlots.Find(RplComponent.Cast(entity.FindComponent(RplComponent)).Id()), input);
			SlottingChangesUpdate();
		};
	}
	
	//Initializes playable entities and adds their values into the replicated arrays
	//------------------------------------------------------------------------------------------------
	int AddPlayableEntity(IEntity entity)
	{
		int index = m_aSlots.Insert(0);
		m_aEntitySlots.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
		m_aPlayerGroupIDs.Insert(RplComponent.Cast(SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup()).FindComponent(RplComponent)).Id());
		m_aSlotNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
		m_aSlotPrefabs.Insert(entity.GetPrefabData().GetPrefabName());
		m_aSlotIcons.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetInfo().GetIconPath());
		m_aEntityDeathStatus.Insert(false);
		m_aSlotPlayerNames.Insert("");

		if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsLeader())
			m_aEntitySlotTypes.Insert(0);
		else if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsSpecialty())
			m_aEntitySlotTypes.Insert(1);
		else
			m_aEntitySlotTypes.Insert(2);

		if (m_aSlots.Count() == 1)
			entity.GetTransform(m_vGenericSpawn);

		return index;

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void RemovePlayableEntity(RplId entityID)
	{
		if (!Replication.FindItem(entityID) || SCR_PossessingManagerComponent.GetInstance().GetIdFromMainEntity(RplComponent.Cast(Replication.FindItem(entityID)).GetEntity()) != 0)
			return;

		int index = m_aEntitySlots.Find(entityID);
		m_aSlots.RemoveOrdered(index);
		m_aPlayerGroupIDs.RemoveOrdered(index);
		m_aSlotNames.RemoveOrdered(index);
		m_aSlotIcons.RemoveOrdered(index);
		m_aSlotPrefabs.RemoveOrdered(index);
		m_aEntityDeathStatus.RemoveOrdered(index);
		m_aSlotPlayerNames.RemoveOrdered(index);
		m_aEntitySlotTypes.RemoveOrdered(index);
		m_aEntitySlots.RemoveOrdered(index);

		SCR_EntityHelper.DeleteEntityAndChildren(RplComponent.Cast(Replication.FindItem(entityID)).GetEntity());

		SlottingChangesUpdate();
	}
}