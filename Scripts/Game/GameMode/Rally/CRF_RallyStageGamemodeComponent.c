//------------------------------------------------------------------------------------
// CRF_RallyStageGamemodeComponent: rally stage with staggered starts, like a real rally
//
// Instead of everyone racing off the line together, crews are given a start order and leave one at a
// time, m_fStartInterval seconds apart. Each crew's stage clock starts at its own start time and stops
// at the finish; the fastest stage time wins. It is the CRF_RallyGamemodeComponent course (same
// rally_start / rally_cpN / rally_end entities, smoke, map markers, HUD and results screen) with
// different timing:
//
// - The start order is drawn when the round goes live (random by default). Players in the same
//   vehicle - or the same group, in team mode - are one crew and share a start time. Players who
//   turn up later are added at the back.
// - Crews get a countdown (30, 10, 5..1, GO) and the HUD shows their start time.
// - The clock starts at the start time whether or not the crew has crossed the start line, as in a
//   real rally. Crossing it early is a jump start: the clock starts there instead, plus a penalty.
// - A car waiting inside the start zone is held until its start time: handbrake on, gearbox in neutral,
//   re-applied ten times a second so the driver can't release it (EnforceStartHold). At GO it is
//   released into first gear. The jump start penalty stays as the backstop.
// - Split times are taken at every checkpoint and compared with the fastest split so far, so the
//   standings show who is up on the leader before anyone has finished.
// - Point to point only: the lap count is forced to 1.
//
// Setup: put this component on the gamemode entity instead of CRF_RallyGamemodeComponent (not both)
// and lay out the course as for that component.
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Per-player stage timing - server side only
class CRF_RallyStageEntry
{
	int m_iSlot = -1;              // start order (0 = first away), -1 = not drawn yet
	int m_iClockStartMs;           // server world time the stage clock started (the start time, or a jump start)
	int m_iPenaltyMs;
	bool m_bJumpStart;
	int m_iLastAnnouncedSecs = -1; // last countdown second announced to this player
	int m_iLastSplitIndex = -1;    // checkpoint of the latest split
	int m_iLastSplitDeltaMs;       // that split against the fastest at that checkpoint (<= 0 = fastest)
	int m_iFinishTimeMs = -1;      // stage time including penalties
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Rally stage with staggered starts: crews leave one at a time and race the clock. Uses the same course entities as CRF_RallyGamemodeComponent.")]
class CRF_RallyStageGamemodeComponentClass : CRF_RallyGamemodeComponentClass {}


class CRF_RallyStageGamemodeComponent : CRF_RallyGamemodeComponent
{
	[Attribute("20", UIWidgets.EditBox, desc: "Seconds from the round going live to the first crew's start", params: "0 600 1", category: "Rally - Stage")]
	protected float m_fFirstStartDelay;

	[Attribute("30", UIWidgets.EditBox, desc: "Seconds between one crew's start and the next", params: "5 600 1", category: "Rally - Stage")]
	protected float m_fStartInterval;

	[Attribute("10", UIWidgets.EditBox, desc: "Seconds added to a crew's stage time for crossing the start line before their start time", params: "0 600 1", category: "Rally - Stage")]
	protected float m_fJumpStartPenalty;

	[Attribute("true", UIWidgets.CheckBox, desc: "Draw the start order at random. Off: in the order players joined.", category: "Rally - Stage")]
	protected bool m_bRandomStartOrder;

	// Server state
	protected ref map<int, ref CRF_RallyStageEntry> m_mStage = new map<int, ref CRF_RallyStageEntry>();
	protected ref array<int> m_aBestSplitsMs = {};   // fastest split per checkpoint index, -1 = none yet
	protected bool m_bStartOrderDrawn;
	protected int m_iNextSlot;
	protected int m_iLastSlotStartMs;

	//===================================================================================
	// SERVER
	//===================================================================================

