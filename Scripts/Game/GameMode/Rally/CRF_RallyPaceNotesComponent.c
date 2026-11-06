//------------------------------------------------------------------------------------
// CRF_RallyPaceNotesComponent: pace notes for a rally stage
//
// The mission lists the calls in course order, each with the point on the road it belongs to. The
// calls are read out by the co-driver, so only they see them: anyone in a car who isn't driving.
// Each time the car reaches a checkpoint, they get a hint with the calls for the road ahead, giving
// them time to tell the driver before the car gets there.
//
// Which checkpoint gives a call is worked out from the course entities CRF_RallyGamemodeComponent
// uses (rally_start, rally_cp1, rally_cp2... rally_end): the last one before the call's point, or
// the one before that if the point is closer than m_fMinLeadMetres. A checkpoint with no calls of
// its own shows nothing and leaves the last hint up.
//
// Checkpoints only go forward: reaching a later one skips those in between (a cut or a missed one),
// and the notes start again when the car is back at the start line.
//
// Grades in the calls: 1 (tightest) to 6 (fastest), plus square (a 90 degree junction) and hairpin.
// Distances are to the next call in metres; "into" and "and" mean the next one follows straight on.
//
// Setup: add this component to the gamemode entity and fill in the calls. Purely client side: no
// replication, and nothing runs on a dedicated server.
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! One call: what is said and where
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sCall")]
class CRF_RallyPaceNote
{
	[Attribute(desc: "World position of the call (on the road, before the corner it describes). Decides which checkpoint gives it.")]
	vector m_vPosition;

	[Attribute(desc: "The call, e.g. \"Left 3 tightens, 100\"")]
	string m_sCall;
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Rally pace notes: at each checkpoint the co-driver gets a hint with the calls for the road ahead.")]
class CRF_RallyPaceNotesComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_RallyPaceNotesComponent : SCR_BaseGameModeComponent
{
	[Attribute(desc: "The calls, in course order", category: "Rally - Pace Notes")]
	protected ref array<ref CRF_RallyPaceNote> m_aCalls;

	[Attribute("25", UIWidgets.EditBox, "A checkpoint's calls are given when the car comes within this many metres of it", params: "5 100 1", category: "Rally - Pace Notes")]
	protected float m_fTriggerRadius;

	[Attribute("150", UIWidgets.EditBox, "A call closer than this many metres to the checkpoint before it comes from the checkpoint before that, so there's time to read it out", params: "0 1000 10", category: "Rally - Pace Notes")]
	protected float m_fMinLeadMetres;

	[Attribute("15", UIWidgets.EditBox, "Seconds a hint stays on screen (the next checkpoint's replaces it straight away)", params: "1 60 0.5", category: "Rally - Pace Notes")]
	protected float m_fDisplaySeconds;

	// Course points in order, rally_end last; the calls each one gives (rally_end gives none, so one fewer)
	protected ref array<vector> m_aCoursePoints = {};
	protected ref array<string> m_aCheckpointCalls = {};
	protected int m_iNextCheckpoint;
	protected bool m_bCourseReady;
	protected bool m_bWarnedNoCourse;

	// Checkpoints further ahead than this are never jumped to, so a road that passes near itself can't skip half the stage
	protected static const int LOOKAHEAD = 4;
	// Stretches of road searched for a call, from the one the last call was on
	protected static const int SEARCH_SEGMENTS = 3;
	protected static const int UPDATE_MS = 200;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || System.IsConsoleApp() || !m_aCalls || m_aCalls.IsEmpty())
			return;

