class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	const static ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
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
	static bool IsSpectator(IEntity entity)
	{
		if(!entity)
			return false;
		
		return entity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsSpectator()
	{
		if (SCR_PlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;
		
		if (SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;

		return false;
	}

	//Called to enter the actual game, just puts the player into a slot or spectator.
	//------------------------------------------------------------------------------------------------
	void InitilizePlayer(int playerId, vector overrideLocation = vector.Zero)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		if (!slottingManager.IsPlayerInASlot(playerId) || slottingManager.IsPlayerConsideredDead(playerId)) 
		{
			if(!CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
				EnterSpectator(playerId);
			
			return;
		}

		IEntity playerCharacter = slottingManager.GetPlayerSlotCharacter(playerId);

		if (!playerCharacter)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			
			if(overrideLocation != vector.Zero)
				spawnParams.Transform[3] = overrideLocation;
			else
				slottingManager.GetPlayerSlotVector(playerId, spawnParams.Transform);
			
			playerCharacter = GetGame().SpawnEntityPrefab(Resource.Load(slottingManager.GetPlayerSlotResource(playerId)), GetGame().GetWorld(), spawnParams);
		
			slottingManager.UpdateSlotCharacter(slottingManager.GetPlayerSlotID(playerId), RplComponent.Cast(playerCharacter.FindComponent(RplComponent)).Id());
			
			CRF_PlayableCharacter playabeCharComp = CRF_PlayableCharacter.Cast(playerCharacter.FindComponent(CRF_PlayableCharacter));
			
			if(playabeCharComp)
				playabeCharComp.SetIsSlotSpawned();
		};

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		playerController.SetInitialMainEntity(playerCharacter);

		SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(slottingManager.GetPlayerSlotFaction(playerId));

		int groupId = slottingManager.GetPlayerSlotGroup(playerId).GetGroupID();

		if (groupId != -1)
		{
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
		if (damManager)
			damManager.EnableDamageHandling(false);
		
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		vector cameraPos[4];
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (CRF_SlottingManager.GetInstance().IsPlayerInASlot(playerId) && entity != null)
			{
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else
				cameraPos[3] = CRF_Gamemode.GetInstance().m_vGenericSpawn[3];
		} else
			cameraPos[3] = "0 10000 0";

		CRF_RplBroadcastManager.GetInstance().SendSpecClientInit(playerId, cameraPos);
	}
	
	//------------------------------------------------------------------------------------------------
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	};
	
	//------------------------------------------------------------------------------------------------
	void SetServerWorldTime(string input)
	{
		m_sServerWorldTime = input;
		
		Replication.BumpMe();
	};
	
	//------------------------------------------------------------------------------------------------
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = CRF_SafestartManager.GetInstance().m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		Replication.BumpMe();
	};

	//------------------------------------------------------------------------------------------------
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = CRF_SafestartManager.GetInstance().m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		};

		Replication.BumpMe();
	};
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		m_aModerators.Insert(playerId);
		Replication.BumpMe();
	};
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if given player is an moderator.
	\param playerId ID of queried player
	\return True when player with given ID is an moderator.
	*/
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if local player is an moderator.
	\return True when local player is a moderator.
	*/
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
}
