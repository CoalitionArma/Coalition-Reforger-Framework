//------------------------------------------------------------------------------------------------
//! Game mode objectives as vanilla tasks (SCR_TaskSystem), so they show in the map's task list and
//! update live: created, renamed, progressed, finished and removed from the server, replicated to every
//! client (late joiners included). Not to be confused with CRF_TaskCreatorComponent, the mission-maker
//! "task" props (CollectIntel, PlantDefuseBomb), which are their own system.
//!
//! Rules every mode follows:
//!  - Server only. Every call is a no-op on clients.
//!  - One task per faction: a task has one state, so "completed" for one side and "failed" for the other
//!    are two tasks. Ids are built with MakeId(mode, objective, faction).
//!  - List only (CRF_ObjectiveTask.et): no map icon - each mode keeps drawing its own markers.
//!  - No secrets. Every client receives every task's text and position, whichever faction it's for, so
//!    a task never carries anything its faction couldn't already see (no exact hidden-cache positions,
//!    no hidden player positions).
//!  - Silent: there are no popups for task changes; the modes already announce what happens.
//!  - Tasks still open when the game reaches AAR are closed with the state given at creation
//!    (roundEndState): e.g. a "Defend" task COMPLETED, an "Attack" task FAILED. Modes that know the
//!    result call Finish themselves first.
class CRF_ObjectiveTasks
{
	static const ResourceName TASK_PREFAB = "{9F121A740C69942D}Prefabs/Systems/Tasks/CRF_ObjectiveTask.et";
	static const ResourceName ICONS = "{10C0A9A305E8B3A4}UI/Imagesets/Tasks/Task_Icons.imageset";

	// Icon names in ICONS
	static const string ICON_CAPTURE = "Icon_M_Task_Capture";
	static const string ICON_DEFEND = "Icon_M_Task_Defend";
	static const string ICON_DESTROY = "Icon_M_Task_Blow_Up";
	static const string ICON_KILL = "Icon_M_Task_Neutralize";
	static const string ICON_PROTECT = "Icon_M_Task_Protect";
	static const string ICON_SEARCH = "Icon_M_Task_Recon";
	static const string ICON_INTEL = "Icon_M_Task_Intel";
	static const string ICON_SEIZE = "Icon_M_Task_Seize";

	// Task id -> state to close it with if it's still open when the game reaches AAR
	protected static ref map<string, SCR_ETaskState> s_mRoundEndStates = new map<string, SCR_ETaskState>();
	// Statics outlive the mission: the game mode they were set up for
	protected static COA_Gamemode s_Gamemode;

	//------------------------------------------------------------------------------------------------
	//! "CRF_<mode>_<objective>_<faction>" - unique per objective and side
	static string MakeId(string mode, string objective, FactionKey faction = FactionKey.Empty)
	{
		if (faction.IsEmpty())
			return string.Format("CRF_%1_%2", mode, objective);

		return string.Format("CRF_%1_%2_%3", mode, objective, faction);
	}

	//------------------------------------------------------------------------------------------------
	//! Create a task visible to one faction, or update it if it already exists and is still open.
	//! \param[in] roundEndState state it closes with if still open at AAR
	//! \param[in] position where it is - the task list's "show on map" uses it. Leave zero for objectives
	//!            without a place, or whose place the faction mustn't learn from the task
	//! \return the task, or null on a client, without the task system, or if the faction doesn't exist
	static SCR_Task Create(string id, FactionKey faction, string name, string description = "", string icon = ICON_SEIZE, SCR_ETaskState roundEndState = SCR_ETaskState.CANCELLED, vector position = vector.Zero)
	{
		SCR_TaskSystem taskSystem = GetServerTaskSystem();
		if (!taskSystem || id.IsEmpty() || faction.IsEmpty())
			return null;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager || !factionManager.GetFactionByKey(faction))
		{
			Print(string.Format("[CRF_ObjectiveTasks] No faction '%1' in this mission - task %2 not created", faction, id), LogLevel.WARNING);
			return null;
		}

		SCR_Task task = SCR_TaskSystem.GetTaskFromTaskID(id, false);
		if (task && !IsFinishedState(taskSystem.GetTaskState(task)))
		{
			SetText(id, name, description);
			if (position != vector.Zero)
				taskSystem.MoveTask(task, position);

			s_mRoundEndStates.Set(id, roundEndState);
			return task;
		}

		// A finished task with this id is replaced by a fresh one (repeating objectives)
		if (task)
			taskSystem.DeleteTask(task);

		task = taskSystem.CreateTask(TASK_PREFAB, id, name, description, position);
		if (!task)
		{
			Print(string.Format("[CRF_ObjectiveTasks] Could not create task %1", id), LogLevel.WARNING);
			return null;
		}

		taskSystem.AddTaskFaction(task, faction);
		task.SetTaskIconPath(ICONS);
		task.SetTaskIconSetName(icon);

