//=============================================================================
// CRF_BaseTaskHandler.c
// Abstract base class for CRF task type handlers.
//
// Each task type implements a subclass that encapsulates all type-specific
// behaviour: the prefab to spawn, the HUD action label, the perform logic,
// and any extra CanPerform conditions.
//
// The core components (TaskCreatorComponent, TaskCreatorAction,
// TaskCreatorObjectComponent, TaskCreatorPreviewComponent) reference only
// CRF_BaseTaskHandler — they never know the concrete type and never need
// to be modified when new types are added.
//
// =============================================================================
// HOW TO ADD A NEW TASK TYPE
// =============================================================================
// 1. Add a new value to CRF_EObjectiveTaskType  (CRF_TaskCreatorComponent.c)
// 2. Create CRF_TaskHandler_<YourType>.c        (new file, subclass below, this is essentially your objectcentric house for play logic, is only scripteduseraction oriented)
// 3. Add ref + one case to GetHandlerForType()  (CRF_TaskCreatorComponent.c)
// 4. Build the task prefab with
//      CRF_TaskCreatorPreviewComponent + CRF_TaskCreatorObjectComponent + ActionManager->CRF_TaskCreatorAction (Or see CRF_TaskHandler_CollectIntel.c for an example.)
// =============================================================================
// VIRTUAL METHOD REFERENCE
// =============================================================================
// GetActionName(int taskObjectState)
//   HUD label. Branch on taskObjectState to show different text per state
//   (e.g. "Plant Bomb" when UNPLANTED, "Defuse Bomb" when PLANTED).
//
// GetPrefab()
//   Prefab resource spawned at the task anchor.
//
// OnPerform(int taskIndex, int taskObjectState, IEntity user)
//   Client — hold action completed. Send server RPCs here.
//
// CanBeShownExtra(int taskIndex, int taskObjectState, IEntity user)
//   Client — controls action visibility (return false to hide from HUD entirely).
//   Default returns true. Override to hide based on state or faction
//   (e.g. hide "Defuse Bomb" when the bomb is not yet planted).
//
// CanPerformExtra(int taskIndex, int taskObjectState, IEntity user)
//   Client — optional extra gate after completion check. Return false to gray out.
//   taskObjectState is read from the action's direct component reference — valid
//   on all clients including dedicated server clients.
//
// OnActionStart(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
//   Client — player begins holding the action. Use for planting/defusing sounds.
//
// OnActionCanceled(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
//   Client — player releases before completion. Use to stop hold sounds.
//
// OnObjectStateReplicated(int newState, IEntity taskObject)
//   All clients — fired when m_iTaskObjectState replicates. Use for client-
//   side reactions: start/stop looping sounds, swap visuals, etc.
//
// OnObjectStateChangedServer(int newState, int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
//   Server only — fired immediately when SetTaskObjectState() is called.
//   Use to: start/stop countdowns, broadcast 3D sounds to all clients.
//
// OnObjectCountdownExpired(int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
//   Server only — fired when a countdown started via StartCountdown() expires.
//   Use for: explosion effects, auto-complete/fail, final state transitions.
// =============================================================================

// Base handler — defines the interface every task type must satisfy.
// Safe to call any method on a null-checked handler reference; all defaults
// return sensible no-op values.
[BaseContainerProps()]
class CRF_BaseTaskHandler
{
	// ==========================================================================
	// VIRTUAL METHODS — Core Interface
	// ==========================================================================
	// Subclasses override these to define type-specific behaviour.

	// Returns the interaction HUD label for the given task object state.
	// taskObjectState is the raw int from CRF_TaskCreatorObjectComponent;
	// cast to your handler's state enum to interpret it.
	string GetActionName(int taskObjectState)
	{
		return "Interact";
	}

	// Returns the prefab resource that CRF_TaskCreatorComponent spawns at the
	// task anchor for this type.
	ResourceName GetPrefab()
	{
		return ResourceName.Empty;
	}

