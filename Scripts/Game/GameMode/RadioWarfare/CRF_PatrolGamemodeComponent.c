//------------------------------------------------------------------------------------
// CRF_PatrolGamemodeComponent: Patrol & Ambush
//
// The patrol has to work its way through a route of checkpoints, in order, and report in by radio at
// each one: once they reach a checkpoint they have a short window to send a SITREP (any radio
// transmission from the patrol, or on one set frequency). The ambushers only know roughly where the route
// runs (the 1 km grid squares the checkpoints are in) and have to set up on it and stop the patrol.
//
//   Patrol wins: reaches the last checkpoint having sent enough SITREPs.
//   Ambushers win: the patrol is wiped out, the time limit runs out, or the patrol reaches the end
//   without enough SITREPs.
//
// Navigation is map and compass: players get grid references, not map markers.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity and set the two faction keys.
// 2. Place a GenericEntity with a CRF_PatrolCheckpointComponent at each checkpoint, numbered in route
//    order (m_iOrder 1, 2, 3...). The highest number is the end of the route.
// 3. Give the patrol at least one radio. Optionally set m_sReportFrequency so SITREPs only count on the
//    patrol's net. The round starts when safestart ends.
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Patrol & Ambush: a patrol works through a checkpoint route reporting in by radio while the other side ambushes it.")]
class CRF_PatrolGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_PatrolGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction running the patrol.", category: "Patrol - Factions")]
	string m_sPatrolFactionKey;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction setting the ambushes.", category: "Patrol - Factions")]
	string m_sAmbushFactionKey;

	[Attribute("1", UIWidgets.Auto, "Alive patrol members that have to be within a checkpoint's radius to reach it.", category: "Patrol - Route")]
	int m_iMinPlayersAtCheckpoint;

	[Attribute("90", UIWidgets.Auto, "Seconds the patrol has after reaching a checkpoint to send its SITREP.", category: "Patrol - Reports")]
	float m_fReportWindowSeconds;

	[Attribute("", UIWidgets.EditBox, "Frequency SITREPs have to be sent on, e.g. \"45500\". Empty = any radio.", category: "Patrol - Reports")]
	string m_sReportFrequency;

	[Attribute("0", UIWidgets.Auto, "SITREPs the patrol needs by the end of the route. 0 = one for every checkpoint before the last.", category: "Patrol - Victory")]
	int m_iReportsRequired;

	[Attribute("45", UIWidgets.Auto, "Minutes the patrol has to finish the route. When it runs out the ambushers win. 0 = no time limit.", category: "Patrol - Victory")]
	int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Ambushers win when no patrol member is left alive.", category: "Patrol - Victory")]
	bool m_bAmbushWinsIfPatrolWiped;

	[Attribute("1", UIWidgets.CheckBox, "Record the winning faction and advance the gamemode to the AAR when a side wins.", category: "Patrol - Victory")]
	bool m_bEndMissionOnVictory;

	[Attribute("1", UIWidgets.CheckBox, "Tell the ambushers the 1 km grid squares the route runs through.", category: "Patrol - Intel")]
	bool m_bAmbushKnowsRoute;

	//===================================================================================
	// SERVER STATE
	//===================================================================================

	protected ref array<CRF_PatrolCheckpointComponent> m_aRoute = {};
	protected int m_iNextCheckpoint;		// index into m_aRoute
	protected int m_iPendingReport = -1;	// checkpoint index waiting for its SITREP, -1 = none
	protected float m_fReportTimeLeft;
	protected int m_iReportsMade;

	protected bool m_bStarted;
	protected bool m_bEnded;
	protected float m_fElapsedSeconds;
	protected int m_iNoPatrolSeconds;

	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Seconds with nobody alive in the patrol before the ambushers win (covers respawn gaps)
	protected static const int NO_PATROL_GRACE = 10;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif

		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!Replication.IsServer())
			return;

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback for missions without safestart
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);

		CRF_RadioTransmissionEvents.GetOnTransmissionStarted().Insert(OnTransmissionStarted);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);

		CRF_RadioTransmissionEvents.GetOnTransmissionStarted().Remove(OnTransmissionStarted);

		super.OnDelete(owner);
	}

	//===================================================================================
	// ROUND START
	//===================================================================================

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void StartRound()
	{
		if (m_bStarted)
			return;

		BuildRoute();
		if (m_aRoute.IsEmpty())
		{
			Print("[CRF_Patrol] No CRF_PatrolCheckpointComponent found in the mission - Patrol & Ambush is not running.", LogLevel.WARNING);
			return;
		}

		m_bStarted = true;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			SendBriefing(playerId);
	}

	//! Checkpoints sorted by m_iOrder
	protected void BuildRoute()
	{
		m_aRoute.Clear();
		foreach (CRF_PatrolCheckpointComponent checkpoint : CRF_PatrolCheckpointComponent.GetCheckpoints())
		{
			if (!checkpoint)
				continue;

			int insertAt = m_aRoute.Count();
			for (int i = 0; i < m_aRoute.Count(); i++)
			{
				if (checkpoint.m_iOrder < m_aRoute[i].m_iOrder)
				{
					insertAt = i;
					break;
				}
			}

			if (insertAt < m_aRoute.Count())
				m_aRoute.InsertAt(checkpoint, insertAt);
			else
				m_aRoute.Insert(checkpoint);
		}
	}

	//! Each side's orders: at round start, and when a player spawns later (late joiners, respawns)
	protected void SendBriefing(int playerId)
	{
		string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
		if (factionKey == m_sPatrolFactionKey)
		{
			if (m_iNextCheckpoint >= m_aRoute.Count())
				return;

			CRF_PatrolCheckpointComponent nextCheckpoint = m_aRoute[m_iNextCheckpoint];
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Patrol: next checkpoint " + nextCheckpoint.m_sCheckpointName,
				string.Format("Grid %1. %2 checkpoints on the route; send a SITREP by radio at each. %3 SITREPs so far.",
					CRF_RadioTransmissionEvents.GetGrid100(nextCheckpoint.GetPosition()), m_aRoute.Count(), m_iReportsMade), 10);
		}
		else if (factionKey == m_sAmbushFactionKey)
		{
			string text2 = "Find their route and stop them before they reach the end.";
			if (m_bAmbushKnowsRoute)
				text2 = "Intel puts their route through grid squares " + GetRouteSquares() + ". Stop them before the end.";
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Ambush the enemy patrol", text2, 10);
		}
	}

	//! The 1 km squares the checkpoints are in, in route order, without repeats
	protected string GetRouteSquares()
	{
		array<string> squares = {};
		foreach (CRF_PatrolCheckpointComponent checkpoint : m_aRoute)
		{
			string square = CRF_RadioTransmissionEvents.GetGrid1000(checkpoint.GetPosition());
			if (!squares.Contains(square))
				squares.Insert(square);
		}

		string text = "";
		foreach (int i, string square : squares)
		{
			if (i > 0)
				text += ", ";
			text += square;
		}
		return text;
	}

	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		if (m_bStarted && !m_bEnded)
			SendBriefing(playerId);
	}

	//===================================================================================
	// SITREPS
	//===================================================================================

	protected void OnTransmissionStarted(int playerId, string frequency, RplId radioId)
	{
		if (!m_bStarted || m_bEnded || m_iPendingReport < 0)
			return;

		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sPatrolFactionKey)
			return;

		if (!m_sReportFrequency.IsEmpty() && frequency != m_sReportFrequency)
			return;

		m_iReportsMade++;
		string checkpointName = m_aRoute[m_iPendingReport].m_sCheckpointName;
		m_iPendingReport = -1;
		CRF_RadioTransmissionEvents.PopupToFaction(m_sPatrolFactionKey, "SITREP received: " + checkpointName,
			string.Format("%1 of %2 SITREPs needed.", m_iReportsMade, GetReportsRequired()), 5);
	}

	protected int GetReportsRequired()
	{
		if (m_iReportsRequired > 0)
			return m_iReportsRequired;
		if (m_aRoute.Count() > 1)
			return m_aRoute.Count() - 1;
		return 0;
	}

	protected void MissPendingReport()
	{
		if (m_iPendingReport < 0)
			return;

		string checkpointName = m_aRoute[m_iPendingReport].m_sCheckpointName;
		m_iPendingReport = -1;
		CRF_RadioTransmissionEvents.PopupToFaction(m_sPatrolFactionKey, "SITREP missed: " + checkpointName,
			string.Format("%1 of %2 SITREPs needed.", m_iReportsMade, GetReportsRequired()), 5);
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded)
			return;

		m_fElapsedSeconds += 1;

		// Report window
		if (m_iPendingReport >= 0)
		{
			m_fReportTimeLeft -= 1;
			if (m_fReportTimeLeft <= 0)
				MissPendingReport();
		}

		// Next checkpoint
		if (m_iNextCheckpoint < m_aRoute.Count() && CountPatrolAt(m_aRoute[m_iNextCheckpoint]) >= m_iMinPlayersAtCheckpoint)
		{
			CheckpointReached();
			if (m_bEnded)
				return;
		}

		if (m_iTimeLimitMinutes > 0 && m_fElapsedSeconds >= m_iTimeLimitMinutes * 60)
		{
			Victory(m_sAmbushFactionKey, "Time is up - the patrol never made it through.");
			return;
		}

		if (m_bAmbushWinsIfPatrolWiped)
		{
			if (CountAlivePatrol() == 0)
				m_iNoPatrolSeconds++;
			else
				m_iNoPatrolSeconds = 0;

			if (m_iNoPatrolSeconds >= NO_PATROL_GRACE)
				Victory(m_sAmbushFactionKey, "The patrol has been wiped out.");
		}
	}

	protected void CheckpointReached()
	{
		CRF_PatrolCheckpointComponent checkpoint = m_aRoute[m_iNextCheckpoint];
		bool isLast = m_iNextCheckpoint == m_aRoute.Count() - 1;

		// Reaching the next checkpoint closes the previous report window
		MissPendingReport();

		if (isLast)
		{
			if (m_iReportsMade >= GetReportsRequired())
				Victory(m_sPatrolFactionKey, string.Format("The patrol reached %1 with %2 SITREPs.", checkpoint.m_sCheckpointName, m_iReportsMade));
			else
				Victory(m_sAmbushFactionKey, string.Format("The patrol reached %1 but only sent %2 of %3 SITREPs.", checkpoint.m_sCheckpointName, m_iReportsMade, GetReportsRequired()));
			return;
		}

		m_iPendingReport = m_iNextCheckpoint;
		m_fReportTimeLeft = m_fReportWindowSeconds;
		m_iNextCheckpoint++;

		CRF_PatrolCheckpointComponent nextCheckpoint = m_aRoute[m_iNextCheckpoint];
		string reportOn = "on the radio";
		if (!m_sReportFrequency.IsEmpty())
			reportOn = "on " + m_sReportFrequency;
		CRF_RadioTransmissionEvents.PopupToFaction(m_sPatrolFactionKey, checkpoint.m_sCheckpointName + " reached - send your SITREP",
			string.Format("Report %1 within %2 seconds. Next: %3, grid %4.", reportOn, (int)m_fReportWindowSeconds,
				nextCheckpoint.m_sCheckpointName, CRF_RadioTransmissionEvents.GetGrid100(nextCheckpoint.GetPosition())), 8);
	}

	protected int CountPatrolAt(CRF_PatrolCheckpointComponent checkpoint)
	{
		if (!checkpoint)
			return 0;

		vector center = checkpoint.GetPosition();
		float radiusSq = checkpoint.m_fRadius * checkpoint.m_fRadius;
		int count = 0;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sPatrolFactionKey)
				continue;

			IEntity character = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId);
			if (character && vector.DistanceSq(character.GetOrigin(), center) <= radiusSq)
				count++;
		}
		return count;
	}

	protected int CountAlivePatrol()
	{
		int count = 0;
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sPatrolFactionKey)
				continue;
			if (CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId))
				count++;
		}
		return count;
	}

	//===================================================================================
	// VICTORY
	//===================================================================================

	protected void Victory(string factionKey, string reason)
	{
		if (m_bEnded)
			return;

		m_bEnded = true;
		GetGame().GetCallqueue().Remove(Tick);

		CRF_RadioTransmissionEvents.PopupToFaction("", factionKey + " wins", reason, 10);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(factionKey, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}
}
