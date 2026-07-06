//=============================================================================
// CRF_TaskCreatorComponent.c
// Spawnable objective task system for CRF missions.
//
// Spawns designer-configured prefabs at named world entities, tracks per task
// completion, sends faction filtered popup notifications, and manages map markers.
// Can serve as a standalone gamemode component or supplement any other gamemode.
//
// FOLDER STRUCTURE CONVENTION
// Scripts/Game/Systems/Components/Objectives/
//   CRF_TaskCreatorComponent.c  — Enums, entry container, main component
//   CRF_TaskCreatorAction.c     — Type-aware ScriptedUserAction for task prefabs
//
// Prefabs/Structures/ObjectiveTasks/
//   (organise prefab subfolders per task type as needed)
//
// TODO:
// - Insure bug free
// - Figure out more task types
//
// SETUP
// 1. Add CRF_TaskCreatorComponent to the CRF_Gamemode entity.
// 2. Add task entries in the component, filling in:
//      - Task Label   : display name (editor/debug only)
//      - Task Type    : category enum (COLLECT_INTEL, etc.)
//      - Spawn Entity : name of an empty GenericEntity placed in the world
//      - Prefab       : .et resource to spawn at that entity's position
//      - Notification settings
//      - Marker settings
// 3. Place empty GenericEntity objects in the world using the spawn entity names.
// 4. Task prefabs should carry a CRF_TaskCreatorAction with the matching
//    task index configured in the action's attribute.
// =============================================================================

//=============================================================================
// ENUMS
//=============================================================================

// Categorizes the task type. Drives action text, perform logic, and prefab
// subfolder organisation. Add new values here when extending the system.
enum CRF_EObjectiveTaskType
{
	COLLECT_INTEL,       // Retrieve documents, data drives, or intel items
	PLANT_DEFUSE_BOMB    // Plant a bomb to detonate, or defuse before it explodes
}

// Defines which faction(s) receive the completion notification popup.
enum CRF_EObjectiveNotifySide
{
	ALL,     // Broadcast to all connected players regardless of faction
	BLUFOR,  // BLUFOR faction only
	OPFOR,   // OPFOR faction only
	INDFOR,  // INDFOR faction only
	CIV,     // CIV faction only
	NONE     // Suppress notification entirely
}

// Determines when the mission is won for a faction.
enum CRF_ETaskWinCondition
{
	NONE,                  // No automatic win condition; missions use task system for tracking only
//	MAJORITY,              // First side to complete >50% of tasks wins
	SET_AMOUNT_PER_SIDE    // Each side must reach m_iTasksRequiredForWin to win
}

//=============================================================================
// TASK ENTRY CONTAINER
//=============================================================================

// One entry = one spawnable task object with its own completion logic hooks,notification config, and optional map marker.
//
// SCR_BaseContainerCustomTitleField drives the editor list title to m_sTaskLabel.
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTaskLabel")]
class CRF_TaskCreatorEntry
{
	// -------------------------------------------------------------------------
	// Task Identity
	// -------------------------------------------------------------------------
	[Attribute("New Task", UIWidgets.EditBox, "Display label used in the editor list and debug output. Does not affect gameplay.", category: "Task Identity")]
	string m_sTaskLabel;

	[Attribute("0", UIWidgets.ComboBox, "Objective category. Drives action text and perform logic in CRF_TaskCreatorAction. Add new values to CRF_EObjectiveTaskType to extend.", "", ParamEnumArray.FromEnum(CRF_EObjectiveTaskType), category: "Task Identity")]
	CRF_EObjectiveTaskType m_eTaskType;

	[Attribute("0", UIWidgets.ComboBox, "Which faction can interact with this task. ALL = any faction. BLUFOR/OPFOR/INDFOR restricts the action to players of that faction. NONE disables interaction for everyone.", "", ParamEnumArray.FromEnum(CRF_EObjectiveNotifySide), category: "Task Identity")]
	CRF_EObjectiveNotifySide m_eAssignedSide;

