# Tank Battle: Mission Maker's Guide

One player, one tank, three capture points. A War Thunder style special.

- Players spawn on foot at their side's flag and pick a tank from a menu. It spawns at one of the side's tank spawns with them in the driver's seat.
- One player runs the whole tank. They drive with the normal vehicle keys, and the main gun answers to them as well: the mouse aims, left click fires, right click goes to the gun sight. The engine starts by itself when the round goes live.
- The camera stays in third person while in the tank. The gun sights still work.
- Players can't get out or change seats. When the tank is destroyed, so are they. After **Respawn Seconds** they're back at their flag to pick again.
- Points are taken by tanks, not people. A tank alone in a point captures it; more tanks capture faster; tanks from both sides stop it. An enemy point is neutralised first, then captured.
- Every point a side holds fills its bar. Holding them all fills it faster still. The first side to fill its bar wins.
- On screen, each point has a marker over it. It shows the point's letter in the holder's colour, a bar with the capture progress, and who is capturing it ("CAPTURING", "ENEMY CAPTURING", "CONTESTED") or how far away it is.

## Setup

1. Add `CRF_TankBattleGamemodeComponent` to the `COA_Lobby` entity in your `_INIT` layer.
2. Fill in **Sides** with exactly two entries:
   - **Faction Key**: `BLUFOR`, `OPFOR` and so on.
   - **Tanks**: what the side can pick, in menu order. The menu only shows the vehicles the loaded mods can spawn, so a list can name vehicles from optional mods. The server log names each one left out. Each has a name, a one-line description, the prefab, an optional **Limit** (how many the side can have out at once, 0 = no limit) and a **Turret Seat**: which turret the driver controls, counting the tank's turret seats from 0. The main gun is normally 0. Try 1 if the mouse turns the commander's machine gun instead. Tick **Can Repair** on the light IFVs, APCs and scout cars: they can repair a friendly vehicle's destroyed engine or gearbox in the field (see `m_fRepairSeconds`).
   - **Spawn Entities**: names of entities where the side's tanks appear. Each tank faces the way its entity faces. A spawn with a tank parked on it is skipped while another is free. Every tank sits on its spawn until the round goes live, so give each side **one spawn per player**, about 15 m apart (rows across the field).
3. Place an empty named entity at the centre of each capture point and list them in **Points** with a name (A, B, C) and a radius. Three is the classic layout; up to five fit on the HUD.
4. Give each side one respawn flag. Players spawn there on foot, pick a tank, and come back there after dying. Keep it close to the tank spawns.
5. Make the sides even: the same number of slots on each, all as crew or riflemen. The example missions give each player a group of their own, one Vehicle Lead slot (tank commander) each. Without group spawns, everyone starts at their side's flag. The mode doesn't care which role. At the start it warns everyone if the sides are uneven. The admin slot balancer fixes it.
6. Leave the mission's own respawn off (`m_bRespawnEnabled` 0). The mode brings players back itself after **Respawn Seconds**. Turn **Respawn Players** off only if you want the mission's respawn settings instead.

### Vehicle prefabs

A vehicle needs a **Tank Battle prefab**: a stock vehicle won't let its driver use the gun. CRF has these ready:

| Vehicle | Type | Prefab |
|---|---|---|
| M1A1 Abrams, woodland | tank | `{7C2E5A91B3D04F64}Prefabs/Vehicles/Tracked/M1A1/TankBattle/M1A1_Woodland_TankBattle.et` |
| M1A1 Abrams, desert | tank | `{7C2E5A91B3D04F66}Prefabs/Vehicles/Tracked/M1A1/TankBattle/M1A1_Desert_TankBattle.et` |
| T-14 Armata (RHS) | tank | `{7C2E5A91B3D04F68}Prefabs/Vehicles/TankBattle/T14/T14_TankBattle.et` |
| T-90M (RHS, from its `T90M2_base`) | tank | `{7C2E5A91B3D04F78}Prefabs/Vehicles/TankBattle/T90M/T90M_TankBattle.et` |
| T-72A (WCS) | tank | `{7C2E5A91B3D04F7A}Prefabs/Vehicles/TankBattle/T72/T72A_Green_TankBattle.et` |
| T-72B (WCS) | tank | `{7C2E5A91B3D04F7C}Prefabs/Vehicles/TankBattle/T72/T72B_Green_TankBattle.et` |
| Stryker MGS (WCS) | wheeled 105 mm assault gun | `{7C2E5A91B3D04F76}Prefabs/Vehicles/TankBattle/Stryker/Stryker_MGS_TankBattle.et` |
| Kurganets-25, 57 mm (RHS) | tracked IFV | `{7C2E5A91B3D04F6A}Prefabs/Vehicles/TankBattle/Kurganets25/Kurganets25_57mm_TankBattle.et` |
| LAV-25 (RHS) | wheeled IFV, 25 mm | `{7C2E5A91B3D04F72}Prefabs/Vehicles/TankBattle/LAV25/LAV25_RHS_TankBattle.et` |
| Kurganets-25, Kord RWS (RHS) | tracked APC, 12.7 mm | `{7C2E5A91B3D04F6C}Prefabs/Vehicles/TankBattle/Kurganets25/Kurganets25_Kord_TankBattle.et` |
| BTR-70 (RHS AFRF) | wheeled APC, 14.5 mm | `{7C2E5A91B3D04F6E}Prefabs/Vehicles/TankBattle/BTR70/BTR70_AFRF_TankBattle.et` |
| BRDM-2 (RHS AFRF) | scout car, 14.5 mm | `{7C2E5A91B3D04F70}Prefabs/Vehicles/TankBattle/BRDM2/BRDM2_AFRF_TankBattle.et` |
| 2S1 Gvozdika (RHS) | 122 mm self-propelled gun | `{7C2E5A91B3D04F74}Prefabs/Vehicles/TankBattle/2S1/2S1_Green_TankBattle.et` |

