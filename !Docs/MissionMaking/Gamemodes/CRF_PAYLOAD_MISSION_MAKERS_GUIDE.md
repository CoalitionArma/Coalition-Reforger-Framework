# Payload: Mission Maker's Guide

Attackers escort a truck along a route. Defenders stop it.

- The truck moves by itself while attackers are close to it and no defender is. More attackers push it faster.
- Left without attackers, it stops, then rolls back, but never past the last checkpoint it reached.
- Each checkpoint adds time to the clock, can move both sides' respawn flags and brings back both sides' dead.
- Attackers win when the truck reaches the end of the route.
- Defenders win when the clock runs out, unless attackers are still with the truck. Then it's overtime until the truck arrives or they leave it.

The truck can't be driven, destroyed or stuck. It's a prop that every machine places on the terrain at a distance the server replicates.

`Worlds/Chernarus/tanaka_special_payload` is a working example.

## Setup

1. Add `CRF_PayloadGamemodeComponent` to the `COA_Lobby` entity in your `_INIT` layer.
2. Place `Prefabs/Props/Payload/CRF_PayloadTruck.et` anywhere and name it `payload`. The mode moves it to the start of the route. Any static prop works, but not a vehicle.
3. Draw a `PolylineShapeEntity` along the road from start to finish and name it `payload_route`. Keep points close enough to follow the bends: straight lines between them cut corners through whatever is there.
4. Place an empty named entity at each checkpoint, on or near the route. List them in **Checkpoints** in route order.
5. For each checkpoint, optionally place and name an entity where each side's respawn flag should move, and enter the names on that checkpoint.
6. Place one spawn point per side. Put the defenders' up the road from the attackers', far enough that they can reach their first position during safestart.
7. Turn respawns on (`m_bRespawnEnabled`), with wave respawns if you want them, and turn on **Hide Other Spectator Factions** (`m_bHideOtherSpectatorFactions`) so the dead can't scout for their team. Without respawns the round usually ends with one side gone and the clock running.

Set `m_fHeightOffset` to about `1.1` for `CRF_PayloadTruck.et` (the default of 0 suits a prop whose origin is on the ground).

## Avoid

- **Bridges and tunnels.** The truck follows the terrain, so it drives under a bridge. Route around them.
- **Narrow gaps and trees.** The truck passes through anything in its way, so check the route is clear on foot.
- **Very long routes.** At the default speeds the truck takes about 17 minutes per 2.5 km unopposed. Budget for at least double that once anyone is shooting.

## Settings

| Setting | Default | What it does |
|---|---|---|
| `m_sAttackerFaction` / `m_sDefenderFaction` | `BLUFOR` / `OPFOR` | The two sides |
| `m_sPayloadName` / `m_sRouteName` | `payload` / `payload_route` | Entity names |
| `m_fHeightOffset` | 0 | Metres the prop's origin sits above the ground |
| `m_fWheelbase` | 5 | Metres between its axles, for how it turns and climbs |
| `m_fEscortRadius` | 30 | Metres from the truck a player counts as being with it |
| `m_iMinAttackers` | 1 | Attackers needed to move it |
| `m_iMinDefenders` | 1 | Defenders needed to stop it |
| `m_fBaseSpeed` | 2.5 | Metres per second with the fewest attackers |
| `m_fSpeedPerAttacker` | 0.25 | Added by each extra attacker |
| `m_fMaxSpeed` | 4 | Fastest it goes |
| `m_fRollbackDelay` | 30 | Seconds without attackers before it rolls back (0 = never) |
| `m_fRollbackSpeed` | 1 | Metres per second it rolls back |
| `m_bRespawnOnCheckpoint` | on | Bring both sides' dead back at each checkpoint |
| `m_iStartSeconds` | 1500 | Seconds on the clock at the start |
| `m_fOvertimeGrace` | 10 | In overtime, seconds without attackers before the defenders win |
| `m_bEndMissionOnVictory` | on | Advance to the AAR when decided. Off: an admin ends the mission |

On a checkpoint: `m_sName`, `m_sEntityName`, `m_iBonusSeconds` (default 300), `m_sAttackerSpawnEntity`, `m_sDefenderSpawnEntity`.

## Balance

- **One defender stops it.** With big teams, raise `m_iMinDefenders` to 2 or 3 so a single player in a bush can't hold it alone.
- **Length vs clock.** Start with a clock about 1.5 times the unopposed drive time, and let the checkpoints add the rest.
- **Spawn distance.** The further attackers respawn behind the truck, the more a wipe costs them.

## Logs

Problems print with the `[CRF_Payload]` prefix. A missing route or prop is reported when the mission loads, and the truck won't move.
