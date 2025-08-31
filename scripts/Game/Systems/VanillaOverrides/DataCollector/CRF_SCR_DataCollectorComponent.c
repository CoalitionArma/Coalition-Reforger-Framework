// This class is here so we can unprotect many of the methods and use them in our data collection
// This is a requirement due to how we spawn players into entities rather than using the base invokers
// .....I hate it
modded class SCR_DataCollectorComponent
{
	CRF_LoggingManager LM;
	
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
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerSpawned(playerId, controlledEntity);
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
	
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int playerId = instigatorContextData.GetVictimPlayerID();
		IEntity playerEntity = instigatorContextData.GetVictimEntity();
		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		Instigator instigator = instigatorContextData.GetInstigator();
		
		// Logging player kill to file
		LM.GetInstance().LogPlayerKill(instigatorContextData);
		
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
		SCR_PlayerData playerDisconnectedData = GetPlayerData(playerId);
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerDisconnected(playerId);
		}

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
}