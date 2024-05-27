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
	string m_sLogPath = "$profile:COAServerLog.txt";
	private ref FileHandle m_handle;
	string m_sKillerName;
	string m_sKillerFaction;
	string m_sKilledName;
	string m_sKilledFaction;
	string m_sMissionName;
	SCR_FactionManager m_FM;
	
	override void OnPostInit(IEntity owner)
	{
		// Only run if in a real game and always in workbench
		#ifdef WORKBENCH
		#else 
			if (GetGame().GetPlayerManager().GetPlayerCount() < 10)
			{
				Print("CRF < 10 players");
				return;
			}
		#endif			
	}

	// Setup
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		m_sMissionName = GetGame().GetMissionName();
		
		bool serverLogExists = FileIO.FileExists(m_sLogPath);
		
		if (serverLogExists)
			m_handle = FileIO.OpenFile(m_sLogPath, FileMode.APPEND);
		else
			m_handle = FileIO.OpenFile(m_sLogPath, FileMode.WRITE);

		//PrintFormat("[CRF] Handle: %1",m_handle);
	}
	
	// Killfeed log
	override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killer.GetInstigatorPlayerID());
		m_sKillerFaction = m_FM.GetPlayerFaction(killer.GetInstigatorPlayerID()).GetFactionName();
		m_sKilledName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sKilledFaction = m_FM.GetPlayerFaction(playerId).GetFactionName();
		// TODO: determine weapon used, damage location IE "headshot", and roles for both killer and killed
		// string weaponUsed 
		// string damageLocation
		
		m_handle.WriteLine("kill:" + m_sKilledName + " (" + m_sKilledFaction + ")" + " was killed by " + m_sKillerName + " (" + m_sKillerFaction + ")");
	}
	
	// Player Connected
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		// Log to file
		m_handle.WriteLine("connect:" + playerName + " connected");
	}
	
	// Player Disconnected 
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		m_handle.WriteLine("disconnect:" + playerName + " disconnected");
	}
	
	// Mission status messages 
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		
		switch (state)
		{
			case 3: //slotting
			{
				m_handle.WriteLine("mission:" + m_sMissionName + " is now slotting!");
				break;
			}
			case 5: //briefing
			{
				m_handle.WriteLine("mission:" + m_sMissionName + " is now in the briefing phase!");
				break;
			}
			case 1: //game
			{
				m_handle.WriteLine("mission:" + m_sMissionName + " is now in the safestart phase!");
				break;
			}
			case 6: //debrief
			{
				m_handle.WriteLine("mission:" + m_sMissionName + " has ended!");
				break;
			}
			default: // no ongamestate for loading a mission in reforger lobby 
			{
				m_handle.WriteLine("mission:" + "A new mission is now beginning: " + m_sMissionName);
				break;
			}
		}
	}
	
	// Method called from safestart to annotate a game has begun
	void GameStarted()
	{
		m_handle.WriteLine("started:" + m_sMissionName + " is now live!");
	}
}