	// -------------------------------------------------------------------------
	// Spawning
	// -------------------------------------------------------------------------
	[Attribute("", UIWidgets.EditBox, "Exact name of the empty GenericEntity placed in the world where the task prefab will spawn. Case-sensitive.", category: "Spawning")]
	string m_sSpawnEntityName;

	// -------------------------------------------------------------------------
	// Notification
	// -------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, "Show a popup notification to players when this task is completed.", category: "Notification")]
	bool m_bSendNotification;

	[Attribute("0", UIWidgets.ComboBox, "Which faction(s) receive the completion notification. ALL = everyone. NONE = no notification.", "", ParamEnumArray.FromEnum(CRF_EObjectiveNotifySide), category: "Notification")]
	CRF_EObjectiveNotifySide m_eNotifySide;

	[Attribute("%TASKNAME% complete.", UIWidgets.EditBox, "Notification text shown on task completion. Use %TASKNAME% for objective name. Avoid pipe characters ( | ).", category: "Notification")]
	string m_sNotificationText;

	// -------------------------------------------------------------------------
	// Map Marker
	// -------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, "Create a CRF scripted map marker at the task object's spawn position. Marker is removed on task completion.", category: "Marker")]
	bool m_bCreateMarker;

	[Attribute("Objective", UIWidgets.EditBox, "Text label shown on the map marker.", category: "Marker")]
	string m_sMarkerLabel;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", uiwidget: "resourcePickerThumbnail", "Map marker icon. Defaults to the agnostic objective waypoint icon used across other gamemodes.", params: "edds", category: "Marker")]
	ResourceName m_sMarkerIcon;
}

//=============================================================================
// COMPONENT CLASS / COMPONENT
//=============================================================================

[ComponentEditorProps(category: "GameScripted/Objectives", description: "Spawns and tracks objective task objects. Each entry spawns a prefab at a named world entity and tracks completion, notifications, and map markers. Attach to the CRF_Gamemode entity as a supplemental system component.")]
class CRF_TaskCreatorComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_TaskCreatorComponent : SCR_BaseGameModeComponent
{
	//=========================================================================
	// EDITOR ATTRIBUTES
	//=========================================================================

	[Attribute(desc: "One entry per objective. Each entry spawns a task prefab at a named world entity and handles completion logic, notifications, and markers.", category: "Objective Tasks")]
	ref array<ref CRF_TaskCreatorEntry> m_aTaskEntries;

	// One handler per task type — holds the prefab, action name, and perform logic
	// for that type. Configure once here; entries and actions read from it automatically.
	[Attribute(desc: "Handler for COLLECT_INTEL tasks. Configure the prefab here.", category: "Task Type Handlers")]
	ref CRF_TaskHandler_CollectIntel m_CollectIntel;

	[Attribute(desc: "Handler for PLANT_DEFUSE_BOMB tasks. Configure the prefab and timings here.", category: "Task Type Handlers")]
	ref CRF_TaskHandler_PlantDefuseBomb m_PlantDefuseBomb;

	[Attribute("1", UIWidgets.CheckBox, "Delete the spawned task object prefab from the world when the task is marked complete.", category: "Settings")]
	bool m_bDeleteObjectOnComplete;

	[Attribute("0", UIWidgets.ComboBox, "Win condition: NONE (no auto-win), MAJORITY (first to >50% wins), SET_AMOUNT_PER_SIDE (each side needs m_iTasksRequiredForWin).", "", ParamEnumArray.FromEnum(CRF_ETaskWinCondition), category: "Win Condition")]
	CRF_ETaskWinCondition m_eWinCondition;

	[Attribute("0", UIWidgets.SpinBox, "For SET_AMOUNT_PER_SIDE: number of tasks a faction must complete to win. 0 = disabled.", "-1 65535", category: "Win Condition")]
	int m_iTasksRequiredForWin;

	[Attribute("1", UIWidgets.CheckBox, "Send a notification when a faction wins (if m_eWinCondition is not NONE).", category: "Win Condition")]
	bool m_bNotifyOnSideWin;

