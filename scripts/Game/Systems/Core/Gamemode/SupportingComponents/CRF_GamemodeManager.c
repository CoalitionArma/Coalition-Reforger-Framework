class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	const static ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	
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
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		// Get all managers we need for this manager
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
	};
	
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
		if (SCR_PlayerController.GetLocalMainEntity() && SCR_PlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;
		
		if (SCR_PlayerController.GetLocalControlledEntity() && SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;

		return false;
	}

	//Called to enter the actual game, just puts the player into a slot or spectator.
	//------------------------------------------------------------------------------------------------
	void InitilizePlayer(int playerId, vector overrideLocation = vector.Zero)
	{
		if (playerId <= 0)
			return;
		
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId)) 
		{
			 IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!IsSpectator(playerEntity))
				EnterSpectator(playerId);
			else if (IsSpectator(playerEntity))
			{
				vector cameraPos[4];
				cameraPos[3] = m_Gamemode.m_vGenericSpawn[3];
				m_RplBroadcastManager.SendSpecClientInit(playerId, cameraPos);
			};
			
			return;
		}

		IEntity playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);

		if (!playerCharacter)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			
			if(overrideLocation != vector.Zero)
				spawnParams.Transform[3] = overrideLocation;
			else
				m_SlottingManager.GetPlayerSlotVector(playerId, spawnParams.Transform);
			
			playerCharacter = GetGame().SpawnEntityPrefab(Resource.Load(m_SlottingManager.GetPlayerSlotResource(playerId)), GetGame().GetWorld(), spawnParams);
		
			m_SlottingManager.UpdateSlotCharacter(m_SlottingManager.GetPlayerSlotID(playerId), RplComponent.Cast(playerCharacter.FindComponent(RplComponent)).Id());
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), false);
			
			CRF_PlayableCharacter playabeCharComp = CRF_PlayableCharacter.Cast(playerCharacter.FindComponent(CRF_PlayableCharacter));
			
			if(playabeCharComp)
				playabeCharComp.SetIsSlotSpawned();
		};

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		playerController.SetInitialMainEntity(playerCharacter);

		SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(m_SlottingManager.GetPlayerSlotFaction(playerId));

		int groupId = m_SlottingManager.GetPlayerSlotGroup(playerId).GetGroupID();

		if (groupId != -1)
		{
			m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);
			SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
		}

		m_RplBroadcastManager.InitilizePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void EnterSpectator(int playerId, IEntity entity = null)
	{
		IEntity specEntity = GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"), GetGame().GetWorld());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		GetGame().GetCallqueue().CallLater(pc.SetInitialMainEntity, 250, false, specEntity);

		SCR_AIGroup currentGroup = m_GroupsManagerComponent.GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
		
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(specEntity.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if (damManager)
			damManager.EnableDamageHandling(false);
		
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		vector cameraPos[4];
		m_Gamemode = CRF_Gamemode.GetInstance();
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (m_SlottingManager.IsPlayerInASlot(playerId) && entity != null)
			{
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else {
				cameraPos[3] = m_Gamemode.m_vGenericSpawn[3];
			}
		} else {
			cameraPos[3] = "0 10000 0";
		}
		
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_RplBroadcastManager.SendSpecClientInit(playerId, cameraPos);
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
		float millis = m_SafestartManager.m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		Replication.BumpMe();
	};

	//------------------------------------------------------------------------------------------------
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeMissionEnds - currentTime;
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
