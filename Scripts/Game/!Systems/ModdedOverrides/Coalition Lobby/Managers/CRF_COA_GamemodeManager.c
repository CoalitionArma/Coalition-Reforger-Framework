modded class COA_GamemodeManager
{	
	//------------------------------------------------------------------------------------------------
	//! Initialize a player into the game either as a playable character or spectator
	//! \param[in] playerId ID of the player to initialize
	//! \param[in] spawnPointID the ID of the spawn point we want to spawn this player at (either set manually with the respawn screen or automatic if -1;
	//! \param[in] entityRplID the rplID of the entity  we want to spawn this player at (either set manually or automatic if invalid rpl id;
	override bool InitilizePlayer(int playerId, int spawnPointID = -1, RplId entityRplID = RplId.Invalid())
	{	
		if (playerId <= 0)
			return true;
		
		if (!EnsureManagersReady())
			return false;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return false;
			
		COA_PlayerCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = GetOrCreateSpectatorEntity(playerId, playerController);

			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");

			COA_InitializationHelper.RemovePlayerFromCurrentGroup(playerId);
		} else {
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnPointID, entityRplID, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			m_MenuManager.RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (!playerCharacter)
			return false;
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (!playerRplComp)
			return false;
		
		if (playerCharacter && playerRplComp)
		{
			playerCharacter.DisableAI();

			if (!COA_EntityHelper.IsSpectator(playerCharacter))
			{
				// Playable characters wait for their gear before being handed over
				// ScheduleAssignPlayerToCharacter owns assignment, faction, group and radios.
				ScheduleAssignPlayerToCharacter(playerCharacter, playerId, playerController, playerRplComp.Id(), 0);

				// Notify the CRF-native stats manager so it begins tracking this player.
				// Retry briefly in case component init/replication order delays availability.
				TryNotifyStatsManager(playerId, playerRplComp.Id(), 0);
			} else {
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);

				COA_InitializationHelper.AssignCharacterToPlayer(playerController, playerCharacter);
				COA_InitializationHelper.AssignFactionToPlayer(playerController, faction);

				m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
			};
		};
		return true;
	}


	//------------------------------------------------------------------------------------------------
	//! Resolve delayed stats-manager availability and delayed replicated-entity availability.
	protected void TryNotifyStatsManager(int playerId, RplId playerEntityRplId, int attempt)
	{
		if (playerId <= 0 || playerEntityRplId == RplId.Invalid())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		CRF_ServerStatsManager statsManager = CRF_ServerStatsManager.GetInstance();
		IEntity playerEntity = null;

		Managed replicatedItem = Replication.FindItem(playerEntityRplId);
		if (replicatedItem)
		{
			RplComponent playerRplComp = RplComponent.Cast(replicatedItem);
			if (playerRplComp)
				playerEntity = playerRplComp.GetEntity();
		}

		if (statsManager && playerEntity)
		{
			if (playerManager.GetPlayerControlledEntity(playerId) != playerEntity)
				return;

			statsManager.NotifyPlayerSpawned(playerId, playerEntity);
			return;
		}

		if (attempt + 1 >= STATS_TRACKING_INIT_MAX_RETRIES)
		{
			Print(string.Format("[COA_GamemodeManager] WARNING: Failed to initialize stats tracking for player %1 after %2 attempts", playerId, STATS_TRACKING_INIT_MAX_RETRIES), LogLevel.WARNING);
			return;
		}

		GetGame().GetCallqueue().CallLater(TryNotifyStatsManager, STATS_TRACKING_INIT_RETRY_DELAY_MS, false, playerId, playerEntityRplId, attempt + 1);
	}
}