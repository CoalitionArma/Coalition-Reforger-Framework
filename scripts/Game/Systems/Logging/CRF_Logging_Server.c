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
	float m_fRange;
	SCR_FactionManager m_FM;
	
	override void OnPostInit(IEntity owner)
	{
		// Only run if in a real game and always in workbench
		#ifdef WORKBENCH
		#else 
			if (GetGame().GetPlayerManager().GetPlayerCount() < 10)
				return;
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
		
		// Check if killer is AI
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity) == 0)
			m_sKillerName = "AI";
		else
		{
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killer.GetInstigatorPlayerID());
			m_sKillerFaction = m_FM.GetPlayerFaction(killer.GetInstigatorPlayerID()).GetFactionName();
		}
		m_sKilledName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sKilledFaction = m_FM.GetPlayerFaction(playerId).GetFactionName();
		m_fRange = vector.Distance(playerEntity.GetOrigin(),killerEntity.GetOrigin());
		// TODO: determine weapon used, damage location IE "headshot", and roles for both killer and killed
		// string weaponUsed 
		// string damageLocation
		
		m_handle.WriteLine("kill:" + m_sKilledName + ":" + m_sKilledFaction + ":" + m_sKillerName + ":" + m_sKillerFaction + ":" + m_fRange);
	}
	
	// Player Connected
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		// Log to file
		m_handle.WriteLine("connect:" + playerName);
	}
	
	// Player Disconnected 
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		// Get player name
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		m_handle.WriteLine("disconnect:" + playerName);
	}
	
	// Mission status messages 
	override void OnGameStateChanged(SCR_EGameModeState state)
	{
		super.OnGameStateChanged(state);
		
		switch (state)
		{
			case 3: //slotting
			{
				m_handle.WriteLine("mission:slotting:" + m_sMissionName);
				break;
			}
			case 5: //briefing
			{
				m_handle.WriteLine("mission:briefing:" + m_sMissionName);
				break;
			}
			case 1: //game
			{
				m_handle.WriteLine("mission:safestart:" + m_sMissionName);
				break;
			}
			case 6: //debrief
			{
				m_handle.WriteLine("mission:ended:" + m_sMissionName);
				break;
			}
			default: // no ongamestate for loading a mission in reforger lobby 
			{
				m_handle.WriteLine("mission:beginning:" + m_sMissionName);
				break;
			}
		}
	}
	
	// Method called from safestart to annotate a game has begun
	void GameStarted()
	{
		m_handle.WriteLine("mission:started:" + m_sMissionName);
	}
}