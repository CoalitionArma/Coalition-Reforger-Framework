//------------------------------------------------------------------------------------
// CRF_RallyGamemodeComponent: Rally / circuit race gamemode for CRF
//
// Features
// - Ordered checkpoint course defined by naming world entities in the editor
// - Each CP must be hit in sequence (out-of-order crosses are ignored)
// - First entity in the list = Start Line (crossing it starts lap timer)
// - Last entity in the list  = Finish Line (crossing it completes a lap)
// - Configurable lap count (1 = point-to-point, >1 = circuit)
// - Live standings sorted by progress (laps + checkpoint index) and time
// - Server-authoritative: standings replicated every second via RplProp string
// - Per-player checkpoint notifications via broadcast RPC (client-side filtered)
// - On-screen HUD panel showing position, running lap timer, and full leaderboard
// - Course minimap under the panel with the player's car on it (CRF_RallyMinimap, m_bShowMinimap)
//
// Setup for mission makers
// 1. Add this component to the gamemode entity
// 2. Place and name checkpoint entities in the world using the following convention:
//      rally_start          — Start line (crossing it starts the lap timer)
//      rally_cp1            — First intermediate checkpoint
//      rally_cp2, cp3, ...  — Additional intermediate checkpoints (unlimited, numbered sequentially)
//      rally_end            — Finish line (crossing it completes a lap)
//    Intermediate checkpoints are discovered automatically at race start — no pre-configuration needed.
// 3. Set m_fCheckpointRadius to match how wide the checkpoint zone should be
// 4. Set m_iLapsToComplete (1 for point-to-point, 2+ for circuit)
// 5. For circuits the rally_end entity should be adjacent to rally_start so players loop back
// 6. Optional: place an entity named "<checkpoint name>_smoke" (e.g. rally_cp3_smoke) to put that
//    checkpoint's smoke exactly there. Otherwise the smoke goes beside the nearest road, past its
//    edge, on the downwind side so it drifts away from the racing line.
// 7. The minimap traces the course along the shortest road route between checkpoints. Where the
//    stage takes a longer road than that, add a checkpoint on it.
//
// Crossings only count while the round is live (COA_EGamemodeState.GAME), and are checked along the
// path travelled since the last poll, so fast vehicles can't skip a checkpoint between polls.
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// Per-player runtime race state — server side only
class CRF_RallyPlayerEntry
{
	int m_iPlayerId;
	int m_iNextCheckpoint;       // Index of the NEXT checkpoint this player must cross
	int m_iLapsCompleted;        // Full laps finished
	int m_iLapStartTimeMs;       // GetWorldTime() in ms when current lap started (0 = not started)
	int m_iLastCPTimeMs;         // GetWorldTime() in ms when last CP was crossed (tiebreaker)
	int m_iRaceStartTimeMs;      // GetWorldTime() in ms when player crossed the start line for the first time
	float m_fBestLapTime;        // Best completed lap in seconds (-1 = none yet)
	float m_fFinishTimeSecs;     // Total race time from start to finish in seconds (-1 = DNF / not set)
	bool m_bFinished;
	bool m_bDead;
	int m_iDeathTimeMs;          // GetWorldTime() in ms when this player died (0 = not dead)
	vector m_vLastPos;           // Tracked position at the previous poll, for the swept crossing check
	bool m_bHasLastPos;

	void CRF_RallyPlayerEntry(int playerId)
	{
		m_iPlayerId         = playerId;
		m_iNextCheckpoint   = 0;
		m_iLapsCompleted    = 0;
		m_iLapStartTimeMs   = 0;
		m_iLastCPTimeMs     = 0;
		m_iRaceStartTimeMs  = 0;
		m_fBestLapTime      = -1;
		m_fFinishTimeSecs   = -1;
		m_bFinished         = false;
		m_bDead             = false;
		m_iDeathTimeMs      = 0;
	}

	// Higher progress = further ahead in the race
	int GetProgress(int totalCheckpoints)
	{
		return m_iLapsCompleted * totalCheckpoints + m_iNextCheckpoint;
	}
}


//------------------------------------------------------------------------------------------------
// One standings row as the client HUD reads it from the replicated standings string
class CRF_RallyHUDRow
{
	int m_iPlayerId;
	int m_iLaps;
	int m_iNextCP;
	int m_iLapStartMs;   // server world time; 0 = not started
	bool m_bFinished;
	int m_iGroupId;
	bool m_bDead;
	int m_iDeathMs;      // server world time
	string m_sName;      // display name (set by BuildDisplayRows)
	bool m_bLocal;       // the local player, or their team
	ref array<string> m_aExtra = {}; // a variant's extra fields (GetExtraStandingFields)

	//! An extra field as an int, or fallback when it isn't there
	int GetExtraInt(int index, int fallback = 0)
	{
		if (!m_aExtra.IsIndexValid(index))
			return fallback;

		return m_aExtra[index].ToInt();
	}
}


//------------------------------------------------------------------------------------------------
// Results screen menu — opened on all clients after the race ends.
// Parses the packed results string and populates the layout widgets.
class CRF_RallyResultsMenu : ChimeraMenuBase
{
	override void OnMenuOpen()
	{
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetRootWidget().SetOpacity(0);
	}

	override void OnMenuUpdate(float tDelta)
	{
		float opacity = GetRootWidget().GetOpacity();
		if (opacity < 1)
		{
			opacity = opacity + tDelta;
			if (opacity > 1)
				opacity = 1;
			GetRootWidget().SetOpacity(opacity);
		}
	}

