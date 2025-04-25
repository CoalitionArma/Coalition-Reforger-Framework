class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	
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
		
		// Early return conditions
		if (GetGame().InPlayMode() && RplSession.Mode() != RplMode.Client && entity && entity.GetPrefabData())
			// Schedule gear setup with delay
			GetGame().GetCallqueue().CallLater(CRF_GearscriptManager.GetInstance().SetupAddGearToEntity, 250, false, entity, entity.GetPrefabData().GetPrefabName());
		
		GetGame().GetCallqueue().CallLater(LogCharacter, 500, false, entity);
	}

	//------------------------------------------------------------------------------------------------
	void LogCharacter(IEntity entity)
	{
		if (RplSession.Mode() != RplMode.Client)
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
	}

	//Called to enter the actual game, just puts the player into a slot or spectator.
	//------------------------------------------------------------------------------------------------
	void InitilizePlayer(int playerId)
	{
		if (m_aSlots.Find(playerId) == -1 
			|| GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" 
			|| m_aEntityDeathStatus.Get(m_aSlots.Find(playerId))) {
				EnterSpectator(playerId);
				return;
		}

		// WHAT THE FUCK IS THISSSSSSSSSSSSSSS
		RplId oldGroup = RplId.Invalid();
		if (GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			oldGroup = m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aEntitySlots.Find(RplComponent.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).FindComponent(RplComponent)).Id()))));

		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId)).SetInitialMainEntity(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(m_aSlots.Find(playerId)))).GetEntity());

		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))).GetEntity()).GetFaction());

		int groupId = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))).GetEntity()).GetGroupID();

		if (oldGroup != RplId.Invalid())
		{
			if (oldGroup != m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))
			{
				SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
				SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
			}
		} else {
			SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
			SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
		}
		
		CRF_RplBroadcastManager.GetInstance().InitilizePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void EnterSpectator(int playerId, IEntity entity = null)
	{
		IEntity initialEntity = GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"), GetGame().GetWorld());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		GetGame().GetCallqueue().CallLater(pc.SetInitialMainEntity, 250, false, initialEntity);

		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
		
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(initialEntity.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if(damManager)
			damManager.EnableDamageHandling(false);
		
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		vector cameraPos[4];
		if (m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (m_aSlots.Find(playerId) != -1 && entity != null)
			{
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else
				cameraPos[3] = m_vGenericSpawn[3];
		} else
			cameraPos[3] = "0 10000 0";

		CRF_RplBroadcastManager.GetInstance().SendSpecClientInit(playerId, cameraPos);
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
