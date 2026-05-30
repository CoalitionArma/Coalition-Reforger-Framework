// This class is here so we can unprotect many of the methods and use them in our data collection
// This is a requirement due to how we spawn players into entities rather than using the base invokers
// .....I hate it
modded class SCR_DataCollectorComponent
{
	CRF_LoggingManager LM;

	// Per-session kill/death name tracking for the AAR stats panel.
	// Populated server-side in OnPlayerKilled; sent to each client at game end.
	protected ref map<int, ref array<string>> m_mSessionKills = new map<int, ref array<string>>();
	protected ref map<int, string> m_mSessionDeaths = new map<int, string>();

	// Staggered stat-send queue — spreads SendData RPCs so clients receive them gradually.
	protected ref array<int> m_aPendingStatSends = {};
	protected ref array<FactionKey> m_aCachedFactionKeys = {};
	protected ref array<float> m_aCachedFactionValues = {};
	protected int m_iCachedFactionValuesSize;

	// Staggered StoreProfile queue — serialises all platform save transactions.
	// The platform only allows one 'playersave' transaction at a time; firing
	// StoreProfile() simultaneously for multiple players causes:
	//   "Save data transaction to 'playersave' failed. Another transaction in progress."
	// All StoreProfile() calls are routed through QueueProfileSave() so they are
	// spaced 500 ms apart.  OnGameEnd() skips players already in this queue/saved
	// to eliminate the double-save that vanilla OnGameEnd() + OnPlayerDisconnected()
	// would otherwise produce.
	protected ref array<int> m_aPendingProfileSaves = {};
	protected ref array<int> m_aSavedOrQueuedPlayerIds = {};
	protected bool m_bProfileSaveProcessing = false;

	// Event for tracking damage
	protected ref ScriptInvoker m_OnPlayerDamageReceived = new ScriptInvoker();
	
	// Getter for the damage event invoker
	ScriptInvoker GetOnPlayerDamageReceived()
	{
		return m_OnPlayerDamageReceived;
	}
	
	protected override void OnPlayerAuditSuccess(int playerId)
	{
		//Print("[CRF] Player with id " + playerId + " was auditted succesfully and admitted on the Data Collector");
		//We create the player's PlayerData here
		GetPlayerData(playerId);

		//And then let the modules handle the newly connected player if they need to
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerAuditSuccess(playerId);
		}		
	}
	
	override SCR_PlayerData GetPlayerData(int playerID, bool createNew = true, bool requestFromBackend = true)
	{
		SCR_PlayerData playerData = m_mPlayerData.Get(playerID);
		if (!playerData && createNew)
		{
			playerData = new SCR_PlayerData(playerID, true, requestFromBackend);
			m_mPlayerData.Insert(playerID, playerData);
		}

		return playerData;
	}
	
	protected override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		// CRF's InitilizePlayer always uses SetInitialMainEntity and calls NotifyPlayerSpawned
		// directly, so this callback is NOT part of the normal CRF initialization path.
		// It remains here as a safety net for any vanilla RequestSpawn flows (e.g. if the
		// vanilla respawn menu is ever enabled) so that stats modules are still notified.
		NotifyPlayerSpawned(requestComponent.GetPlayerId(), entity);
	}
	
	//------------------------------------------------------------------------------------------------
	// Notifies all data-collector modules that a playable entity has been spawned for a player.
	// Called either from OnPlayerSpawnFinalize_S (RequestSpawn pipeline) or directly from
	// CRF_GamemodeManager.InitilizePlayer (SetInitialMainEntity fallback path).
	// Must never be called twice for the same spawn event — the caller is responsible for
	// ensuring exactly one call reaches this method per spawn.
	void NotifyPlayerSpawned(int playerId, IEntity entity)
	{
		if (!entity || playerId <= 0)
			return;

		// Skip spectator entities — they must never be fed into stat-tracking modules.
		if (CRF_EntityHelper.IsSpectator(entity))
			return;
		
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.CRF_CleanupInvokers(entity);
		}
		
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerSpawned(playerId, entity);
		}
	}
	
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnGameModeEnd();
		}
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		int playerID;
		PlayerController playerController;
		SCR_DataCollectorCommunicationComponent communicationComponent;

		// Here we add to the faction the scores of all the players who haven't disconnected yet.
		// Use CRF_SlottingManager to resolve the player's combat faction from their slot so that
		// spectators (whose controlled entity has faction "SPEC") are still attributed correctly.
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		Faction faction;

		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			playerID = m_mPlayerData.GetKey(i);

			// We update the duration of the session here because it should not be connected to any module
			m_mPlayerData.Get(playerID).CalculateSessionDuration();

			// Prefer slot faction (correct even when player is in spectator);
			// fall back to entity faction for unslotted players.
			if (slottingManager)
				faction = slottingManager.GetPlayerSlotFaction(playerID, true);

			if (!faction)
			{
				SCR_ChimeraCharacter playerChimera = SCR_ChimeraCharacter.Cast(playerManager.GetPlayerControlledEntity(playerID));
				if (playerChimera)
					faction = playerChimera.GetFaction();
			}

			if (!faction)
				continue;

			AddStatsToFaction(faction.GetFactionKey(), m_mPlayerData.Get(playerID).CalculateStatsDifference());
		}

		// We replicate the faction stats now, so they can be found in the client's machine
		array<FactionKey> factionKeys = {};
		array<float> factionValues = {};
		int valuesSize = 0;

		foreach (FactionKey key, array<float> value : m_mFactionScore)
		{
			factionKeys.Insert(key);
			factionValues.InsertAll(value);
			if (valuesSize == 0)
				valuesSize = value.Count();
		}

		// Build the staggered send queue — both SendData (triggers platform save) and
		// SendAARKillStats are dispatched together per player with a 500 ms gap between
		// each player to avoid "Another transaction in progress" platform errors.
		m_aCachedFactionKeys = factionKeys;
		m_aCachedFactionValues = factionValues;
		m_iCachedFactionValuesSize = valuesSize;
		m_aPendingStatSends.Clear();

		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			playerID = m_mPlayerData.GetKey(i);
			playerController = playerManager.GetPlayerController(playerID);
			if (!playerController)
				continue;

			communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(playerController.FindComponent(SCR_DataCollectorCommunicationComponent));
			if (!communicationComponent)
				continue;

			m_aPendingStatSends.Insert(playerID);

			// Queue a staggered platform save for every still-connected player.
			// This ensures profiles are saved here rather than in OnGameEnd()'s
			// safety-net, which would fire all StoreProfile() calls simultaneously.
			QueueProfileSave(playerID);
		}

		GetGame().GetCallqueue().CallLater(SendNextPlayerStats, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	// Processes one pending player stat send per call, then reschedules itself 500 ms later
	// until the queue is empty. The 500 ms gap prevents simultaneous platform save transactions.
	protected void SendNextPlayerStats()
	{
		if (m_aPendingStatSends.IsEmpty())
			return;

		int playerID = m_aPendingStatSends[0];
		m_aPendingStatSends.RemoveOrdered(0);

		PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerID);
		if (playerController)
		{
			SCR_DataCollectorCommunicationComponent communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(
				playerController.FindComponent(SCR_DataCollectorCommunicationComponent));
			if (communicationComponent)
				communicationComponent.SendData(m_mPlayerData.Get(playerID), m_aCachedFactionKeys, m_aCachedFactionValues, m_iCachedFactionValuesSize);

			CRF_PlayerRplToOwnerManager rplManager = CRF_PlayerRplToOwnerManager.Cast(
				playerController.FindComponent(CRF_PlayerRplToOwnerManager));
			if (rplManager)
			{
				array<string> kills = m_mSessionKills.Get(playerID);
				if (!kills)
					kills = new array<string>();

				string killedBy = "";
				if (m_mSessionDeaths.Contains(playerID))
					killedBy = m_mSessionDeaths.Get(playerID);

				rplManager.SendAARKillStats(kills, killedBy);
			}
		}

		if (!m_aPendingStatSends.IsEmpty())
			GetGame().GetCallqueue().CallLater(SendNextPlayerStats, 500, false);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle damage received by players to track weapons that cause damage
	// This is called from CRF_SCR_CharacterDamageManagerComponent
	// Must be accessible from other components
	void OnPlayerDamageReceived(int victimId, IEntity killerEntity, int damageType)
	{
		// Make sure our logging manager instance is available
		if (!LM)
			LM = CRF_LoggingManager.GetInstance();
			
		if (!LM)
			return;
			
		// Forward to the logging manager to track the weapon
		LM.PlayerTookDamage(victimId, killerEntity, damageType);
		
		// Notify any listeners
		m_OnPlayerDamageReceived.Invoke(victimId, killerEntity, damageType);
	}
	
	protected override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int playerId = instigatorContextData.GetVictimPlayerID();
		IEntity playerEntity = instigatorContextData.GetVictimEntity();
		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		Instigator instigator = instigatorContextData.GetInstigator();

		// Route non-player victims (possessed AI) through the AI kill path instead.
		// LogPlayerKill is intentionally placed AFTER this guard so it only fires
		// for real player deaths, not AI entities.
		if (playerId <= 0)
		{
			OnAIKilledCRF(playerEntity, killerEntity, instigator, instigatorContextData);
			return;
		}

		// Make sure our logging manager instance is available
		if (!LM)
			LM = CRF_LoggingManager.GetInstance();

		// Logging player kill to file
		if (LM)
			LM.LogPlayerKill(instigatorContextData);
			
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerKilled(playerId, playerEntity, killerEntity, instigator, instigatorContextData);
		}
		
		if (m_bOptionalKicking)
			m_OptionalKicking.OnControllableDestroyed(playerEntity, killerEntity, instigator, instigatorContextData);

		// Track which player killed whom, and who killed this player, for the AAR stats panel.
		int killerPlayerId = 0;
		if (instigator)
			killerPlayerId = instigator.GetInstigatorPlayerID();

		if (killerPlayerId > 0)
		{
			if (!m_mSessionKills.Contains(killerPlayerId))
				m_mSessionKills.Insert(killerPlayerId, new array<string>());
			m_mSessionKills.Get(killerPlayerId).Insert(GetGame().GetPlayerManager().GetPlayerName(playerId));
		}

		string killerName = "";
		if (killerPlayerId > 0)
			killerName = GetGame().GetPlayerManager().GetPlayerName(killerPlayerId);

		if (killerName != "")
		{
			if (m_mSessionDeaths.Contains(playerId))
				m_mSessionDeaths.Set(playerId, killerName);
			else
				m_mSessionDeaths.Insert(playerId, killerName);
		}
	}

	void OnAIKilledCRF(IEntity AIEntity, IEntity killerEntity, notnull Instigator instigator, notnull SCR_InstigatorContextData instigatorContextData)
	{
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnAIKilled(AIEntity, killerEntity, instigator, instigatorContextData);
		}
		
		if (m_bOptionalKicking)
			m_OptionalKicking.OnControllableDestroyed(AIEntity, killerEntity, instigator, instigatorContextData);
	}
	
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Get player data - let it be created if it doesn't exist (vanilla behavior)
		SCR_PlayerData playerDisconnectedData = GetPlayerData(playerId, false);
		
		// Notify all modules about disconnect first
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerDisconnected(playerId);
		}
		
		// Safety check: Player might disconnect before data was initialized
		// In vanilla, this can't happen because GetPlayerData creates it, but we use false to match their pattern
		if (!playerDisconnectedData)
			return;
		
		// Calculate session duration before storing
		playerDisconnectedData.CalculateSessionDuration();
		QueueProfileSave(playerId);

		// ADD STATS TO FACTION
		// Here we add the stats of the individual player who desconnected to the faction
		// We only do that if the game is not in POSTGAME state, because if it is we already added this player's stats to the faction in the OnGameModeEnd method

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() != SCR_EGameModeState.POSTGAME)
		{
			// Prefer slot faction so that a player who disconnected while in spectator is still
			// attributed to their combat faction rather than "SPEC".
			Faction faction = null;
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			if (slottingManager)
				faction = slottingManager.GetPlayerSlotFaction(playerId, true);

			if (!faction)
			{
				SCR_ChimeraCharacter playerChimera = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId));
				if (playerChimera)
					faction = playerChimera.GetFaction();
			}

			if (faction)
				AddStatsToFaction(faction.GetFactionKey(), playerDisconnectedData.CalculateStatsDifference());
		}

		// DONE ADDING STATS TO THE FACTION
		//We cannot remove this instance of data from the player collector because the event has not been sent yet to the Database for tracking purposes
		//m_mPlayerData.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	// Routes a StoreProfile() call through the staggered queue to prevent simultaneous platform
	// save transactions.  Safe to call multiple times for the same player — duplicates are ignored.
	protected void QueueProfileSave(int playerId)
	{
		if (m_aSavedOrQueuedPlayerIds.Contains(playerId))
			return;

		m_aSavedOrQueuedPlayerIds.Insert(playerId);
		m_aPendingProfileSaves.Insert(playerId);

		if (!m_bProfileSaveProcessing)
		{
			m_bProfileSaveProcessing = true;
			GetGame().GetCallqueue().CallLater(ProcessNextProfileSave, 0, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Pops one player from the save queue, calls StoreProfile(), and reschedules itself
	// 500 ms later until the queue is empty.
	protected void ProcessNextProfileSave()
	{
		if (m_aPendingProfileSaves.IsEmpty())
		{
			m_bProfileSaveProcessing = false;
			return;
		}

		int playerID = m_aPendingProfileSaves[0];
		m_aPendingProfileSaves.RemoveOrdered(0);

		SCR_PlayerData playerData = GetPlayerData(playerID, false);
		if (playerData)
			playerData.StoreProfile();

		if (!m_aPendingProfileSaves.IsEmpty())
			GetGame().GetCallqueue().CallLater(ProcessNextProfileSave, 500, false);
		else
			m_bProfileSaveProcessing = false;
	}

	//------------------------------------------------------------------------------------------------
	// Override OnGameEnd to prevent the vanilla double-save.
	// Vanilla SCR_DataCollectorComponent.OnGameEnd() iterates all of m_mPlayerData and calls
	// StoreProfile() for every entry — including players already saved (or queued to be saved)
	// by OnPlayerDisconnected().  We skip those and only synchronously save players whose
	// disconnect-save never ran (e.g. server force-killed before they cleanly disconnected).
	// The stagger queue's remaining entries are also drained here synchronously as a safety net,
	// since CallLater callbacks will not fire once the session is tearing down.
	override void OnGameEnd()
	{
		// Cancel pending staggered callbacks — we're tearing down now.
		GetGame().GetCallqueue().Remove(ProcessNextProfileSave);
		GetGame().GetCallqueue().Remove(SendNextPlayerStats);
		m_bProfileSaveProcessing = false;

		// Drain any profiles that were queued but not yet processed.
		foreach (int playerID : m_aPendingProfileSaves)
		{
			SCR_PlayerData playerData = GetPlayerData(playerID, false);
			if (playerData)
				playerData.StoreProfile();
		}
		m_aPendingProfileSaves.Clear();

		// Safety-net: save any players who never went through OnPlayerDisconnected at all.
		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			int playerID = m_mPlayerData.GetKey(i);
			if (m_aSavedOrQueuedPlayerIds.Contains(playerID))
				continue;

			SCR_PlayerData playerData = GetPlayerData(playerID, false);
			if (playerData)
				playerData.StoreProfile();
		}
	}

}