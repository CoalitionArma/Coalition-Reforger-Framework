// This class is here so we can unprotect many of the methods and use them in our data collection
// This is a requirement due to how we spawn players into entities rather than using the base invokers
// .....I hate it
modded class SCR_DataCollectorComponent
{
	CRF_LoggingManager LM;
	
	// Event for tracking damage
	protected ref ScriptInvoker m_OnPlayerDamageReceived = new ScriptInvoker();
	
	// Getter for the damage event invoker
	ScriptInvoker GetOnPlayerDamageReceived()
	{
		return m_OnPlayerDamageReceived;
	}
	
	override void OnPlayerAuditSuccess(int playerId)
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
	
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		int playerId = requestComponent.GetPlayerId();
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerSpawned(playerId, entity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method for custom spawn systems that don't use the spawn request system
	// Call this when spawning players through custom methods to ensure data collector modules are notified
	void NotifyPlayerSpawned(int playerId, IEntity entity)
	{
		if (!entity || playerId <= 0)
			return;
		
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

		// Here we add to the faction the scores of all the players who haven't disconnected yet
		SCR_ChimeraCharacter playerChimera;
		Faction faction;

		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			playerID = m_mPlayerData.GetKey(i);

			// We update the duration of the session here because it should not be connected to any module
			m_mPlayerData.Get(playerID).CalculateSessionDuration();

			playerChimera = SCR_ChimeraCharacter.Cast(playerManager.GetPlayerControlledEntity(playerID));
			if (!playerChimera)
				continue;

			faction = playerChimera.GetFaction();

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
	
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int playerId = instigatorContextData.GetVictimPlayerID();
		IEntity playerEntity = instigatorContextData.GetVictimEntity();
		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		Instigator instigator = instigatorContextData.GetInstigator();
		
		// Make sure our logging manager instance is available
		if (!LM)
			LM = CRF_LoggingManager.GetInstance();
		
		// Logging player kill to file
		if (LM)
			LM.LogPlayerKill(instigatorContextData);
		
		if (instigatorContextData.GetVictimPlayerID() <= 0) {
			OnAIKilledCRF(playerEntity,killerEntity,instigator,instigatorContextData);
			return;
		}
			
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
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Get player data without creating new instance
		SCR_PlayerData playerDisconnectedData = GetPlayerData(playerId, false);
		
		// Notify all modules about disconnect first
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerDisconnected(playerId);
		}
		
		// Safety check: Player might disconnect before data was initialized
		if (!playerDisconnectedData)
			return;
		
		// Save player profile to backend IMMEDIATELY after modules are notified
		// This must happen before any calculations to ensure data is saved
		playerDisconnectedData.StoreProfile();

		// ADD STATS TO FACTION
		// Here we add the stats of the individual player who desconnected to the faction
		// We only do that if the game is not in POSTGAME state, because if it is we already added this player's stats to the faction in the OnGameModeEnd method

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode.GetState() != SCR_EGameModeState.POSTGAME)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			SCR_ChimeraCharacter playerChimera = SCR_ChimeraCharacter.Cast(player);
			if (playerChimera)
			{
				Faction faction = playerChimera.GetFaction();
				if (faction)
					AddStatsToFaction(faction.GetFactionKey(), playerDisconnectedData.CalculateStatsDifference());
			}
		}

		// DONE ADDING STATS TO THE FACTION
		//We cannot remove this instance of data from the player collector because the event has not been sent yet to the Database for tracking purposes
		//m_mPlayerData.Remove(playerId);

		//As an alternative, in GetPlayerDataStats we put this instance to be removed after its used in C++
	}
	
	//------------------------------------------------------------------------------------------------
	// When game shuts down, store the profile of every player who hasn't disconnected yet
	override void OnGameEnd()
	{	
		for (int i = m_mPlayerData.Count() - 1; i >= 0; i--)
		{
			SCR_PlayerData playerData = GetPlayerData(m_mPlayerData.GetKey(i), false);
			if (playerData)
				playerData.StoreProfile();
		}
	}
}