	override protected void ServerInit()
	{
		// Point to point: one run from start to finish
		m_iLapsToComplete = 1;
		super.ServerInit();

		m_aBestSplitsMs.Clear();
		for (int i = 0; i < m_aCheckpointPositions.Count(); i++)
		{
			m_aBestSplitsMs.Insert(-1);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_RallyStageEntry GetStage(int playerId)
	{
		CRF_RallyStageEntry stage = m_mStage.Get(playerId);
		if (!stage)
		{
			stage = new CRF_RallyStageEntry();
			m_mStage.Set(playerId, stage);
		}

		return stage;
	}

	//------------------------------------------------------------------------------------------------
	override protected void PollAllPlayerCheckpoints()
	{
		super.PollAllPlayerCheckpoints();

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		if (!m_bStartOrderDrawn)
			DrawStartOrder(now);
		else
			AddLateCrews(now);

		AnnounceCountdowns(now);
	}

	//------------------------------------------------------------------------------------------------
	//! Who a player races with: their group in team mode, else the vehicle they're in, else nobody
	protected string GetCrewKey(int playerId)
	{
		if (m_bTeamMode)
		{
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (groupsManager)
			{
				SCR_AIGroup group = groupsManager.GetPlayerGroup(playerId);
				if (group)
					return "group" + group.GetID();
			}
		}

		IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (controlled && controlled.GetParent())
			return "vehicle" + controlled.GetParent().GetID();

		return "player" + playerId;
	}

	//------------------------------------------------------------------------------------------------
	//! When the round goes live: every registered racer gets a crew start time
	protected void DrawStartOrder(int now)
	{
		m_bStartOrderDrawn = true;

		// Crews, in join order
		array<string> crewKeys = {};
		map<string, ref array<int>> crews = new map<string, ref array<int>>();
		foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
		{
			string key = GetCrewKey(entry.m_iPlayerId);
			array<int> members = crews.Get(key);
			if (!members)
			{
				members = {};
				crews.Set(key, members);
				crewKeys.Insert(key);
			}
			members.Insert(entry.m_iPlayerId);
		}

		if (m_bRandomStartOrder)
		{
			// Fisher-Yates
			for (int i = crewKeys.Count() - 1; i > 0; i--)
			{
				int j = Math.RandomInt(0, i + 1);
				string swap = crewKeys[i];
				crewKeys[i] = crewKeys[j];
				crewKeys[j] = swap;
			}
		}

		int intervalMs = m_fStartInterval * 1000;
		int firstStartMs = now + m_fFirstStartDelay * 1000;

		m_iNextSlot = 0;
		foreach (int slot, string crewKey : crewKeys)
		{
			AssignSlot(crews.Get(crewKey), firstStartMs + slot * intervalMs);
		}

		BuildAndReplicateStandings();

		int intervalSecs = m_fStartInterval;
		int firstSecs = m_fFirstStartDelay;
		Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] Start order drawn - %1 crew(s), %2 s apart. First away in %3 s.",
			crewKeys.Count(), intervalSecs, firstSecs));
	}