		GetGame().GetCallqueue().CallLater(Update, UPDATE_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Update);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void Update()
	{
		IEntity vehicle;
		bool isDriver;
		if (!GetLocalSeat(vehicle, isDriver) || isDriver)
			return;

		if (!m_bCourseReady && !BuildCourse())
			return;

		vector position = vehicle.GetOrigin();
		int count = m_aCheckpointCalls.Count();

		// Back at the start after checkpoints have been given: a fresh run
		if (m_iNextCheckpoint > 1 && vector.DistanceXZ(position, m_aCoursePoints[0]) <= m_fTriggerRadius)
			m_iNextCheckpoint = 0;

		int last = Math.Min(count, m_iNextCheckpoint + LOOKAHEAD);
		for (int i = m_iNextCheckpoint; i < last; i++)
		{
			if (vector.DistanceXZ(position, m_aCoursePoints[i]) > m_fTriggerRadius)
				continue;

			m_iNextCheckpoint = i + 1;
			ShowCalls(i);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The course from its entities, and which checkpoint gives each call
	//! \return false if the course isn't in the world (yet)
	protected bool BuildCourse()
	{
		m_aCoursePoints.Clear();
		m_aCheckpointCalls.Clear();

		IEntity start = GetGame().GetWorld().FindEntityByName("rally_start");
		IEntity finish = GetGame().GetWorld().FindEntityByName("rally_end");
		if (!start || !finish)
		{
			if (!m_bWarnedNoCourse)
				Print("[CRF_RallyPaceNotes] rally_start and rally_end not found: no pace notes until they are", LogLevel.WARNING);

			m_bWarnedNoCourse = true;
			return false;
		}

		m_aCoursePoints.Insert(start.GetOrigin());
		int number = 1;
		IEntity checkpoint = GetGame().GetWorld().FindEntityByName(string.Format("rally_cp%1", number));
		while (checkpoint)
		{
			m_aCoursePoints.Insert(checkpoint.GetOrigin());
			number++;
			checkpoint = GetGame().GetWorld().FindEntityByName(string.Format("rally_cp%1", number));
		}

		m_aCoursePoints.Insert(finish.GetOrigin());

		int segments = m_aCoursePoints.Count() - 1;
		for (int i = 0; i < segments; i++)
			m_aCheckpointCalls.Insert(string.Empty);

		int segment;
		foreach (CRF_RallyPaceNote call : m_aCalls)
		{
			segment = FindSegment(call.m_vPosition, segment);

			int checkpointIndex = segment;
			if (checkpointIndex > 0 && vector.DistanceXZ(call.m_vPosition, m_aCoursePoints[checkpointIndex]) < m_fMinLeadMetres)
				checkpointIndex--;

			if (m_aCheckpointCalls[checkpointIndex].IsEmpty())
				m_aCheckpointCalls[checkpointIndex] = call.m_sCall;
			else
				m_aCheckpointCalls[checkpointIndex] = m_aCheckpointCalls[checkpointIndex] + "\n" + call.m_sCall;
		}

		m_bCourseReady = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The stretch of road between two course points nearest a call, from the last call's stretch on.
	//! A call at a checkpoint counts as on the stretch leading up to it.
	protected int FindSegment(vector position, int fromSegment)
	{
		int best = fromSegment;
		float bestDistance = float.MAX;
		int last = Math.Min(m_aCheckpointCalls.Count(), fromSegment + SEARCH_SEGMENTS);
		for (int i = fromSegment; i < last; i++)
		{
			float distance = DistanceToSegmentXZ(position, m_aCoursePoints[i], m_aCoursePoints[i + 1]);
			if (distance < bestDistance - 1)
			{
				bestDistance = distance;
				best = i;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected float DistanceToSegmentXZ(vector point, vector from, vector to)
	{
		float segmentX = to[0] - from[0];
		float segmentZ = to[2] - from[2];
		float lengthSquared = segmentX * segmentX + segmentZ * segmentZ;

		float along;
		if (lengthSquared > 0)
			along = Math.Clamp(((point[0] - from[0]) * segmentX + (point[2] - from[2]) * segmentZ) / lengthSquared, 0, 1);

		float offsetX = from[0] + along * segmentX - point[0];
		float offsetZ = from[2] + along * segmentZ - point[2];
		return Math.Sqrt(offsetX * offsetX + offsetZ * offsetZ);
	}

	//------------------------------------------------------------------------------------------------
	//! The vehicle the local player is sitting in, and whether they are driving it
	protected bool GetLocalSeat(out IEntity vehicle, out bool isDriver)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character || !character.IsInVehicle())
			return false;

		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access)
			return false;

		BaseCompartmentSlot compartment = access.GetCompartment();
		if (!compartment)
			return false;

		vehicle = compartment.GetVehicle();
		isDriver = PilotCompartmentSlot.Cast(compartment) != null;
		return vehicle != null;
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowCalls(int checkpointIndex)
	{
		string calls = m_aCheckpointCalls[checkpointIndex];
		if (calls.IsEmpty())
			return;

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.ShowLocalHint("[Pace notes] " + calls, m_fDisplaySeconds * 1000);
	}
}