	// Called on the performing client when the hold action completes.
	// taskObjectState is the current state of the task object at the time of completion,
	// read from the direct component reference on the action (valid on clients).
	// Send server RPCs here (e.g. RequestObjectiveTaskComplete, RequestTaskObjectSetState).
	void OnPerform(int taskIndex, int taskObjectState, IEntity user)
	{
	}

	// Controls whether the action appears in the interaction menu at all.
	// Returns true by default. Override to hide based on state or faction
	// (ex hide "Defuse Bomb" when the bomb is not yet planted).
	bool CanBeShownExtra(int taskIndex, int taskObjectState, IEntity user)
	{
		return true;
	}

	// Optional extra condition evaluated after the completion check.
	// Return false to gray out the action. taskObjectState is passed from the action's
	// direct m_TaskObjectComp reference, which is valid on all clients.
	//
	// Default implementation enforces m_eAssignedSide restriction.
	bool CanPerformExtra(int taskIndex, int taskObjectState, IEntity user)
	{
		CRF_TaskCreatorComponent taskSystem = CRF_TaskCreatorComponent.GetInstance();
		if (!taskSystem)
			return true;

		CRF_TaskCreatorEntry entry = taskSystem.GetTaskEntry(taskIndex);
		if (!entry || entry.m_eAssignedSide == CRF_EObjectiveNotifySide.ALL)
			return true;

		if (entry.m_eAssignedSide == CRF_EObjectiveNotifySide.NONE)
			return false;

		return (GetUserSide(user) == entry.m_eAssignedSide);
	}

	// Called on the performing client when the player begins holding the action.
	// taskObject is the entity the action lives on; user is the interacting character.
	// Use to start 3D planting/defusing sounds via RequestTaskObjectPlaySound RPC.
	void OnActionStart(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
	{
	}

	// Called on the performing client when the hold is released before completion.
	// Use to stop 3D planting/defusing sounds via RequestTaskObjectStopSound RPC.
	void OnActionCanceled(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
	{
	}

	// Called on ALL clients when m_iTaskObjectState on the object component replicates.
	// Use for client-side reactions to state transitions (start/stop looping sounds,
	// swap visual state, etc.).
	void OnObjectStateReplicated(int newState, IEntity taskObject)
	{
	}

	// Called SERVER-ONLY immediately when SetTaskObjectState() is invoked.
	// Use to start/stop countdowns and broadcast 3D sounds to all clients
	// in response to a state change.
	void OnObjectStateChangedServer(int newState, int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
	{
	}

	// Called SERVER-ONLY when a countdown started via StartCountdown() expires.
	// Use for explosion effects, auto-complete/fail logic, and final state transitions.
	void OnObjectCountdownExpired(int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
	{
	}

	// ==========================================================================
	// UTILITIES — Shared Helper Methods
	// ==========================================================================
	// Call these from OnPerform or other handlers to extract common task data.

	// Extracts the user's affiliated faction and returns the corresponding
	// CRF_EObjectiveNotifySide enum value for RPC notification routing.
	// Returns ALL (0) if the entity or component is null.
	//
	// Usage:
	//   int side = GetUserSide(user);
	//   rpl.RequestObjectiveTaskComplete(taskIndex, side);
	int GetUserSide(IEntity user)
	{
		if (!user)
			return CRF_EObjectiveNotifySide.ALL;

		FactionAffiliationComponent userAff = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (!userAff)
			return CRF_EObjectiveNotifySide.ALL;

		Faction userFaction = userAff.GetAffiliatedFaction();
		if (!userFaction)
			return CRF_EObjectiveNotifySide.ALL;

		string factionKey = userFaction.GetFactionKey();
		if (factionKey == "BLUFOR")
			return CRF_EObjectiveNotifySide.BLUFOR;
		else if (factionKey == "OPFOR")
			return CRF_EObjectiveNotifySide.OPFOR;
		else if (factionKey == "INDFOR")
			return CRF_EObjectiveNotifySide.INDFOR;
		else if (factionKey == "CIV")
			return CRF_EObjectiveNotifySide.CIV;

		return CRF_EObjectiveNotifySide.ALL;
	}
}