The Stryker needs WCS_Stryker and the T-72s need WCS_T-72. Both became CRF dependencies on 2026-11-18. CRF-MissionKit's `tools/tankbattle/gen_tb_vehicles.py` makes these prefabs from a vehicle's own prefab chain. Use it for a new vehicle rather than writing them by hand.

To make another, create two child prefabs:

1. **Turret**: a child of the vehicle's main turret. On its `SCR_TurretControllerComponent` set:
   - `CanBeOverridden` 1, so another seat can take it over.
   - `GetUpdateEventsWhenEmpty` 1, so it updates with nobody in the gunner's seat.
   - `CanAimOnlyInADS` 0, so the mouse aims it outside the sight. WCS tanks have this on.
   - Note its `UniqueTurretName`, for example `mainturret`.
2. **Hull**: a child of the vehicle.
   - In `SlotManagerComponent`, point the turret slot at your turret prefab.
   - In `SCR_BaseCompartmentManagerComponent`, give the driver's `PilotCompartmentSlot` `AutoConnectToTurret "<UniqueTurretName>"` and `AutoConnectContextIndices 0x1e 0`. That's aim, sight, fire and zeroing, but not the W/A/S/D turret keys.
   - On the same seat set `ForcedFreeLook 0`. Vanilla's driver seat forces free-look, and free-look takes the mouse before the turret does, so without this the gun fires but won't turn.

This is how vanilla's armed Mi-8 gives its pilot the rocket pods. At round start the mode logs once per tank whether the driver controls the gun. A warning there means the prefab is missing one of these settings.

CRF's older `T72A_BLUE.et` / `T72A_red.et` (WCS_T-72 parts) now load too, but they aren't Tank Battle prefabs: use the T-72A/B above.

Example mission: **COA TANKBATTLE80 Pustoshka Fields** (Chernarus, 40 v 40, the same twelve vehicle types for both sides, tanks capped per side), `Worlds/Chernarus/tanaka_tankbattle_pustoshka.ent`. It's generated from `specs/pustoshka.toml` in CRF-MissionKit.

### Laying out a field

Put the spawns at the two ends of the field and the points across the field between them, so every point is the same distance from both sides. Pustoshka Fields has the spawns 2 km apart and the points 760 m apart, in a field about 2.9 × 2.5 km. The field must be open and wide: tanks can't get through forest. Kolguyev has nothing wider than its airfield, about 300 m, so Steel Runway runs long instead: from the runway down the coastal plain, spawns 1.55 km apart.

Keep the two spawn areas out of each other's sight, or tanks shoot each other while the round is held. Both spawns are held still until the round goes live. Use the ground: put each side's spawn behind a rise or a tree line. Pustoshka Fields puts BLUFOR 310 m north of the axis and OPFOR 310 m south, so each one hides behind a rise. It mirrors them through B, so each side has the same distances to the points. Steel Runway hides OPFOR behind the coastal woods past the runway's end. On open ground with nothing in between, build cover: a wall of `BlastCover_E_01_18x5` aircraft revetments in front of each grid, side by side, closed side towards the enemy (each about 4 m high and 22 m wide; Steel Runway used seven a side before it was lengthened). Leave room at the ends of the wall for the tanks to drive out, and keep it clear of the capture zones.

## Scoring

| Held | Per second (defaults) | Time to fill 1000 |
|---|---|---|
| 1 point | 2 | 8 min 20 s |
| 2 points | 4 | 4 min 10 s |
| all 3 | 6 + 4 bonus = 10 | 1 min 40 s |

Turn up **Score Per Kill** if you want kills to count as well. 25 a kill makes a wipe worth about a minute of holding two points.

## Settings

