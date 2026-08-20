modded class COA_GamemodeManager
{
	protected const int STATS_TRACKING_INIT_RETRY_DELAY_MS = 250;
	protected const int STATS_TRACKING_INIT_MAX_RETRIES = 20;

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
		
		// GM possession slots are handled entirely by the base class now (COALITION-Lobby owns
		// COA_GMPossessionManager/COA_SlottingManager.RegisterGMPossessionGroup) this override is a
		// full carbon copy for everything else and doesn't call super for the rest of the method, so
		// possession slots need to be peeled off here and handed to super explicitly, or the base
		// class's possession handling would never run for CRF servers. The stats-tracking hook is
		// CRF-only, so it's re-attached here as a follow-up rather than living in the base class.
		int gmSlotId = m_SlottingManager.GetPlayerSlotID(playerId);
		RplId possessionTargetId;
		if (gmSlotId >= 0 && COA_GMPossessionManager.GetInstance().TryGetPossessionTarget(gmSlotId, possessionTargetId))
		{
			bool initialized = super.InitilizePlayer(playerId, spawnPointID, entityRplID);
			if (initialized)
			{
				// Re-resolve rather than trusting possessionTargetId as-is: if the target body had
				// already died, the base class falls back to a normal role-based spawn instead, and
				// the entity the player actually ended up controlling won't be possessionTargetId.
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				RplComponent controlledRplComp;
				if (controlledEntity)
					controlledRplComp = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
				if (controlledRplComp)
					TryNotifyStatsManager(playerId, controlledRplComp.Id(), 0);
			}
			return initialized;
		}

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
			
			COA_PlayerHelper.RemovePlayerFromCurrentGroup(playerId);
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
			// NOTE: this method is a full override of COA_GamemodeManager.InitilizePlayer rather than
			// an extension, so changes to the base version do not reach here. Keep it in sync.

			// GEAR IS APPLIED AFTER POSSESSION, NOT BEFORE. Do not "fix" this ordering.
			// Equipping the character before handover left players unable to reload: the weapons and
			// magazines were inserted while the client did not yet own the entity. Vanilla equips
			// after assignment too - SCR_SpawnHandlerComponent runs AssignEntity_S (line 156) before
			// OnPlayerSpawnFinalize_S (line 160), and the loadout hook OnLoadoutSpawned lives in the
			// latter. The gearscript runs from COA_GearscriptCharacter.EOnInit's deferred call, which
			// lands after this function hands the character over.
			COA_PlayerHelper.AssignFactionToPlayer(playerController, faction);

			// DisableAI() removed: SCR_PlayerController.SetInitialMainEntity() already calls
			// SetAIActivation(entity, false), and doing it here ran before ownership transfer.
			COA_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);

			if (!COA_EntityHelper.IsSpectator(playerCharacter))
			{
				// Radios are handled by SetEntityGear() once the gearscript has been applied to a
				// character the player already controls.
				AssignPlayerToGroup(playerId);

				// Notify the CRF-native stats manager so it begins tracking this player.
				// Retry briefly in case component init/replication order delays availability.
				TryNotifyStatsManager(playerId, playerRplComp.Id(), 0);
			}
			else
			{
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);
			}

			m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
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