	void Action_Exit()
	{
		GetGame().GetMenuManager().CloseAllMenus();
	}
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Rally / circuit race. Define an ordered course using world entity names and track live standings for all players on the HUD.")]
class CRF_RallyGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_RallyGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("25", UIWidgets.Auto, desc: "Detection sphere radius in metres. A player/vehicle must enter this sphere around a checkpoint entity to register the crossing.", category: "Rally - Course")]
	float m_fCheckpointRadius;

	[Attribute("1", UIWidgets.Auto, desc: "Number of laps required to finish the race. 1 = point-to-point (start → finish once). 2+ = circuit races where players loop back to the start after each finish crossing.", category: "Rally - Race")]
	int m_iLapsToComplete;

	[Attribute("false", UIWidgets.CheckBox, desc: "When enabled the player's controlled entity must be parented to a vehicle (i.e. they must be seated in a vehicle) for a checkpoint crossing to be counted. Use this for vehicle-only race modes.", category: "Rally - Race")]
	bool m_bRequireVehicle;

	[Attribute("5", UIWidgets.Auto, desc: "Maximum number of entries (teams or individual players) shown in the HUD standings panel.", category: "Rally - HUD")]
	int m_iMaxStandingsRows;

	[Attribute("false", UIWidgets.CheckBox, desc: "When enabled, players sharing the same in-game group (e.g. driver + co-driver) are treated as one team entry on the HUD and results screen. Team position is determined by the furthest-ahead member.", category: "Rally - HUD")]
	bool m_bTeamMode;

	[Attribute("true", UIWidgets.CheckBox, desc: "Show a minimap of the course under the standings panel, with the player's car on it (see CRF_RallyMinimap).", category: "Rally - HUD")]
	bool m_bShowMinimap;

	//===================================================================================
	// SERVER-SIDE RUNTIME STATE
	//===================================================================================

	protected ref array<ref CRF_RallyPlayerEntry> m_aPlayerEntries = {};
	protected ref array<vector> m_aCheckpointPositions = {};
	protected bool m_bRaceInitialised = false;

	// Smoke entities spawned at each checkpoint (server-managed, replicated via world state). The
	// previous cycle's smokes are kept a few seconds after a refresh so the markers never blink out.
	protected ref array<IEntity> m_aSmokeEntities = {};
	protected ref array<IEntity> m_aOldSmokeEntities = {};
	// Where each checkpoint's smoke goes - worked out once at race start (road lookup is not free)
	protected ref array<vector> m_aSmokePositions = {};
	// Checkpoint entity names, in course order, for the "<name>_smoke" overrides
	protected ref array<string> m_aCheckpointNames = {};

	// Map markers created for each checkpoint at race start
	protected ref array<ref SCR_MapMarkerBase> m_aCheckpointMarkers = {};

	// First player to die during the race
	protected int m_iFirstDeathPlayerId = -1;
	protected string m_sFirstDeathPlayerName = "";

	// Smoke prefab resource names — start=green, finish=red, intermediates cycle yellow/purple
	static const ResourceName SMOKE_START  = "{6BDBEFF5CE6E2F28}Prefabs/Systems/Smoke/Wrapper_Smoke_Green.et";
	static const ResourceName SMOKE_FINISH = "{49EBEAAC42260651}Prefabs/Systems/Smoke/Wrapper_Smoke_Red.et";
	static const ResourceName SMOKE_CP_A   = "{1D8922C3FE426E4E}Prefabs/Systems/Smoke/Wrapper_Smoke_Yellow.et";
	static const ResourceName SMOKE_CP_B   = "{2D854F52D8D6B31E}Prefabs/Systems/Smoke/Wrapper_Smoke_Purple.et";

	[Attribute("8", UIWidgets.EditBox, "How far past the edge of the road (metres) the checkpoint smoke is placed. Without a road nearby, the distance from the checkpoint centre.", category: "Rally - Smoke")]
	protected float m_fSmokeOffsetDistance;

	[Attribute("true", UIWidgets.CheckBox, "Put the smoke on the downwind side of the road so it drifts away from the racing line", category: "Rally - Smoke")]
	protected bool m_bSmokeDownwind;

	[Attribute("30", UIWidgets.EditBox, "A road further than this (metres) from a checkpoint is ignored when placing its smoke", category: "Rally - Smoke")]
	protected float m_fSmokeRoadSearchRadius;

	// Smoke lasts ~60 s; refreshed before then, with the old smoke kept briefly so it never blinks out
	protected static const int SMOKE_REFRESH_MS = 50000;
	protected static const int SMOKE_OVERLAP_MS = 8000;

	// A tracked position further than this from the previous poll is a respawn or teleport, not driving
	protected static const float MAX_SWEEP_DISTANCE = 150;

	//===================================================================================
	// REPLICATED STATE
	//===================================================================================

	// Packed standings updated by the server each second:
	// "playerId;lapsCompleted;nextCP;lapStartMs;finished | ..."  (sorted 1st → last, "|" separator)
	[RplProp()]
	protected string m_sReplicatedStandings = "";

	[RplProp(onRplName: "OnRaceActiveChanged")]
	protected bool m_bRaceActive = false;

	// The server's world time (ms), sent with every standings update. The lap start times in the
	// standings are server world times, and a client's own world clock started when it loaded the
	// world, so clients time laps against this instead (see GetServerNowMs).
	[RplProp(onRplName: "OnServerClockReceived")]
	protected int m_iServerClockMs = 0;
	protected int m_iServerClockReceivedAtMs = -1; // client world time when m_iServerClockMs arrived

	// Guard to ensure results are only triggered once
	protected bool m_bResultsTriggered = false;

	//===================================================================================
	// CLIENT-SIDE HUD STATE
	//===================================================================================

	// CRF_RallyHUD.layout: a panel (position, lap time, what to do next, a course progress bar, then
	// one CRF_RallyHUDRow.layout row per racer shown) sized to its rows, and a checkpoint arrow
	protected static const ResourceName HUD_LAYOUT = "{4CBBF00D5B6E1A24}UI/layouts/HUD/Rally/CRF_RallyHUD.layout";
	protected static const ResourceName HUD_ROW_LAYOUT = "{4CBBF00D5B6E1B01}UI/layouts/HUD/Rally/CRF_RallyHUDRow.layout";
	protected static const float HUD_PANEL_WIDTH = 300;
	protected static const float HUD_PANEL_RIGHT = 16;		// RallyPanel's gap to the screen edge
	protected static const float HUD_PANEL_TOP = 150;		// RallyPanel's PositionY
	protected static const float HUD_MINIMAP_GAP = 10;		// between RallyPanel and MinimapPanel
	protected static const float HUD_ROWS_TOP = 104;		// "Rows" in the layout
	protected static const float HUD_ROW_HEIGHT = 24;		// RallyRow's height override
	protected static const float HUD_PANEL_PADDING = 8;
	protected static const float HUD_PROGRESS_INSET = 14;	// ProgressTrack's left/right offset

	protected Widget m_wHUD = null;
	protected Widget m_wPanel = null;
	protected TextWidget m_wHeaderText = null;    // what to do next: head to start / next CP / finish
	protected TextWidget m_wPositionText = null;
	protected TextWidget m_wPositionTotalText = null;
	protected TextWidget m_wLapTimeText = null;
	protected TextWidget m_wLapTimeLabel = null;
	protected Widget m_wProgressFill = null;
	protected Widget m_wRows = null;
	protected ref array<Widget> m_aRowWidgets = {};
	protected ImageWidget m_wCPArrow = null;
	protected TextWidget m_wCPDistanceText = null;
	protected Widget m_wMinimapPanel = null;			// follows the bottom of RallyPanel
	protected ref CRF_RallyMinimap m_Minimap;			// kept when the HUD goes down - its course trace isn't cheap

	//===================================================================================
	// STATIC ACCESSOR
	//===================================================================================

	protected static CRF_RallyGamemodeComponent m_sInstance;

	void CRF_RallyGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	void ~CRF_RallyGamemodeComponent()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	static CRF_RallyGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		SetEventMask(owner, EntityEvent.FRAME);

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(ServerInit, 2500, false);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		DestroyHUD();
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(SpawnCheckpointSmokes);
			GetGame().GetCallqueue().Remove(DeleteOldSmokes);
			GetGame().GetCallqueue().Remove(CheckAllRacersFinishedOrDead);
			ClearCheckpointSmokes();
			ClearCheckpointMarkers();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnServerClockReceived()
	{
		m_iServerClockReceivedAtMs = GetGame().GetWorld().GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	//! The server's current world time in ms (estimated on clients), or -1 before the first update
	protected int GetServerNowMs()
	{
		int localNow = GetGame().GetWorld().GetWorldTime();
		if (Replication.IsServer())
			return localNow;

		if (m_iServerClockReceivedAtMs < 0)
			return -1;

		return m_iServerClockMs + (localNow - m_iServerClockReceivedAtMs);
	}

	//===================================================================================
	// SERVER INITIALISATION
	//===================================================================================

	protected void ServerInit()
	{
		CacheCheckpointPositions();

		if (m_aCheckpointPositions.Count() < 2)
		{
			Print("[CRF_Rally] ERROR: Fewer than 2 checkpoint entities found. Ensure 'rally_start' and 'rally_end' are present in the world.", LogLevel.ERROR);
			return;
		}

		if (m_iLapsToComplete < 1)
			m_iLapsToComplete = 1;

		// Racers are registered by PollAllPlayerCheckpoints once they have a body, so players still
		// in slotting or spectating never show up in the standings

		m_bRaceInitialised = true;
		m_bRaceActive      = true;
		Replication.BumpMe();

		// Spawn smoke at each checkpoint and refresh it before it burns out (smoke TTL ~60s)
		ComputeSmokePositions();
		SpawnCheckpointSmokes();
		GetGame().GetCallqueue().CallLater(SpawnCheckpointSmokes, SMOKE_REFRESH_MS, true);

		// Place persistent map markers for each checkpoint
		SpawnCheckpointMarkers();

		Print(string.Format("[CRF_Rally] Race initialised — %1 checkpoints, %2 lap(s), radius=%.0fm",
			m_aCheckpointPositions.Count(), m_iLapsToComplete, m_fCheckpointRadius), LogLevel.NORMAL);
	}

	protected void CacheCheckpointPositions()
	{
		m_aCheckpointPositions.Clear();
		m_aCheckpointNames.Clear();

		// Start line
		if (!AddCheckpoint("rally_start"))
		{
			Print("[CRF_Rally] ERROR: 'rally_start' entity not found in the world!", LogLevel.ERROR);
			return;
		}

		// Intermediate checkpoints — rally_cp1, rally_cp2, ... (stop at first gap)
		int cpIndex = 1;
		while (AddCheckpoint(string.Format("rally_cp%1", cpIndex)))
		{
			cpIndex++;
		}

		// Finish line
		if (!AddCheckpoint("rally_end"))
			Print("[CRF_Rally] ERROR: 'rally_end' entity not found in the world!", LogLevel.ERROR);
	}

	//! \return false if there's no entity with this name
	protected bool AddCheckpoint(string entityName)
	{
		IEntity checkpoint = GetGame().GetWorld().FindEntityByName(entityName);
		if (!checkpoint)
			return false;

		m_aCheckpointPositions.Insert(checkpoint.GetOrigin());
		m_aCheckpointNames.Insert(entityName);
		return true;
	}

	// Works out where every checkpoint's smoke goes (see ComputeSmokePosition)
	protected void ComputeSmokePositions()
	{
		m_aSmokePositions.Clear();
		foreach (int i, vector cpPos : m_aCheckpointPositions)
		{
			m_aSmokePositions.Insert(ComputeSmokePosition(i));
		}
	}

	// Beside the road rather than on it, where it would blind drivers:
	//  1. an entity named "<checkpoint name>_smoke", if the mission maker placed one
	//  2. else past the edge of the nearest road, on the downwind side (m_bSmokeDownwind) so the plume
	//     drifts away from the racing line
	//  3. else (no road nearby) beside the course, perpendicular to its direction through the checkpoint
	protected vector ComputeSmokePosition(int index)
	{
		vector cpPos = m_aCheckpointPositions[index];

		IEntity overrideEnt = GetGame().GetWorld().FindEntityByName(m_aCheckpointNames[index] + "_smoke");
		if (overrideEnt)
			return overrideEnt.GetOrigin();

		vector roadCentre;
		vector roadDir;
		float roadHalfWidth;
		vector basePos = cpPos;
		vector forward;
		float clearance = m_fSmokeOffsetDistance;
		if (FindRoadAt(cpPos, roadCentre, roadDir, roadHalfWidth))
		{
			basePos = roadCentre;
			forward = roadDir;
			clearance = roadHalfWidth + m_fSmokeOffsetDistance;
		}
		else
		{
			forward = GetCourseDirection(index);
		}

		// Horizontal perpendicular to the road/course: (z, 0, -x)
		vector side = Vector(forward[2], 0, -forward[0]).Normalized();
		if (side == vector.Zero)
			side = "1 0 0";

		if (m_bSmokeDownwind && vector.DotXZ(side, GetDownwindDirection()) < 0)
			side = side * -1;

		vector smokePos = basePos + side * clearance;
		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);
		return smokePos;
	}

	// The road nearest the checkpoint (within m_fSmokeRoadSearchRadius): the closest point on its
	// centreline, the road's direction there, and half its width
	protected bool FindRoadAt(vector pos, out vector centre, out vector direction, out float halfWidth)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return false;

		RoadNetworkManager roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
			return false;

		BaseRoad road;
		float distance;
		roadManager.GetClosestRoad(pos, road, distance);
		if (!road || distance > m_fSmokeRoadSearchRadius)
			return false;

		array<vector> points = {};
		road.GetPoints(points);
		if (points.Count() < 2)
			return false;

		// Closest centreline segment to the checkpoint (horizontal distance)
		float bestDistSq = 1e10;
		for (int i = 0; i < points.Count() - 1; i++)
		{
			vector a = points[i];
			vector b = points[i + 1];
			vector closest = ClosestPointOnSegment2D(pos, a, b);
			float distSq = vector.DistanceSqXZ(pos, closest);
			if (distSq >= bestDistSq)
				continue;

			bestDistSq = distSq;
			centre = closest;
			direction = Vector(b[0] - a[0], 0, b[2] - a[2]).Normalized();
		}

		if (direction == vector.Zero)
			return false;

		halfWidth = Math.Max(road.GetWidth() * 0.5, 2);
		return true;
	}

	// Direction of the course through a checkpoint: the average of the way in and the way out
	protected vector GetCourseDirection(int index)
	{
		int last = m_aCheckpointPositions.Count() - 1;
		vector cpPos = m_aCheckpointPositions[index];
		vector dir;
		if (index > 0)
			dir += (cpPos - m_aCheckpointPositions[index - 1]).Normalized();
		if (index < last)
			dir += (m_aCheckpointPositions[index + 1] - cpPos).Normalized();

		dir[1] = 0;
		return dir.Normalized();
	}

	// Horizontal direction the wind blows towards. GetWindDirection is where it comes from (the map's
	// weather display turns it 180° to draw the arrow), clockwise from north.
	protected vector GetDownwindDirection()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return vector.Zero;

		TimeAndWeatherManagerEntity weather = world.GetTimeAndWeatherManager();
		if (!weather)
			return vector.Zero;

		float towards = (weather.GetWindDirection() + 180) * Math.DEG2RAD;
		return Vector(Math.Sin(towards), 0, Math.Cos(towards));
	}

	// Spawns colored smoke at every checkpoint. Called at race start, then every SMOKE_REFRESH_MS
	// before the previous smoke burns out; the old smokes are removed SMOKE_OVERLAP_MS later.
	protected void SpawnCheckpointSmokes()
	{
		DeleteOldSmokes();
		foreach (IEntity smokeEnt : m_aSmokeEntities)
			m_aOldSmokeEntities.Insert(smokeEnt);
		m_aSmokeEntities.Clear();

		if (m_aSmokePositions.Count() != m_aCheckpointPositions.Count())
			ComputeSmokePositions();

		int last = m_aCheckpointPositions.Count() - 1;
		for (int i = 0; i <= last; i++)
		{
			ResourceName prefab;
			if (i == 0)
				prefab = SMOKE_START;        // green  — start line
			else if (i == last)
				prefab = SMOKE_FINISH;       // red    — finish line
			else if (i % 2 == 1)
				prefab = SMOKE_CP_A;         // yellow — odd intermediate CPs
			else
				prefab = SMOKE_CP_B;         // purple — even intermediate CPs

			EntitySpawnParams params = new EntitySpawnParams();
			Math3D.MatrixIdentity4(params.Transform);
			params.Transform[3] = m_aSmokePositions[i];

			Resource resource = Resource.Load(prefab); // kept alive for the spawn
			if (!resource || !resource.IsValid())
				continue;

			IEntity smoke = GetGame().SpawnEntityPrefab(resource, null, params);
			if (smoke)
				m_aSmokeEntities.Insert(smoke);
		}

		GetGame().GetCallqueue().Remove(DeleteOldSmokes);
		GetGame().GetCallqueue().CallLater(DeleteOldSmokes, SMOKE_OVERLAP_MS, false);
	}

	protected void DeleteOldSmokes()
	{
		foreach (IEntity ent : m_aOldSmokeEntities)
		{
			if (ent)
				SCR_EntityHelper.DeleteEntityAndChildren(ent);
		}
		m_aOldSmokeEntities.Clear();
	}

	protected void ClearCheckpointSmokes()
	{
		DeleteOldSmokes();
		foreach (IEntity ent : m_aSmokeEntities)
		{
			if (ent)
				SCR_EntityHelper.DeleteEntityAndChildren(ent);
		}
		m_aSmokeEntities.Clear();
	}

	// Closest point to p on segment a-b, ignoring height (the result takes the segment's height there)
	protected static vector ClosestPointOnSegment2D(vector p, vector a, vector b)
	{
		float abX = b[0] - a[0];
		float abZ = b[2] - a[2];
		float lengthSq = abX * abX + abZ * abZ;
		if (lengthSq <= 0.0001)
			return a;

		float t = Math.Clamp(((p[0] - a[0]) * abX + (p[2] - a[2]) * abZ) / lengthSq, 0, 1);
		return a + (b - a) * t;
	}

	// Shortest distance from p to segment a-b, in 3D
	protected static float DistanceToSegment(vector p, vector a, vector b)
	{
		vector ab = b - a;
		float lengthSq = ab.LengthSq();
		if (lengthSq <= 0.0001)
			return vector.Distance(p, a);

		float t = Math.Clamp(vector.Dot(p - a, ab) / lengthSq, 0, 1);
		return vector.Distance(p, a + ab * t);
	}

	// Places a labelled map marker at every checkpoint position.
	// rally_start → "Start" (green flag), rally_cpN → "Checkpoint N" (orange waypoint), rally_end → "Finish" (red flag).
	protected void SpawnCheckpointMarkers()
	{
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMan)
			return;

		m_aCheckpointMarkers.Clear();
		int last = m_aCheckpointPositions.Count() - 1;
		for (int i = 0; i <= last; i++)
		{
			vector pos = m_aCheckpointPositions[i];
			SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
			marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);

			string label;
			if (i == 0)
			{
				label = "Start";
				marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.FLAG);
				marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.GREEN);
			}
			else if (i == last)
			{
				label = "Finish";
				marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.FLAG);
				marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.RED);
			}
			else
			{
				label = string.Format("Checkpoint %1", i);
				marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.WAYPOINT);
				marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.ORANGE);
			}

			marker.SetCustomText(label);
			marker.SetWorldPos(pos[0], pos[2]);
			marker.m_bIsShared = true;
			markerMan.InsertStaticMarker(marker, false, true);
			m_aCheckpointMarkers.Insert(marker);
		}
	}

	protected void ClearCheckpointMarkers()
	{
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		foreach (SCR_MapMarkerBase marker : m_aCheckpointMarkers)
		{
			if (markerMan)
				markerMan.RemoveStaticMarker(marker);
		}
		m_aCheckpointMarkers.Clear();
	}

	//===================================================================================
	// PLAYER MANAGEMENT
	//===================================================================================

	protected CRF_RallyPlayerEntry GetOrCreateEntry(int playerId)
	{
		CRF_RallyPlayerEntry existing = GetEntry(playerId);
		if (existing)
			return existing;

		CRF_RallyPlayerEntry entry = new CRF_RallyPlayerEntry(playerId);
		m_aPlayerEntries.Insert(entry);
		return entry;
	}

	protected CRF_RallyPlayerEntry GetEntry(int playerId)
	{
		foreach (CRF_RallyPlayerEntry e : m_aPlayerEntries)
		{
			if (e.m_iPlayerId == playerId)
				return e;
		}
		return null;
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (!Replication.IsServer())
			return;

		for (int i = m_aPlayerEntries.Count() - 1; i >= 0; i--)
		{
			if (m_aPlayerEntries[i].m_iPlayerId == playerId)
			{
				m_aPlayerEntries.Remove(i);
				break;
			}
		}
	}

	//===================================================================================
	// FRAME UPDATE
	//===================================================================================

	protected float m_fServerTickBuffer    = 0;
	protected float m_fStandingsTickBuffer = 0;
	protected float m_fHUDBuffer           = 0;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (Replication.IsServer() && m_bRaceInitialised)
		{
			// Checkpoint polls at 4 Hz — fast enough for vehicle speeds
			m_fServerTickBuffer += timeSlice;
			if (m_fServerTickBuffer >= 0.25)
			{
				m_fServerTickBuffer = 0;
				PollAllPlayerCheckpoints();
			}

			// Standings replication at ~1 Hz
			m_fStandingsTickBuffer += timeSlice;
			if (m_fStandingsTickBuffer >= 1.0)
			{
				m_fStandingsTickBuffer = 0;
				BuildAndReplicateStandings();
			}
		}

		// HUD update (skip on pure dedicated server)
		if (RplSession.Mode() != RplMode.Dedicated && m_bRaceActive)
		{
			m_fHUDBuffer += timeSlice;
			if (m_fHUDBuffer >= 0.1) // 10 Hz HUD refresh
			{
				m_fHUDBuffer = 0;
				UpdateHUD();
			}
		}
	}

	//===================================================================================
	// SERVER — CHECKPOINT POLLING
	//===================================================================================

	protected void PollAllPlayerCheckpoints()
	{
		// Nothing counts before the round is live (briefing, slotting, safestart)
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		bool live = gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.GAME;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int pid : playerIds)
		{
			IEntity playerEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);

			// Only players with a body race - slotting, spectating and dead players aren't registered
			if (!playerEnt || COA_EntityHelper.IsSpectator(playerEnt))
				continue;

			CRF_RallyPlayerEntry entry = GetOrCreateEntry(pid);
			if (entry.m_bFinished)
				continue;

			// Track from the vehicle position when seated, otherwise from the character
			IEntity trackEnt = playerEnt;
			if (playerEnt.GetParent())
				trackEnt = playerEnt.GetParent();

			vector pos = trackEnt.GetOrigin();
			vector lastPos = pos;
			if (entry.m_bHasLastPos && vector.Distance(entry.m_vLastPos, pos) <= MAX_SWEEP_DISTANCE)
				lastPos = entry.m_vLastPos;

			entry.m_vLastPos = pos;
			entry.m_bHasLastPos = true;

			// Optional vehicle requirement: character must be seated in a vehicle
			if (!live || (m_bRequireVehicle && !playerEnt.GetParent()))
				continue;

			CheckCheckpointCrossing(entry, lastPos, pos);
		}
	}

	// Checks the path travelled since the last poll (lastPos to pos), not just where the player is now:
	// at 4 Hz a fast car covers 10+ m between polls and could otherwise clip past a checkpoint unseen
	protected void CheckCheckpointCrossing(notnull CRF_RallyPlayerEntry entry, vector lastPos, vector pos)
	{
		int cpIndex = entry.m_iNextCheckpoint;
		if (cpIndex >= m_aCheckpointPositions.Count())
			return;

		if (DistanceToSegment(m_aCheckpointPositions[cpIndex], lastPos, pos) > m_fCheckpointRadius)
			return;

		OnCheckpointCrossed(entry, cpIndex);
	}

	protected void OnCheckpointCrossed(notnull CRF_RallyPlayerEntry entry, int cpIndex)
	{
		int worldTimeMs          = GetGame().GetWorld().GetWorldTime();
		int totalCPs             = m_aCheckpointPositions.Count();
		int pid                  = entry.m_iPlayerId;

		if (cpIndex == 0)
		{
			// ── START LINE ──────────────────────────────────────────────────────────
			entry.m_iLapStartTimeMs = worldTimeMs;
			entry.m_iLastCPTimeMs   = worldTimeMs;
			entry.m_iNextCheckpoint = 1;

			// Record race start only once (first crossing of start line)
			if (entry.m_iRaceStartTimeMs == 0)
				entry.m_iRaceStartTimeMs = worldTimeMs;

			string startName = GetGame().GetPlayerManager().GetPlayerName(pid);
			Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 crossed the start line!", startName));
			Rpc(RpcDo_CheckpointNotify, pid, "START LINE — GO!");
		}
		else if (cpIndex == totalCPs - 1)
		{
			// ── FINISH LINE ─────────────────────────────────────────────────────────
			float lapTimeSecs = (worldTimeMs - entry.m_iLapStartTimeMs) * 0.001;
			if (entry.m_iLapStartTimeMs == 0)
				lapTimeSecs = 0; // safety for the unusual case they skipped start

			entry.m_iLapsCompleted++;
			entry.m_iLastCPTimeMs = worldTimeMs;

			if (entry.m_fBestLapTime < 0 || lapTimeSecs < entry.m_fBestLapTime)
				entry.m_fBestLapTime = lapTimeSecs;

			if (entry.m_iLapsCompleted >= m_iLapsToComplete)
			{
				// ── RACE FINISHED ───────────────────────────────────────────────────
				entry.m_bFinished       = true;
				entry.m_iNextCheckpoint = totalCPs; // clamp past last CP

				// Record total race time from when the player first crossed the start line
				if (entry.m_iRaceStartTimeMs > 0)
					entry.m_fFinishTimeSecs = (worldTimeMs - entry.m_iRaceStartTimeMs) * 0.001;
				else
					entry.m_fFinishTimeSecs = lapTimeSecs;

				BuildAndReplicateStandings(); // immediate update for accurate finish position
				int finishPos = GetPlayerStandingPosition(pid);

				// Total race time, not the last lap (they only differ on multi-lap races)
				string finisherName = GetGame().GetPlayerManager().GetPlayerName(pid);
				Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 finished %2 in %3!", finisherName, PositionLabel(finishPos), FormatTime(entry.m_fFinishTimeSecs)));

				Rpc(RpcDo_FinishNotify, pid, lapTimeSecs, entry.m_iLapsCompleted, finishPos);

				if (finishPos >= 1 && finishPos <= 3)
				{
					Rpc(RpcDo_PodiumNotify, finishPos, finisherName);
				}

				KillPlayerAndVehicle(pid);

				// Check if all players are now done — schedule with delay for death to resolve
				GetGame().GetCallqueue().CallLater(CheckAllRacersFinishedOrDead, 5000, false);
			}
			else
			{
				// ── LAP COMPLETE — more laps remain ─────────────────────────────────
				entry.m_iNextCheckpoint = 0;
				entry.m_iLapStartTimeMs = worldTimeMs;
				string lapName = GetGame().GetPlayerManager().GetPlayerName(pid);
				Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 completed lap %2 in %3.", lapName, entry.m_iLapsCompleted, FormatTime(lapTimeSecs)));
				Rpc(RpcDo_LapCompleteNotify, pid, lapTimeSecs, entry.m_iLapsCompleted);
			}
		}
		else
		{
			// ── INTERMEDIATE CHECKPOINT ─────────────────────────────────────────────
			entry.m_iNextCheckpoint = cpIndex + 1;
			entry.m_iLastCPTimeMs   = worldTimeMs;

			// cpIndex 1 is the first real CP after start; adjust the label for display.
			// Only the racer is told - a chat line per checkpoint per racer flooded chat for everyone.
			string cpLabel = string.Format("Checkpoint %1 / %2", cpIndex, totalCPs - 2);
			Rpc(RpcDo_CheckpointNotify, pid, cpLabel);
		}
	}

	//===================================================================================
	// SERVER — STANDINGS
	//===================================================================================

	protected void BuildAndReplicateStandings()
	{
		array<ref CRF_RallyPlayerEntry> sorted = GetSortedEntries();
		int totalCPs = Math.Max(1, m_aCheckpointPositions.Count());

		string packed = "";
		foreach (CRF_RallyPlayerEntry e : sorted)
		{
			if (!packed.IsEmpty())
				packed += "|";
			// Format: playerId;lapsCompleted;nextCP;lapStartMs;finished;groupId
			int finishedInt = 0;
			if (e.m_bFinished)
				finishedInt = 1;

			int groupId = 0;
			SCR_GroupsManagerComponent gMgr = SCR_GroupsManagerComponent.GetInstance();
			if (gMgr)
			{
				SCR_AIGroup grp = gMgr.GetPlayerGroup(e.m_iPlayerId);
				if (grp)
					groupId = grp.GetID();
			}

			int deadInt = 0;
			if (e.m_bDead)
				deadInt = 1;

			packed += string.Format("%1;%2;%3;%4;%5;%6;%7;%8",
				e.m_iPlayerId,
				e.m_iLapsCompleted,
				e.m_iNextCheckpoint,
				e.m_iLapStartTimeMs,
				finishedInt,
				groupId,
				deadInt,
				e.m_iDeathTimeMs);
			packed += GetExtraStandingFields(e);
		}

		// The clock goes out with every update (once a second) so client lap timers stay in step
		m_sReplicatedStandings = packed;
		m_iServerClockMs = GetGame().GetWorld().GetWorldTime();
		Replication.BumpMe();
	}

	// Extra standings fields for a variant to send to its HUD (each starting with ";"), read back from
	// CRF_RallyHUDRow.m_aExtra. See CRF_RallyStageGamemodeComponent.
	protected string GetExtraStandingFields(CRF_RallyPlayerEntry e)
	{
		return "";
	}

	// Returns entries sorted best → worst by (progress desc, then lastCPTime asc)
	protected array<ref CRF_RallyPlayerEntry> GetSortedEntries()
	{
		array<ref CRF_RallyPlayerEntry> sorted = {};
		foreach (CRF_RallyPlayerEntry e : m_aPlayerEntries)
			sorted.Insert(e);

		int n         = sorted.Count();
		int totalCPs  = Math.Max(1, m_aCheckpointPositions.Count());

		// Insertion sort — fast enough for the small player counts in a race
		for (int i = 1; i < n; i++)
		{
			CRF_RallyPlayerEntry key = sorted[i];
			int j = i - 1;

			while (j >= 0 && IsAhead(key, sorted[j], totalCPs))
			{
				sorted[j + 1] = sorted[j];
				j--;
			}
			sorted[j + 1] = key;
		}

		return sorted;
	}

	// Returns true if 'a' should be ranked higher (ahead of) 'b'
	protected bool IsAhead(CRF_RallyPlayerEntry a, CRF_RallyPlayerEntry b, int totalCPs)
	{
		int progA = a.GetProgress(totalCPs);
		int progB = b.GetProgress(totalCPs);

		if (progA != progB)
			return progA > progB;

		// Same progress: the player who crossed the last checkpoint EARLIER is ahead
		// (they've had less time at that checkpoint = edge lead)
		if (a.m_iLastCPTimeMs != b.m_iLastCPTimeMs)
			return a.m_iLastCPTimeMs < b.m_iLastCPTimeMs;

		return false;
	}

	// Get the 1-based standing position of a player from the current sorted list
	protected int GetPlayerStandingPosition(int playerId)
	{
		array<ref CRF_RallyPlayerEntry> sorted = GetSortedEntries();
		for (int i = 0; i < sorted.Count(); i++)
		{
			if (sorted[i].m_iPlayerId == playerId)
				return i + 1;
		}
		return -1;
	}

	//===================================================================================
	// SERVER — HELPERS
	//===================================================================================

	protected void KillPlayerAndVehicle(int pid)
	{
		IEntity charEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		if (!charEnt)
			return;

		// Kill the character
		DamageManagerComponent charDmg = DamageManagerComponent.Cast(charEnt.FindComponent(DamageManagerComponent));
		if (charDmg && !charDmg.IsDestroyed())
			charDmg.SetHealthScaled(0);

		// Kill the vehicle if the character is seated in one
		IEntity vehicle = charEnt.GetParent();
		if (!vehicle)
			return;

		DamageManagerComponent vehDmg = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		if (vehDmg && !vehDmg.IsDestroyed())
			vehDmg.SetHealthScaled(0);
	}

	// Called (with a delay) after each player finishes to check if all racers are done.
	protected void CheckAllRacersFinishedOrDead()
	{
		if (!Replication.IsServer())
			return;

		foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
		{
			if (entry.m_bFinished)
				continue;

			IEntity ent = GetGame().GetPlayerManager().GetPlayerControlledEntity(entry.m_iPlayerId);
			if (!ent || COA_EntityHelper.IsSpectator(ent))
				continue; // no body = disconnected, dead or spectating

			DamageManagerComponent dmg = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
			if (dmg && !dmg.IsDestroyed())
				return; // at least one player is still alive and hasn't finished
		}

		TriggerResults();
	}

	// Ends the race and broadcasts the results to all clients.
	protected void TriggerResults()
	{
		if (m_bResultsTriggered)
			return;

		m_bResultsTriggered = true;
		m_bRaceActive = false;
		Replication.BumpMe();

		string packed = BuildResultsString();
		Rpc(RpcDo_ShowRallyResults, packed, m_sFirstDeathPlayerName);
	}

	// Tracks the first player to die during an active race.
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!m_bRaceActive)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0)
			return;

		// Mark entry as dead and freeze the death time for HUD display
		CRF_RallyPlayerEntry entry = GetEntry(victimId);
		if (entry && !entry.m_bDead && !entry.m_bFinished)
		{
			entry.m_bDead        = true;
			entry.m_iDeathTimeMs = GetGame().GetWorld().GetWorldTime();
			BuildAndReplicateStandings();

			// The last racer out ends the race too - before, the check only ran after someone
			// finished, so a race where the remaining racers all died never showed its results
			GetGame().GetCallqueue().Remove(CheckAllRacersFinishedOrDead);
			GetGame().GetCallqueue().CallLater(CheckAllRacersFinishedOrDead, 5000, false);
		}

		if (m_iFirstDeathPlayerId >= 0)
			return;

		m_iFirstDeathPlayerId = victimId;
		m_sFirstDeathPlayerName = GetGame().GetPlayerManager().GetPlayerName(victimId);
	}

	// Builds a packed results string sorted by: finishers (by race time asc) then DNF (by progress desc).
	// Format per entry: "playerName;finishTimeSecs;bestLapSecs"  separated by "|"
	protected string BuildResultsString()
	{
		array<ref CRF_RallyPlayerEntry> finishedEntries = {};
		array<ref CRF_RallyPlayerEntry> dnfEntries = {};

		foreach (CRF_RallyPlayerEntry e : m_aPlayerEntries)
		{
			if (e.m_bFinished)
				finishedEntries.Insert(e);
			else
				dnfEntries.Insert(e);
		}

		// Sort finishers by total race time ascending (selection sort)
		for (int i = 0; i < finishedEntries.Count() - 1; i++)
		{
			int bestIdx = i;
			for (int j = i + 1; j < finishedEntries.Count(); j++)
			{
				if (finishedEntries[j].m_fFinishTimeSecs < finishedEntries[bestIdx].m_fFinishTimeSecs)
					bestIdx = j;
			}
			if (bestIdx != i)
			{
				CRF_RallyPlayerEntry tmp = finishedEntries[i];
				finishedEntries[i] = finishedEntries[bestIdx];
				finishedEntries[bestIdx] = tmp;
			}
		}

		// DNF entries in standings order (furthest progress first)
		array<ref CRF_RallyPlayerEntry> sortedAll = GetSortedEntries();
		array<ref CRF_RallyPlayerEntry> dnfSorted = {};
		foreach (CRF_RallyPlayerEntry e : sortedAll)
		{
			if (!e.m_bFinished)
				dnfSorted.Insert(e);
		}

		SCR_GroupsManagerComponent gMgr = SCR_GroupsManagerComponent.GetInstance();
		ref array<int> seenGroupIds = {};
		string packed = "";

		foreach (CRF_RallyPlayerEntry e : finishedEntries)
		{
			string displayName = "";
			int groupId = 0;

			if (m_bTeamMode && gMgr)
			{
				SCR_AIGroup grp = gMgr.GetPlayerGroup(e.m_iPlayerId);
				if (grp)
				{
					groupId = grp.GetID();
					if (groupId > 0)
					{
						if (seenGroupIds.Contains(groupId))
							continue;
						seenGroupIds.Insert(groupId);
						displayName = "Team " + grp.GetCustomNameWithOriginal();
					}
				}
			}

			if (displayName.IsEmpty())
				displayName = GetGame().GetPlayerManager().GetPlayerName(e.m_iPlayerId);
			displayName.Replace(";", " ");
			displayName.Replace("|", " ");

			if (!packed.IsEmpty())
				packed += "|";
			packed += string.Format("%1;%2;%3", displayName, e.m_fFinishTimeSecs, e.m_fBestLapTime);
		}

		foreach (CRF_RallyPlayerEntry e : dnfSorted)
		{
			string displayName = "";
			int groupId = 0;

			if (m_bTeamMode && gMgr)
			{
				SCR_AIGroup grp = gMgr.GetPlayerGroup(e.m_iPlayerId);
				if (grp)
				{
					groupId = grp.GetID();
					if (groupId > 0)
					{
						if (seenGroupIds.Contains(groupId))
							continue;
						seenGroupIds.Insert(groupId);
						displayName = "Team " + grp.GetCustomNameWithOriginal();
					}
				}
			}

			if (displayName.IsEmpty())
				displayName = GetGame().GetPlayerManager().GetPlayerName(e.m_iPlayerId);
			displayName.Replace(";", " ");
			displayName.Replace("|", " ");

			if (!packed.IsEmpty())
				packed += "|";
			packed += string.Format("%1;%2;%3", displayName, -1.0, e.m_fBestLapTime);
		}

		return packed;
	}

	//===================================================================================
	// RPCs — sent by server, received by all clients (clients filter by player ID)
	//===================================================================================

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_CheckpointNotify(int targetPlayerId, string message)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != targetPlayerId)
			return;

		SCR_PopUpNotification.GetInstance().PopupMsg(message, 4);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_LapCompleteNotify(int targetPlayerId, float lapTimeSecs, int lapsCompleted)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != targetPlayerId)
			return;

		SCR_PopUpNotification.GetInstance().PopupMsg(
			string.Format("LAP %1 COMPLETE  —  %2", lapsCompleted, FormatTime(lapTimeSecs)), 6);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_FinishNotify(int targetPlayerId, float lapTimeSecs, int lapsCompleted, int position)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != targetPlayerId)
			return;

		SCR_PopUpNotification.GetInstance().PopupMsg(
			string.Format("FINISHED!  —  %1  |  Last lap: %2", PositionLabel(position), FormatTime(lapTimeSecs)), 10);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PodiumNotify(int position, string playerName)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(
			string.Format("%1 finishes %2!", playerName, PositionLabel(position)), 8);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ChatBroadcast(string message)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComp)
			return;

		chatComp.ShowMessage(message);
	}

	// Broadcast by server when the race is fully over.
	// Parses packed results and opens the results menu on every client.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ShowRallyResults(string packed, string firstDeathName)
	{
		GetGame().GetMenuManager().CloseAllMenus();
		MenuBase resultsMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RallyResults);
		if (!resultsMenu)
			return;

		Widget root = resultsMenu.GetRootWidget();
		if (!root)
			return;

		// Parse: "playerName;finishTimeSecs;bestLapSecs|playerName2;..."
		array<string> entries = {};
		packed.Split("|", entries, true);

		string tableText = "";
		float fastestLapTime = -1;
		string fastestLapName = "";

		for (int i = 0; i < entries.Count(); i++)
		{
			array<string> fields = {};
			entries[i].Split(";", fields, true);
			if (fields.Count() < 3)
				continue;

			string playerName = fields[0];
			float finishTime  = fields[1].ToFloat();
			float bestLap     = fields[2].ToFloat();

			if (playerName.Length() > 16)
				playerName = playerName.Substring(0, 16);

			string finishStr;
			if (finishTime >= 0)
				finishStr = FormatTime(finishTime);
			else
				finishStr = "DNF";

			string bestLapStr;
			if (bestLap >= 0)
				bestLapStr = FormatTime(bestLap);
			else
				bestLapStr = "--:--.---";

			if (!tableText.IsEmpty())
				tableText += "\n";
			tableText += string.Format("%1  %2  %3  BL: %4",
				PositionLabel(i + 1), playerName, finishStr, bestLapStr);

			if (bestLap >= 0 && (fastestLapTime < 0 || bestLap < fastestLapTime))
			{
				fastestLapTime = bestLap;
				fastestLapName = playerName;
			}
		}

		TextWidget resultsWidget = TextWidget.Cast(root.FindAnyWidget("ResultsText"));
		if (resultsWidget)
			resultsWidget.SetText(tableText);

		TextWidget fastestWidget = TextWidget.Cast(root.FindAnyWidget("FastestLapText"));
		if (fastestWidget)
		{
			if (fastestLapTime >= 0)
				fastestWidget.SetText(string.Format("Fastest Lap: %1  —  %2", fastestLapName, FormatTime(fastestLapTime)));
			else
				fastestWidget.SetText("Fastest Lap: —");
		}

		TextWidget firstDeathWidget = TextWidget.Cast(root.FindAnyWidget("FirstDeathText"));
		if (firstDeathWidget)
		{
			if (!firstDeathName.IsEmpty())
				firstDeathWidget.SetText(string.Format("First Death: %1", firstDeathName));
			else
				firstDeathWidget.SetText("First Death: —");
		}

		DestroyHUD();
	}

	//===================================================================================
	// CLIENT — HUD
	//===================================================================================

	protected void OnRaceActiveChanged()
	{
		if (!m_bRaceActive)
			DestroyHUD();
	}

	protected void CreateHUD()
	{
		if (m_wHUD || RplSession.Mode() == RplMode.Dedicated)
			return;

		if (!SCR_PlayerController.GetLocalControlledEntity())
			return;

		m_wHUD = GetGame().GetWorkspace().CreateWidgets(HUD_LAYOUT);
		if (!m_wHUD)
		{
			Print("[CRF_Rally] WARNING: Could not load CRF_RallyHUD.layout — HUD will not be shown.", LogLevel.WARNING);
			return;
		}

		m_wPanel             = m_wHUD.FindAnyWidget("RallyPanel");
		m_wHeaderText        = TextWidget.Cast(m_wHUD.FindAnyWidget("HeaderText"));
		m_wPositionText      = TextWidget.Cast(m_wHUD.FindAnyWidget("PositionText"));
		m_wPositionTotalText = TextWidget.Cast(m_wHUD.FindAnyWidget("PositionTotalText"));
		m_wLapTimeText       = TextWidget.Cast(m_wHUD.FindAnyWidget("LapTimeText"));
		m_wLapTimeLabel      = TextWidget.Cast(m_wHUD.FindAnyWidget("LapTimeLabel"));
		m_wProgressFill      = m_wHUD.FindAnyWidget("ProgressFill");
		m_wRows              = m_wHUD.FindAnyWidget("Rows");
		m_wCPArrow           = ImageWidget.Cast(m_wHUD.FindAnyWidget("CPArrowWidget"));
		m_wCPDistanceText    = TextWidget.Cast(m_wHUD.FindAnyWidget("CPDistanceText"));
		m_wMinimapPanel      = m_wHUD.FindAnyWidget("MinimapPanel");
		m_aRowWidgets.Clear();

		if (m_wMinimapPanel)
		{
			m_wMinimapPanel.SetVisible(m_bShowMinimap);
			if (m_bShowMinimap)
			{
				if (!m_Minimap)
					m_Minimap = new CRF_RallyMinimap();

				m_Minimap.SetCanvas(CanvasWidget.Cast(m_wMinimapPanel.FindAnyWidget("MinimapCanvas")));
			}
		}
	}

	protected void DestroyHUD()
	{
		if (m_wHUD)
		{
			m_wHUD.RemoveFromHierarchy();
			m_wHUD = null;
			m_wPanel             = null;
			m_wHeaderText        = null;
			m_wPositionText      = null;
			m_wPositionTotalText = null;
			m_wLapTimeText       = null;
			m_wLapTimeLabel      = null;
			m_wProgressFill      = null;
			m_wRows              = null;
			m_wCPArrow           = null;
			m_wCPDistanceText    = null;
			m_wMinimapPanel      = null;
			m_aRowWidgets.Clear();
		}

		if (m_Minimap)
			m_Minimap.SetCanvas(null);
	}

	// Exactly count row widgets in the standings list (rows are reused between updates)
	protected void SetRowWidgetCount(int count)
	{
		if (!m_wRows)
			return;

		while (m_aRowWidgets.Count() > count)
		{
			Widget extra = m_aRowWidgets[m_aRowWidgets.Count() - 1];
			m_aRowWidgets.Remove(m_aRowWidgets.Count() - 1);
			if (extra)
				extra.RemoveFromHierarchy();
		}

		while (m_aRowWidgets.Count() < count)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(HUD_ROW_LAYOUT, m_wRows);
			if (!row)
				return;

			m_aRowWidgets.Insert(row);
		}
	}

	// A standings row's state (lap / FIN / KIA / WAIT), its colour, and the time shown
	protected void DescribeRow(CRF_RallyHUDRow row, int serverNowMs, out string status, out Color statusColor, out string time)
	{
		status = "";
		statusColor = Color.FromSRGBA(168, 178, 204, 255);
		time = FormatTime(GetRowTimeSecs(row, serverNowMs));
		if (row.m_bFinished)
		{
			status = "FIN";
			statusColor = Color.FromSRGBA(92, 196, 128, 255);
			time = "";
		}
		else if (row.m_bDead)
		{
			status = "KIA";
			statusColor = Color.FromSRGBA(232, 96, 96, 255);
		}
		else if (row.m_iLapStartMs == 0)
		{
			status = "WAIT";
			time = "";
		}
		else if (m_iLapsToComplete > 1)
		{
			status = string.Format("L%1", row.m_iLaps + 1);
		}
	}

	// The big timer for the local player's row
	protected string GetLapTimeText(CRF_RallyHUDRow row, int serverNowMs)
	{
		if (!row)
			return "--:--.---";

		if (row.m_bFinished)
			return "DONE";

		return FormatTime(GetRowTimeSecs(row, serverNowMs));
	}

	// The label above the big timer
	protected string GetLapTimeLabel()
	{
		return "LAP TIME";
	}

	// Fills one standings row: position, name, state and time (see DescribeRow)
	protected void FillRowWidget(Widget rowWidget, CRF_RallyHUDRow row, int position, int serverNowMs)
	{
		TextWidget posText = TextWidget.Cast(rowWidget.FindAnyWidget("PosText"));
		TextWidget nameText = TextWidget.Cast(rowWidget.FindAnyWidget("NameText"));
		TextWidget statusText = TextWidget.Cast(rowWidget.FindAnyWidget("StatusText"));
		TextWidget timeText = TextWidget.Cast(rowWidget.FindAnyWidget("TimeText"));
		Widget highlight = rowWidget.FindAnyWidget("Highlight");

		string status;
		Color statusColor;
		string time;
		DescribeRow(row, serverNowMs, status, statusColor, time);

		if (posText)
			posText.SetText(position.ToString());

		if (nameText)
			nameText.SetText(row.m_sName);

		if (statusText)
		{
			statusText.SetText(status);
			statusText.SetColor(statusColor);
		}

		if (timeText)
			timeText.SetText(time);

		// The local player's (or team's) row stands out
		if (highlight)
			highlight.SetOpacity(0);
		if (highlight && row.m_bLocal)
		{
			highlight.SetColor(Color.FromSRGBA(255, 200, 0, 36));
			highlight.SetOpacity(1);
		}
	}

	// How far through the whole race the player is, 0-1 (each lap is start, checkpoints, finish)
	protected float GetRaceProgress(CRF_RallyHUDRow own)
	{
		if (!own || own.m_iLapStartMs == 0)
			return 0;

		if (own.m_bFinished)
			return 1;

		int legsPerLap = Math.Max(1, m_aCheckpointPositions.Count() - 1);
		int legsDone = own.m_iLaps * legsPerLap + Math.Max(0, own.m_iNextCP - 1);
		float legsTotal = legsPerLap * Math.Max(1, m_iLapsToComplete);
		return Math.Clamp(legsDone / legsTotal, 0, 1);
	}

	// What the arrow points at: "START", "CP 3" or "FINISH"
	protected string GetCheckpointLabel(int index)
	{
		if (index <= 0)
			return "START";

		if (index >= m_aCheckpointPositions.Count() - 1)
			return "FINISH";

		return string.Format("CP %1", index);
	}

	// The replicated standings, in standings order
	protected array<ref CRF_RallyHUDRow> ParseStandings()
	{
		array<ref CRF_RallyHUDRow> rows = {};
		array<string> entries = {};
		m_sReplicatedStandings.Split("|", entries, true);

		foreach (string entryText : entries)
		{
			// playerId;lapsCompleted;nextCP;lapStartMs;finished;groupId;dead;deathMs
			array<string> fields = {};
			entryText.Split(";", fields, true);
			if (fields.Count() < 5)
				continue;

			CRF_RallyHUDRow row = new CRF_RallyHUDRow();
			row.m_iPlayerId   = fields[0].ToInt();
			row.m_iLaps       = fields[1].ToInt();
			row.m_iNextCP     = fields[2].ToInt();
			row.m_iLapStartMs = fields[3].ToInt();
			row.m_bFinished   = fields[4].ToInt() == 1;
			if (fields.Count() >= 6)
				row.m_iGroupId = fields[5].ToInt();
			row.m_bDead = fields.Count() >= 7 && fields[6].ToInt() == 1;
			if (fields.Count() >= 8)
				row.m_iDeathMs = fields[7].ToInt();

			for (int extra = 8; extra < fields.Count(); extra++)
			{
				row.m_aExtra.Insert(fields[extra]);
			}

			rows.Insert(row);
		}

		return rows;
	}

	// The standings as shown: one row per racer, or in team mode one per group (its furthest-ahead
	// member, since the standings are already sorted), with display names and the local row marked
	protected array<ref CRF_RallyHUDRow> BuildDisplayRows(array<ref CRF_RallyHUDRow> standings, int localPlayerId)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		int localGroupId = 0;
		if (m_bTeamMode && groupsManager)
		{
			SCR_AIGroup localGroup = groupsManager.GetPlayerGroup(localPlayerId);
			if (localGroup)
				localGroupId = localGroup.GetID();
		}

		array<ref CRF_RallyHUDRow> rows = {};
		array<int> seenGroupIds = {};
		foreach (CRF_RallyHUDRow row : standings)
		{
			bool asTeam = m_bTeamMode && row.m_iGroupId > 0;
			if (asTeam)
			{
				if (seenGroupIds.Contains(row.m_iGroupId))
					continue;

				seenGroupIds.Insert(row.m_iGroupId);
			}

			row.m_sName = "";
			if (asTeam && groupsManager)
			{
				SCR_AIGroup group = groupsManager.GetPlayerGroup(row.m_iPlayerId);
				if (group)
					row.m_sName = "Team " + group.GetCustomNameWithOriginal();
			}
			if (row.m_sName.IsEmpty())
				row.m_sName = GetGame().GetPlayerManager().GetPlayerName(row.m_iPlayerId);
			if (row.m_sName.Length() > 14)
				row.m_sName = row.m_sName.Substring(0, 14);

			row.m_bLocal = row.m_iPlayerId == localPlayerId || (asTeam && row.m_iGroupId == localGroupId);
			rows.Insert(row);
		}

		return rows;
	}

	// The running time shown for a row: the lap so far, frozen at death; -1 when there's none
	protected float GetRowTimeSecs(CRF_RallyHUDRow row, int serverNowMs)
	{
		if (row.m_bFinished || row.m_iLapStartMs <= 0)
			return -1;

		if (row.m_bDead)
		{
			if (row.m_iDeathMs > 0)
				return (row.m_iDeathMs - row.m_iLapStartMs) * 0.001;

			return -1;
		}

		if (serverNowMs < 0)
			return -1;

		return Math.Max(0, serverNowMs - row.m_iLapStartMs) * 0.001;
	}

	// What the local player should do next, for the HUD header
	protected string GetProgressText(CRF_RallyHUDRow own)
	{
		if (!own)
			return "RALLY";

		if (own.m_bFinished)
			return "FINISHED";

		if (own.m_bDead)
			return "OUT OF THE RACE";

		if (own.m_iLapStartMs == 0)
			return "HEAD TO THE START";

		int totalCPs = m_aCheckpointPositions.Count();
		int intermediates = Math.Max(0, totalCPs - 2);

		string next;
		if (own.m_iNextCP >= totalCPs - 1)
			next = "NEXT: FINISH";
		else if (own.m_iNextCP <= 0)
			next = "NEXT: START";
		else
			next = string.Format("NEXT: CP %1/%2", own.m_iNextCP, intermediates);

		if (m_iLapsToComplete > 1)
			return string.Format("LAP %1/%2  -  %3", Math.Min(own.m_iLaps + 1, m_iLapsToComplete), m_iLapsToComplete, next);

		return next;
	}

	protected void UpdateHUD()
	{
		IEntity localEnt = SCR_PlayerController.GetLocalControlledEntity();
		if (!localEnt)
		{
			DestroyHUD();
			return;
		}

		if (!m_wHUD)
		{
			COA_Gamemode crf = COA_Gamemode.GetInstance();
			if (!crf || crf.m_GamemodeState != COA_EGamemodeState.GAME)
				return;

			CreateHUD();
		}

		if (!m_wHUD)
			return;

		if (m_sReplicatedStandings.IsEmpty())
			return;

		// Lazily cache checkpoint positions on clients (server already has them from ServerInit)
		if (m_aCheckpointPositions.Count() == 0 && m_bRaceActive)
			CacheCheckpointPositions();

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int serverNowMs   = GetServerNowMs();

		array<ref CRF_RallyHUDRow> standings = ParseStandings();

		// The local player's own entry - their next checkpoint drives the arrow even in team mode
		CRF_RallyHUDRow own;
		foreach (CRF_RallyHUDRow standing : standings)
		{
			if (standing.m_iPlayerId == localPlayerId)
			{
				own = standing;
				break;
			}
		}

		array<ref CRF_RallyHUDRow> rows = BuildDisplayRows(standings, localPlayerId);
		int localIndex = -1;
		foreach (int rowIndex, CRF_RallyHUDRow row : rows)
		{
			if (row.m_bLocal)
			{
				localIndex = rowIndex;
				break;
			}
		}

		// Top rows, plus the local row underneath when it's further down than that (its position
		// number shows the gap)
		array<int> shownIndexes = {};
		for (int i = 0; i < rows.Count() && i < m_iMaxStandingsRows; i++)
		{
			shownIndexes.Insert(i);
		}
		if (localIndex >= m_iMaxStandingsRows)
			shownIndexes.Insert(localIndex);

		SetRowWidgetCount(shownIndexes.Count());
		foreach (int slot, int index : shownIndexes)
		{
			if (slot < m_aRowWidgets.Count())
				FillRowWidget(m_aRowWidgets[slot], rows[index], index + 1, serverNowMs);
		}

		// The panel ends under its last row, and the minimap sits under the panel
		float panelHeight = HUD_ROWS_TOP + m_aRowWidgets.Count() * HUD_ROW_HEIGHT + HUD_PANEL_PADDING;
		if (m_wPanel)
			FrameSlot.SetSize(m_wPanel, HUD_PANEL_WIDTH, panelHeight);

		if (m_wMinimapPanel)
			FrameSlot.SetPos(m_wMinimapPanel, -(HUD_PANEL_WIDTH + HUD_PANEL_RIGHT), HUD_PANEL_TOP + panelHeight + HUD_MINIMAP_GAP);

		if (m_wProgressFill)
			FrameSlot.SetSize(m_wProgressFill, (HUD_PANEL_WIDTH - 2 * HUD_PROGRESS_INSET) * GetRaceProgress(own), 3);

		if (m_wHeaderText)
			m_wHeaderText.SetText(GetProgressText(own));

		if (m_wPositionText)
		{
			if (localIndex >= 0)
				m_wPositionText.SetText(string.Format("P%1", localIndex + 1));
			else
				m_wPositionText.SetText("--");
		}

		if (m_wPositionTotalText)
		{
			if (localIndex >= 0)
				m_wPositionTotalText.SetText(string.Format("OF %1", rows.Count()));
			else
				m_wPositionTotalText.SetText("");
		}

		if (m_wLapTimeText)
		{
			CRF_RallyHUDRow localRow;
			if (localIndex >= 0)
				localRow = rows[localIndex];
			m_wLapTimeText.SetText(GetLapTimeText(localRow, serverNowMs));
		}

		if (m_wLapTimeLabel)
			m_wLapTimeLabel.SetText(GetLapTimeLabel());

		// CP arrow - towards the next checkpoint (the start line before the race), relative to the
		// player/vehicle heading. Hidden for spectators, the dead and finishers.
		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(localEnt.FindComponent(SCR_CharacterControllerComponent));
		bool isPlayerAlive = !COA_EntityHelper.IsSpectator(localEnt) && (!charCtrl || !charCtrl.IsDead());
		bool showArrow = isPlayerAlive && own && !own.m_bFinished && !own.m_bDead
			&& own.m_iNextCP >= 0 && own.m_iNextCP < m_aCheckpointPositions.Count();

		if (showArrow)
		{
			// Use vehicle position/heading when seated, character otherwise
			IEntity trackEnt = localEnt;
			if (localEnt.GetParent())
				trackEnt = localEnt.GetParent();

			vector playerPos = trackEnt.GetOrigin();
			vector cpPos     = m_aCheckpointPositions[own.m_iNextCP];
			vector delta     = cpPos - playerPos;

			// World bearing to CP: atan2(East, North) in Arma's XYZ coordinate space
			float cpBearing  = Math.Atan2(delta[0], delta[2]) * Math.RAD2DEG;
			float playerYaw  = trackEnt.GetAngles()[1];
			float relAngle   = cpBearing - playerYaw;

			if (m_wCPArrow)
			{
				m_wCPArrow.SetOpacity(1);
				m_wCPArrow.SetRotation(relAngle);
			}

			if (m_wCPDistanceText)
			{
				int distInt = vector.Distance(playerPos, cpPos);
				string distStr;
				if (distInt >= 1000)
					distStr = string.Format("%1.%2 km", distInt / 1000, (distInt % 1000) / 100);
				else
					distStr = string.Format("%1 m", distInt);
				m_wCPDistanceText.SetText(string.Format("%1  ·  %2", GetCheckpointLabel(own.m_iNextCP), distStr));
			}
		}
		else
		{
			if (m_wCPArrow)
				m_wCPArrow.SetOpacity(0);
			if (m_wCPDistanceText)
				m_wCPDistanceText.SetText("");
		}

		UpdateMinimap(localEnt, own, isPlayerAlive);
	}

	//------------------------------------------------------------------------------------------------
	//! The course minimap: traced the first time the checkpoints are known here, then the car moved
	//! along it and the legs coloured by the local player's progress
	protected void UpdateMinimap(notnull IEntity localEnt, CRF_RallyHUDRow own, bool isPlayerAlive)
	{
		if (!m_Minimap || m_aCheckpointPositions.Count() < 2)
			return;

		// Circuits also get the run from the finish back to the start line
		bool closedLoop = m_iLapsToComplete > 1;
		if (!m_Minimap.IsBuilt())
			m_Minimap.Build(m_aCheckpointPositions, closedLoop);

		// Leg i runs from checkpoint i to i + 1. Heading for the start after a lap is the run back.
		int currentLeg = -1;
		int nextCP = -1;
		if (own)
		{
			if (own.m_bFinished)
				currentLeg = m_aCheckpointPositions.Count();
			else if (own.m_iNextCP > 0)
				currentLeg = own.m_iNextCP - 1;
			else if (own.m_iLaps > 0 && closedLoop)
				currentLeg = m_aCheckpointPositions.Count() - 1;

			if (!own.m_bFinished && !own.m_bDead)
				nextCP = own.m_iNextCP;
		}

		// Vehicle position/heading when seated, character otherwise (as for the arrow)
		IEntity trackEnt = localEnt;
		if (localEnt.GetParent())
			trackEnt = localEnt.GetParent();

		bool showCar = isPlayerAlive && (!own || !own.m_bDead);
		m_Minimap.Update(showCar, trackEnt.GetOrigin(), trackEnt.GetAngles()[1], currentLeg, nextCP);
	}


	//===================================================================================
	// UTILITIES
	//===================================================================================

	// Formats seconds as  m:ss.mmm   (e.g. 1:23.456)
	static string FormatTime(float seconds)
	{
		if (seconds < 0)
			return "--:--.---";

		int totalMs  = seconds * 1000;
		int mins     = totalMs / 60000;
		int secs     = (totalMs % 60000) / 1000;
		int millis   = totalMs % 1000;

		return string.Format("%1:%2.%3", mins.ToString(1), secs.ToString(2), millis.ToString(3));
	}

	// Returns "1st", "2nd", "3rd", "4th" etc.
	static string PositionLabel(int pos)
	{
		switch (pos)
		{
			case 1:  return "1st";
			case 2:  return "2nd";
			case 3:  return "3rd";
			default: return string.Format("%1th", pos);
		}
		return string.Format("%1th", pos);
	}
}
