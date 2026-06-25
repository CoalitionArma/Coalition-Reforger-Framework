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

	//===================================================================================
	// SERVER-SIDE RUNTIME STATE
	//===================================================================================

	protected ref array<ref CRF_RallyPlayerEntry> m_aPlayerEntries = {};
	protected ref array<vector> m_aCheckpointPositions = {};
	protected bool m_bRaceInitialised = false;

	// Smoke entities spawned at each checkpoint (server-managed, replicated via world state)
	protected ref array<IEntity> m_aSmokeEntities = {};

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

	[Attribute("5", UIWidgets.EditBox, "Distance in metres to offset smoke markers sideways from the checkpoint centre, to keep them off the road.")]
	protected float m_fSmokeOffsetDistance;

	//===================================================================================
	// REPLICATED STATE
	//===================================================================================

	// Packed standings updated by the server each second:
	// "playerId;lapsCompleted;nextCP;lapStartMs;finished | ..."  (sorted 1st → last, "|" separator)
	[RplProp()]
	protected string m_sReplicatedStandings = "";

	[RplProp(onRplName: "OnRaceActiveChanged")]
	protected bool m_bRaceActive = false;

	// Guard to ensure results are only triggered once
	protected bool m_bResultsTriggered = false;

	//===================================================================================
	// CLIENT-SIDE HUD STATE
	//===================================================================================

	protected Widget m_wHUD = null;
	protected TextWidget m_wPositionText = null;
	protected TextWidget m_wLapTimeText = null;
	protected TextWidget m_wStandingsText = null;
	protected ImageWidget m_wCPArrow = null;
	protected TextWidget m_wCPDistanceText = null;

	//===================================================================================
	// STATIC ACCESSOR
	//===================================================================================

	protected static CRF_RallyGamemodeComponent m_sInstance;

	void CRF_RallyGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
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
			ClearCheckpointSmokes();
			ClearCheckpointMarkers();
		}
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

		// Pre-register currently slotted players
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int pid : playerIds)
			GetOrCreateEntry(pid);

		m_bRaceInitialised = true;
		m_bRaceActive      = true;
		Replication.BumpMe();

		// Spawn smoke at each checkpoint and re-spawn every 50s (smoke TTL ~60s)
		SpawnCheckpointSmokes();
		GetGame().GetCallqueue().CallLater(SpawnCheckpointSmokes, 50000, true);

		// Place persistent map markers for each checkpoint
		SpawnCheckpointMarkers();

		Print(string.Format("[CRF_Rally] Race initialised — %1 checkpoints, %2 lap(s), radius=%.0fm",
			m_aCheckpointPositions.Count(), m_iLapsToComplete, m_fCheckpointRadius), LogLevel.NORMAL);
	}

	protected void CacheCheckpointPositions()
	{
		m_aCheckpointPositions.Clear();

		// Start line
		IEntity startEnt = GetGame().GetWorld().FindEntityByName("rally_start");
		if (!startEnt)
		{
			Print("[CRF_Rally] ERROR: 'rally_start' entity not found in the world!", LogLevel.ERROR);
			return;
		}
		m_aCheckpointPositions.Insert(startEnt.GetOrigin());

		// Intermediate checkpoints — rally_cp1, rally_cp2, ... (stop at first gap)
		int cpIndex = 1;
		while (true)
		{
			IEntity cpEnt = GetGame().GetWorld().FindEntityByName(string.Format("rally_cp%1", cpIndex));
			if (!cpEnt)
				break;
			m_aCheckpointPositions.Insert(cpEnt.GetOrigin());
			cpIndex++;
		}

		// Finish line
		IEntity endEnt = GetGame().GetWorld().FindEntityByName("rally_end");
		if (!endEnt)
		{
			Print("[CRF_Rally] ERROR: 'rally_end' entity not found in the world!", LogLevel.ERROR);
			return;
		}
		m_aCheckpointPositions.Insert(endEnt.GetOrigin());
	}

	// Spawns (or re-spawns) colored smoke at every checkpoint position.
	// Called once at race start then every 50 s to replenish expiring smokes.
	protected void SpawnCheckpointSmokes()
	{
		// Delete any previous smoke entities from the prior cycle
		ClearCheckpointSmokes();

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

			// Compute a world-space side offset so the smoke sits beside the road.
			// Direction is derived from the course heading at this checkpoint:
			// use the vector toward the next checkpoint (or from the previous one at the finish).
			vector cpPos = m_aCheckpointPositions[i];
			vector heading;
			if (i < last)
				heading = (m_aCheckpointPositions[i + 1] - cpPos).Normalized();
			else
				heading = (cpPos - m_aCheckpointPositions[i - 1]).Normalized();

			// Right-hand perpendicular in the horizontal plane: (z, 0, -x)
			vector sideways = Vector(heading[2], 0, -heading[0]).Normalized();
			vector smokePos = cpPos + sideways * m_fSmokeOffsetDistance;

			EntitySpawnParams params = new EntitySpawnParams();
			Math3D.MatrixIdentity4(params.Transform);
			params.Transform[3] = smokePos;

			IEntity smoke = GetGame().SpawnEntityPrefab(Resource.Load(prefab), null, params);
			if (smoke)
				m_aSmokeEntities.Insert(smoke);
		}
	}

	protected void ClearCheckpointSmokes()
	{
		foreach (IEntity ent : m_aSmokeEntities)
		{
			if (ent)
				SCR_EntityHelper.DeleteEntityAndChildren(ent);
		}
		m_aSmokeEntities.Clear();
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

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (m_bRaceInitialised && Replication.IsServer())
			GetOrCreateEntry(playerId);
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
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int pid : playerIds)
		{
			CRF_RallyPlayerEntry entry = GetOrCreateEntry(pid);
			if (entry.m_bFinished)
				continue;

			IEntity playerEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (!playerEnt)
				continue;

			// Optional vehicle requirement: character must be seated in a vehicle
			if (m_bRequireVehicle && !playerEnt.GetParent())
				continue;

			CheckCheckpointCrossing(entry, playerEnt);
		}
	}

	protected void CheckCheckpointCrossing(notnull CRF_RallyPlayerEntry entry, notnull IEntity playerEnt)
	{
		int cpIndex = entry.m_iNextCheckpoint;
		if (cpIndex >= m_aCheckpointPositions.Count())
			return;

		// Track from the vehicle position when seated, otherwise from the character
		IEntity trackEnt = playerEnt;
		if (playerEnt.GetParent())
			trackEnt = playerEnt.GetParent();

		float dist = vector.Distance(trackEnt.GetOrigin(), m_aCheckpointPositions[cpIndex]);
		if (dist > m_fCheckpointRadius)
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

				string finisherName = GetGame().GetPlayerManager().GetPlayerName(pid);
				Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 finished %2 in %3!", finisherName, PositionLabel(finishPos), FormatTime(lapTimeSecs)));

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

			// cpIndex 1 is the first real CP after start; adjust the label for display
			string cpLabel = string.Format("Checkpoint %1 / %2", cpIndex, totalCPs - 2);
			string cpPlayerName = GetGame().GetPlayerManager().GetPlayerName(pid);
			Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 passed %2.", cpPlayerName, cpLabel));
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
		}

		if (packed == m_sReplicatedStandings)
			return;

		m_sReplicatedStandings = packed;
		Replication.BumpMe();
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
			if (!ent)
				continue; // no controlled entity = disconnected or already dead

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
						displayName = "Team " + grp.GetCustomName();
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
						displayName = "Team " + grp.GetCustomName();
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

		m_wHUD = GetGame().GetWorkspace().CreateWidgets("{4CBBF00D5B6E1A24}UI/layouts/HUD/Rally/CRF_RallyHUD.layout");
		if (!m_wHUD)
		{
			Print("[CRF_Rally] WARNING: Could not load CRF_RallyHUD.layout — HUD will not be shown.", LogLevel.WARNING);
			return;
		}

		m_wPositionText  = TextWidget.Cast(m_wHUD.FindAnyWidget("PositionText"));
		m_wLapTimeText   = TextWidget.Cast(m_wHUD.FindAnyWidget("LapTimeText"));
		m_wStandingsText = TextWidget.Cast(m_wHUD.FindAnyWidget("StandingsText"));
		m_wCPArrow       = ImageWidget.Cast(m_wHUD.FindAnyWidget("CPArrowWidget"));
		m_wCPDistanceText = TextWidget.Cast(m_wHUD.FindAnyWidget("CPDistanceText"));
	}

	protected void DestroyHUD()
	{
		if (m_wHUD)
		{
			m_wHUD.RemoveFromHierarchy();
			m_wHUD = null;
			m_wPositionText   = null;
			m_wLapTimeText    = null;
			m_wStandingsText  = null;
			m_wCPArrow        = null;
			m_wCPDistanceText = null;
		}
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
			CRF_Gamemode crf = CRF_Gamemode.GetInstance();
			if (!crf || crf.m_GamemodeState != CRF_EGamemodeState.GAME)
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
		int worldTimeMs   = GetGame().GetWorld().GetWorldTime();

		array<string> entries = {};
		m_sReplicatedStandings.Split("|", entries, true);

		string standingsLines = "";
		int    localPosition  = -1;
		int    localLapStart  = 0;
		bool   localFinished  = false;
		bool   localDead      = false;
		int    localDeathTime = 0;
		int    localNextCP    = -1;

		// Pre-scan to find this client's own next checkpoint (used for the CP arrow)
		for (int k = 0; k < entries.Count(); k++)
		{
			array<string> pf = {};
			entries[k].Split(";", pf, true);
			if (pf.Count() >= 3 && pf[0].ToInt() == localPlayerId)
			{
				localNextCP = pf[2].ToInt();
				break;
			}
		}

		if (m_bTeamMode)
		{
			SCR_GroupsManagerComponent gMgr = SCR_GroupsManagerComponent.GetInstance();

			// Look up local player's group once
			int localGroupId = 0;
			if (gMgr)
			{
				SCR_AIGroup localGrp = gMgr.GetPlayerGroup(localPlayerId);
				if (localGrp)
					localGroupId = localGrp.GetID();
			}

			// Pre-count unique teams/solos for the overflow indicator
			ref array<int> countSeen = {};
			int totalTeams = 0;
			for (int i = 0; i < entries.Count(); i++)
			{
				array<string> cf = {};
				entries[i].Split(";", cf, true);
				if (cf.Count() < 5)
					continue;
				int gId = 0;
				if (cf.Count() >= 6)
					gId = cf[5].ToInt();
				if (gId > 0 && countSeen.Contains(gId))
					continue;
				if (gId > 0)
					countSeen.Insert(gId);
				totalTeams++;
			}

			ref array<int> seenGroupIds = {};
			int teamRank = 0;

			for (int i = 0; i < entries.Count() && teamRank < m_iMaxStandingsRows; i++)
			{
				array<string> fields = {};
				entries[i].Split(";", fields, true);
				if (fields.Count() < 5)
					continue;

				int  pid       = fields[0].ToInt();
				int  laps      = fields[1].ToInt();
				int  lapStart  = fields[3].ToInt();
				bool finished  = fields[4].ToInt() == 1;
				int  groupId   = 0;
				if (fields.Count() >= 6)
					groupId = fields[5].ToInt();
				bool dead      = fields.Count() >= 7 && fields[6].ToInt() == 1;
				int  deathTime = 0;
				if (fields.Count() >= 8)
					deathTime = fields[7].ToInt();

				if (groupId > 0 && seenGroupIds.Contains(groupId))
					continue;
				if (groupId > 0)
					seenGroupIds.Insert(groupId);

				teamRank++;

				// Get display name: "Team <groupname>" for groups, player name for solos
				string displayName = "";
				if (groupId > 0 && gMgr)
				{
					SCR_AIGroup grp = gMgr.GetPlayerGroup(pid);
					if (grp)
						displayName = "Team " + grp.GetCustomName();
				}
				if (displayName.IsEmpty())
					displayName = GetGame().GetPlayerManager().GetPlayerName(pid);
				if (displayName.Length() > 14)
					displayName = displayName.Substring(0, 14);

				bool isLocalTeam = (pid == localPlayerId) || (groupId > 0 && groupId == localGroupId);
				if (isLocalTeam)
				{
					localPosition = teamRank;
					localLapStart = lapStart;
					localFinished = finished;
					localDead     = dead;
					localDeathTime = deathTime;
				}

				string lapTag;
				if (finished)
					lapTag = "FIN";
				else if (dead)
					lapTag = "KIA";
				else if (lapStart == 0)
					lapTag = "WAIT";
				else
					lapTag = string.Format("L%1", laps + 1);

				string timeStr;
				if (finished)
					timeStr = "---";
				else if (dead && deathTime > 0 && lapStart > 0)
					timeStr = FormatTime((deathTime - lapStart) * 0.001);
				else if (lapStart == 0)
					timeStr = "--:--.---";
				else
					timeStr = FormatTime((worldTimeMs - lapStart) * 0.001);

				string marker = " ";
				if (isLocalTeam)
					marker = ">";

				string line = string.Format("%1P%2 %3  %4  %5", marker, teamRank, displayName, lapTag, timeStr);
				if (!standingsLines.IsEmpty())
					standingsLines += "\n";
				standingsLines += line;
			}

			if (totalTeams > m_iMaxStandingsRows)
				standingsLines += string.Format("\n  ... +%1 more", totalTeams - m_iMaxStandingsRows);
		}
		else
		{
			int rowsToShow = Math.Min(entries.Count(), m_iMaxStandingsRows);

			for (int i = 0; i < rowsToShow; i++)
			{
				array<string> fields = {};
				entries[i].Split(";", fields, true);
				if (fields.Count() < 5)
					continue;

				int  pid       = fields[0].ToInt();
				int  laps      = fields[1].ToInt();
				int  lapStart  = fields[3].ToInt();
				bool finished  = fields[4].ToInt() == 1;
				bool dead      = fields.Count() >= 7 && fields[6].ToInt() == 1;
				int  deathTime = 0;
				if (fields.Count() >= 8)
					deathTime = fields[7].ToInt();

				string playerName = GetGame().GetPlayerManager().GetPlayerName(pid);
				if (playerName.Length() > 14)
					playerName = playerName.Substring(0, 14);

				string lapTag;
				if (finished)
					lapTag = "FIN";
				else if (dead)
					lapTag = "KIA";
				else if (lapStart == 0)
					lapTag = "WAIT";
				else
					lapTag = string.Format("L%1", laps + 1);

				string timeStr;
				if (finished)
					timeStr = "---";
				else if (dead && deathTime > 0 && lapStart > 0)
					timeStr = FormatTime((deathTime - lapStart) * 0.001);
				else if (lapStart == 0)
					timeStr = "--:--.---";
				else
					timeStr = FormatTime((worldTimeMs - lapStart) * 0.001);

				string marker = " ";
				if (pid == localPlayerId)
					marker = ">";

				string line = string.Format("%1P%2 %3  %4  %5", marker, i + 1, playerName, lapTag, timeStr);
				if (!standingsLines.IsEmpty())
					standingsLines += "\n";
				standingsLines += line;

				if (pid == localPlayerId)
				{
					localPosition  = i + 1;
					localLapStart  = lapStart;
					localFinished  = finished;
					localDead      = dead;
					localDeathTime = deathTime;
				}
			}

			if (entries.Count() > rowsToShow)
				standingsLines += string.Format("\n  ... +%1 more", entries.Count() - rowsToShow);
		}

		if (m_wStandingsText)
			m_wStandingsText.SetText(standingsLines);

		if (m_wPositionText)
		{
			if (localPosition > 0)
				m_wPositionText.SetText(string.Format("P%1", localPosition));
			else
				m_wPositionText.SetText("--");
		}

		if (m_wLapTimeText)
		{
			if (localFinished)
				m_wLapTimeText.SetText("DONE");
			else if (localDead && localDeathTime > 0 && localLapStart > 0)
				m_wLapTimeText.SetText(FormatTime((localDeathTime - localLapStart) * 0.001));
			else if (localLapStart > 0)
				m_wLapTimeText.SetText(FormatTime((worldTimeMs - localLapStart) * 0.001));
			else
				m_wLapTimeText.SetText("--:--.---");
		}

		// CP arrow — rotate toward the next checkpoint relative to the player/vehicle heading
		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(localEnt.FindComponent(SCR_CharacterControllerComponent));
		bool isPlayerAlive = !charCtrl || !charCtrl.IsDead();
		bool showArrow = isPlayerAlive && !localFinished && localLapStart > 0
			&& localNextCP >= 0 && localNextCP < m_aCheckpointPositions.Count();

		if (showArrow)
		{
			// Use vehicle position/heading when seated, character otherwise
			IEntity trackEnt = localEnt;
			if (localEnt.GetParent())
				trackEnt = localEnt.GetParent();

			vector playerPos = trackEnt.GetOrigin();
			vector cpPos     = m_aCheckpointPositions[localNextCP];
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
					distStr = string.Format("%1.%2km", distInt / 1000, (distInt % 1000) / 100);
				else
					distStr = string.Format("%1m", distInt);
				m_wCPDistanceText.SetText(distStr);
			}
		}
		else
		{
			if (m_wCPArrow)
				m_wCPArrow.SetOpacity(0);
			if (m_wCPDistanceText)
				m_wCPDistanceText.SetText("");
		}
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
