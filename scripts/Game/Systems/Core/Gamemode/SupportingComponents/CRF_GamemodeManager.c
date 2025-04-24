class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

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
	
	RplId m_iSlotCurrentGroupId;
	
	ref CRF_SlotUIData m_UIData;
}

class CRF_SlotUIData
{
	string m_sSlotName;

	ResourceName m_rSlotIconResource;
	
	CRF_ESlotType m_iSlotType;
	
	bool m_bIsDeadSlot;
}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	//Slot ID given to an entity
	[RplProp()]
	ref array<int> m_aSlots = {};

	//Stores the group ID for each slot, so I can reference what group a slot is in. CAUSE THERE IS NO WAY TO DO THAT ON THE CLIENT.
	[RplProp()]
	ref array<RplId> m_aPlayerGroupIDs = {};

	//Communicates change across all clients so they can refresh their slots in the UI
	[RplProp()]
	int m_iSlotChanges = 0;

	//Is a group locked
	[RplProp()]
	ref array<bool> m_aGroupLockedStatus = {};

	//Stores SCR_AIGroup RplId, CAUSE YOU CAN'T FUCKING GRAB NON PLAYABLE GROUPS BOHEMIA
	[RplProp()]
	ref array<RplId> m_aGroupRplIDs = {};

	//Stores the playable group created whenever an AI group is created in the editor
	[RplProp()]
	ref array<RplId> m_aActivePlayerGroupsIDs = {};
	
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GamemodeManager.Cast(gameMode.FindComponent(CRF_GamemodeManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		GetGame().GetCallqueue().CallLater(LogCharacter, 500, false, entity);
	}

	//------------------------------------------------------------------------------------------------
	void LogCharacter(IEntity entity)
	{
		#ifdef WORKBENCH
		if (!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
					m_aCharacterNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
				else
					m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			}
			else
				m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			Replication.BumpMe();
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			if (!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
					m_aCharacterNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
				else
					m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			}
			else
				m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			Replication.BumpMe();
		}
		#endif
	}
	
	//Updates all players the slotting information has changed
	//------------------------------------------------------------------------------------------------
	void SlottingChangesUpdate()
	{
		m_iSlotChanges++;
		Replication.BumpMe();
	}
	
	//Initializes group into the replicated arrays
	//------------------------------------------------------------------------------------------------
	void AddGroup(SCR_AIGroup group)
	{
		m_aGroupRplIDs.Insert(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
		m_aGroupLockedStatus.Insert(false);
		SCR_AIGroup newGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(group.GetFaction());
		newGroup.SetCanDeleteIfNoPlayer(false);
		newGroup.SetMaxMembers(12);
		m_aActivePlayerGroupsIDs.Insert(RplComponent.Cast(newGroup.FindComponent(RplComponent)).Id());
		Replication.BumpMe();
	}

	//Sets the group to be locked
	//------------------------------------------------------------------------------------------------
	void SetGroupLockedStatus(int index, bool input)
	{
		m_aGroupLockedStatus.Set(index, input);
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

	//------------------------------------------------------------------------------------------------
	// Moderator Functions/Variables
	//------------------------------------------------------------------------------------------------
	override void OnPlayerAuditSuccess(int playerId)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_Gamemode.GetInstance().InitilizePlayer(playerId);
		
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		
		if (!playerIdentity.IsEmpty() && CRF_ModeratorConfig.IsModerator(playerIdentity))
			GetGame().GetCallqueue().CallLater(SetPlayerModerator, 5000, false, playerId);
	};
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		//GetGame().GetPlayerManager().GivePlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	};
}
