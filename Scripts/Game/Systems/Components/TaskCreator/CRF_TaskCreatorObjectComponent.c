//=============================================================================
// CRF_TaskCreatorObjectComponent.c
// Data carrier ScriptComponent for CRF task object prefabs.
//
// Place this component on every task object prefab alongside
// CRF_TaskCreatorAction. At spawn time, CRF_TaskCreatorComponent calls
// SetTaskData() to inject the task index and type. The action then reads
// from this component via FindComponent — same-entity lookup, no timing or
// name-matching issues.
//
// m_iTaskIndex and m_eTaskType are [RplProp] so clients receive the correct
// values for action text display and RPC construction.
//
// m_iTaskObjectState is an agnostic per-object state slot. Its meaning is
// owned entirely by the handler — cast to the handler's state enum to interpret
// the value. Use SetTaskObjectState() (server only) to transition; the handler
// is notified server-side via OnObjectStateChangedServer() and all clients
// receive OnObjectStateReplicated() via the [RplProp] callback.
//
// StartCountdown(float seconds) starts a server-side timer. When it expires,
// OnObjectCountdownExpired() is called on the handler (e.g. for explosions).
// CancelCountdown() cancels any pending timer.
// =============================================================================

[ComponentEditorProps(category: "GameScripted/ObjectiveTasks", description: "Data carrier for CRF task object prefabs. Populated at spawn by CRF_TaskCreatorComponent. Required alongside CRF_TaskCreatorAction.")]
class CRF_TaskCreatorObjectComponentClass : ScriptComponentClass {}

class CRF_TaskCreatorObjectComponent : ScriptComponent
{
	// Injected by CRF_TaskCreatorComponent.SpawnTaskObject after entity spawn.
	// RplProp ensures clients receive the values before the first HUD frame.
	[RplProp()]
	protected int m_iTaskIndex = -1;

	[RplProp()]
	protected CRF_EObjectiveTaskType m_eTaskType;

	// Agnostic per-object state. Integer value is owned by the handler;
	// cast to the handler's enum to interpret. -1 = not yet initialised.
	// OnTaskObjectStateReplicated fires on all clients when this value changes.
	[RplProp(onRplName: "OnTaskObjectStateChanged")]
	protected int m_iTaskObjectState = -1;

	// Tracks which faction planted the bomb (for PLANT_DEFUSE_BOMB tasks).
	// Set to CRF_EObjectiveNotifySide value when bomb transitions to PLANTED.
	// Used to credit the correct faction when the bomb explodes.
	[RplProp()]
	protected int m_iPlanterSide = 0;

	// Cached reference to the task system — for IsComplete() checks.
	protected CRF_TaskCreatorComponent m_TaskSystem;

	//=========================================================================
	// LIFECYCLE
	//=========================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		IEntity gameMode = GetGame().GetGameMode();
		if (gameMode)
			m_TaskSystem = CRF_TaskCreatorComponent.Cast(gameMode.FindComponent(CRF_TaskCreatorComponent));
	}

	//=========================================================================
	// SERVER INJECTION
	//=========================================================================

	// Called by CRF_TaskCreatorComponent.SpawnTaskObject immediately after
	// SpawnEntityPrefab returns. Server-side only.
	void SetTaskData(int taskIndex, CRF_EObjectiveTaskType taskType)
	{
		m_iTaskIndex = taskIndex;
		m_eTaskType = taskType;
		m_iTaskObjectState = 0;  // Initialize to the first/default state (e.g. UNPLANTED for bomb tasks).
		Replication.BumpMe();
	}

	//=========================================================================
	// ACCESSORS
	//=========================================================================

	int GetTaskIndex()
	{
		return m_iTaskIndex;
	}

	CRF_EObjectiveTaskType GetTaskType()
	{
		return m_eTaskType;
	}

	// Returns true if the task linked to this object has been completed.
	// Safe to call on client — reads from replicated m_iTaskIndex.
	bool IsComplete()
	{
		if (!m_TaskSystem || m_iTaskIndex < 0)
			return false;

		return m_TaskSystem.IsTaskComplete(m_iTaskIndex);
	}

	//=========================================================================
	// TASK OBJECT STATE
	//=========================================================================

	// Sets the agnostic per-object state. Server only.
	// Replicates to all clients (fires OnTaskObjectStateReplicated) and
	// immediately notifies the handler server-side via OnObjectStateChangedServer.
	void SetTaskObjectState(int newState)
	{
		if (!Replication.IsServer())
			return;

		m_iTaskObjectState = newState;
		Replication.BumpMe();

		if (!m_TaskSystem || m_iTaskIndex < 0)
			return;

		CRF_BaseTaskHandler handler = m_TaskSystem.GetHandlerForType(m_eTaskType);
		if (handler)
			handler.OnObjectStateChangedServer(newState, m_iTaskIndex, this);
	}

	int GetTaskObjectState()
	{
		return m_iTaskObjectState;
	}

	int GetPlanterSide()
	{
		return m_iPlanterSide;
	}

	void SetPlanterSide(int side)
	{
		if (!Replication.IsServer())
			return;
		m_iPlanterSide = side;
		Replication.BumpMe();
	}

	// Fires on ALL clients (including server) when m_iTaskObjectState replicates.
	protected void OnTaskObjectStateChanged()
	{
		if (!m_TaskSystem || m_iTaskIndex < 0)
			return;

		CRF_BaseTaskHandler handler = m_TaskSystem.GetHandlerForType(m_eTaskType);
		if (handler)
			handler.OnObjectStateReplicated(m_iTaskObjectState, GetOwner());
	}

	//=========================================================================
	// SERVER COUNTDOWN
	//=========================================================================

	// Starts a server-side countdown timer. When duration seconds elapse,
	// OnObjectCountdownExpired() is called on the active handler.
	// Only valid server-side. Safe to call multiple times — each call
	// replaces the previous pending callback.
	void StartCountdown(float duration)
	{
		if (!Replication.IsServer())
			return;

		// Cancel any previous pending timer before scheduling a new one.
		GetGame().GetCallqueue().Remove(OnCountdownExpiredCallback);
		GetGame().GetCallqueue().CallLater(OnCountdownExpiredCallback, duration * 1000, false);
	}

	// Cancels a pending countdown. No-op if no countdown is running.
	void CancelCountdown()
	{
		if (!Replication.IsServer())
			return;

		GetGame().GetCallqueue().Remove(OnCountdownExpiredCallback);
	}

	// Internal. Called by the CallQueue when the countdown expires.
	protected void OnCountdownExpiredCallback()
	{
		if (!m_TaskSystem || m_iTaskIndex < 0)
			return;

		CRF_BaseTaskHandler handler = m_TaskSystem.GetHandlerForType(m_eTaskType);
		if (handler)
			handler.OnObjectCountdownExpired(m_iTaskIndex, this);
	}
}
