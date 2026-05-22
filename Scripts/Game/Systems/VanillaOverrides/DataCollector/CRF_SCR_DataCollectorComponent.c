// This class is here so we can unprotect many of the methods and use them in our data collection
// This is a requirement due to how we spawn players into entities rather than using the base invokers
// .....I hate it
modded class SCR_DataCollectorComponent
{
	CRF_LoggingManager LM;
	
	protected bool m_bCRFProfilesSaved = false;
	
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
		// Delegate to NotifyPlayerSpawned so that both the RequestSpawn pipeline and the
		// SetInitialMainEntity fallback path use a single, consistent notification point.
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
		
		// Remove any stale invokers from the entity before registering new ones.
		// We use CRF_CleanupInvokers (a thin public wrapper around the protected RemoveInvokers)
		// rather than OnPlayerDisconnected because some modules (e.g. HealingItemsModule) override
		// OnPlayerDisconnected to clear their internal entity-tracking map without calling
		// RemoveInvokers. Those modules rely on the map being intact inside their own OnPlayerSpawned
		// cleanup logic, so calling OnPlayerDisconnected first would break them.
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

		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			playerID = m_mPlayerData.GetKey(i);
			playerController = playerManager.GetPlayerController(playerID);
			if (!playerController)
				continue;

			communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(playerController.FindComponent(SCR_DataCollectorCommunicationComponent));
			if (!communicationComponent)
				continue;

			communicationComponent.SendData(m_mPlayerData.Get(playerID), factionKeys, factionValues, valuesSize);
		}
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
		
		// Skip if OnGameEnd() already issued a StoreProfile() for all connected players — firing
		// a second save while the first async transaction is still in flight causes the platform
		// error "Save data transaction to 'playersave' failed. Another transaction in progress."
		if (!m_bCRFProfilesSaved)
			playerDisconnectedData.StoreProfile();

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
	// When game shuts down, store the profile of every player who hasn't disconnected yet
	override void OnGameEnd()
	{
		// Mark that we have issued saves for all remaining players so that any subsequent
		// OnPlayerDisconnected() calls (e.g. players leaving the AAR screen) do not fire
		// a duplicate StoreProfile() while the async backend transaction is still pending.
		m_bCRFProfilesSaved = true;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			int playerID = m_mPlayerData.GetKey(i);
			
			// Only save players who are still connected. Players who disconnected before
			// AAR were already saved by OnPlayerDisconnected() — calling StoreProfile() a
			// second time while the first async transaction is still in flight triggers the
			// platform error "Save data transaction to 'playersave' failed. Another
			// transaction in progress."
			if (!playerManager.IsPlayerConnected(playerID))
				continue;
			
			SCR_PlayerData playerData = GetPlayerData(playerID, false);
			if (playerData)
				playerData.StoreProfile();
		}
	}
}