	//------------------------------------------------------------------------------------------------
	//! Racers registered after the draw join a crew-mate's slot, or go to the back
	protected void AddLateCrews(int now)
	{
		map<string, ref array<int>> newCrews = new map<string, ref array<int>>();
		array<string> newKeys = {};
		foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
		{
			if (GetStage(entry.m_iPlayerId).m_iSlot >= 0)
				continue;

			// Riding with a crew that hasn't left yet: same start
			string key = GetCrewKey(entry.m_iPlayerId);
			CRF_RallyStageEntry crewMate = FindWaitingCrewMate(key, entry.m_iPlayerId, now);
			if (crewMate)
			{
				CRF_RallyStageEntry stage = GetStage(entry.m_iPlayerId);
				stage.m_iSlot = crewMate.m_iSlot;
				stage.m_iClockStartMs = crewMate.m_iClockStartMs;
				entry.m_iLapStartTimeMs = stage.m_iClockStartMs;
				entry.m_iRaceStartTimeMs = stage.m_iClockStartMs;
				continue;
			}

			array<int> members = newCrews.Get(key);
			if (!members)
			{
				members = {};
				newCrews.Set(key, members);
				newKeys.Insert(key);
			}
			members.Insert(entry.m_iPlayerId);
		}

		int intervalMs = m_fStartInterval * 1000;
		foreach (string newKey : newKeys)
		{
			// At the back of the queue, and never sooner than one interval from now
			int startMs = m_iLastSlotStartMs + intervalMs;
			if (startMs < now + intervalMs)
				startMs = now + intervalMs;

			AssignSlot(newCrews.Get(newKey), startMs);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_RallyStageEntry FindWaitingCrewMate(string crewKey, int exceptPlayerId, int now)
	{
		foreach (int playerId, CRF_RallyStageEntry stage : m_mStage)
		{
			if (playerId == exceptPlayerId || stage.m_iSlot < 0 || stage.m_iClockStartMs <= now)
				continue;

			if (GetCrewKey(playerId) == crewKey)
				return stage;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void AssignSlot(array<int> members, int startMs)
	{
		if (!members)
			return;

		int slot = m_iNextSlot;
		m_iNextSlot++;
		m_iLastSlotStartMs = startMs;

		foreach (int playerId : members)
		{
			CRF_RallyStageEntry stage = GetStage(playerId);
			stage.m_iSlot = slot;
			stage.m_iClockStartMs = startMs;

			// The base standings time racers from m_iLapStartTimeMs, so it carries the stage clock
			CRF_RallyPlayerEntry entry = GetEntry(playerId);
			if (entry)
			{
				entry.m_iLapStartTimeMs = startMs;
				entry.m_iRaceStartTimeMs = startMs;
			}

			Rpc(RpcDo_CheckpointNotify, playerId, string.Format("You are car %1 - you start in %2",
				slot + 1, FormatCountdown(startMs - GetGame().GetWorld().GetWorldTime())));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! "Start in 30", 10, 5..1, then "GO!" to each crew as its start approaches. Polled at 4 Hz, so
	//! every whole second is seen; each is announced once.
	protected void AnnounceCountdowns(int now)
	{
		array<int> countdownSeconds = {30, 10, 5, 4, 3, 2, 1, 0};
		foreach (int playerId, CRF_RallyStageEntry stage : m_mStage)
		{
			// Not drawn yet, or jumped the start (no countdown after leaving)
			if (stage.m_iSlot < 0 || stage.m_bJumpStart)
				continue;

			int msLeft = stage.m_iClockStartMs - now;
			if (msLeft <= -1000)
				continue; // long gone

			int secsLeft = Math.Ceil(msLeft / 1000.0);
			if (secsLeft < 0)
				secsLeft = 0;

			if (!countdownSeconds.Contains(secsLeft) || stage.m_iLastAnnouncedSecs == secsLeft)
				continue;

			stage.m_iLastAnnouncedSecs = secsLeft;
			if (secsLeft > 0)
			{
				Rpc(RpcDo_CheckpointNotify, playerId, string.Format("Start in %1", secsLeft));
				continue;
			}

			Rpc(RpcDo_CheckpointNotify, playerId, "GO! GO! GO!");
			if (IsCrewLeader(playerId, stage.m_iSlot))
				Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] Car %1 (%2) is away.", stage.m_iSlot + 1, GetGame().GetPlayerManager().GetPlayerName(playerId)));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The first-registered member of a slot, so a crew is announced once
	protected bool IsCrewLeader(int playerId, int slot)
	{
		foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
		{
			CRF_RallyStageEntry stage = m_mStage.Get(entry.m_iPlayerId);
			if (stage && stage.m_iSlot == slot)
				return entry.m_iPlayerId == playerId;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Crews wait at the start line, inside its detection radius. Before their start time only leaving
	// that radius counts (a jump start); from their start time on, being in it counts as starting.
	override protected void CheckCheckpointCrossing(notnull CRF_RallyPlayerEntry entry, vector lastPos, vector pos)
	{
		CRF_RallyStageEntry stage = m_mStage.Get(entry.m_iPlayerId);
		if (entry.m_iNextCheckpoint != 0 || !stage || stage.m_iSlot < 0 || m_aCheckpointPositions.IsEmpty())
		{
			super.CheckCheckpointCrossing(entry, lastPos, pos);
			return;
		}

		if (GetGame().GetWorld().GetWorldTime() >= stage.m_iClockStartMs)
		{
			super.CheckCheckpointCrossing(entry, lastPos, pos);
			return;
		}

		vector startPos = m_aCheckpointPositions[0];
		bool wasInside = vector.Distance(lastPos, startPos) <= m_fCheckpointRadius;
		bool isInside = vector.Distance(pos, startPos) <= m_fCheckpointRadius;
		if (wasInside && !isInside)
			OnCheckpointCrossed(entry, 0);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnCheckpointCrossed(notnull CRF_RallyPlayerEntry entry, int cpIndex)
	{
		int now = GetGame().GetWorld().GetWorldTime();
		int pid = entry.m_iPlayerId;
		int totalCPs = m_aCheckpointPositions.Count();
		CRF_RallyStageEntry stage = GetStage(pid);

		// No start time yet (the draw happens when the round goes live): not racing
		if (stage.m_iSlot < 0)
			return;

		entry.m_iLastCPTimeMs = now;

		if (cpIndex == 0)
		{
			// ── START LINE ──────────────────────────────────────────────────────────
			entry.m_iNextCheckpoint = 1;
			if (now < stage.m_iClockStartMs)
			{
				// Jump start: the clock starts now, not at the start time, plus a penalty
				int penaltyMs = m_fJumpStartPenalty * 1000;
				int penaltySecs = m_fJumpStartPenalty;
				stage.m_bJumpStart = true;
				stage.m_iPenaltyMs = stage.m_iPenaltyMs + penaltyMs;
				stage.m_iClockStartMs = now;
				entry.m_iLapStartTimeMs = now;
				entry.m_iRaceStartTimeMs = now;

				Rpc(RpcDo_CheckpointNotify, pid, string.Format("JUMP START  —  +%1 s penalty", penaltySecs));
				Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] Jump start: %1 (+%2 s).", GetGame().GetPlayerManager().GetPlayerName(pid), penaltySecs));
			}
			return;
		}

		int stageMs = now - stage.m_iClockStartMs + stage.m_iPenaltyMs;

		if (cpIndex < totalCPs - 1)
		{
			// ── SPLIT ───────────────────────────────────────────────────────────────
			entry.m_iNextCheckpoint = cpIndex + 1;

			int best = -1;
			if (m_aBestSplitsMs.IsIndexValid(cpIndex))
				best = m_aBestSplitsMs[cpIndex];

			int delta = 0;
			if (best >= 0)
				delta = stageMs - best;

			if (m_aBestSplitsMs.IsIndexValid(cpIndex) && (best < 0 || stageMs < best))
				m_aBestSplitsMs[cpIndex] = stageMs;

			stage.m_iLastSplitIndex = cpIndex;
			stage.m_iLastSplitDeltaMs = delta;

			string split = string.Format("CP %1/%2  —  %3", cpIndex, totalCPs - 2, FormatTime(stageMs * 0.001));
			if (best < 0 || delta < 0)
				split += "  FASTEST";
			else
				split += "  " + FormatDelta(delta);

			Rpc(RpcDo_CheckpointNotify, pid, split);
			return;
		}

		// ── FINISH ──────────────────────────────────────────────────────────────────
		entry.m_iLapsCompleted = 1;
		entry.m_bFinished = true;
		entry.m_iNextCheckpoint = totalCPs;
		stage.m_iFinishTimeMs = stageMs;
		entry.m_fFinishTimeSecs = stageMs * 0.001;
		entry.m_fBestLapTime = entry.m_fFinishTimeSecs;

		BuildAndReplicateStandings();
		int position = GetFinishPosition(stageMs);
		string name = GetGame().GetPlayerManager().GetPlayerName(pid);

		string penaltyNote = "";
		if (stage.m_iPenaltyMs > 0)
			penaltyNote = string.Format(" (incl. %1 s penalty)", stage.m_iPenaltyMs / 1000);

		Rpc(RpcDo_ChatBroadcast, string.Format("[Rally] %1 finished the stage in %2%3 - %4 so far.", name, FormatTime(entry.m_fFinishTimeSecs), penaltyNote, PositionLabel(position)));
		Rpc(RpcDo_FinishNotify, pid, entry.m_fFinishTimeSecs, 1, position);
		if (position == 1)
			Rpc(RpcDo_PodiumNotify, 1, name);

		KillPlayerAndVehicle(pid);
		GetGame().GetCallqueue().CallLater(CheckAllRacersFinishedOrDead, 5000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! 1-based place among the finishers so far for this stage time
	protected int GetFinishPosition(int stageMs)
	{
		int position = 1;
		foreach (int playerId, CRF_RallyStageEntry stage : m_mStage)
		{
			if (stage.m_iFinishTimeMs >= 0 && stage.m_iFinishTimeMs < stageMs)
				position++;
		}

		return position;
	}

	//------------------------------------------------------------------------------------------------
	//! 0 finished, 1 on stage, 2 waiting to start, 3 no start time yet, 4 out
	protected int GetRankClass(CRF_RallyPlayerEntry e, int now)
	{
		CRF_RallyStageEntry stage = m_mStage.Get(e.m_iPlayerId);
		if (e.m_bFinished)
			return 0;
		if (e.m_bDead)
			return 4;
		if (!stage || stage.m_iSlot < 0)
			return 3;
		if (now < stage.m_iClockStartMs)
			return 2;
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	// Finishers by stage time; crews on the stage by how they stand against the fastest split at their
	// latest checkpoint; then crews still to start, in start order; then those out of the stage
	override protected bool IsAhead(CRF_RallyPlayerEntry a, CRF_RallyPlayerEntry b, int totalCPs)
	{
		int now = GetGame().GetWorld().GetWorldTime();
		int classA = GetRankClass(a, now);
		int classB = GetRankClass(b, now);
		if (classA != classB)
			return classA < classB;

		CRF_RallyStageEntry stageA = m_mStage.Get(a.m_iPlayerId);
		CRF_RallyStageEntry stageB = m_mStage.Get(b.m_iPlayerId);

		switch (classA)
		{
			case 0:
				return stageA.m_iFinishTimeMs < stageB.m_iFinishTimeMs;

			case 1:
			{
				bool splitA = stageA.m_iLastSplitIndex >= 0;
				bool splitB = stageB.m_iLastSplitIndex >= 0;
				if (splitA != splitB)
					return splitA;
				if (splitA && stageA.m_iLastSplitDeltaMs != stageB.m_iLastSplitDeltaMs)
					return stageA.m_iLastSplitDeltaMs < stageB.m_iLastSplitDeltaMs;
				return stageA.m_iClockStartMs < stageB.m_iClockStartMs;
			}

			case 2:
				return stageA.m_iClockStartMs < stageB.m_iClockStartMs;

			case 4:
				return a.GetProgress(totalCPs) > b.GetProgress(totalCPs);
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Extra standings fields: ;car number;penalty ms;finish ms;latest split checkpoint;split delta ms
	override protected string GetExtraStandingFields(CRF_RallyPlayerEntry e)
	{
		CRF_RallyStageEntry stage = m_mStage.Get(e.m_iPlayerId);
		if (!stage)
			return ";-1;0;-1;-1;0";

		return string.Format(";%1;%2;%3;%4;%5", stage.m_iSlot, stage.m_iPenaltyMs, stage.m_iFinishTimeMs, stage.m_iLastSplitIndex, stage.m_iLastSplitDeltaMs);
	}

	//===================================================================================
	// CLIENT HUD
	//===================================================================================

	// Fields of CRF_RallyHUDRow.m_aExtra, as packed by GetExtraStandingFields
	protected static const int EXTRA_SLOT = 0;
	protected static const int EXTRA_PENALTY = 1;
	protected static const int EXTRA_FINISH = 2;
	protected static const int EXTRA_SPLIT_INDEX = 3;
	protected static const int EXTRA_SPLIT_DELTA = 4;

	// Client: the start hold (EnforceStartHold)
	protected bool m_bHoldingStart;
	protected CarControllerComponent m_HeldController;

	//------------------------------------------------------------------------------------------------
	override protected void UpdateHUD()
	{
		super.UpdateHUD();
		EnforceStartHold();
	}

	//------------------------------------------------------------------------------------------------
	//! Client, for the driver: until the crew's start time, a car inside the start zone is held with the
	//! handbrake on and the gearbox in neutral. Re-applied every HUD tick (10 Hz), so letting it off does
	//! nothing. It runs here and not on the server because a driven car is simulated on its driver's
	//! machine - a handbrake set on the server wouldn't reliably hold it.
	protected void EnforceStartHold()
	{
		CarControllerComponent controller;
		if (ShouldHoldAtStart(controller))
		{
			ApplyStartHold(controller);
			return;
		}

		if (m_bHoldingStart)
			ReleaseStartHold();
	}

	//------------------------------------------------------------------------------------------------
	//! True while the local player drives a car inside the start zone before their crew's start time
	protected bool ShouldHoldAtStart(out CarControllerComponent controller)
	{
		if (!m_bRaceActive || m_aCheckpointPositions.IsEmpty())
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return false;

		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access)
			return false;

		BaseCompartmentSlot seat = access.GetCompartment();
		if (!PilotCompartmentSlot.Cast(seat))
			return false;

		Vehicle vehicle = Vehicle.Cast(seat.GetOwner().GetRootParent());
		if (!vehicle)
			return false;

		controller = CarControllerComponent.Cast(vehicle.GetVehicleController());
		if (!controller)
			return false;

		// The local player's own standings: has a start time, hasn't left yet, and it's not time
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		array<ref CRF_RallyHUDRow> standings = ParseStandings();
		CRF_RallyHUDRow own;
		foreach (CRF_RallyHUDRow row : standings)
		{
			if (row.m_iPlayerId == localPlayerId)
			{
				own = row;
				break;
			}
		}

		if (!own || own.m_bFinished || own.m_bDead || own.m_iNextCP != 0 || own.GetExtraInt(EXTRA_SLOT, -1) < 0)
			return false;

		int serverNowMs = GetServerNowMs();
		if (serverNowMs < 0 || serverNowMs >= own.m_iLapStartMs)
			return false;

		return vector.Distance(vehicle.GetOrigin(), m_aCheckpointPositions[0]) <= m_fCheckpointRadius;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyStartHold(notnull CarControllerComponent controller)
	{
		if (!controller.GetPersistentHandBrake())
			controller.SetPersistentHandBrake(true);

		VehicleWheeledSimulation simulation = controller.GetSimulation();
		if (simulation)
		{
			simulation.SetBreak(1, true);

			// Gears run reverse(s), neutral, forward gears (as in SCR_ToggleHandbrakeAction)
			int neutral = simulation.GearboxGearsCount() - simulation.GearboxForwardGearsCount() - 1;
			if (simulation.GetGear() != neutral)
				simulation.SetGear(neutral);
		}

		if (m_HeldController != controller && m_HeldController)
			ReleaseStartHold();

		m_HeldController = controller;
		if (!m_bHoldingStart)
		{
			m_bHoldingStart = true;
			SCR_PopUpNotification.GetInstance().PopupMsg("Held at the start line until your start time", 4);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Lets the car go: handbrake off, first gear, physics woken
	protected void ReleaseStartHold()
	{
		m_bHoldingStart = false;
		CarControllerComponent controller = m_HeldController;
		m_HeldController = null;
		if (!controller)
			return;

		controller.SetPersistentHandBrake(false);

		VehicleWheeledSimulation simulation = controller.GetSimulation();
		if (simulation)
		{
			simulation.SetBreak(0, false);
			simulation.SetGear(simulation.GearboxGearsCount() - simulation.GearboxForwardGearsCount()); // first
		}

		IEntity vehicle = controller.GetOwner();
		if (vehicle && vehicle.GetPhysics())
			vehicle.GetPhysics().SetActive(ActiveState.ACTIVE);
	}

	//------------------------------------------------------------------------------------------------
	override protected void DescribeRow(CRF_RallyHUDRow row, int serverNowMs, out string status, out Color statusColor, out string time)
	{
		statusColor = Color.FromSRGBA(168, 178, 204, 255);
		status = "";
		time = "";

		int slot = row.GetExtraInt(EXTRA_SLOT, -1);
		int penaltyMs = row.GetExtraInt(EXTRA_PENALTY);

		if (row.m_bFinished)
		{
			status = "FIN";
			statusColor = Color.FromSRGBA(92, 196, 128, 255);
			time = FormatTime(row.GetExtraInt(EXTRA_FINISH, -1) * 0.001);
			return;
		}

		if (row.m_bDead)
		{
			status = "DNF";
			statusColor = Color.FromSRGBA(232, 96, 96, 255);
			return;
		}

		if (slot < 0)
		{
			status = "WAIT";
			return;
		}

		int msToStart = row.m_iLapStartMs - serverNowMs;
		if (serverNowMs < 0 || msToStart > 0)
		{
			// Still to start: car number and countdown
			status = string.Format("CAR %1", slot + 1);
			if (serverNowMs >= 0)
				time = "-" + FormatCountdown(msToStart);
			return;
		}

		// On the stage: running time, and the latest split against the fastest
		time = FormatTime((serverNowMs - row.m_iLapStartMs + penaltyMs) * 0.001);
		if (row.GetExtraInt(EXTRA_SPLIT_INDEX, -1) >= 0)
		{
			int delta = row.GetExtraInt(EXTRA_SPLIT_DELTA);
			status = FormatDelta(delta);
			if (delta <= 0)
				statusColor = Color.FromSRGBA(92, 196, 128, 255);
			else
				statusColor = Color.FromSRGBA(232, 170, 72, 255);
		}
		else
		{
			status = string.Format("CAR %1", slot + 1);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetLapTimeText(CRF_RallyHUDRow row, int serverNowMs)
	{
		if (!row || serverNowMs < 0)
			return "--:--.---";

		if (row.m_bFinished)
			return FormatTime(row.GetExtraInt(EXTRA_FINISH, -1) * 0.001);

		if (row.GetExtraInt(EXTRA_SLOT, -1) < 0)
			return "--:--.---";

		int msToStart = row.m_iLapStartMs - serverNowMs;
		if (msToStart > 0)
			return "-" + FormatCountdown(msToStart);

		if (row.m_bDead)
			return "DNF";

		return FormatTime((serverNowMs - row.m_iLapStartMs + row.GetExtraInt(EXTRA_PENALTY)) * 0.001);
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetLapTimeLabel()
	{
		return "STAGE TIME";
	}

	//------------------------------------------------------------------------------------------------
	override protected string GetProgressText(CRF_RallyHUDRow own)
	{
		if (!own)
			return "RALLY STAGE";

		if (own.m_bFinished)
			return "STAGE COMPLETE";

		if (own.m_bDead)
			return "OUT OF THE STAGE";

		int slot = own.GetExtraInt(EXTRA_SLOT, -1);
		if (slot < 0)
			return "WAITING FOR THE START ORDER";

		int serverNowMs = GetServerNowMs();
		if (serverNowMs >= 0 && own.m_iLapStartMs > serverNowMs)
			return string.Format("CAR %1  -  START IN %2", slot + 1, FormatCountdown(own.m_iLapStartMs - serverNowMs));

		if (own.m_iNextCP <= 0)
			return string.Format("CAR %1  -  GO! CROSS THE START", slot + 1);

		return super.GetProgressText(own);
	}

	//===================================================================================
	// UTILITIES
	//===================================================================================

	//! Whole seconds as m:ss, rounded up (a countdown)
	static string FormatCountdown(int ms)
	{
		int secs = Math.Max(0, Math.Ceil(ms / 1000.0));
		int mins = secs / 60;
		secs = secs % 60;
		return string.Format("%1:%2", mins, secs.ToString(2));
	}

	//! A split difference: "+1.234" / "-0.456"
	static string FormatDelta(int ms)
	{
		string sign = "+";
		if (ms < 0)
			sign = "-";

		int absMs = Math.AbsInt(ms);
		return string.Format("%1%2.%3", sign, absMs / 1000, (absMs % 1000).ToString(3));
	}
}
