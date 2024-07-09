[EDF_DbName.Automatic()]
class CRF_PersistentInfo : EDF_DbEntity
{
    float m_fNumber;
    string m_sText;

    //------------------------------------------------------------------------------------------------
    //! Db entities can not have a constructor with parameters, this is a limitation of the engine.
    //! Consult the docs for more info on this.
    static CRF_PersistentInfo Create(float number, string text)
    {
        CRF_PersistentInfo instance();
        instance.m_fNumber = number;
        instance.m_sText = text;
        return instance;
    }
};

[ComponentEditorProps(category: "CRF Logging Component", description: "")]
class CRF_LoggingSQLComponentClass: ScriptComponentClass {}

class CRF_LoggingSQLComponent: ScriptComponent
{
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static CRF_LoggingSQLComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_LoggingSQLComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_LoggingSQLComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------

	// Functions for stats replication
	
	//------------------------------------------------------------------------------------------------
	
	// !SHOULD ONLY BE RUN ON DEDICATED SERVER!
	void UpdateStats()
	{	
		if(!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated)
			return;
		
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(outPlayers);
		
		foreach(int playerID : outPlayers)
		{
			string playerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerID);
			
			// Get the connection info as an attribute or parse it from CLI params etc.
        	EDF_JsonFileDbConnectionInfo connectInfo();
        	connectInfo.m_sDatabaseName = playerGUID;

        	// Get a db context instance and save it somewhere to re-use in e.g. a singleton
        	EDF_DbContext dbContext = EDF_DbContext.Create(connectInfo);

        	// For convenience interact with the DB context through a repository
        	EDF_DbRepository<CRF_PersistentInfo> repository = EDF_DbEntityHelper<CRF_PersistentInfo>.GetRepository(dbContext);
			
			array<float> playerStats = GetGame().GetDataCollector().GetPlayerData(playerID).GetStats();
			
			array<float> playerStatsToUpdate = {
				playerStats[2],  //SESSION_DURATION
				playerStats[7],  //DISTANCE_WALKED (m)
				playerStats[8],  //KILLS
				playerStats[9],  //AI_KILLS
				playerStats[10], //SHOTS
				playerStats[11], //GRENADES_THROWN
				playerStats[12], //FRIENDLY_KILLS
				playerStats[14], //DEATHS
				playerStats[15], //DISTANCE_DRIVEN (m)
				playerStats[16], //PLAYERS_DIED_IN_VEHICLE
				playerStats[17], //ROADKILLS
				playerStats[18], //FRIENDLY_ROADKILLS
				playerStats[19], //AI_ROADKILLS
				playerStats[21], //DISTANCE_AS_OCCUPANT
				playerStats[22], //BANDAGE_SELF
				playerStats[23], //BANDAGE_FRIENDLIES
				playerStats[24], //TOURNIQUET_SELF
				playerStats[25], //TOURNIQUET_FRIENDLIES
				playerStats[26], //SALINE_SELF
				playerStats[27], //SALINE_FRIENDLIES
				playerStats[28], //MORPHINE_SELF
				playerStats[29], //MORPHINE_FRIENDLIES
			};
			
			//debugging
			Print(playerStatsToUpdate);
			
			foreach(int i, float stat : playerStatsToUpdate) {
				repository.AddOrUpdateAsync(CRF_PersistentInfo.Create(i, stat.ToString()));
			};
		};
	}
}

/*
enum SCR_EDataStats
{
	RANK,//!< Rank of the player
	LEVEL_EXPERIENCE, //!< XP points
	SESSION_DURATION, //!< Total duration of sessions
	SPPOINTS0, //!< INFANTRY
	SPPOINTS1, //!< LOGISTICS
	SPPOINTS2, //!< MEDICAL
	WARCRIMES, //!< War criminal points
	DISTANCE_WALKED, //!< Distance walked on foot
	KILLS, //!< Enemies killed
	AI_KILLS, //!< AI enemies killed
	SHOTS, //!< Bullets shot
	GRENADES_THROWN, //!< Grenades thrown
	FRIENDLY_KILLS, //!< Friendly human kills
	FRIENDLY_AI_KILLS, //!< Friendly AI kills
	DEATHS, //!< Deaths
	DISTANCE_DRIVEN, //!< Distance driven with a vehicle
	POINTS_AS_DRIVER_OF_PLAYERS, //!< Points obtained for driving players around
	PLAYERS_DIED_IN_VEHICLE, //!< Players that died inside a vehicle while the player drives it
	ROADKILLS, //!< Human enemies killed with a vehicle
	FRIENDLY_ROADKILLS, //!< Human friendlies killed with a vehicle
	AI_ROADKILLS, //!< AI enemies killed with a vehicle
	FRIENDLY_AI_ROADKILLS, //!< AI friendlies killed with a vehicle
	DISTANCE_AS_OCCUPANT, //!< Distance the player was driven around with a vehicle
	BANDAGE_SELF, //!< Number of times player bandaged themself
	BANDAGE_FRIENDLIES, //!< Number of times player bandaged friendlies (not including themself)
	TOURNIQUET_SELF, //!< Number of times player tourniquetted themself
	TOURNIQUET_FRIENDLIES, //!< Number of times player tourniquetted friendlies (not including themself)
	SALINE_SELF, //!< Number of times player salined themself
	SALINE_FRIENDLIES, //!< Number of times player salined friendlies (not including themself)
	MORPHINE_SELF, //!< Number of times player morphined themself
	MORPHINE_FRIENDLIES, //!< Number of times player morphined friendlies (not including themself)
	WARCRIME_HARMING_FRIENDLIES, //!< Harming friendlies
	CRIME_ACCELERATION, //!< Kick & Ban acceleration
	KICK_SESSION_DURATION, //!< Session duration at the time player was kicked the last time
	KICK_STREAK, //!< How many times was the player kicked in a row after last kick in a short succession
	LIGHTBAN_SESSION_DURATION, //!< Session duration at the time player was lightbanned the last time
	LIGHTBAN_STREAK, //!< How many times was the player lightbanned in a row after the last lightban in a short succession
	HEAVYBAN_SESSION_DURATION, //!< Session duration at the time player was heavybanned the last time
	HEAVYBAN_STREAK //!< How many times was the player heavybanned in a row after the last heavyban in a short succession
};
*/