	[Attribute("%SIDE% has won!", UIWidgets.EditBox, "Notification text shown when a faction wins. Use %SIDE% for faction name.", category: "Win Condition")]
	string m_sSideWinText;

	//=========================================================================
	// RUNTIME — REPLICATED
	//=========================================================================

	// Per-task completion flags. Array is sized from m_aTaskEntries at spawn time.
	// onRplName fires on all clients (including JIP) whenever any entry changes.
	[RplProp(onRplName: "OnTaskStateChanged")]
	ref array<bool> m_aTasksCompleted = {};

	// Stores which faction completed each task (NONE = not yet completed).
	// Index corresponds to task index. Value = CRF_EObjectiveNotifySide enum.
	[RplProp()]
	protected ref array<int> m_aTaskCompletedBySide = {};

	// Stores the index of the most recently completed task that has a notification.
	// Changing this triggers OnTaskCompleteNotify on all clients.
	[RplProp(onRplName: "OnTaskCompleteNotify")]
	protected int m_iLastNotifiedTaskIndex = -1;

	// Set to the winning faction index (1-4) when a faction wins.
	// Triggers OnSideWinNotify on all clients to show the win banner.
	[RplProp(onRplName: "OnSideWinNotify")]
	protected int m_iLastWinSide = -1;



	//=========================================================================
	// RUNTIME — SERVER ONLY
	//=========================================================================

	protected ref array<IEntity> m_aSpawnedTaskObjects;   // Spawned prefab entity refs
	protected ref array<bool> m_aSideWonNotified;          // Guards against double-fire per side on win


	//=========================================================================
	// STATIC ACCESSOR
	//=========================================================================

	protected static CRF_TaskCreatorComponent m_sInstance;

	void CRF_TaskCreatorComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	static CRF_TaskCreatorComponent GetInstance()
	{
		return m_sInstance;
	}

	//=========================================================================
	// LIFECYCLE
	//=========================================================================

