/*
* Logging component for COALITION games
* Component overrides base game mode so it always runs
*
* Note that write files are formatted for parsing by an external program
* which splits strings via commas
*
* Server only
*/
[ComponentEditorProps(category: "CRF Logging Component", description: "Handles logging for server events")]
class CRF_LoggingManagerClass: SCR_BaseGameModeComponentClass
{
	// Class definition for the component class
}

class CRF_LoggingManager: SCR_BaseGameModeComponent
{    
	// Constants
	const string SEPARATOR = ",";
	const string LOG_PATH = "$profile:COAServerLog.txt";
	
	// Mission and player data
	private string m_sMissionName;
	private string m_sPlayerName;
	private string m_sAuthorName;
	private string m_sMaxPlayers;
	private string m_sMissionDetails;
	private string m_sGameMode;
	private string m_sPlayerGUID;
	private string m_sTerrain;
	
	// Kill data
	string m_sKillerName;
	string m_sKillerGUID;
	string m_sKillerFaction;
	string m_sVictimName;
	string m_sVictimGUID;
	string m_sVictimFaction;
	string m_sTime;
	string m_sWeaponName;
	float m_fRange;
	float m_fTotalTime;
	int m_iTotalSeconds;
	
	// Player counts
	private int m_iPlayerCount;
	private string m_sPlayerCountMax;
	private int m_iBluforCount;
	private int m_iOpforCount;
	private int m_iIndforCount;
	private int m_iCivCount;
	private ref array<int> m_aSideCounts = new array<int>;
	
	// File handling and faction management
	private ref FileHandle m_LogFileHandle;
	private SCR_FactionManager m_FactionManager;
	private BaseWeaponManagerComponent m_WMC;
	private SCR_ChimeraCharacter m_PlayerChimera;
	private PlayerManager m_PlayerManager;
	private SCR_CharacterInventoryStorageComponent m_Inventory;
	private BaseWeaponComponent m_BWC;
	private FactionManager m_FM;
	private SCR_FactionManager m_SFM;
	private Faction m_Faction;
	private CRF_Gamemode m_GM;
	
	// Singleton instance
	private static CRF_LoggingManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	// Constructor
	void CRF_LoggingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	// Singleton instance getter
	static CRF_LoggingManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Return the file handle for external use
	FileHandle GetLogFileHandle()
	{
		return m_LogFileHandle;
	}
	
	//------------------------------------------------------------------------------------------------
	// Component initialization
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server in play mode
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		if (!GetGame().InPlayMode())
			return;
		
		m_PlayerManager = GetGame().GetPlayerManager();
		m_GM = CRF_Gamemode.GetInstance();
		
		UpdatePlayerCount();
		InitializeLogging();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize logging system
	private void InitializeLogging()
	{
		// Initialize mission data
		m_sMissionName = GetGame().GetMissionName();
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		//if (m_iPlayerCount < 9)
			//return;
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		m_sAuthorName = header.m_sAuthor;
		m_sMaxPlayers = header.m_iPlayerCount.ToString();
		m_sMissionDetails = header.m_sDetails;
		m_sGameMode = header.m_sGameMode;
		m_sTerrain = header.m_sTerrainName;
		m_sMissionName = m_sMissionName + " (" + m_sTerrain + ")"; // append terrain onto mission name due to constraints
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers; // same here
		
		// Open global log file
		m_LogFileHandle = FileIO.OpenFile(LOG_PATH, FileMode.APPEND);
		
		// Log mission beginning
		LogMissionEvent("beginning");
		
		// Register for gamemode state changes
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode)
			gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle player connection events
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sPlayerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("connect" + SEPARATOR + m_sPlayerName + SEPARATOR + m_sPlayerGUID);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle player disconnection events
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("disconnect" + SEPARATOR + m_sPlayerName + SEPARATOR + cause);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle gamemode state changes
	void OnGamemodeStateChanged()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		UpdatePlayerCount();
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return;
		
