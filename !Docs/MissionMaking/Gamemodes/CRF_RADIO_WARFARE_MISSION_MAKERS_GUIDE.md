# CRF Radio Warfare Gamemodes

The Radio Warfare scripts provide two radio-based game modes: **Radio Direction Finding** and **Patrol & Ambush**. Add the component for the mode the scenario uses to its Game Mode Entity. Both modes start when safe start ends and use radio transmissions detected by the CRF/CVON integration; direct proximity voice does not count.

## Radio Direction Finding

One faction builds a score by transmitting on radios. The other faction hunts transmitters using direction-finding bearings while they are on the air.

### Setup

1. Add `CRF_RadioDFGamemodeComponent` to the Game Mode Entity.
2. Set transmitter and hunter faction keys.
3. Give transmitter players radios. Give hunters equipment matching **DF Equipment**.
4. Configure the transmission goal, time limit, DF range, and bearing settings.

| Setting | Description | Default |
|---|---|---|
| Transmitter Faction Key | Side that must accumulate transmission time | `INDFOR` |
| Hunter Faction Key | Side that receives direction-finding bearings | `BLUFOR` |
| Transmission Goal Seconds | Total radio transmission time needed to win | `300` |
| Only Long Range Counts | Restrict scoring and bearings to long-range radios | `false` |
| Time Limit Minutes | Time hunters have to stop the transmitters; `0` disables the limit | `60` |
| Hunters Win When Transmitters Dead | Give hunters a win if no transmitter-side player remains alive | `true` |
| End Mission On Victory | Record the winner and advance to AAR | `true` |
| DF Equipment | Required hunter equipment: `MANPACK`, `LONG_RANGE_RADIO`, or `ANY_RADIO` | `MANPACK` |
| DF Range | Maximum bearing range in metres | `4000` |
| Bearing Interval | Seconds between bearings while a transmitter is on air | `5` |
| Bearing Error Degrees | Maximum error at the edge of DF range | `8` |
| Notify Increment Percent | Transmission progress between notifications | `20` |

The default `MANPACK` equipment is a radio backpack with a long-range radio. Bearings are compass directions rather than map markers; hunters must compare bearings to locate the transmitter.

## Patrol & Ambush

The patrol travels through an ordered checkpoint route and reports by radio at each required stop. Ambushers receive optional approximate route intel and try to stop the patrol.

### Setup

1. Add `CRF_PatrolGamemodeComponent` to the Game Mode Entity.
2. Place one `GenericEntity` with `CRF_PatrolCheckpointComponent` at each checkpoint.
3. Set each checkpoint's order, display name, and patrol approach radius. Use consecutive order numbers starting at `1`; the highest order is the route endpoint.
4. Give patrol players radios. Optionally require reports to use a specific frequency.

| Setting | Description | Default |
|---|---|---|
| Patrol Faction Key | Faction running the route | `BLUFOR` |
| Ambush Faction Key | Faction opposing the patrol | `OPFOR` |
| Min Players At Checkpoint | Alive patrol members required in range | `1` |
| Report Window Seconds | Time allowed for a SITREP after reaching a checkpoint | `90` |
| Report Frequency | Required radio frequency; empty accepts any radio frequency | empty |
| Reports Required | SITREPs required by the route's end; `0` requires one at every checkpoint except the last | `0` |
| Time Limit Minutes | Route time limit; `0` disables the limit | `45` |
| Ambush Wins If Patrol Wiped | Give ambushers a win if the patrol is wiped out | `true` |
| End Mission On Victory | Record the winner and advance to AAR | `true` |
| Ambush Knows Route | Give ambushers the 1 km grid squares the route crosses | `true` |
| Patrol Checkpoint Markers | Show exact checkpoint markers to the patrol | `true` |
| Ambush Search Areas | Show approximate square search areas to ambushers | `true` |

Each checkpoint has **Order** (default `1`), **Checkpoint Name** (default `Checkpoint`), and **Radius Metres** (default `30`). Patrol markers show exact locations; ambusher search areas are offset squares that contain the checkpoint.
The search-area defaults are a 400 m square with a 0.6 offset factor and a 2.5 px border; tune these values to control how much route intelligence the ambushers receive.

## Implementation

- [`CRF_RadioDFGamemodeComponent.c`](../../../Scripts/Game/GameMode/RadioWarfare/CRF_RadioDFGamemodeComponent.c)
- [`CRF_PatrolGamemodeComponent.c`](../../../Scripts/Game/GameMode/RadioWarfare/CRF_PatrolGamemodeComponent.c)
- [`CRF_PatrolCheckpointComponent.c`](../../../Scripts/Game/GameMode/RadioWarfare/CRF_PatrolCheckpointComponent.c)
- [`CRF_RadioTransmissionEvents.c`](../../../Scripts/Game/GameMode/RadioWarfare/CRF_RadioTransmissionEvents.c)
