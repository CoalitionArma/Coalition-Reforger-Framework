# CRF Task Creator

A generic objective task system. Spawns prefabs at named world entities, tracks per-task completion per faction, fires notifications, and manages map markers. Can be added to any gamemode.

---

## For Mission Makers

### 1. Add the Component

1. Open your gamemode entity (e.g. `CRF_Gamemode`) in the Workbench.
2. Add `CRF_TaskCreatorComponent` as a script component.

---

### 2. Configure Task Type Handlers

Under **Task Type Handlers**, expand the handler for each task type you intend to use.

| Handler | Use for |
|---|---|
| `m_CollectIntel` | Single-interaction pickup/collect tasks |
| `m_PlantDefuseBomb` | Timed plant + defuse mechanics |

Set the **Prefab** inside the handler to the `.et` task object you want spawned. For `PlantDefuseBomb`, also configure timings, sounds, and explosion prefabs here.

> Each handler is shared across all entries of that type. The prefab and settings are defined once in the handler, not per-entry.

---

### 3. Add Task Entries

Under **Objective Tasks**, add one entry per objective to `m_aTaskEntries`.

Each entry has:

- **Task Label** — editor-only display name
- **Task Type** — must match the handler you configured (e.g. `PLANT_DEFUSE_BOMB`)
- **Assigned Side** — faction that can interact (`ALL`, `BLUFOR`, `OPFOR`, etc.)
- **Spawn Entity Name** — exact name of the empty `GenericEntity` placed in the world where the prefab will spawn (case-sensitive)
- **Notification** — toggle, target faction, and message text (`%TASKNAME%` is replaced at runtime)
- **Map Marker** — toggle, label text, and icon

---

### 4. Place Spawn Entities

For each entry, place an empty `GenericEntity` in the world and name it to match `m_sSpawnEntityName`. The task prefab spawns at that entity's world position.

---

### 5. Win Condition (Optional)

Under **Win Condition**:

- `NONE` — no automatic win; use task tracking only
- `SET_AMOUNT_PER_SIDE` — each faction wins when it completes `m_iTasksRequiredForWin` tasks

---

### 6. Build Your Task Prefab

If you are using a custom object (not one provided by the framework):

1. Open your object prefab in the Workbench.
2. Add `CRF_TaskCreatorObjectComponent` as a script component. No configuration needed — it is populated at runtime.
3. Add an `ActionManagerComponent` if not already present.
4. Inside `ActionManagerComponent`, add a `CRF_TaskCreatorAction` entry.
   - Set the **Action Hold Time** (Workbench attribute on the action) to the desired hold duration.
   - No other configuration on the action is required; it reads everything from `CRF_TaskCreatorObjectComponent` at runtime.
5. Optionally add `CRF_TaskCreatorPreviewComponent` to show an interaction preview marker.

> The task type and index are injected into the prefab at spawn time by `CRF_TaskCreatorComponent`. You do not set them manually on the prefab.

---

## For Devs

### Architecture Overview

```
CRF_TaskCreatorComponent          — spawns prefabs, tracks completion, fires RPCs
  └─ CRF_TaskCreatorEntry[]       — one per objective (data only, no logic)
  └─ CRF_BaseTaskHandler (x N)    — one per task type, holds all type-specific logic

CRF_TaskCreatorObjectComponent    — lives on each spawned task prefab
                                    carries taskIndex, taskType, taskObjectState (all RplProp)

CRF_TaskCreatorAction             — ScriptedUserAction on each spawned task prefab
                                    delegates everything to the handler via ResolveHandler()
```

The core components (`TaskCreatorComponent`, `TaskCreatorAction`, `TaskCreatorObjectComponent`) never reference concrete handler types. Adding a new task type does not require modifying them.

---

### How to Add a New Task Type

#### Step 1 — Register the enum value

In `CRF_TaskCreatorComponent.c`, add a value to `CRF_EObjectiveTaskType`:

```c
enum CRF_EObjectiveTaskType
{
    COLLECT_INTEL,
    PLANT_DEFUSE_BOMB,
    YOUR_NEW_TYPE       // add here
}
```

#### Step 2 — Create the handler file

Create `Scripts/Game/Systems/Components/TaskCreator/Handlers/CRF_TaskHandler_YourType.c`.

Subclass `CRF_BaseTaskHandler`. Override only the methods you need; all base defaults are safe no-ops.

Minimal example (single-interaction, no state):

