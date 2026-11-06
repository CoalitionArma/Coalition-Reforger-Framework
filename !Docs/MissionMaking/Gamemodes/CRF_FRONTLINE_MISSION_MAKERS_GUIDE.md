# CRF Frontline Gamemode

Frontline is a linear, zone-control mode. The two configured factions begin with control of opposite ends of the zone list and fight over the intervening zones. Capturing all zones starts the configured victory hold timer.

## Setup

1. Add `CRF_FrontlineGamemodeManager` to the Game Mode Entity.
2. Place an entity for every capture zone and add each entity's exact name to **Zone Object Names** in progression order.
3. Make every listed zone object invincible; the component expects the zones to persist for the whole match.
4. Set the two faction keys and adjust capture, unlock, and victory timers for the mission.

The HUD has five zone slots. Keep the configured zone list to five or fewer entries so each zone has a corresponding display slot.

## Attributes

| Setting | Description | Default |
|---|---|---|
| Blufor Side | Faction key assigned to the first half of the zones | `US` |
| Blufor Side Nickname | Display name for that side | `USA` |
| Opfor Side | Faction key assigned to the last half of the zones | `USSR` |
| Opfor Side Nickname | Display name for that side | `USSR` |
| Zone Object Names | Ordered list of zone entity names; all entities must exist and be invincible | — |
| Zone Capture Time | Seconds required to capture a zone | `30` |
| Zone Unlock Time | Delay before locked zones unlock | `600` |
| Minimum Players Needed | Players required to capture a zone | `5` |
| Time To Win | Hold time, in seconds, after a side controls every zone | `600` |
| Initial Time | Delay before the middle zone unlocks and the match begins | `900` |

Use faction keys that match the mission's actual factions; the defaults are not suitable for every scenario. The zone list's order determines which zones are assigned to each side and how the front advances.

## Implementation

- [`CRF_Frontline_Game.c`](../../../Scripts/Game/GameMode/FrontLine/CRF_Frontline_Game.c) — capture logic and component attributes
- [`CRF_Frontline_Display.c`](../../../Scripts/Game/GameMode/FrontLine/CRF_Frontline_Display.c) — frontline HUD
