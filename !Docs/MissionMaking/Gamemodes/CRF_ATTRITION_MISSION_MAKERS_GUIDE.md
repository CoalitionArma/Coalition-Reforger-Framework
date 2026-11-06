# CRF Attrition Gamemode

Attrition tracks each side's force pool and awards victory when the opposing side's losses reach its configured threshold. Player deaths and vehicle destruction count as losses.

## Setup

1. Add `CRF_AttritionGamemodeComponent` to the Game Mode Entity.
2. Set **Team A Faction Key** and **Team B Faction Key** to the factions used by the mission.
3. Configure each team's victory threshold and choose whether to count players and vehicles.
4. Choose the force composition mode before pairing Attrition with a respawn-based mode.

Pools are finalized when safe start ends. If the scenario does not use safe start, finalization occurs when the gamemode reaches the `GAME` state. No extra world entities are required.

## Attributes

| Setting | Description | Default |
|---|---|---|
| Team A Faction Key | Faction whose losses are tracked as Team A | `BLUFOR` |
| Team B Faction Key | Faction whose losses are tracked as Team B | `OPFOR` |
| Team A Victory Threshold | Percent of Team B's force that Team A must destroy | `100` |
| Team B Victory Threshold | Percent of Team A's force that Team B must destroy | `100` |
| End Mission On Victory | Record the winner and advance to AAR when a threshold is reached | `true` |
| Notify Increment Percent | Loss percentage increase between progress notifications | `10` |
| Force Composition Mode | `STATIC` counts tracked lives once; `DYNAMIC` adds each player spawn as a new life | `STATIC` |
| Count Players | Include slotted players in the force pools | `true` |
| Count Vehicles | Include spawned vehicles in the force pools | `true` |
| Vehicle Grace Period Seconds | Continue scanning for late-spawning vehicles after pool finalization | `30` |

## Force Composition Modes

- **STATIC**: The pool is fixed at safe-start end. A tracked player or vehicle can only count as destroyed once. Use this for a single-life/limited-life match.
- **DYNAMIC**: Players re-enter the pool when they spawn, including respawns and late joiners. Use this when pairing Attrition with an objective mode such as Frontline or Rush.

Vehicle tracking uses the configured grace period to catch vehicles that appear around the start of the round. Set it to `0` to count only vehicles present at the instant the pool is finalized.

## Implementation

See [`CRF_AttritionGamemodeComponent.c`](../../../Scripts/Game/GameMode/Attrition/CRF_AttritionGamemodeComponent.c) for the component and editor attributes.
