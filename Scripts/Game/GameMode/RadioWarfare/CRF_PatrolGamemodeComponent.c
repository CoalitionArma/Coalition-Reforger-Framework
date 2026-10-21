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
// Map markers (from a few seconds after load, so both sides can plan during safestart):
//   Patrol: a labelled marker on every checkpoint, marked "(reached)" as they go.
//   Ambushers: a square search area around each checkpoint, offset at random so it doesn't give
//   away the exact spot. The checkpoint is always somewhere inside its square.
// Both are drawn locally on each client and only for the matching faction, re-checked every couple
// of seconds so late joiners and respawns (which pass through the SPEC faction) get them too.
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

	[Attribute("1", UIWidgets.CheckBox, "Mark every checkpoint on the patrol's map.", category: "Patrol - Map Markers")]
	bool m_bPatrolCheckpointMarkers;

	[Attribute("{FDE4238500835875}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_Move.edds", UIWidgets.ResourcePickerThumbnail, "Icon for the patrol's checkpoint markers.", params: "edds", category: "Patrol - Map Markers")]
	ResourceName m_rCheckpointMarkerIcon;

	[Attribute("1", UIWidgets.CheckBox, "Draw a square search area around each checkpoint on the ambushers' map.", category: "Patrol - Map Markers")]
	bool m_bAmbushSearchAreas;

	[Attribute("{B1A844B7FEF4BC41}PrefabsMissionMaking/Markers/ShapeMarkers/ShapeMarker_Base.et", UIWidgets.ResourceNamePicker, "Marker entity spawned as each search area. Must be a COA_ShapeMarker.", params: "et", category: "Patrol - Map Markers")]
	ResourceName m_rSearchAreaPrefab;

	[Attribute("400", UIWidgets.Slider, "Side length of each search area square, in metres.", params: "50 2000 25", category: "Patrol - Map Markers")]
	float m_fSearchAreaSize;

	[Attribute("0.6", UIWidgets.Slider, "How far the square's centre can be offset from the checkpoint, as a fraction of the most it could move and still contain it. 0 centres the square on the checkpoint.", params: "0 1 0.05", category: "Patrol - Map Markers")]
	float m_fSearchAreaOffsetFactor;

	[Attribute("2.5", UIWidgets.Slider, "Outline thickness of the search areas, in pixels.", params: "0.25 12 0.25", category: "Patrol - Map Markers")]
	float m_fSearchAreaBorderWidth;

	[Attribute("1 0.25 0.25 1", UIWidgets.ColorPicker, "Colour of the search area outlines.", category: "Patrol - Map Markers")]
	ref Color m_SearchAreaColor;

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
	// REPLICATED MARKER DATA (server decides, clients draw)
	//===================================================================================

	[RplProp(onRplName: "OnMarkersReplicated")]
	protected ref array<vector> m_aCheckpointMarkerPositions = {};

	[RplProp(onRplName: "OnMarkersReplicated")]
	protected ref array<string> m_aCheckpointMarkerLabels = {};

	[RplProp(onRplName: "OnMarkersReplicated")]
	protected ref array<vector> m_aSearchAreaCenters = {};

	protected bool m_bMarkersPublished;

	//===================================================================================
	// LOCAL MARKER STATE (each machine with a player)
	//===================================================================================

	protected ref array<string> m_aLocalCheckpointPositions = {};
	protected ref array<string> m_aLocalCheckpointLabels = {};
	protected ref array<IEntity> m_aLocalSearchAreas = {};
	protected ref array<vector> m_aLocalSearchAreaCenters = {};

	// Gives checkpoint components time to register before the route is built
	protected static const int MARKER_PUBLISH_DELAY_MS = 3000;
	protected static const int MARKER_POLL_INTERVAL_MS = 2000;
	protected static const int MARKER_WIDGET_DELAY_MS = 100;
	protected static const int CHECKPOINT_MARKER_UPDATE_DELAY = 1000;
	protected static const int CHECKPOINT_MARKER_Z_ORDER = 500;
	protected static const int CHECKPOINT_MARKER_COLOR = ARGB(255, 90, 160, 255);
	protected static const int CHECKPOINT_REACHED_COLOR = ARGB(255, 140, 140, 140);
	protected static const string REACHED_SUFFIX = " (reached)";

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Runs everywhere: the server runs the round, clients draw the map markers
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Markers are drawn by any machine with a local player, which includes a listen host and
		// Workbench play, so gate on "not a dedicated server" rather than Replication.IsClient()
		if (RplSession.Mode() != RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(UpdateLocalMarkers, MARKER_POLL_INTERVAL_MS, true);

		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(PublishMarkers, MARKER_PUBLISH_DELAY_MS, false);

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
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(PublishMarkers);
			GetGame().GetCallqueue().Remove(UpdateLocalMarkers);
		}

		ClearLocalCheckpointMarkers();
		ClearLocalSearchAreas();

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

		// Normally already done during safestart; covers missions that start straight away
		PublishMarkers();

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
			if (m_bAmbushSearchAreas)
				text2 += " Each of their checkpoints is somewhere inside a search area on your map.";
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

		MarkCheckpointReached(m_iNextCheckpoint);

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
	// MAP MARKERS (SERVER)
	//===================================================================================

	//! Decides where every marker goes, once. Search areas are offset at random, so they must
	//! not be recomputed or the ambushers' squares would jump around.
	protected void PublishMarkers()
	{
		if (m_bMarkersPublished)
			return;

		if (m_aRoute.IsEmpty())
			BuildRoute();

		if (m_aRoute.IsEmpty())
			return;

		m_bMarkersPublished = true;
		m_aCheckpointMarkerPositions.Clear();
		m_aCheckpointMarkerLabels.Clear();
		m_aSearchAreaCenters.Clear();

		foreach (int i, CRF_PatrolCheckpointComponent checkpoint : m_aRoute)
		{
			m_aCheckpointMarkerPositions.Insert(checkpoint.GetPosition());
			m_aCheckpointMarkerLabels.Insert(GetCheckpointLabel(i, false));
			m_aSearchAreaCenters.Insert(ComputeSearchAreaCenter(checkpoint));
		}

		Replication.BumpMe();

		// A listen host or Workbench play is also the server and gets no replication callback
		if (RplSession.Mode() != RplMode.Dedicated)
			UpdateLocalMarkers();
	}

	//! "2. Bridge", or "End: Bridge" for the last checkpoint, with "(reached)" once the patrol has been there
	protected string GetCheckpointLabel(int index, bool reached)
	{
		string label = string.Format("%1. %2", index + 1, m_aRoute[index].m_sCheckpointName);
		if (index == m_aRoute.Count() - 1)
			label = "End: " + m_aRoute[index].m_sCheckpointName;

		if (reached)
			label += REACHED_SUFFIX;

		return label;
	}

	//! A square of side m_fSearchAreaSize that always contains the checkpoint's whole radius
	protected vector ComputeSearchAreaCenter(CRF_PatrolCheckpointComponent checkpoint)
	{
		vector checkpointPos = checkpoint.GetPosition();
		float maxOffset = Math.Max(0, m_fSearchAreaSize * 0.5 - checkpoint.m_fRadius) * m_fSearchAreaOffsetFactor;
		float offsetX = Math.RandomFloat(-maxOffset, maxOffset);
		float offsetZ = Math.RandomFloat(-maxOffset, maxOffset);
		return Vector(checkpointPos[0] + offsetX, checkpointPos[1], checkpointPos[2] + offsetZ);
	}

	protected void MarkCheckpointReached(int index)
	{
		if (!m_bMarkersPublished || !m_aCheckpointMarkerLabels.IsIndexValid(index))
			return;

		m_aCheckpointMarkerLabels[index] = GetCheckpointLabel(index, true);
		Replication.BumpMe();

		if (RplSession.Mode() != RplMode.Dedicated)
			UpdateLocalMarkers();
	}

	//===================================================================================
	// MAP MARKERS (CLIENT)
	//===================================================================================

	protected void OnMarkersReplicated()
	{
		UpdateLocalMarkers();
	}

	//! Polled as well as driven by replication, because the local player's faction changes between
	//! slotting, death (SPEC) and respawn
	protected void UpdateLocalMarkers()
	{
		string localFactionKey = GetLocalFactionKey();
		UpdateCheckpointMarkers(m_bPatrolCheckpointMarkers && !localFactionKey.IsEmpty() && localFactionKey == m_sPatrolFactionKey);
		UpdateSearchAreas(m_bAmbushSearchAreas && !localFactionKey.IsEmpty() && localFactionKey == m_sAmbushFactionKey);
	}

	protected string GetLocalFactionKey()
	{
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!localFaction)
			return "";

		return localFaction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! The patrol's checkpoint markers, through the COA scripted marker manager (local to this client)
	protected void UpdateCheckpointMarkers(bool show)
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		array<string> wantedPositions = {};
		array<string> wantedLabels = {};
		if (show)
		{
			foreach (int i, vector markerPos : m_aCheckpointMarkerPositions)
			{
				if (!m_aCheckpointMarkerLabels.IsIndexValid(i))
					continue;

				wantedPositions.Insert(string.Format("%1 %2 %3", markerPos[0], markerPos[1], markerPos[2]));
				wantedLabels.Insert(m_aCheckpointMarkerLabels[i]);
			}
		}

		// Drop markers that are no longer wanted (left the faction, or relabelled as reached)
		for (int existing = m_aLocalCheckpointPositions.Count() - 1; existing >= 0; existing--)
		{
			if (IsMarkerInList(m_aLocalCheckpointPositions[existing], m_aLocalCheckpointLabels[existing], wantedPositions, wantedLabels))
				continue;

			RemoveCheckpointMarker(markerManager, m_aLocalCheckpointPositions[existing], m_aLocalCheckpointLabels[existing]);
			m_aLocalCheckpointPositions.Remove(existing);
			m_aLocalCheckpointLabels.Remove(existing);
		}

		// Add the missing ones
		foreach (int wanted, string wantedPos : wantedPositions)
		{
			string wantedLabel = wantedLabels[wanted];
			if (IsMarkerInList(wantedPos, wantedLabel, m_aLocalCheckpointPositions, m_aLocalCheckpointLabels))
				continue;

			markerManager.AddScriptedMarker("Static Marker", wantedPos, CHECKPOINT_MARKER_UPDATE_DELAY, wantedLabel,
				m_rCheckpointMarkerIcon, CHECKPOINT_MARKER_Z_ORDER, GetCheckpointMarkerColor(wantedLabel));
			m_aLocalCheckpointPositions.Insert(wantedPos);
			m_aLocalCheckpointLabels.Insert(wantedLabel);
		}
	}

	//! Removal has to repeat every parameter the marker was added with, colour included
	protected void RemoveCheckpointMarker(notnull COA_PlayerScriptedMarkerManager markerManager, string markerPos, string markerLabel)
	{
		markerManager.RemoveScriptedMarker("Static Marker", markerPos, CHECKPOINT_MARKER_UPDATE_DELAY, markerLabel,
			m_rCheckpointMarkerIcon, CHECKPOINT_MARKER_Z_ORDER, GetCheckpointMarkerColor(markerLabel));
	}

	protected int GetCheckpointMarkerColor(string markerLabel)
	{
		if (markerLabel.EndsWith(REACHED_SUFFIX))
			return CHECKPOINT_REACHED_COLOR;

		return CHECKPOINT_MARKER_COLOR;
	}

	protected bool IsMarkerInList(string markerPos, string markerLabel, notnull array<string> positions, notnull array<string> labels)
	{
		foreach (int i, string candidate : positions)
		{
			if (candidate == markerPos && labels.IsIndexValid(i) && labels[i] == markerLabel)
				return true;
		}

		return false;
	}

	protected void ClearLocalCheckpointMarkers()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (markerManager)
		{
			foreach (int i, string markerPos : m_aLocalCheckpointPositions)
			{
				if (m_aLocalCheckpointLabels.IsIndexValid(i))
					RemoveCheckpointMarker(markerManager, markerPos, m_aLocalCheckpointLabels[i]);
			}
		}

		m_aLocalCheckpointPositions.Clear();
		m_aLocalCheckpointLabels.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! The ambushers' search squares. COA_ShapeMarker has no RplComponent, so each ambusher spawns
	//! their own copies; the server picked the centres, so everyone searches the same ground.
	protected void UpdateSearchAreas(bool show)
	{
		if (!show)
		{
			ClearLocalSearchAreas();
			return;
		}

		// The centres never change once published, so only rebuild when they differ from what's drawn
		bool upToDate = m_aLocalSearchAreaCenters.Count() == m_aSearchAreaCenters.Count();
		if (upToDate)
		{
			foreach (int i, vector center : m_aSearchAreaCenters)
			{
				if (vector.DistanceSq(m_aLocalSearchAreaCenters[i], center) > 0.01)
				{
					upToDate = false;
					break;
				}
			}
		}

		if (upToDate)
			return;

		ClearLocalSearchAreas();
		foreach (vector areaCenter : m_aSearchAreaCenters)
		{
			IEntity area = SpawnSearchArea(areaCenter);
			if (area)
				m_aLocalSearchAreas.Insert(area);

			// Tracked even if the spawn failed, so a bad prefab doesn't retry every poll
			m_aLocalSearchAreaCenters.Insert(areaCenter);
		}
	}

	protected IEntity SpawnSearchArea(vector areaCenter)
	{
		Resource markerResource = Resource.Load(m_rSearchAreaPrefab);
		if (!markerResource || !markerResource.IsValid())
		{
			Print(string.Format("[CRF_Patrol] Search area prefab '%1' could not be loaded.", m_rSearchAreaPrefab), LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = areaCenter;

		IEntity markerEntity = GetGame().SpawnEntityPrefab(markerResource, GetGame().GetWorld(), spawnParams);
		if (!markerEntity)
			return null;

		COA_ShapeMarker marker = COA_ShapeMarker.Cast(markerEntity);
		if (!marker)
		{
			Print(string.Format("[CRF_Patrol] Search area prefab '%1' is not a COA_ShapeMarker.", m_rSearchAreaPrefab), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(markerEntity);
			return null;
		}

		// Only ambushers ever have this entity, so it can show for any faction
		marker.m_bShowForAnyFaction = true;
		marker.m_bVisibleForEmptyFaction = true;
		marker.m_bUseWorldScale = true;
		marker.m_MarkerColor = m_SearchAreaColor;

		marker.SetShapeType(COA_EMapShapeMarkerType.SQUARE);
		marker.SetShapeBorderWidth(m_fSearchAreaBorderWidth);
		marker.SetShapeSize(m_fSearchAreaSize, m_fSearchAreaSize);
		marker.SetShapeName("Patrol checkpoint search area");

		// A manual marker only builds its widget on the map-open event it subscribes to in EOnInit,
		// so one spawned while the map is open stays invisible until reopened. Build it now,
		// a beat later because EOnInit hasn't run yet on a freshly spawned entity.
		GetGame().GetCallqueue().CallLater(EnsureMarkerWidget, MARKER_WIDGET_DELAY_MS, false, markerEntity);

		return markerEntity;
	}

	protected void EnsureMarkerWidget(IEntity markerEntity)
	{
		COA_ManualMarker marker = COA_ManualMarker.Cast(markerEntity);
		if (!marker)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		marker.CreateMapWidget(mapEntity.GetMapConfig());
	}

	protected void ClearLocalSearchAreas()
	{
		foreach (IEntity area : m_aLocalSearchAreas)
		{
			if (area)
				SCR_EntityHelper.DeleteEntityAndChildren(area);
		}

		m_aLocalSearchAreas.Clear();
		m_aLocalSearchAreaCenters.Clear();
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