	override protected void OnWorldPostProcess(World world) // Testing OnWorldPostProcess as it fires after OnPostinit
	{
		if (!GetGame().InPlayMode())
			return;

		if (!m_aTaskEntries || m_aTaskEntries.Count() == 0)
			return;

		// Size completion array on all machines.
		m_aTasksCompleted.Clear();
		m_aTaskCompletedBySide.Clear();
		for (int i = 0; i < m_aTaskEntries.Count(); i++)
		{
			m_aTasksCompleted.Insert(false);
			m_aTaskCompletedBySide.Insert(CRF_EObjectiveNotifySide.NONE);  // Initialize to NONE (not completed)
		}

		if (Replication.IsServer())
		{
			// Server: allocate entity ref array and spawn prefabs
			m_aSpawnedTaskObjects = new array<IEntity>();
			for (int i = 0; i < m_aTaskEntries.Count(); i++)
				m_aSpawnedTaskObjects.Insert(null);

			foreach (int index, CRF_TaskCreatorEntry entry : m_aTaskEntries)
				SpawnTaskObject(index, entry);

			// Win notification guard — six slots indexed by CRF_EObjectiveNotifySide: ALL=0, BLUFOR=1, OPFOR=2, INDFOR=3, CIV=4, NONE=5.
			m_aSideWonNotified = new array<bool>();
			for (int si = 0; si < 6; si++)
				m_aSideWonNotified.Insert(false);

			Replication.BumpMe();
		}

		// All machines: create map markers after a short delay so the player
		// controller (and CRF_PlayerScriptedMarkerManager) has time to initialize.
		// Follows the same pattern as Rush InitializeMapMarkers.
		GetGame().GetCallqueue().CallLater(InitializeTaskMarkers, 1000, false);
	}

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);

		// Bump replication so JIP clients receive the current task state promptly.
		if (Replication.IsServer())
			Replication.BumpMe();
	}

	// Deferred marker creation — retries until CRF_PlayerScriptedMarkerManager is ready.
	// Called via CallLater from OnWorldPostProcess; follows Rush InitializeMapMarkers pattern.
	protected void InitializeTaskMarkers()
	{
		CRF_PlayerScriptedMarkerManager markerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
		{
			GetGame().GetCallqueue().CallLater(InitializeTaskMarkers, 2000, false);
			return;
		}

		foreach (CRF_TaskCreatorEntry entry : m_aTaskEntries)
		{
			if (entry && entry.m_bCreateMarker && !entry.m_sSpawnEntityName.IsEmpty())
				CreateTaskMarker(entry);
		}
	}

	//=========================================================================
	// SPAWNING
	//=========================================================================

	protected void SpawnTaskObject(int index, CRF_TaskCreatorEntry entry)
	{
		if (!Replication.IsServer())
			return;

		CRF_BaseTaskHandler handler = GetHandlerForType(entry.m_eTaskType);
		ResourceName prefab = string.Empty;
		if (handler)
			prefab = handler.GetPrefab();

		if (entry.m_sSpawnEntityName.IsEmpty() || prefab.IsEmpty())
		{
			Print(string.Format("[CRF_TaskCreator] Entry %1 ('%2'): spawn entity name not configured or no prefab set for task type — skipping.", index, entry.m_sTaskLabel), LogLevel.WARNING);
			return;
		}

		IEntity spawnAnchor = GetGame().GetWorld().FindEntityByName(entry.m_sSpawnEntityName);
		if (!spawnAnchor)
		{
			Print(string.Format("[CRF_TaskCreator] Entry %1 ('%2'): spawn entity '%3' not found in world.", index, entry.m_sTaskLabel, entry.m_sSpawnEntityName), LogLevel.WARNING);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnAnchor.GetTransform(spawnParams.Transform);

		IEntity spawnedObj = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!spawnedObj)
		{
			Print(string.Format("[CRF_TaskCreator] Entry %1 ('%2'): failed to spawn prefab '%3'.", index, entry.m_sTaskLabel, prefab), LogLevel.ERROR);
			return;
		}

		m_aSpawnedTaskObjects[index] = spawnedObj;

		// Inject task data into the object's CRF_TaskCreatorObjectComponent.
		// This is how CRF_TaskCreatorAction identifies its task index and type
		// without a designer-set attribute (same pattern as Vehicle Depot).
		CRF_TaskCreatorObjectComponent taskObjComp = CRF_TaskCreatorObjectComponent.Cast(spawnedObj.FindComponent(CRF_TaskCreatorObjectComponent));
		if (taskObjComp)
			taskObjComp.SetTaskData(index, entry.m_eTaskType);
		else
			Print(string.Format("[CRF_TaskCreator] Entry %1 ('%2'): spawned prefab has no CRF_TaskCreatorObjectComponent — add it to the prefab.", index, entry.m_sTaskLabel), LogLevel.WARNING);

		Print(string.Format("[CRF_TaskCreator] Entry %1 ('%2'): spawned at entity '%3'.", index, entry.m_sTaskLabel, entry.m_sSpawnEntityName), LogLevel.NORMAL);
	}

	//=========================================================================
	// TASK COMPLETION
	//=========================================================================

	// Called server-side by CRF_PlayerRplToAuthorityManager.RpcAsk_RequestObjectiveTaskComplete.
	// Marks the task complete, stores which side completed it, optionally removes the object,
	// fires notifications, and propagates state to all clients via RplProp.
	void MarkTaskComplete(int taskIndex, int completingSideInt = 0)
	{
		if (!Replication.IsServer())
			return;

		CRF_EObjectiveNotifySide completingSide = completingSideInt;

		if (!m_aTaskEntries || taskIndex < 0 || taskIndex >= m_aTaskEntries.Count())
			return;

		if (m_aTasksCompleted.Count() <= taskIndex || m_aTasksCompleted[taskIndex])
			return;  // Already complete or out of range

		m_aTasksCompleted[taskIndex] = true;
		
		// Store which side completed this task
		if (m_aTaskCompletedBySide.Count() > taskIndex)
			m_aTaskCompletedBySide[taskIndex] = completingSide;
		
		Replication.BumpMe();

		CRF_TaskCreatorEntry entry = m_aTaskEntries[taskIndex];
		if (!entry)
			return;

		Print(string.Format("[CRF_TaskCreator] Task %1 ('%2') marked complete by %3.", taskIndex, entry.m_sTaskLabel, GetSideName(completingSide)), LogLevel.NORMAL);
		TryDeleteTaskObject(taskIndex);

		// Trigger notification on all clients if configured
		if (entry.m_bSendNotification && entry.m_eNotifySide != CRF_EObjectiveNotifySide.NONE)
		{
			m_iLastNotifiedTaskIndex = taskIndex;
			Replication.BumpMe();
			OnTaskCompleteNotify();
		}

		CheckSideWins();
	}

	// Checks if any faction has achieved the win condition and fires notifications.
	// Called after every task completion. Follows similar pattern to CheckSideCompletions.
	protected void CheckSideWins()
	{
		if (m_eWinCondition == CRF_ETaskWinCondition.NONE || !m_aSideWonNotified)
			return;

		// BLUFOR=1, OPFOR=2, INDFOR=3, CIV=4  —  skip ALL(0) and NONE(5)
		for (int sideIdx = 1; sideIdx <= 4; sideIdx++)
		{
			if (sideIdx >= m_aSideWonNotified.Count())
				continue;

			if (m_aSideWonNotified[sideIdx])
				continue;  // Already won, don't fire again

			CRF_EObjectiveNotifySide checkSide = sideIdx;

			bool hasWon = false;
			if (m_eWinCondition == CRF_ETaskWinCondition.SET_AMOUNT_PER_SIDE)
			{
				// Win if completed >= m_iTasksRequiredForWin
				if (m_iTasksRequiredForWin > 0)
				{
					if (GetCompletedTaskCountForSide(checkSide) >= m_iTasksRequiredForWin)
						hasWon = true;
				}
			}

			if (hasWon)
			{
				m_aSideWonNotified[sideIdx] = true;
				// Set RplProp so all clients receive the win notification.
				// Also call directly for listen-server host.
				m_iLastWinSide = sideIdx;
				Replication.BumpMe();
				OnSideWinNotify();
			}
		}
	}

	// Removes the spawned task object entity if m_bDeleteObjectOnComplete is enabled.
	protected void TryDeleteTaskObject(int taskIndex)
	{
		if (!m_bDeleteObjectOnComplete || !m_aSpawnedTaskObjects)
			return;

		if (taskIndex < 0 || taskIndex >= m_aSpawnedTaskObjects.Count())
			return;

		IEntity obj = m_aSpawnedTaskObjects[taskIndex];
		if (!obj)
			return;

		SCR_EntityHelper.DeleteEntityAndChildren(obj);
		m_aSpawnedTaskObjects[taskIndex] = null;
	}

	// Returns true if the local player belongs to the requested notification side.
	// ALL always returns true. NONE always returns false.
	protected bool CanNotifyLocalSide(CRF_EObjectiveNotifySide side)
	{
		if (side == CRF_EObjectiveNotifySide.ALL)
			return true;

		if (side == CRF_EObjectiveNotifySide.NONE)
			return false;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return false;

		Faction localFaction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!localFaction)
			return false;

		string localKey = localFaction.GetFactionKey();

		if (side == CRF_EObjectiveNotifySide.BLUFOR)
			return localKey == "BLUFOR";

		if (side == CRF_EObjectiveNotifySide.OPFOR)
			return localKey == "OPFOR";

		if (side == CRF_EObjectiveNotifySide.INDFOR)
			return localKey == "INDFOR";

		if (side == CRF_EObjectiveNotifySide.CIV)
			return localKey == "CIV";

		return false;
	}

	// Parses notification text and replaces placeholders with actual values.
	// Supported placeholders:
	//   %SIDE% = faction name of the completing side (e.g. "BLUFOR", "OPFOR")
	//   %TASKNAME% = display label of the completed task
	protected string ParseNotificationText(string messageText, CRF_EObjectiveNotifySide completingSide = CRF_EObjectiveNotifySide.ALL, int taskIndex = -1)
	{
		string result = messageText;

		// Replace %SIDE% with faction name
		if (result.Contains("%SIDE%"))
		{
			string sideName = GetSideName(completingSide);
			result.Replace("%SIDE%", sideName);
		}

		// Replace %TASKNAME% with task label
		if (result.Contains("%TASKNAME%") && taskIndex >= 0 && m_aTaskEntries && taskIndex < m_aTaskEntries.Count())
		{
			CRF_TaskCreatorEntry entry = m_aTaskEntries[taskIndex];
			if (entry)
				result.Replace("%TASKNAME%", entry.m_sTaskLabel);
		}

		return result;
	}

	// Returns the display name for a given side/faction.
	protected string GetSideName(CRF_EObjectiveNotifySide side)
	{
		switch (side)
		{
			case CRF_EObjectiveNotifySide.BLUFOR: return "BLUFOR";
			case CRF_EObjectiveNotifySide.OPFOR:  return "OPFOR";
			case CRF_EObjectiveNotifySide.INDFOR: return "INDFOR";
			case CRF_EObjectiveNotifySide.CIV:    return "Civilians";
			case CRF_EObjectiveNotifySide.ALL:    return "All Teams";
		}
		return "Unknown";
	}

	// Removes markers for all tasks flagged true in the given state array.
	protected void RemoveMarkersForStates(array<bool> stateArray)
	{
		if (!stateArray || !m_aTaskEntries)
			return;

		for (int i = 0; i < stateArray.Count(); i++)
		{
			if (!stateArray[i])
				continue;

			if (i >= m_aTaskEntries.Count())
				continue;

			CRF_TaskCreatorEntry entry = m_aTaskEntries[i];
			if (!entry || !entry.m_bCreateMarker)
				continue;

			RemoveTaskMarker(entry);
		}
	}

	//=========================================================================
	// REPLICATION CALLBACKS
	//=========================================================================

	// Fires on all clients (including JIP) when m_aTasksCompleted changes.
	// Removes markers for any newly completed tasks.
	void OnTaskStateChanged()
	{
		RemoveMarkersForStates(m_aTasksCompleted);
	}

	// Fires on all clients when m_iLastNotifiedTaskIndex changes.
	// Checks the local player's faction against the configured notification side
	// and shows a popup if appropriate.
	void OnTaskCompleteNotify()
	{
		int taskIndex = m_iLastNotifiedTaskIndex;

		if (taskIndex < 0 || !m_aTaskEntries || taskIndex >= m_aTaskEntries.Count())
			return;

		CRF_TaskCreatorEntry entry = m_aTaskEntries[taskIndex];
		if (!entry || !entry.m_bSendNotification)
			return;

		CRF_EObjectiveNotifySide notifSide = entry.m_eNotifySide;
		if (!CanNotifyLocalSide(notifSide))
			return;

		// Parse notification text with placeholders — use actual completing side from array
		CRF_EObjectiveNotifySide completingSide = CRF_EObjectiveNotifySide.ALL;
		if (m_aTaskCompletedBySide && m_aTaskCompletedBySide.Count() > taskIndex)
			completingSide = m_aTaskCompletedBySide[taskIndex];
		
		string parsedText = ParseNotificationText(entry.m_sNotificationText, completingSide, taskIndex);

		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.TASK_SUCCEED);
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(parsedText, 5.0);
	}

	// Fires on all clients when m_iLastWinSide changes.
	// Shows the win notification banner for the winning faction.
	void OnSideWinNotify()
	{
		if (m_iLastWinSide < 1 || m_iLastWinSide > 4)
			return;

		CRF_EObjectiveNotifySide side = m_iLastWinSide;
		if (!m_bNotifyOnSideWin || !CanNotifyLocalSide(side))
			return;

		string parsedText = ParseNotificationText(m_sSideWinText, side);
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.TASK_SUCCEED);
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg(parsedText, 5.0);
	}

	//=========================================================================
	// MARKERS
	//=========================================================================

	// Creates a CRF scripted map marker at the spawn anchor's world position.
	protected void CreateTaskMarker(CRF_TaskCreatorEntry entry)
	{
		CRF_PlayerScriptedMarkerManager markerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		IEntity anchorEntity = GetGame().GetWorld().FindEntityByName(entry.m_sSpawnEntityName);
		if (!anchorEntity)
			return;

		vector position = anchorEntity.GetOrigin();
		string posStr = string.Format("%1 %2 %3", position[0], position[1], position[2]);

		markerManager.AddScriptedMarker(
			"Static Marker",
			posStr,
			0,
			entry.m_sMarkerLabel,
			entry.m_sMarkerIcon,
			50,
			GetColorForSide(entry.m_eAssignedSide)
		);
	}

	// Removes the scripted map marker for a completed task.
	// Parameters must match exactly what was passed to AddScriptedMarker.
	protected void RemoveTaskMarker(CRF_TaskCreatorEntry entry)
	{
		CRF_PlayerScriptedMarkerManager markerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		IEntity anchorEntity = GetGame().GetWorld().FindEntityByName(entry.m_sSpawnEntityName);
		if (!anchorEntity)
			return;

		vector position = anchorEntity.GetOrigin();
		string posStr = string.Format("%1 %2 %3", position[0], position[1], position[2]);

		markerManager.RemoveScriptedMarker(
			"Static Marker",
			posStr,
			0,
			entry.m_sMarkerLabel,
			entry.m_sMarkerIcon,
			50,
			GetColorForSide(entry.m_eAssignedSide)
		);
	}

	//=========================================================================
	// ACCESSORS
	//=========================================================================

	// Returns true if the task at taskIndex has been marked complete.
	bool IsTaskComplete(int taskIndex)
	{
		if (taskIndex < 0 || taskIndex >= m_aTasksCompleted.Count())
			return false;
		return m_aTasksCompleted[taskIndex];
	}

	// Returns the number of configured task entries.
	int GetTaskCount()
	{
		if (!m_aTaskEntries)
			return 0;
		return m_aTaskEntries.Count();
	}

	// Returns how many tasks have been completed.
	int GetCompletedTaskCount()
	{
		int count = 0;
		foreach (bool completed : m_aTasksCompleted)
		{
			if (completed)
				count++;
		}
		return count;
	}

	// Returns true when all configured tasks are complete.
	bool AreAllTasksComplete()
	{
		if (!m_aTaskEntries || m_aTaskEntries.Count() == 0)
			return false;
		return GetCompletedTaskCount() >= m_aTaskEntries.Count();
	}

	// Returns the task entry at taskIndex, or null if out of range.
	// Used by CRF_TaskCreatorAction to read the task type for text/logic switching.
	CRF_TaskCreatorEntry GetTaskEntry(int taskIndex)
	{
		if (!m_aTaskEntries || taskIndex < 0 || taskIndex >= m_aTaskEntries.Count())
			return null;
		return m_aTaskEntries[taskIndex];
	}

	// Returns how many tasks are assigned to the given faction.
	// ALL tasks count toward every faction's total — they represent contested
	// objectives that both sides must compete for.
	int GetTaskCountForSide(CRF_EObjectiveNotifySide side)
	{
		if (!m_aTaskEntries)
			return 0;
		int count = 0;
		foreach (CRF_TaskCreatorEntry e : m_aTaskEntries)
		{
			if (e && (e.m_eAssignedSide == side || e.m_eAssignedSide == CRF_EObjectiveNotifySide.ALL))
				count++;
		}
		return count;
	}

	// Returns how many tasks the given faction has completed.
	// Counts any task where the completing side matches — this covers:
	//   - Faction-specific tasks completed by their assigned side
	//   - Cross-faction completions (e.g. defenders defusing an attacker's bomb)
	//   - ALL-assigned tasks completed by anyone (counted for the completing side)
	// NOTE: MAJORITY win condition uses GetTaskCountForSide for the total, which only
	// counts tasks assigned to that faction (or ALL). For cross-faction plant/defuse
	// scenarios, use SET_AMOUNT_PER_SIDE with a known required count instead.
	int GetCompletedTaskCountForSide(CRF_EObjectiveNotifySide side)
	{
		if (!m_aTaskEntries)
			return 0;
		int count = 0;
		for (int i = 0; i < m_aTaskEntries.Count(); i++)
		{
			if (i >= m_aTasksCompleted.Count() || !m_aTasksCompleted[i])
				continue;

			int completedBy = CRF_EObjectiveNotifySide.NONE;
			if (i < m_aTaskCompletedBySide.Count())
				completedBy = m_aTaskCompletedBySide[i];

			// Count if this side completed it directly.
			if (completedBy == side)
				count++;
			// ALL-broadcast completions (completedBy=ALL) count for every faction.
			else if (completedBy == CRF_EObjectiveNotifySide.ALL)
				count++;
		}
		return count;
	}

	// Returns true when all tasks assigned to the given faction (including ALL tasks)
	// are complete and at least one such task exists.
	bool AreAllSideTasksComplete(CRF_EObjectiveNotifySide side)
	{
		int total = GetTaskCountForSide(side);
		if (total == 0)
			return false;
		return GetCompletedTaskCountForSide(side) >= total;
	}

	// Returns the handler for a given task type, or null if not configured.
	// Add a new case when adding a new CRF_EObjectiveTaskType.
	CRF_BaseTaskHandler GetHandlerForType(CRF_EObjectiveTaskType taskType)
	{
		switch (taskType)
		{
			case CRF_EObjectiveTaskType.COLLECT_INTEL:    return m_CollectIntel;
			case CRF_EObjectiveTaskType.PLANT_DEFUSE_BOMB: return m_PlantDefuseBomb;
		}
		return null;
	}

	// Returns the spawned task object entity at the given task index, or null.
	// Used by server-side RPCs that need to locate the task object (e.g. for
	// 3D sound position, state component lookup, explosion effects).
	IEntity GetSpawnedTaskObject(int index)
	{
		if (!m_aSpawnedTaskObjects || index < 0 || index >= m_aSpawnedTaskObjects.Count())
			return null;
		return m_aSpawnedTaskObjects[index];
	}

	// Returns the ARGB marker color for a given faction side.
	// Follows standard BLUFOR/OPFOR/INDFOR color conventions.
	static int GetColorForSide(CRF_EObjectiveNotifySide side)
	{
		switch (side)
		{
			case CRF_EObjectiveNotifySide.BLUFOR: return ARGB(255,  50, 120, 220);
			case CRF_EObjectiveNotifySide.OPFOR:  return ARGB(255, 220,  50,  50);
			case CRF_EObjectiveNotifySide.INDFOR: return ARGB(255,  50, 180,  80);
			case CRF_EObjectiveNotifySide.CIV:    return ARGB(255, 200, 180,  50);
			case CRF_EObjectiveNotifySide.NONE:   return ARGB(255, 130, 130, 130);
		}
		return ARGB(255, 255, 165, 0); // ALL = orange
	}

	// Returns the number of tasks remaining for a faction (given - completed).
	int GetTasksRemainingForSide(CRF_EObjectiveNotifySide side)
	{
		return GetTaskCountForSide(side) - GetCompletedTaskCountForSide(side);
	}

	// Returns which faction completed a specific task, or NONE if not yet completed.
	CRF_EObjectiveNotifySide GetTaskCompletedBySide(int taskIndex)
	{
		if (!m_aTaskCompletedBySide || taskIndex < 0 || taskIndex >= m_aTaskCompletedBySide.Count())
			return CRF_EObjectiveNotifySide.NONE;
		return m_aTaskCompletedBySide[taskIndex];
	}


}
