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
// - Crews can be put straight into a car as they spawn (m_bSeatCrewsInCars): the crew's own car, or the
//   nearest free one, with the Driver role at the wheel.
// - A car waiting inside the start zone is held until its start time: handbrake on, gearbox in neutral,
//   re-applied so the driver can't release it. The server decides which cars are held and replicates
//   the list (m_aHeldCars); the server and every client apply it (ApplyStartHolds). At GO the car is
//   released into first gear. The jump start penalty stays as the backstop.
// - With m_bHoldUntilCalledUp every racer's car is held wherever it is: before the start order is
//   drawn, and after it until the crew is called up to the start line m_fCallUpSeconds before its start.
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

	[Attribute("true", UIWidgets.CheckBox, desc: "Draw the start order at random. Off: in team mode the groups' slotting order (the same order they're put in cars on the grid, front first), otherwise the order players joined.", category: "Rally - Stage")]
	protected bool m_bRandomStartOrder;

	[Attribute("true", UIWidgets.CheckBox, desc: "Put each racer in a car as they spawn: their crew's car, or the nearest free one. The Driver role (VEHICLE_DRIVER) takes the wheel, everyone else a passenger seat. Done once per life - a player who gets out is left alone.", category: "Rally - Stage")]
	protected bool m_bSeatCrewsInCars;

	[Attribute("300", UIWidgets.EditBox, desc: "How far from a spawned racer (metres) a free car is looked for", params: "10 2000 10", category: "Rally - Stage")]
	protected float m_fCrewCarSearchRadius;

	[Attribute("true", UIWidgets.CheckBox, desc: "Hold every racer's car wherever it is until its start time (or until it is called up, Call Up Seconds before), not only inside the start zone. Before the start order is drawn nobody drives.", category: "Rally - Stage")]
	protected bool m_bHoldUntilCalledUp;

	[Attribute("0", UIWidgets.EditBox, desc: "Seconds before a crew's start time that its car is let go to drive up to the start line, where it is held again until GO. 0 = held wherever it is until GO.", params: "0 300 1", category: "Rally - Stage")]
	protected float m_fCallUpSeconds;

	[Attribute("30", UIWidgets.EditBox, desc: "A crew that sets off from away from the start line (a grid behind it) has this long after its start time to reach the line: its clock starts when it crosses, or when this runs out. 0 = the clock always starts at the start time.", params: "0 300 1", category: "Rally - Stage")]
	protected float m_fMoveUpSeconds;

	// Server state
	protected ref map<int, ref CRF_RallyStageEntry> m_mStage = new map<int, ref CRF_RallyStageEntry>();

	// Server: seating racers in cars. The car each crew was put in, and per player the body last seated
	// with how many tries it has had and when the last one was
	protected ref map<string, RplId> m_mCrewCars = new map<string, RplId>();
	protected ref map<int, RplId> m_mSeatBody = new map<int, RplId>();
	protected ref map<int, int> m_mSeatTries = new map<int, int>();
	protected ref map<int, int> m_mSeatTriedAtMs = new map<int, int>();
	protected ref array<IEntity> m_aFoundCars = {};

	protected static const int SEAT_MAX_TRIES = 3;
	protected static const int SEAT_RETRY_MS = 2000;
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

		int now = GetGame().GetWorld().GetWorldTime();

		// From the first spawn on, safestart included
		if (m_bSeatCrewsInCars)
			SeatRacersInCars(now);

		// Which cars wait - decided here, replicated to every machine
		UpdateStartHolds(now);

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;
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
	//! Server: racers on foot who haven't started go into their crew's car, or the nearest free one.
	//! The seat change runs on the player's own machine (MoveInVehicle sends it there), so a body that
	//! still isn't seated after SEAT_RETRY_MS is tried again, up to SEAT_MAX_TRIES times per life.
	protected void SeatRacersInCars(int now)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
		{
			int playerId = entry.m_iPlayerId;
			if (entry.m_bFinished || entry.m_iNextCheckpoint != 0)
				continue;

			// Already away
			CRF_RallyStageEntry stage = m_mStage.Get(playerId);
			if (stage && stage.m_iSlot >= 0 && now >= stage.m_iClockStartMs)
				continue;

			ChimeraCharacter character = ChimeraCharacter.Cast(playerManager.GetPlayerControlledEntity(playerId));
			if (!character || COA_EntityHelper.IsSpectator(character) || character.GetCharacterController().IsDead())
				continue;

			RplComponent bodyRpl = RplComponent.Cast(character.FindComponent(RplComponent));
			if (!bodyRpl)
				continue;

			// A new body (first spawn or a respawn) gets a fresh set of tries
			RplId seatedBody;
			if (!m_mSeatBody.Find(playerId, seatedBody) || seatedBody != bodyRpl.Id())
			{
				m_mSeatBody.Set(playerId, bodyRpl.Id());
				m_mSeatTries.Set(playerId, 0);
			}

			int tries = m_mSeatTries.Get(playerId);
			if (character.IsInVehicle())
			{
				// Seated - by us or by themselves. Either way, leave them be from now on
				m_mSeatTries.Set(playerId, SEAT_MAX_TRIES);
				continue;
			}

			if (tries >= SEAT_MAX_TRIES || (tries > 0 && now - m_mSeatTriedAtMs.Get(playerId) < SEAT_RETRY_MS))
				continue;

			SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(character.GetCompartmentAccessComponent());
			if (!access)
				continue;

			string crewKey = GetCrewKey(playerId);
			IEntity car = GetCrewCar(crewKey, playerId, character.GetOrigin());
			if (!car)
				continue; // no car yet (still spawning in): try again next poll, without using a try

			// The driver takes the wheel, the rest a passenger seat; whichever is free if that one isn't
			bool driver = IsDriverRole(playerId);
			bool seated;
			if (driver)
				seated = access.MoveInVehicle(car, ECompartmentType.PILOT) || access.MoveInVehicle(car, ECompartmentType.CARGO);
			else
				seated = access.MoveInVehicle(car, ECompartmentType.CARGO) || access.MoveInVehicle(car, ECompartmentType.PILOT);

			if (!seated)
			{
				// Full: let the crew find another car
				m_mCrewCars.Remove(crewKey);
				continue;
			}

			m_mSeatTries.Set(playerId, tries + 1);
			m_mSeatTriedAtMs.Set(playerId, now);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The car a crew was put in, or else a free one within m_fCrewCarSearchRadius, claimed for the crew.
	//! The cars are ranked front to back by how near they are to the start line, and the crews by their
	//! group's place in the slotting (team mode), so the first group gets the front car, the second the
	//! next and so on. Without a rank, or if that car is taken, the front-most free car.
	protected IEntity GetCrewCar(string crewKey, int playerId, vector near)
	{
		RplId carId;
		if (m_mCrewCars.Find(crewKey, carId))
		{
			IEntity claimed = COA_EntityHelper.GetEntityFromRplId(carId);
			if (claimed)
				return claimed;

			m_mCrewCars.Remove(crewKey);
		}

		m_aFoundCars.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(near, m_fCrewCarSearchRadius, CollectCar, null, EQueryEntitiesFlags.DYNAMIC);

		// Front of the grid first
		vector startLine = near;
		if (!m_aCheckpointPositions.IsEmpty())
			startLine = m_aCheckpointPositions[0];

		array<IEntity> grid = {};
		array<float> gridDistances = {};
		foreach (IEntity found : m_aFoundCars)
		{
			float distance = vector.DistanceSq(found.GetOrigin(), startLine);
			int at = 0;
			while (at < gridDistances.Count() && gridDistances[at] <= distance)
				at++;

			grid.InsertAt(found, at);
			gridDistances.InsertAt(distance, at);
		}
		m_aFoundCars.Clear();

		IEntity chosen;
		int rank = GetCrewRank(playerId);
		if (grid.IsIndexValid(rank) && IsCarFree(grid[rank]))
			chosen = grid[rank];

		if (!chosen)
		{
			foreach (IEntity car : grid)
			{
				if (IsCarFree(car))
				{
					chosen = car;
					break;
				}
			}
		}

		if (chosen)
			m_mCrewCars.Set(crewKey, RplComponent.Cast(chosen.FindComponent(RplComponent)).Id());

		return chosen;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCarFree(IEntity car)
	{
		RplComponent carRpl = RplComponent.Cast(car.FindComponent(RplComponent));
		return carRpl && !IsCarClaimed(carRpl.Id()) && IsCarEmpty(car);
	}

	//------------------------------------------------------------------------------------------------
	//! Team mode: where the player's group comes among the groups that have players, in slotting
	//! order (0 = first). -1 without a group.
	protected int GetCrewRank(int playerId)
	{
		SCR_AIGroup group = GetCrewGroup(playerId);
		if (!group)
			return -1;

		array<SCR_AIGroup> crewGroups = {};
		GetCrewGroupsInOrder(crewGroups);
		return crewGroups.Find(group);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_AIGroup GetCrewGroup(int playerId)
	{
		if (!m_bTeamMode)
			return null;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return null;

		return groupsManager.GetPlayerGroup(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! The slotting groups with at least one player in them, in slotting order (group ID order -
	//! COA_SlottingManager.GetAllGroups sorts that way)
	protected void GetCrewGroupsInOrder(notnull array<SCR_AIGroup> outGroups)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		array<SCR_AIGroup> groups = slottingManager.GetAllGroups();
		foreach (SCR_AIGroup group : groups)
		{
			RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
			if (!groupRpl)
				continue;

			array<int> slotIds = slottingManager.GetAllSlotIDsForGroup(groupRpl.Id());
			foreach (int slotId : slotIds)
			{
				COA_SlotData slotData = slottingManager.GetSlotData(slotId);
				if (slotData && slotData.GetSlotCurrentPlayerId() > 0)
				{
					outGroups.Insert(group);
					break;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectCar(IEntity entity)
	{
		Vehicle vehicle = Vehicle.Cast(entity);
		if (vehicle && vehicle == vehicle.GetRootParent() && CarControllerComponent.Cast(vehicle.GetVehicleController()) && !m_aFoundCars.Contains(vehicle))
			m_aFoundCars.Insert(vehicle);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCarClaimed(RplId carId)
	{
		foreach (string crewKey, RplId claimedId : m_mCrewCars)
		{
			if (claimedId == carId)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCarEmpty(IEntity car)
	{
		BaseCompartmentManagerComponent compartments = BaseCompartmentManagerComponent.Cast(car.FindComponent(BaseCompartmentManagerComponent));
		if (!compartments)
			return false;

		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (slot.GetOccupant())
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsDriverRole(int playerId)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return true;

		COA_SlotData slotData = slottingManager.GetPlayerSlotData(playerId);
		return !slotData || slotData.GetSlotRole() == COA_EGearRole.VEHICLE_DRIVER;
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
		else if (m_bTeamMode)
		{
			SortCrewsBySlotting(crewKeys, crews);
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
	//! Team mode, start order not random: crews in their groups' slotting order (the order they were
	//! put in cars on the grid), crews without a group at the back in join order
	protected void SortCrewsBySlotting(notnull array<string> crewKeys, notnull map<string, ref array<int>> crews)
	{
		array<string> sorted = {};
		array<int> ranks = {};
		array<string> unranked = {};
		foreach (string crewKey : crewKeys)
		{
			array<int> members = crews.Get(crewKey);
			int rank = -1;
			if (members && !members.IsEmpty())
				rank = GetCrewRank(members[0]);

			if (rank < 0)
			{
				unranked.Insert(crewKey);
				continue;
			}

			int at = 0;
			while (at < ranks.Count() && ranks[at] <= rank)
				at++;

			sorted.InsertAt(crewKey, at);
			ranks.InsertAt(rank, at);
		}

		crewKeys.Clear();
		crewKeys.InsertAll(sorted);
		crewKeys.InsertAll(unranked);
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
		int callUpSecs = m_fCallUpSeconds;
		if (m_bHoldUntilCalledUp && callUpSecs > 0 && !countdownSeconds.Contains(callUpSecs))
			countdownSeconds.Insert(callUpSecs);

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
				if (m_bHoldUntilCalledUp && callUpSecs > 0 && secsLeft == callUpSecs)
					Rpc(RpcDo_CheckpointNotify, playerId, string.Format("Start in %1 - drive up to the start line", secsLeft));
				else
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

			// Set off on time from away from the line: the clock starts here, unless the crew took
			// longer than m_fMoveUpSeconds to arrive
			if (now >= stage.m_iClockStartMs && !stage.m_bJumpStart && m_fMoveUpSeconds > 0)
			{
				int clockStartMs = (int)Math.Min(now, stage.m_iClockStartMs + m_fMoveUpSeconds * 1000);
				stage.m_iClockStartMs = clockStartMs;
				entry.m_iLapStartTimeMs = clockStartMs;
				entry.m_iRaceStartTimeMs = clockStartMs;
				Rpc(RpcDo_CheckpointNotify, pid, "Stage clock running");
				return;
			}

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

	// The start hold. The server decides which cars have to wait (UpdateStartHolds) and replicates the
	// list; every machine - server and clients - then holds those cars itself, the way vanilla's
	// handbrake action (SCR_ToggleHandbrakeAction) runs where it is performed and on the server. The
	// driver's machine is the one that counts while a car is driven (it simulates it); the server's
	// copy keeps the car held if it changes hands.
	[RplProp(onRplName: "OnStartHoldsChanged")]
	protected ref array<RplId> m_aHeldCars = {};

	// The held cars that are waiting in the start zone (for the driver's message)
	[RplProp(onRplName: "OnStartHoldsChanged")]
	protected ref array<RplId> m_aHeldAtStartLine = {};

	// Every machine: the cars it is holding now
	protected ref array<RplId> m_aLocallyHeld = {};

	// Client: why the local player's car is held (HOLD_*), for the messages
	protected int m_iLocalHoldReason;

	protected static const int HOLD_NONE = 0;
	protected static const int HOLD_CALL_UP = 1;
	protected static const int HOLD_START_LINE = 2;

	//------------------------------------------------------------------------------------------------
	override protected void UpdateHUD()
	{
		super.UpdateHUD();

		// Re-applied ten times a second, so a driver letting the handbrake off gets nowhere
		ApplyStartHolds();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: which racers' cars have to wait. Inside the start zone before the crew's start time,
	//! and with m_bHoldUntilCalledUp anywhere before the start order is drawn or before the crew is
	//! called up. Replicated when it changes, applied here on the server every poll.
	protected void UpdateStartHolds(int now)
	{
		array<RplId> held = {};
		array<RplId> atStartLine = {};
		if (m_bRaceActive && !m_aCheckpointPositions.IsEmpty())
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			foreach (CRF_RallyPlayerEntry entry : m_aPlayerEntries)
			{
				if (entry.m_bFinished || entry.m_bDead || entry.m_iNextCheckpoint != 0)
					continue;

				Vehicle car = GetDrivenCar(playerManager.GetPlayerControlledEntity(entry.m_iPlayerId));
				if (!car)
					continue;

				RplComponent carRpl = RplComponent.Cast(car.FindComponent(RplComponent));
				if (!carRpl || held.Contains(carRpl.Id()))
					continue;

				bool inStartZone = vector.Distance(car.GetOrigin(), m_aCheckpointPositions[0]) <= m_fCheckpointRadius;
				if (!MustWait(entry.m_iPlayerId, now, inStartZone))
					continue;

				held.Insert(carRpl.Id());
				if (inStartZone)
					atStartLine.Insert(carRpl.Id());
			}
		}

		if (!SameIds(held, m_aHeldCars) || !SameIds(atStartLine, m_aHeldAtStartLine))
		{
			m_aHeldCars.Copy(held);
			m_aHeldAtStartLine.Copy(atStartLine);
			Replication.BumpMe();
		}

		ApplyStartHolds();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: whether a racer's car has to wait right now
	protected bool MustWait(int playerId, int now, bool inStartZone)
	{
		// No start time yet: nobody drives until the order is drawn
		CRF_RallyStageEntry stage = m_mStage.Get(playerId);
		if (!stage || stage.m_iSlot < 0)
			return m_bHoldUntilCalledUp;

		if (stage.m_bJumpStart || now >= stage.m_iClockStartMs)
			return false;

		if (inStartZone)
			return true;

		return m_bHoldUntilCalledUp && now < stage.m_iClockStartMs - m_fCallUpSeconds * 1000;
	}

	//------------------------------------------------------------------------------------------------
	protected bool SameIds(notnull array<RplId> a, notnull array<RplId> b)
	{
		if (a.Count() != b.Count())
			return false;

		foreach (RplId id : a)
		{
			if (!b.Contains(id))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Clients: the held list arrived - apply it now rather than at the next HUD tick
	protected void OnStartHoldsChanged()
	{
		ApplyStartHolds();
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine: hold the listed cars, let go of the ones no longer listed
	protected void ApplyStartHolds()
	{
		for (int i = m_aLocallyHeld.Count() - 1; i >= 0; i--)
		{
			if (m_aHeldCars.Contains(m_aLocallyHeld[i]))
				continue;

			ReleaseStartHold(m_aLocallyHeld[i]);
			m_aLocallyHeld.Remove(i);
		}

		foreach (RplId carId : m_aHeldCars)
		{
			// A car this machine hasn't streamed in is skipped until it has
			if (ApplyStartHold(carId) && !m_aLocallyHeld.Contains(carId))
				m_aLocallyHeld.Insert(carId);
		}

		if (!System.IsConsoleApp())
			UpdateLocalHoldMessage();
	}

	//------------------------------------------------------------------------------------------------
	//! The car a character is driving (in its pilot seat), if any
	protected Vehicle GetDrivenCar(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return null;

		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access)
			return null;

		BaseCompartmentSlot seat = access.GetCompartment();
		if (!PilotCompartmentSlot.Cast(seat))
			return null;

		Vehicle vehicle = Vehicle.Cast(seat.GetOwner().GetRootParent());
		if (!vehicle || !CarControllerComponent.Cast(vehicle.GetVehicleController()))
			return null;

		return vehicle;
	}

	//------------------------------------------------------------------------------------------------
	protected CarControllerComponent GetCarController(RplId carId)
	{
		Vehicle vehicle = Vehicle.Cast(COA_EntityHelper.GetEntityFromRplId(carId));
		if (!vehicle)
			return null;

		return CarControllerComponent.Cast(vehicle.GetVehicleController());
	}

	//------------------------------------------------------------------------------------------------
	//! Handbrake on, brakes full, gearbox in neutral. False if the car isn't here (yet)
	protected bool ApplyStartHold(RplId carId)
	{
		CarControllerComponent controller = GetCarController(carId);
		if (!controller)
			return false;

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

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Lets the car go: handbrake off, first gear, physics woken
	protected void ReleaseStartHold(RplId carId)
	{
		CarControllerComponent controller = GetCarController(carId);
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
	//! Client: tell the local driver when, and why, their car is held
	protected void UpdateLocalHoldMessage()
	{
		int reason = HOLD_NONE;
		Vehicle car = GetDrivenCar(SCR_PlayerController.GetLocalControlledEntity());
		if (car)
		{
			RplComponent carRpl = RplComponent.Cast(car.FindComponent(RplComponent));
			if (carRpl && m_aHeldAtStartLine.Contains(carRpl.Id()))
				reason = HOLD_START_LINE;
			else if (carRpl && m_aHeldCars.Contains(carRpl.Id()))
				reason = HOLD_CALL_UP;
		}

		if (reason == m_iLocalHoldReason)
			return;

		m_iLocalHoldReason = reason;
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
			return;

		if (reason == HOLD_START_LINE)
			popup.PopupMsg("Held at the start line until your start time", 4);
		else if (reason == HOLD_CALL_UP && m_fCallUpSeconds > 0)
			popup.PopupMsg("Held until your crew is called up to the start line", 4);
		else if (reason == HOLD_CALL_UP)
			popup.PopupMsg("Held until your start time", 4);
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine: whether a car is being held at the start (the replicated list). Used to keep its
	//! handbrake from being let off (CRF_SCR_ToggleHandbrakeAction).
	static bool IsCarHeld(IEntity vehicle)
	{
		CRF_RallyStageGamemodeComponent stage = CRF_RallyStageGamemodeComponent.Cast(GetInstance());
		if (!stage || !vehicle)
			return false;

		RplComponent carRpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		return carRpl && stage.m_aHeldCars.Contains(carRpl.Id());
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