		switch (gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.SLOTTING:
			{
				LogMissionEvent("slotting");
				break;
			}
			case CRF_EGamemodeState.GAME:
			{
				LogMissionEvent("safestart");
				break;
			}
			case CRF_EGamemodeState.AAR:
			{
				LogMissionEvent("ended");
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle game mode end
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		// Close file handle to prevent memory leaks
		if (m_LogFileHandle)
		{
			m_LogFileHandle.Close();
			m_LogFileHandle = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Method called from safestart to annotate game start
	void GameStarted()
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		UpdatePlayerCount();
		LogMissionEvent("started"); // global log
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;

		Attendance(); // Attendance log
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to update player count
	private void UpdatePlayerCount()
	{
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers;
		
		m_FM = GetGame().GetFactionManager();
		m_SFM = SCR_FactionManager.Cast(m_FM);
		m_Faction = m_FM.GetFactionByKey("BLUFOR");
		m_iBluforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("OPFOR");
		m_iOpforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("INDFOR");
		m_iIndforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("CIV");
		m_iCivCount = m_SFM.GetFactionPlayerCount(m_Faction);
		
		if (m_aSideCounts.IsEmpty()) {
			m_aSideCounts.Insert(m_iBluforCount);
			m_aSideCounts.Insert(m_iOpforCount);
			m_aSideCounts.Insert(m_iIndforCount);
			m_aSideCounts.Insert(m_iCivCount);
		} else {
			m_aSideCounts.Clear();
			m_aSideCounts.Insert(m_iBluforCount);
			m_aSideCounts.Insert(m_iOpforCount);
			m_aSideCounts.Insert(m_iIndforCount);
			m_aSideCounts.Insert(m_iCivCount);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to log mission events with consistent format
	private void LogMissionEvent(string eventType)
	{
		if (!m_LogFileHandle)
			return;
		
		m_LogFileHandle.WriteLine("mission" + SEPARATOR + eventType + SEPARATOR + m_sMissionName + SEPARATOR + m_sPlayerCountMax + SEPARATOR + m_sAuthorName + SEPARATOR + m_sMissionDetails + SEPARATOR + m_sGameMode + SEPARATOR + m_aSideCounts);
	}
	
	private void Attendance()
	{
		if (!m_LogFileHandle)
			return;
		
		// Log players in attendance
		array<string> playersInAttendance = {};
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int player : players)
		{
			playersInAttendance.Insert(GetGame().GetBackendApi().GetPlayerIdentityId(player)); // array of guids we parse server-side
		}
		
		m_LogFileHandle.WriteLine("attendance" + SEPARATOR + playersInAttendance);
	}
	
	// Logs player death and kill data to file
	void LogPlayerKill(SCR_InstigatorContextData instiContext)
	{
		if (!m_LogFileHandle)
			return;
		
		if (m_GM.m_GamemodeState != CRF_EGamemodeState.GAME) // ignore aar deaths
			return;
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;
		
		// Victim info
		m_PlayerChimera = SCR_ChimeraCharacter.Cast(m_PlayerManager.GetPlayerControlledEntity(instiContext.GetVictimPlayerID()));
		m_sVictimFaction = m_PlayerChimera.GetFactionKey();
		m_sVictimGUID = GetGame().GetBackendApi().GetPlayerIdentityId(instiContext.GetVictimPlayerID());
		if (instiContext.GetVictimPlayerID() > 0) // if it's a player 
			m_sVictimName = GetGame().GetPlayerManager().GetPlayerName(instiContext.GetVictimPlayerID());
		else 
			m_sVictimName = "AI";
		m_sVictimName = m_sVictimName + "(" + m_sVictimFaction + ")"; // we append the faction here due to compiler constraints
		
		// Killer info
		m_PlayerChimera = SCR_ChimeraCharacter.Cast(m_PlayerManager.GetPlayerControlledEntity(instiContext.GetKillerPlayerID()));
		m_sKillerFaction = m_PlayerChimera.GetFactionKey();
		m_sKillerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(instiContext.GetKillerPlayerID());
		if (instiContext.GetKillerPlayerID() > 0) // if it's a player and ignore aar killings
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(instiContext.GetKillerPlayerID());
		else
			m_sKillerName = "AI";
		m_sKillerName = m_sKillerName + "(" + m_sKillerFaction + ")"; // we append the faction here due to compiler constraints
		
		// Killer weapon info
		// Old way
		/*m_WMC = BaseWeaponManagerComponent.Cast(instiContext.GetKillerEntity().FindComponent(BaseWeaponManagerComponent));
		m_sWeaponName = string.Format(m_WMC.GetCurrentWeapon().GetUIInfo().GetName());	
		if (m_sWeaponName == "")
			m_sWeaponName = "Unknown Weapon";*/
		
		// New way - accounts for character being in turret
		m_Inventory = SCR_CharacterInventoryStorageComponent.Cast(instiContext.GetKillerEntity().FindComponent(SCR_CharacterInventoryStorageComponent));
		m_BWC = m_Inventory.GetCurrentCharacterWeapon();
		m_sWeaponName = string.Format(m_BWC.GetUIInfo().GetName());
		
		// Range
		m_fRange = vector.Distance(instiContext.GetVictimEntity().GetOrigin(),instiContext.GetKillerEntity().GetOrigin());
		
		// Time
		m_fTotalTime = GetGame().GetWorld().GetWorldTime();
  		m_iTotalSeconds = (m_fTotalTime / 1000);
		m_sTime = SCR_FormatHelper.FormatTime(m_iTotalSeconds);
		
		// Log to file
		m_LogFileHandle.WriteLine("kill" + SEPARATOR + m_sVictimName + SEPARATOR + m_sVictimGUID + SEPARATOR + m_sKillerName + SEPARATOR + m_sKillerGUID + SEPARATOR + m_sWeaponName + SEPARATOR + m_fRange + SEPARATOR + m_sTime);
	}
	
	// TODO: Implement these on EH where grenade is thrown
	/*void LogGrenadeThrown()
	{
		if (!m_LogFileHandle)
			return;
		
		// TODO: Add username and guid to both loggers
		m_LogFileHandle.WriteLine("grenade" + SEPARATOR + );
	}*/
	
	void LogShots()
	{
		// This should be pulled from the datacollector IMO, once per player, at end of game (enterAAR()).
	}
}