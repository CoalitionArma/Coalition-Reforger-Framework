modded class SCR_DataCollectorComponent
{
	override void OnPlayerAuditSuccess(int playerId)
	{
		Print("[CRF] Player with id " + playerId + " was auditted succesfully and admitted on the Data Collector");
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
		Print("[CRF] OnPlayerSpawned");
		foreach (SCR_DataCollectorModule module : m_aModules)
		{
			module.OnPlayerSpawned(playerId, controlledEntity);
		}
	}
	
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		Print("[CRF] OnGameModeEnd");
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
}