| Setting | Default | What it does |
|---|---|---|
| `m_aSides` | | The two sides, with their tanks and tank spawns |
| `m_aPoints` | | The capture points: name, entity name, radius (60 m) |
| `m_fCaptureSeconds` | 20 | Seconds for one tank to take a neutral point. An enemy point takes twice as long. |
| `m_fExtraTankCaptureSpeed` | 0.5 | How much faster each extra tank captures |
| `m_fMaxCaptureMultiplier` | 2 | Fastest a point can be captured, as a multiple of one tank |
| `m_iScoreLimit` | 1000 | Score that fills a bar and wins |
| `m_fScorePerPoint` | 2 | Score per second for each point held |
| `m_fAllPointsBonus` | 4 | Extra score per second while holding every point |
| `m_iScorePerKill` | 0 | Score for each enemy killed |
| `m_bEndMissionOnVictory` | off | On: advance to the AAR when a bar fills. Off (the default, Tank Battle is a special): the winner is announced, tanks are held, and an admin ends the mission. |
| `m_fRespawnSeconds` | 10 | Seconds dead before a player is back at their flag |
| `m_bRespawnPlayers` | on | The mode respawns players. Off: the mission's respawn settings decide. |
| `m_bLockCrewIn` | on | No getting out or changing seats |
| `m_bForceThirdPerson` | on | Third person in the tank. The sights still work. |
| `m_bHoldUntilLive` | on | Tanks are held (engine off, handbrake on) until the round goes live |
| `m_fWreckSeconds` | 60 | Seconds a wreck, or a tank whose player is gone, stays before it's removed |
| `m_fRepairSeconds` | 15 | Field repair: a vehicle with **Can Repair**, stopped next to a stopped friendly whose engine or gearbox is destroyed, repairs it in this many seconds. 0 = no field repair. |
| `m_fRepairRange` | 15 | Metres between the two vehicles' centres for a repair |
| `m_fRepairMaxSpeed` | 1.5 | Both vehicles have to be under this speed (m/s) |
| `m_fCameraDistanceScale` | 1.6 | Third person camera distance in a tank, as a multiple of the tank's own (1 = vanilla) |
| `m_fCameraExtraFov` | 10 | Degrees added to the third person field of view in a tank (0 = vanilla) |
| `m_fCameraExtraHeight` | 2.5 | Metres the third person camera is raised in a tank |
| `m_bCameraLookAtAim` | on | The third person camera looks at where the gun points, so the crosshair stays in the middle of the screen and the tank sits below it. Off: vanilla, along the gun. |
| `m_bShowCrosshair` | on | A crosshair where the gun would hit, in third person. The sight has its own reticle. |
| `m_bShowMinimap` | on | A minimap in the bottom right while you sit in your tank: capture zones (filled by owner, ringed by who's taking them) and friendly tanks |
| `m_bMinimapSatellite` | on | Draw the map's satellite image under the minimap |
| `m_sMinimapImage` | | An image to use instead of the world's own satellite image (the map entity prefab's "Satellite background image"). It must cover the whole terrain, north up. Some maps set the image on the map entity placed in the world instead of its prefab, which the minimap can't read in game, so set it here. For Chernarus Minus that's `{096230E40A49DAC9}Worlds/MapData/ChernarusMinusMap.edds`. Without it the minimap has no satellite map, and the log warns. |

## How it works

- **Driving.** The player sits in the driver's seat, so driving, the engine, the gears and the tank's network ownership are all vanilla. Like any driver, their own machine runs the tank's physics, so there's no network delay on the controls.
- **The gun.** The main turret is attached to the driver's seat (`BaseCompartmentSlot.AttachTurret`), the way vanilla's armed Mi-8 and UH-1H give the pilot the rocket pods. The driver gets the turret's aim, sight, fire and zeroing controls, but not its W/A/S/D traverse keys, so those only drive. A small camera override gives that seat the turret cameras, so third person follows the gun and right click goes to its sight.
- **Ownership.** Who has which tank is replicated from the component. The no-exit and camera rules come from small vanilla overrides in `TankBattle/VanillaOverrides`.

## Test checklist

Test with a dedicated server and two clients as well as in Workbench.

- [ ] Picking a tank spawns it and seats the player in the driver's seat.
- [ ] Once the round is live the engine starts by itself and W/A/S/D drive. Before that the tank stays put.
- [ ] The mouse turns the main gun, not the commander's MG. If not, set **Turret Seat** to 1. W/A/S/D don't turn the turret.
- [ ] Left click fires the main gun. Right click goes to its sight, and back out.
- [ ] The camera key doesn't leave third person. If the server has `disableThirdPerson` set in its config, check that the camera still goes to third person. If it doesn't, this mode needs a server without that setting.
- [ ] Get out, jump out and switch seat do nothing.
- [ ] A destroyed tank kills its player, who is back at the flag after 10 s with the menu up.
- [ ] Knock out a tank's engine (shoot the engine deck): its driver sees "ENGINE OUT", it shows amber on friendly minimaps, and a friendly IFV/APC stopped next to it repairs it in 15 s ("REPAIRING FRIENDLY 40%" / "BEING REPAIRED 40%"); the engine starts again by itself.
- [ ] Points change colour on the map and HUD. The bar fills faster with more points.
- [ ] The first full bar announces the winner and holds the tanks; the mission waits for an admin (with `m_bEndMissionOnVictory` on it goes to the AAR).
