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
	private string m_MissionName;
	private string m_PlayerName;
	private string m_AuthorName;
	private string m_MaxPlayers;
	private string m_MissionDetails;
	private string m_GameMode;
	
	// Kill data
	string m_sKillerName;
	string m_sKillerGUID;
	//string m_sKillerFaction;
	string m_sVictimName;
	string m_sVictimGUID;
	//string m_sKilledFaction;
	string m_sTime;
	string m_sWeaponName;
	float m_fRange;
	float m_fTotalTime;
	int m_iTotalSeconds;
	
	// Player counts
	private int m_PlayerCount;
	private int m_BluforCount;
	private int m_OpforCount;
	private int m_IndforCount;
	
	// File handling and faction management
	private ref FileHandle m_LogFileHandle;
	private SCR_FactionManager m_FactionManager;
	private BaseWeaponManagerComponent m_WMC;
	
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
		
		InitializeLogging();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize logging system
	private void InitializeLogging()
	{
		// Initialize mission data
		m_MissionName = GetGame().GetMissionName();
		m_PlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		m_AuthorName = header.m_sAuthor;
		m_MaxPlayers = header.m_iPlayerCount.ToString();
		m_MissionDetails = header.m_sDetails;
		m_GameMode = header.m_sGameMode;
		
		// Open log file
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
		
		m_PlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("connect" + SEPARATOR + m_PlayerName);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle player disconnection events
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_PlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("disconnect" + SEPARATOR + m_PlayerName + SEPARATOR + cause);
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
		LogMissionEvent("started");
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to update player count
	private void UpdatePlayerCount()
	{
		m_PlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
		// TODO: Implement faction-specific player counts
		// m_BluforCount = ...
		// m_OpforCount = ...
		// m_IndforCount = ...
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to log mission events with consistent format
	private void LogMissionEvent(string eventType)
	{
		if (!m_LogFileHandle)
		{
			return;
		}
		
		m_LogFileHandle.WriteLine("mission" + SEPARATOR + eventType + SEPARATOR + m_MissionName + SEPARATOR + m_PlayerCount + SEPARATOR + m_MaxPlayers + SEPARATOR + m_AuthorName + SEPARATOR + m_MissionDetails + SEPARATOR + m_GameMode);
	}
	
	// Logs player death and kill data to file
	// TODO: 
	//	- Handle AI kills/deaths
	//  - Faction data
	void LogPlayerKill(SCR_InstigatorContextData instiContext)
	{
		if (!m_LogFileHandle)
			return;
		
		m_sVictimName = GetGame().GetPlayerManager().GetPlayerName(instiContext.GetVictimPlayerID());
		m_sVictimGUID = GetGame().GetBackendApi().GetPlayerIdentityId(instiContext.GetVictimPlayerID());
		m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(instiContext.GetKillerPlayerID());
		m_sKillerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(instiContext.GetKillerPlayerID());
		
		// Killer weapon info
		m_WMC = BaseWeaponManagerComponent.Cast(instiContext.GetKillerEntity().FindComponent(BaseWeaponManagerComponent));
		m_sWeaponName = m_WMC.GetCurrentWeapon().GetUIInfo().GetName();	
		if (m_sWeaponName == "")
			m_sWeaponName = "Unknown Weapon";
		
		// Range
		m_fRange = vector.Distance(instiContext.GetVictimEntity().GetOrigin(),instiContext.GetKillerEntity().GetOrigin());
		
		// Time
		m_fTotalTime = GetGame().GetWorld().GetWorldTime();
  		m_iTotalSeconds = (m_fTotalTime / 1000);
		m_sTime = SCR_FormatHelper.FormatTime(m_iTotalSeconds);
		
		// Log to file
		m_LogFileHandle.WriteLine("kill" + SEPARATOR + m_sVictimName + SEPARATOR + m_sVictimGUID + SEPARATOR + m_sKillerName + SEPARATOR + m_sKillerGUID + SEPARATOR + m_sWeaponName + SEPARATOR + m_fRange + SEPARATOR + m_sTime);
	}
}