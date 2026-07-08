//=============================================================================
// CRF_TaskCreatorAction.c
// Agnostic ScriptedUserAction for CRF task prefabs.
//
// Delegates all type-specific behaviour (action texts, action logic, availability conditions) to the CRF_BaseTaskHandler subclass registered for
// the current task type in CRF_TaskCreatorComponent.
//
// To add a new task type, create a CRF_TaskHandler_<Type> subclass.
// This file never needs to change.
// =============================================================================

class CRF_TaskCreatorAction : ScriptedUserAction
{
	// Same-entity component reference — populated in Init, always valid.
	protected CRF_TaskCreatorObjectComponent m_TaskObjectComp;

	// Cached handler
	protected CRF_BaseTaskHandler m_Handler;

	//=========================================================================
	// INIT
	//=========================================================================

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		if (!GetGame().InPlayMode())
			return;

		// Direct same-entity lookup 
		m_TaskObjectComp = CRF_TaskCreatorObjectComponent.Cast(pOwnerEntity.FindComponent(CRF_TaskCreatorObjectComponent));

		if (!m_TaskObjectComp)
			Print(string.Format("[CRF_TaskCreatorAction] No CRF_TaskCreatorObjectComponent found on '%1' — add it to the task prefab.", pOwnerEntity.GetName()), LogLevel.WARNING);
	}

	//=========================================================================
	// ACTION TEXT
	//=========================================================================

	// Delegates to the handler for the current task type.
	override bool GetActionNameScript(out string outName)
	{
		int state = -1;
		if (m_TaskObjectComp)
			state = m_TaskObjectComp.GetTaskObjectState();
		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler)
			outName = handler.GetActionName(state);
		else
			outName = "Interact";
		return true;
	}

	//=========================================================================
	// ACTION BEHAVIOUR
	//=========================================================================

	// Without these flags, PerformAction fires on both client and server, causing
	// double RPC calls.
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	// Dispatches to the handler for the current task type, then calls super.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_TaskObjectComp)
			return;

		int taskIndex = m_TaskObjectComp.GetTaskIndex();
		if (taskIndex < 0)
			return;  // RplProp not yet received — ignore premature interaction

		// m_TaskObjectComp is always valid on clients; GetSpawnedTaskObject() is server-only.
		int state = m_TaskObjectComp.GetTaskObjectState();
		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler)
			handler.OnPerform(taskIndex, state, pUserEntity);

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	// Notifies the handler when the player begins holding the action so it can
	// start positional sounds.
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);
		if (!m_TaskObjectComp)
			return;

		int taskIndex = m_TaskObjectComp.GetTaskIndex();
		if (taskIndex < 0)
			return;

		int state = m_TaskObjectComp.GetTaskObjectState();
		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler)
			handler.OnActionStart(taskIndex, state, GetOwner(), pUserEntity);
	}

	// Notifies the handler when the player releases before completing so it can
	// stop any in-progress sounds started in OnActionStart.
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);
		if (!m_TaskObjectComp)
			return;

		int taskIndex = m_TaskObjectComp.GetTaskIndex();
		if (taskIndex < 0)
			return;

		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler)
			handler.OnActionCanceled(taskIndex, m_TaskObjectComp.GetTaskObjectState(), pOwnerEntity, pUserEntity);
	}

	//=========================================================================
	// HANDLER RESOLUTION
	//=========================================================================

	// Returns the CRF_BaseTaskHandler for the current task type, or null.
	// Result is cached after the first successful resolve — GetInstance() and
	// GetHandlerForType() are not called again on subsequent frames.
	protected CRF_BaseTaskHandler ResolveHandler()
	{
		if (m_Handler)
			return m_Handler;
		if (!m_TaskObjectComp)
			return null;
		CRF_TaskCreatorComponent taskSystem = CRF_TaskCreatorComponent.GetInstance();
		if (!taskSystem)
			return null;
		m_Handler = taskSystem.GetHandlerForType(m_TaskObjectComp.GetTaskType());
		return m_Handler;
	}

	//=========================================================================
	// VISIBILITY / AVAILABILITY
	//=========================================================================

	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_TaskObjectComp)
			return true;  // Fail open — component missing means misconfigured prefab

		int taskIndex = m_TaskObjectComp.GetTaskIndex();
		if (taskIndex < 0)
			return true;  // RplProp not yet received — fail open until data arrives

		if (m_TaskObjectComp.IsComplete())
			return false;

		// Delegates to the handler — covers faction gates and any custom per-state rules.
		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler && !handler.CanPerformExtra(taskIndex, m_TaskObjectComp.GetTaskObjectState(), user))
			return false;

		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (!m_TaskObjectComp)
			return false;

		int taskIndex = m_TaskObjectComp.GetTaskIndex();
		if (taskIndex < 0)
			return false;

		if (m_TaskObjectComp.IsComplete())
			return false;

		// Delegates visibility to the handler. Handlers can hide the action
		// based on state (e.g. hide "Defuse Bomb" until the bomb is planted).
		CRF_BaseTaskHandler handler = ResolveHandler();
		if (handler && !handler.CanBeShownExtra(taskIndex, m_TaskObjectComp.GetTaskObjectState(), user))
			return false;

		return true;
	}
}
