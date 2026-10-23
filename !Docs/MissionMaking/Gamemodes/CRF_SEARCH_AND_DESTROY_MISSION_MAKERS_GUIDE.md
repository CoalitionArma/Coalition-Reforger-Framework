# CRF Search & Destroy Gamemode

Search & Destroy has two bomb sites. Attackers plant and defend a bomb at a site until its countdown destroys the site; defenders can defuse it. Attackers win by destroying both sites.

## Setup

1. Add `CRF_SearchAndDestroyGamemodeManager` to the Game Mode Entity.
2. Place two empty entities at the bomb-site spawn points and name them exactly `aSiteTrigger` and `bSiteTrigger`.
3. Configure the attacking and defending faction keys.
4. Confirm the configured bomb prefab has the plant/defuse actions and required replication setup.

The gamemode spawns the configured bomb prefab at each trigger's position and rotation. Both named trigger entities are required.

## Attributes

| Setting | Description | Default |
|---|---|---|
| Attacking Side | Faction allowed to plant | `BLUFOR` |
| Defending Side | Faction allowed to defuse | `OPFOR` |
| Bomb Site Prefab | Prefab spawned at both site triggers | `CRF_Bomb.et` |
| Hide Map Markers | Hide the global A/B site markers | `false` |

The site countdown is 120 seconds in the current implementation. Only one site countdown can be active at a time. The plant/defuse actions are included by the bomb prefab; configure their action duration in the prefab if mission balance requires a change.

## Implementation

- [`CRF_S&D_Game.c`](../../../Scripts/Game/GameMode/Search%20And%20Destroy/CRF_S%26D_Game.c) — site spawning, countdown, and marker behavior
- [`CRF_S&D_PlantBombAction.c`](../../../Scripts/Game/GameMode/Search%20And%20Destroy/CRF_S%26D_PlantBombAction.c)
- [`CRF_S&D_DefuseBombAction.c`](../../../Scripts/Game/GameMode/Search%20And%20Destroy/CRF_S%26D_DefuseBombAction.c)