```c
[BaseContainerProps()]
class CRF_TaskHandler_YourType : CRF_BaseTaskHandler
{
    [Attribute("", UIWidgets.ResourceNamePicker, "Task object prefab.", params: "et", category: "Your Type")]
    ResourceName m_sPrefab;

    override string GetActionName(int taskObjectState)
    {
        return "Your Action";
    }

    override ResourceName GetPrefab()
    {
        return m_sPrefab;
    }

    override void OnPerform(int taskIndex, int taskObjectState, IEntity user)
    {
        CRF_PlayerRplToAuthorityManager rpl = CRF_PlayerRplToAuthorityManager.GetInstance();
        if (!rpl)
            return;
        rpl.RequestObjectiveTaskComplete(taskIndex, GetUserSide(user));
    }
}
```

See `CRF_TaskHandler_CollectIntel.c` for the simplest full example.  
See `CRF_TaskHandler_PlantDefuseBomb.c` for a stateful multi-phase example.

#### Step 3 — Register the handler in TaskCreatorComponent

In `CRF_TaskCreatorComponent.c`, add a field and a case in `GetHandlerForType()`:

```c
// In the attribute section:
[Attribute(desc: "Handler for YOUR_NEW_TYPE tasks.", category: "Task Type Handlers")]
ref CRF_TaskHandler_YourType m_YourNewType;

// In GetHandlerForType():
case CRF_EObjectiveTaskType.YOUR_NEW_TYPE:
    return m_YourNewType;
```

#### Step 4 — Build the task prefab

Same as the mission maker prefab steps above. The only requirement is:
- `CRF_TaskCreatorObjectComponent` on the prefab
- `CRF_TaskCreatorAction` in `ActionManagerComponent`

---

### Virtual Method Reference

All methods receive `taskObjectState` as a raw `int`. Cast to your handler's state enum to interpret it.

| Method | Caller | Purpose |
|---|---|---|
| `GetActionName(state)` | Client, every HUD frame | Return the interaction label. Branch on state for multi-phase tasks. |
| `GetPrefab()` | Server, spawn time | Return the `.et` prefab resource. |
| `OnPerform(taskIndex, state, user)` | Client, on action complete | Send server RPCs — `RequestObjectiveTaskComplete`, `RequestTaskObjectSetState`, etc. |
| `CanBeShownExtra(taskIndex, state, user)` | Client, every HUD frame | Return `false` to hide the action entirely. Default returns `true`. |
| `CanPerformExtra(taskIndex, state, user)` | Client, every HUD frame | Return `false` to gray out the action. Default checks `m_eAssignedSide`. |
| `OnActionStart(taskIndex, state, taskObject, user)` | Client, on hold begin | Start positional sounds via `RequestPlayPositionalSound`. |
| `OnActionCanceled(taskIndex, state, taskObject, user)` | Client, on hold cancel | Stop sounds started in `OnActionStart`. |
| `OnObjectStateReplicated(newState, taskObject)` | All clients | React to state replication — client-side visuals, sounds. |
| `OnObjectStateChangedServer(newState, taskIndex, objectComp)` | Server only | Start/stop countdowns, broadcast 3D sounds via `CRF_RplBroadcastManager`. |
| `OnObjectCountdownExpired(taskIndex, objectComp)` | Server only | Fired when `StartCountdown()` elapses — trigger explosions, auto-complete. |

---

### State Machine Pattern

For multi-phase tasks (e.g. plant/defuse), use `m_iTaskObjectState` as a state machine slot:

1. Define a local enum (e.g. `CRF_EBombTaskState`) with integer values starting at `0`.
2. Call `objectComp.SetTaskObjectState(newState)` server-side to transition.
3. `OnObjectStateChangedServer` fires immediately on the server.
4. `OnObjectStateReplicated` fires on all clients when the RplProp replicates.
5. Branch every handler method on `taskObjectState` — no separate action slots needed.

---

### Sound Pattern

For 3D positional sounds tied to the task object:

- **Client-initiated** (planting/defusing hold sounds):
  - Start: `rpl.RequestPlayPositionalSound(resource, event, taskObject.GetOrigin())`
  - Stop: `rpl.RequestStopPositionalSound(event)`
  - The server re-broadcasts to all clients via `CRF_RplBroadcastManager`.

- **Server-initiated** (looping tick sounds on state change):
  - Start: `broadcast.PlayPositionalSound(resource, event, bombEntity.GetOrigin())`
  - Stop: `broadcast.StopPositionalSound(event)`
  - Call directly in `OnObjectStateChangedServer` — no client RPC needed.

One handle is stored per event name in `CRF_RplBroadcastManager`. Only one sound per event can be active at a time.
