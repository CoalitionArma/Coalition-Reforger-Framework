/*
*	Logging component for COALITION games
*	Component overrides base game mode so it always runs
*
*	Note that write files seem weird because they are parsed by an external program
*	which splits strings via colons
*
*	Server only
*/
[ComponentEditorProps(category: "CRF Logging Component", description: "")]
class CRF_LoggingServerComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_LoggingServerComponent: SCR_BaseGameModeComponent
{	
	const string SEPARATOR = ",";
	const string m_sLogPath = "$profile:COAServerLog.txt";
	string m_sMissionName;
	string playerName;
	
	int m_iPlayerCount;
	int m_iBluforCount;
	int m_iOpforCount;
	int m_iIndforCount;
	
	private ref FileHandle m_handle;
	SCR_FactionManager m_FM;
	
	// Instance of this component (this method only works if you KNOW there will only ever be one instance of this component) 
	protected static CRF_LoggingServerComponent s_Instance;
	
	//------------------------------------------------------------------------------------------------
	void CRF_LoggingServerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_LoggingServerComponent GetInstance()
	{
		return s_Instance;
	}
	
	FileHandle ReturnFileHandle()
	{
		return m_handle;
	}

	// Setup
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if ((RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen) || !GetGame().InPlayMode())
			return;
		
		m_sMissionName = GetGame().GetMissionName();
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
		m_handle = FileIO.OpenFile(m_sLogPath, FileMode.APPEND);
		
		m_handle.WriteLine("mission" + SEPARATOR + "beginning" + SEPARATOR + m_sMissionName + SEPARATOR + m_iPlayerCount);
		
		CRF_Gamemode.GetInstance().GetOnStateChanged().Insert(OnGamemodeStateChanged);
	}
	
	// Player Connected
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_handle.WriteLine("connect" + SEPARATOR + playerName);
	}
	
	// Player Disconnected 
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		// Get player name
		playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_handle.WriteLine("disconnect" + SEPARATOR + playerName + SEPARATOR + cause);
	}
	
	// Mission status messages 
	void OnGamemodeStateChanged()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		//PrintFormat("CRF: %1", CRF_Gamemode.GetInstance().m_GamemodeState);
		
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
		switch (CRF_Gamemode.GetInstance().m_GamemodeState)
		{
			case CRF_EGamemodeState.SLOTTING:
			{
				m_handle.WriteLine("mission" + SEPARATOR + "slotting" + SEPARATOR + m_sMissionName + SEPARATOR + m_iPlayerCount);
				break;
			}
			case CRF_EGamemodeState.GAME:
			{
				m_handle.WriteLine("mission" + SEPARATOR + "safestart" + SEPARATOR + m_sMissionName + SEPARATOR + m_iPlayerCount);
				break;
			}
			case CRF_EGamemodeState.AAR:
			{
				m_handle.WriteLine("mission" + SEPARATOR + "ended" + SEPARATOR + m_sMissionName + SEPARATOR + m_iPlayerCount);
				break;
			}
		}
	}
	
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_handle.Close(); // lets avoid a mem leak
	}
	
	// Method called from safestart to annotate a game has begun
	void GameStarted()
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		// Collect mission data 
		// TODO: Get playercount per faction here
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
		// log
		m_handle.WriteLine("mission" + SEPARATOR + "started" + SEPARATOR + m_sMissionName + SEPARATOR + m_iPlayerCount);
	}
}

/*modded class SCR_BaseGameMode
{
	const string SEPARATOR = ",";
	string m_sKillerName;
	string m_sKillerFaction;
	string m_sKilledName;
	string m_sKilledFaction;
	string m_sTime;
	string m_sWeaponName;
	float m_fRange;
	float m_fTotalTime;
	int m_iTotalSeconds;
	
	SCR_FactionManager m_FM;
	BaseWeaponManagerComponent m_WMC;
	private ref FileHandle m_handle;
	
	// Killfeed log
    override void OnPlayerKilled(int playerID, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
    {
		super.OnPlayerKilled(playerID, playerEntity, killerEntity, killer);
		
		m_FM = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_handle = CRF_LoggingServerComponent.GetInstance().ReturnFileHandle();
		
		// Killer
		// Check if killer is AI
		if (GetGame().GetPlayerManager().GetplayerIDFromControlledEntity(killerEntity) == 0)
		{
			m_sKillerName = "AI";
			m_sKillerFaction = "AI";
		} else {
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killer.GetInstigatorplayerID());
			m_sKillerFaction = m_FM.GetPlayerFaction(killer.GetInstigatorplayerID()).GetFactionName();
		}
		
		// Killed
		m_sKilledName = GetGame().GetPlayerManager().GetPlayerName(playerID);
		m_sKilledFaction = m_FM.GetPlayerFaction(playerID).GetFactionName();
		
		// Range
		m_fRange = vector.Distance(playerEntity.GetOrigin(),killerEntity.GetOrigin());
		
		// Killer Weapon 
		m_WMC = BaseWeaponManagerComponent.Cast(killerEntity.FindComponent(BaseWeaponManagerComponent));
		m_sWeaponName = m_WMC.GetCurrentWeapon().GetUIInfo().GetName();	
		if (m_sWeaponName == "")
			m_sWeaponName = "Unknown Weapon";
			
		// Time
		m_fTotalTime = GetGame().GetWorld().GetWorldTime();
  		m_iTotalSeconds = (m_fTotalTime / 1000);
		m_sTime = SCR_FormatHelper.FormatTime(m_iTotalSeconds);
		
		m_handle.WriteLine("kill" + SEPARATOR + m_sKilledName + SEPARATOR + m_sKilledFaction + SEPARATOR + m_sKillerName + SEPARATOR + m_sKillerFaction + SEPARATOR + m_fRange + SEPARATOR + m_sWeaponName + SEPARATOR + m_sTime);
	}
}*/