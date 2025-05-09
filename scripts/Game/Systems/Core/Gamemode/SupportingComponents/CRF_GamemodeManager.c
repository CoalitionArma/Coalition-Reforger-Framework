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
	protected CRF_AdminMenuManager m_AdminMenuManager;
	
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
		m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
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
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		SCR_ChimeraCharacter playerCharacter = null;
		bool isSpectator;
		Faction faction;
		
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			playerCharacter = SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"), GetGame().GetWorld()));
			
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			isSpectator = true;
			
			SCR_AIGroup currentGroup = m_GroupsManagerComponent.GetPlayerGroup(playerId);
			if (currentGroup)
				currentGroup.RemovePlayer(playerId);
			
			SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(playerCharacter.FindComponent(SCR_CharacterDamageManagerComponent)); 
			if (damManager)
				damManager.EnableDamageHandling(false);
		} else {
			playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
			
			if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
				playerCharacter = m_SlottingManager.SpawnPlayableEntity(playerId, overrideLocation);
			
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		}
		
		if (playerCharacter && playerController)
			playerController.SetInitialMainEntity(playerCharacter);

		if (faction && playerController)
			SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(faction);

		if(!isSpectator)
		{
			int groupId = m_SlottingManager.GetPlayerSlotGroup(playerId).GetGroupID();
			
			if (groupId != -1)
			{
				m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);
				SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
			};
		};
		
		m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, isSpectator);
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
