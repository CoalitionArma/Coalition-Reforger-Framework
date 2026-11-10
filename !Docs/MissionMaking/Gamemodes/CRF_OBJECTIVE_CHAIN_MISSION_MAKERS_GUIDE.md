# CRF Objective Chain Gamemode

The attackers take a list of objectives in order. Each objective is a zone they must control. An objective can also have breach points: wall sections the attackers must blow open with a breaching charge before the zone can be taken. When an objective falls, the next one opens. Each faction's respawn point can move up, and every dead player on both sides respawns.

- **Attackers win** when the last objective falls.
- **Defenders win** when the time limit runs out, or when the attackers are wiped out with no respawns left.

Iron Tide (`Missions/Kolguyev/tanaKa_1109_TVT101_IronTide.conf`) is the reference mission.

## Setup

1. Add `CRF_ObjectiveChainGamemodeComponent` to the `COA_Lobby` gamemode entity.
2. Place a named entity at the centre of every objective zone. A plain `GameEntity` works.
3. For each breach point, place `Prefabs/Systems/Objectives/CRF_BreachWall_4m.et` and give it a unique name. Any prefab carrying `CRF_BreachChargeAction` works too.
4. List the objectives in order on the component.
5. For each objective, set its zone entity name and, if it needs breaching, its breach point names.
6. To move a respawn point when an objective falls, place a named marker entity and add a respawn move for that faction.

## Component attributes

| Setting | Description | Default |
|---|---|---|
| Attacker Faction | Faction key of the attackers. | `BLUFOR` |
| Defender Faction | Faction key of the defenders. | `OPFOR` |
| Objectives | The objectives, in the order they're taken. | — |
| Time Limit Minutes | Minutes from the round going live until the defenders win. 0 means no limit. | `60` |
| End Mission On Victory | Record the winner and advance to the AAR when the round is decided. | on |
| Auto Assign Tasks | Assign every player to their side's open objective task, so the HUD shows it without opening the task list. Late joiners are picked up within 2 s. Each player is assigned once per objective; anyone who unassigns stays unassigned until the next objective opens. | on |
| Breach Fuse Seconds | Seconds from setting a charge to the section blowing. | `10` |
| Breach Reach | Maximum distance (m) from a breach point to set a charge. | `6` |
| Breach Charge Keywords | An inventory item counts as a charge if its prefab path, or a parent prefab's path, contains one of these. | `Explosive_Satchels`, `DemoBlock_M112`, `Bangalore` |
| Breach Explosion | Prefab spawned by the server (replicated) when a section blows. | `Explosion_Tnt_Small.et` |

## Objective attributes

| Setting | Description | Default |
|---|---|---|
| Title / Description | The attackers' task. | — |
| Defender Title / Description | The defenders' task. An empty title gives "Hold: \<title\>". | — |
| Zone Entity Name | Entity at the centre of the zone. | — |
| Radius | Zone radius in metres, measured on the map (height is ignored). | `75` |
| Hold Seconds | Seconds of control needed. | `60` |
| Min Attackers | Conscious attackers needed in the zone. | `2` |
| Max Defenders | Most conscious defenders allowed in the zone while it counts as controlled. | `1` |
| Breach Point Names | Breach points that belong to this objective. | — |
| Breaches Required | Breach points that must be blown before the zone can be taken. | `1` |
| Respawn Moves | Marker entity and faction: that faction's default respawn point moves there when the objective falls. | — |
| Respawn Dead On Complete | When the objective falls, every dead player on both sides respawns (`COA_RespawnManager.RespawnSide`). | on |

The capture progress of a zone behaves as follows:

- **Controlled** (enough attackers in the zone, few enough defenders): progress grows.
- **Contested**: progress holds where it is.
- **Empty of attackers**: progress drains.

AI and players both count. The open objective's task shows the progress, and the pause menu shows it too.

## Breaching

- **Who sees it:** "Place breaching charge" (6 s) appears to attackers on the seaward side (local −Z) of an unbroken breach point that belongs to the open objective.
- **What it needs:** a breaching charge in the player's inventory. The server takes the charge when it's set.
- **What happens:** after the fuse, the server spawns the explosion, and every machine deletes its own copy of that section. Wall sections are plain static entities, so there's no network deletion.
- **Map:** the attackers' map shows the open objective and its unbroken breach points.

There are at most 31 breach points per mission.

Place a breach section in line with the wall sections around it. A blown section only leaves a gap if the wall is otherwise continuous.

## Tickets and vehicles

`RespawnSide` respawns dead players whatever the ticket count, but it **skips a side's vehicles when that side has 0 tickets**. Give the attackers `-1` (unlimited) tickets if their vehicles should come back when an objective falls. With timed respawn off, tickets change nothing else.

## Implementation

- [`CRF_ObjectiveChainGamemodeComponent.c`](../../../Scripts/Game/GameMode/ObjectiveChain/CRF_ObjectiveChainGamemodeComponent.c): objectives, capture, breaching, tasks, end of round.
- [`CRF_BreachChargeAction.c`](../../../Scripts/Game/GameMode/ObjectiveChain/CRF_BreachChargeAction.c): the user action on breach points.
- [`CRF_ObjectiveChainRelay.c`](../../../Scripts/Game/GameMode/ObjectiveChain/CRF_ObjectiveChainRelay.c): the client-to-server request (`COA_PlayerRplToAuthorityManager`).
- [`CRF_BreachWall_4m.et`](../../../Prefabs/Systems/Objectives/CRF_BreachWall_4m.et): the 4 m retaining wall section with the action.