		s_mRoundEndStates.Set(id, roundEndState);
		return task;
	}

	//------------------------------------------------------------------------------------------------
	//! Rename an open task and/or change its description (an empty description is left as it is)
	static void SetText(string id, string name, string description = "")
	{
		SCR_Task task = GetOpenTask(id);
		if (!task)
			return;

		if (!name.IsEmpty() && task.GetTaskName() != name)
			task.SetTaskName(name);

		if (!description.IsEmpty() && task.GetTaskDescription() != description)
			task.SetTaskDescription(description);
	}

	//------------------------------------------------------------------------------------------------
	//! Progress bar, 0 to 1. Capped below 100% - vanilla completes a task that reaches 100, and only the
	//! mode decides when an objective is done (Finish)
	static void SetProgress(string id, float fraction)
	{
		SCR_Task task = GetOpenTask(id);
		SCR_ExtendedTask extendedTask = SCR_ExtendedTask.Cast(task);
		if (!extendedTask)
			return;

		float percent = Math.Clamp(fraction * 100, 0, 99);
		if (Math.AbsFloat(extendedTask.GetTaskProgress() - percent) < 0.5)
			return;

		SCR_TaskSystem taskSystem = GetServerTaskSystem();
		if (!extendedTask.CanTaskProgressBeShown())
			taskSystem.ShowProgressForTask(task, true);

		taskSystem.SetTaskProgress(task, percent);
	}

	//------------------------------------------------------------------------------------------------
	//! Move an open task (use only for places its faction may know)
	static void Move(string id, vector position)
	{
		SCR_Task task = GetOpenTask(id);
		if (task)
			GetServerTaskSystem().MoveTask(task, position);
	}

	//------------------------------------------------------------------------------------------------
	//! Close an open task: COMPLETED, FAILED or CANCELLED. It stays in the list as finished.
	static void Finish(string id, SCR_ETaskState state)
	{
		SCR_Task task = GetOpenTask(id);
		if (!task)
			return;

		s_mRoundEndStates.Remove(id);
		GetServerTaskSystem().SetTaskState(task, state);
	}

	//------------------------------------------------------------------------------------------------
	//! Finish the given faction's task COMPLETED and every other faction's FAILED, for one objective
	//! \param[in] ids the objective's tasks, by faction
	static void Resolve(notnull map<string, string> ids, FactionKey winner)
	{
		foreach (string faction, string id : ids)
		{
			if (faction == winner)
				Finish(id, SCR_ETaskState.COMPLETED);
			else
				Finish(id, SCR_ETaskState.FAILED);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Delete a task outright (open or finished) - for objectives that are gone rather than done
	static void Remove(string id)
	{
		SCR_TaskSystem taskSystem = GetServerTaskSystem();
		if (!taskSystem)
			return;

		s_mRoundEndStates.Remove(id);
		SCR_Task task = SCR_TaskSystem.GetTaskFromTaskID(id, false);
		if (task)
			taskSystem.DeleteTask(task);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsOpen(string id)
	{
		return GetOpenTask(id) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Close every task still open with its round-end state. Runs by itself at AAR; modes that end the
	//! round can call it right after setting their own results.
	static void FinishAllOpen()
	{
		array<string> openIds = {};
		foreach (string trackedId, SCR_ETaskState unused : s_mRoundEndStates)
			openIds.Insert(trackedId);

		foreach (string id : openIds)
		{
			SCR_ETaskState roundEndState;
			if (s_mRoundEndStates.Find(id, roundEndState))
				Finish(id, roundEndState);
		}

		s_mRoundEndStates.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! The open task with this id, or null
	protected static SCR_Task GetOpenTask(string id)
	{
		SCR_TaskSystem taskSystem = GetServerTaskSystem();
		if (!taskSystem)
			return null;

		SCR_Task task = SCR_TaskSystem.GetTaskFromTaskID(id, false);
		if (!task || IsFinishedState(taskSystem.GetTaskState(task)))
			return null;

		return task;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsFinishedState(SCR_ETaskState state)
	{
		return state == SCR_ETaskState.COMPLETED || state == SCR_ETaskState.FAILED || state == SCR_ETaskState.CANCELLED;
	}

	//------------------------------------------------------------------------------------------------
	//! The task system on the server, with the AAR sweep hooked to this mission's game mode
	protected static SCR_TaskSystem GetServerTaskSystem()
	{
		if (Replication.IsClient())
			return null;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode != s_Gamemode)
		{
			// New mission: forget the last one's tasks and close this one's at AAR
			s_mRoundEndStates.Clear();
			s_Gamemode = gamemode;
			gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
		}

		return SCR_TaskSystem.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnGamemodeStateChanged()
	{
		if (Replication.IsClient() || !s_Gamemode || s_Gamemode.m_GamemodeState != COA_EGamemodeState.AAR)
			return;

		FinishAllOpen();
	}
}
