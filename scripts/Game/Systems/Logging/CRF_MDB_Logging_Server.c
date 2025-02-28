[ComponentEditorProps(category: "CRF Logging Component", description: "")]
class CRF_MDB_LoggingServerComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_MDB_LoggingServerComponent: SCR_BaseGameModeComponent
{	
	string m_sMissionName;
	string m_sPlayerName;
	string m_sArmaGuid;
	string m_sPlatform;
	string m_sKillerName;
	
	int m_iPlayerCount;
	int m_iBluforCount;
	int m_iOpforCount;
	int m_iIndforCount;
	int m_iCivCount;
	
	SCR_FactionManager m_FM;
	EDF_DbRepository<CRF_PlayersModel> playerCollection;
	
	static CRF_MDB_LoggingServerComponent GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_MDB_LoggingServerComponent.Cast(gameMode.FindComponent(CRF_MDB_LoggingServerComponent));
		else
			return null;
	}
	
	// Setup
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (Replication.IsClient() || !GetGame().InPlayMode())
			return;

		m_sMissionName = GetGame().GetMissionName();
		CRF_Gamemode.GetInstance().GetOnStateChanged().Insert(OnGamemodeStateChanged);
		OnGamemodeStateChanged(CRF_GamemodeState.INITIAL);
	}
	
	// Player Connected
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (Replication.IsClient())
			return;
		
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		#ifdef WORKBENCH
			m_sArmaGuid = GetGame().GetBackendApi().GetLocalIdentityId();
		#else
			m_sArmaGuid = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		#endif
		
		m_sPlatform = GetGame().GetBackendApi().GetPlayerPlatformId(playerId);
		
		GetGame().GetCallqueue().CallLater(CreatePlayer, 1000, false);
	}
	
	// Player Disconnected 
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (Replication.IsClient())
			return;
		
		// Get player name
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
	}
	
	void OnGamemodeStateChanged(CRF_GamemodeState state)
	{
		//Print(state);
		/*if (CRF_Gamemode.GetInstance().GetGameModeState() == CRF_GamemodeState.AAR) // log stats only at AAR
		{
			//TODO: Implement data collector here by iterating through all players and only log data at end of game (AAR screen)
			//SCR_PlayerData playerData = GetGame().GetDataCollector().GetPlayerData(playerID, false);
		}*/
	}
	
	/*override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);
		
		// If killer has a playerId (they are a player)
		if (!GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(instigatorContextData.GetKillerEntity()) == 0)
		{
			// Then add a tvt_death to the killed
			
			
			// Add a tvt_kill to the killer
			
		} else {
			// Otherwise add a coop_death to the killed
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killer.GetInstigatorPlayerID());
			
		}
	}*/
	
	// Callbacks that actually do db operations
	// OnPlayerConnected: Create Player
	void CreatePlayerCallback(EDF_EDbOperationStatusCode statusCode, CRF_PlayersModel result)
    {
        //PrintFormat("[CRF] CreatePlayerCallback invoked! - StatusCode: %1", typename.EnumToString(EDF_EDbOperationStatusCode, statusCode));

		// Create a player if a player is not in the database
        if (!result) 
			playerCollection.AddOrUpdateAsync(CRF_PlayersModel.CreatePlayer(m_sPlayerName,m_sArmaGuid,m_sPlatform));        
    }
	
	void CreatePlayer()
	{
		// DB Params
		EDF_MongoDbConnectionInfo connectInfo();
		connectInfo.m_sDatabaseName = "reforger";
		connectInfo.m_sProxyHost = "localhost";
		connectInfo.m_iProxyPort = 8008;
		
		// Create DB context & repo for usage everywhere else
		EDF_DbContext dbContext = EDF_DbContext.Create(connectInfo);
		playerCollection = EDF_DbEntityHelper<CRF_PlayersModel>.GetRepository(dbContext);
		
		// Create a new player entry in db if one not created
		EDF_DbFindCallbackSingle<CRF_PlayersModel> cb(this, "CreatePlayerCallback");
		playerCollection.FindFirstAsync(EDF_DbFind.Field("armaguid").Contains(m_sArmaGuid), callback: cb);
	}
}

// THIS IS THE FUCKING MODEL, NOT A GOD DAMN ENTITY
[EDF_DbName("players")]
class CRF_PlayersModel : EDF_DbEntity
{
	string name;
	string armaguid;
	string platform;
	int tvt_kills;
	int tvt_deaths;
	int coop_kills;
	int coop_deaths;
	int missions_attended;
	int shots_fired;
	int grenades_thrown;
	int ff_events;
	int civs_killed;
	int leaves;
	int vehicle_kills;
	int heli_kills;
	int gi_roles;
	int leader_roles;
	int specialty_roles;
	int distance_walked;
	int distance_driver;
	int distance_rider;
	int med_bandage_self;
	int med_bandage_others;
	int med_tourn_self;
	int med_tourn_others;
	int med_saline_self;
	int med_saline_others;
	int med_morph_self;
	int med_morph_others;

    //------------------------------------------------------------------------------------------------
    static CRF_PlayersModel CreatePlayer(string playerName, string playerArmaGUID, string playerPlatform)
    {
        CRF_PlayersModel instance();
		instance.name = playerName;
		instance.armaguid = playerArmaGUID;
		instance.platform = playerPlatform;
		instance.tvt_kills = 0;
		instance.tvt_deaths = 0;
		instance.coop_kills = 0;
		instance.coop_deaths = 0;
		instance.missions_attended = 0;
		instance.shots_fired = 0;
		instance.grenades_thrown = 0;
		instance.ff_events = 0;
		instance.civs_killed = 0;
		instance.leaves = 0;
		instance.vehicle_kills = 0;
		instance.heli_kills = 0;
		instance.gi_roles = 0;
		instance.leader_roles = 0;
		instance.specialty_roles = 0;
		instance.distance_walked = 0;
		instance.distance_driver = 0;
		instance.distance_rider = 0;
		instance.med_bandage_self = 0;
		instance.med_bandage_others = 0;
		instance.med_tourn_self = 0;
		instance.med_tourn_others = 0;
		instance.med_saline_self = 0;
		instance.med_saline_others = 0;
		instance.med_morph_self = 0;
		instance.med_morph_others = 0;
		
        return instance;
    }
	
	static CRF_PlayersModel UpdatePlayerName(string playerName)
	{
		CRF_PlayersModel instance();
		instance.name = playerName;
		
		return instance;
	}
	
	static CRF_PlayersModel AddPlayerKill()
	{
		CRF_PlayersModel instance();
		instance.tvt_kills++;
		
		return instance;
	}
};