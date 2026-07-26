# CRF Task Creator

Spawns prefabs at named world entities, tracks per-task completion per faction, fires notifications, and manages map markers. Can be added to any gamemode.

---

## For Mission Makers

### 1. Add the Component

Add `CRF_TaskCreatorComponent` as a script component to your gamemode entity (e.g. `COA_Gamemode`).

### 2. Configure Task Type Handlers

Under **Task Type Handlers**, expand the handler for each task type you intend to use.

| Handler | Use for |
|---|---|
| `m_CollectIntel` | Single-interaction pickup/collect tasks |
| `m_PlantDefuseBomb` | Timed plant + defuse mechanics |

Set the **Prefab** inside the handler to the `.et` task object to spawn. For `PlantDefuseBomb`, also configure timings, sounds, and explosion prefabs here. The prefab and settings are defined once per handler, not per entry.

### 3. Add Task Entries

Under **Objective Tasks**, add one entry per objective to `m_aTaskEntries`.

| Field | Description |
|---|---|
| **Task Label** | Editor-only display name |
| **Task Type** | Must match the configured handler |
| **Assigned Side** | Faction that can interact (`ALL`, `BLUFOR`, `OPFOR`, etc.) |
| **Spawn Entity Name** | Exact name of the anchor entity placed in the world (case-sensitive) |
| **Notification** | Toggle, target faction, and message text (`%TASKNAME%` replaced at runtime) |
| **Map Marker** | Toggle, label text, and icon |

### 4. Place Spawn Anchors

For each entry, place a **`CRF_Task_Empty.et`** prefab in the world and name it to match `m_sSpawnEntityName`. This prefab is a pre-configured empty object with `CRF_TaskCreatorPreviewComponent` attached — it will display the task mesh in the Workbench at edit time so you can visually position anchors.

### 5. Win Condition *(optional)*

Under **Win Condition**:

- `NONE` — no automatic win; use task tracking only
- `SET_AMOUNT_PER_SIDE` — each faction wins when it completes `m_iTasksRequiredForWin` tasks

### 6. Build a Custom Task Prefab *(optional)*

Only needed if you are not using a prefab provided by the framework:

1. Add `CRF_TaskCreatorObjectComponent` — no configuration needed, it is populated at runtime.
2. Add `ActionManagerComponent` if not already present.
3. Inside it, add a `CRF_TaskCreatorAction` entry and set its **Action Hold Time**.
4. The task type and index are injected at spawn time — do not set them manually.

---

## For Devs

### Architecture

```
CRF_TaskCreatorComponent          — spawns prefabs, tracks completion, fires RPCs
  └─ CRF_TaskCreatorEntry[]       — one per objective (data only, no logic)
  └─ CRF_BaseTaskHandler (x N)    — one per task type, holds all type-specific logic

CRF_TaskCreatorObjectComponent    — lives on each spawned task prefab
                                    carries taskIndex, taskType, taskObjectState (all RplProp)

CRF_TaskCreatorAction             — ScriptedUserAction on each spawned task prefab
                                    delegates to the handler via ResolveHandler()
```

Core components never reference concrete handler types — adding a new task type requires no changes to them.

---

### Adding a New Task Type

**Step 1 — Add enum value** in `CRF_TaskCreatorComponent.c`:
```c
enum CRF_EObjectiveTaskType { COLLECT_INTEL, PLANT_DEFUSE_BOMB, YOUR_NEW_TYPE }
```

**Step 2 — Create handler** at `Scripts/Game/Systems/Components/TaskCreator/Handlers/CRF_TaskHandler_YourType.c`:
```c
[BaseContainerProps()]
class CRF_TaskHandler_YourType : CRF_BaseTaskHandler
{
    [Attribute("", UIWidgets.ResourceNamePicker, "Task object prefab.", params: "et", category: "Your Type")]
    ResourceName m_sPrefab;

    override string GetActionName(int taskObjectState) { return "Your Action"; }
    override ResourceName GetPrefab() { return m_sPrefab; }

    override void OnPerform(int taskIndex, int taskObjectState, IEntity user)
    {
        COA_PlayerRplToAuthorityManager rpl = COA_PlayerRplToAuthorityManager.GetInstance();
        if (rpl) rpl.RequestObjectiveTaskComplete(taskIndex, GetUserSide(user));
    }
}
```
See `CRF_TaskHandler_CollectIntel.c` for the simplest full example, `CRF_TaskHandler_PlantDefuseBomb.c` for a stateful multi-phase example.

**Step 3 — Register in `CRF_TaskCreatorComponent`**, both as a field and in `GetHandlerForType()`:
```c
[Attribute(desc: "Handler for YOUR_NEW_TYPE tasks.", category: "Task Type Handlers")]
ref CRF_TaskHandler_YourType m_YourNewType;

// in GetHandlerForType():
case CRF_EObjectiveTaskType.YOUR_NEW_TYPE: return m_YourNewType;
```

**Step 4 — Build the task prefab** — same requirements as mission maker step 6 above.

---

### Virtual Method Reference

All methods receive `taskObjectState` as a raw `int`. Cast to your handler's state enum to interpret it.

| Method | Caller | Purpose Examples |
|---|---|---|
| `GetActionName(state)` | Client, every HUD frame | Return the interaction label. Branch on state for multi-phase tasks. |
| `GetPrefab()` | Server, spawn time | Return the `.et` prefab resource. |
| `OnPerform(taskIndex, state, user)` | Client, on action complete | Send server RPCs. |
| `CanBeShownExtra(taskIndex, state, user)` | Client, every HUD frame | Return `false` to hide the action. Default `true`. |
| `CanPerformExtra(taskIndex, state, user)` | Client, every HUD frame | Return `false` to grey out the action. Default checks `m_eAssignedSide`. |
| `OnActionStart(taskIndex, state, taskObject, user)` | Client, on hold begin | Start positional sounds. |
| `OnActionCanceled(taskIndex, state, taskObject, user)` | Client, on hold cancel | Stop sounds started in `OnActionStart`. |
| `OnObjectStateReplicated(newState, taskObject)` | All clients | React to state replication — visuals, sounds. |
| `OnObjectStateChangedServer(newState, taskIndex, objectComp)` | Server only | Start/stop countdowns, broadcast 3D sounds. |
| `OnObjectCountdownExpired(taskIndex, objectComp)` | Server only | Fired when `StartCountdown()` elapses. |

---

### State Machine Pattern

For multi-phase tasks (e.g. plant/defuse):

1. Define a local enum (e.g. `CRF_EBombTaskState`) with integer values starting at `0`.
2. Call `objectComp.SetTaskObjectState(newState)` server-side to transition.
3. `OnObjectStateChangedServer` fires immediately on the server; `OnObjectStateReplicated` fires on all clients when the RplProp replicates.
4. Branch every handler method on `taskObjectState` — no separate action slots needed.

---

### Sound Pattern

For 3D positional sounds tied to the task object:

- **Client-initiated** (planting/defusing hold sounds):
  - Start: `rpl.RequestPlayPositionalSound(resource, event, taskObject.GetOrigin())`
  - Stop: `rpl.RequestStopPositionalSound(event)`
  - The server re-broadcasts to all clients via `COA_RplBroadcastManager`.

- **Server-initiated** (looping tick sounds on state change):
  - Start: `broadcast.PlayPositionalSound(resource, event, bombEntity.GetOrigin())`
  - Stop: `broadcast.StopPositionalSound(event)`
  - Call directly in `OnObjectStateChangedServer` — no client RPC needed.

One handle is stored per event name in `COA_RplBroadcastManager`. Only one sound per event can be active at a time.
