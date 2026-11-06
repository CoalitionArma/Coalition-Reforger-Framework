# CRF Evacuation Gamemode

In this mode, the defenders don't just hold. They have to get their supplies out by truck while the attackers close in.

The defenders hold a site with a stock of supplies, measured in truckloads. A cargo truck parked in the loading bay loads one truckload, but only while no attackers are inside the site. The truck then drives to the extraction point and stops there, and the load counts as evacuated. The empty truck can go back for another load. A loaded truck that is destroyed loses its load for good.

Every defender loading or escorting a truck is one fewer holding the line. The attackers have two targets: the site and the road.

Example mission: `Missions/Chernarus/tanaKa_1106_EVACUATION78_Zelenogorsk.conf` (US against the ChDKz, Zelenogorsk to Pustoshka).

---

## How a round plays

- **When the clock starts.** The round goes live when safe start ends, and the clock starts then.
- **Loading.**
  - A cargo truck must be inside the loading bay radius and standing still (or close to it).
  - It must not be destroyed and must have no attackers aboard.
  - No living attacker can be anywhere inside the site.
  - One truck loads at a time and takes **Load Seconds** (default 60).
  - When attackers enter the site, loading pauses. The truck keeps its progress and carries on once the site is clear again.
- **Unloading.**
  - A loaded truck must be inside the extraction radius and standing still.
  - At least one living member of the evacuating side must be aboard, and no attackers.
  - After **Unload Seconds** (default 15), the load counts as evacuated and the truck is empty again.
- **Taking the site.**
  - The capture timer climbs while attackers are inside the site and no evacuators are.
  - It holds while both sides are inside and falls back while no attackers are.
  - When it reaches **Capture Seconds** (default 90), the site has fallen.

- **Withdrawal** (with **Require Withdrawal** on, the default).
  - Once **Loads To Win** have been delivered, loading and the site no longer matter: the evacuating side has to get out itself.
  - It needs **Withdraw Share** (default half) of its players still alive at that moment to reach the extraction point, with a minimum of one.
  - A player counts once they come within **Withdraw Radius** of the extraction entity, on foot or in a vehicle.
  - Each one who arrives leaves the round alive and spectates. The clock keeps running.

**The evacuating side wins when:**
- it has delivered **Loads To Win** and then enough of its players have withdrawn (with Require Withdrawal off, delivering the last load is enough); or
- the attackers have nobody left alive (with **Wipe Loses** on).

**The attacking side wins when any of these happen:**
- the site falls;
- so many loads have been destroyed that delivered + on the road + still at the site is less than **Loads To Win**;
- during the withdrawal, too few evacuators are left alive to make up the number still needed;
- the clock runs out;
- the evacuating side has nobody left alive (with **Wipe Loses** on).

The mode has no respawns of its own. Use the Lobby's respawn settings if you want any.

---

## Setup

1. Add `CRF_EvacuationGamemodeComponent` to the **COA_Lobby** gamemode entity.
2. Place three named entities. Empty `GameEntity` objects are fine, because only their position matters:

| Name (default) | What it is |
|---|---|
| `evac_site` | The middle of the site. The site is a circle of **Site Radius** around it. |
| `evac_load` | The loading bay. Put it inside the site, on open ground or a road a truck can stop on. |
| `evac_extract` | The extraction point. |

3. Give the evacuating side cargo trucks. Vehicle spawners work: with the default *Should Respawn On Side Respawn*, a destroyed truck stays destroyed in a mission without respawns. The vehicle depot works too.
4. List the trucks' prefabs in **Cargo Prefabs**. Leave it empty and *any* vehicle can carry a load, including the attackers' Humvees if the evacuators steal them.
5. Set the Lobby time limit (`m_iTimeLimitMinutes`) longer than **Start Seconds** plus safe start, or the Lobby will end the mission before the evacuation clock does.

The mode draws its own map markers:
- the site, for everyone (it turns red while it's falling);
- the extraction point, for everyone;
- the loading bay, for the evacuating side only.

The HUD panel at the top of the screen shows the state, loads out against the target, loads at the site, on the road and lost, and the clock.

---

## Attributes

### Evacuation - Factions
| Attribute | Default | |
|---|---|---|
| Evacuating Faction | `OPFOR` | The side getting its supplies out. |
| Attacking Faction | `BLUFOR` | The side taking the site. |

### Evacuation - Site
| Attribute | Default | |
|---|---|---|
| Site Name | `evac_site` | |
| Site Radius | 150 | Metres. |
| Capture Seconds | 90 | Seconds attackers need inside the site with no evacuators in it. |
| Attackers Stop Loading | on | Any living attacker in the site pauses the loading. |

### Evacuation - Loading
| Attribute | Default | |
|---|---|---|
| Loading Bay Name | `evac_load` | |
| Loading Radius | 15 | Metres from the bay entity a truck has to be. |
| Total Loads | 8 | Truckloads at the site at the start. |
| Loads To Win | 5 | Capped at Total Loads. |
| Load Seconds | 60 | Per truck, one truck at a time. |
| Max Work Speed | 2 | m/s. Faster than this, a truck neither loads nor unloads. |
| Cargo Prefabs | empty | Trucks that can carry a load. Matched by GUID. |

### Evacuation - Extraction
| Attribute | Default | |
|---|---|---|
| Extraction Name | `evac_extract` | |
| Extraction Radius | 30 | Metres. |
| Unload Seconds | 15 | Requires an evacuator aboard and no attacker. |
| Require Withdrawal | on | After the last load, enough of the evacuating side must reach the extraction point to win. Off: the last load wins. |
| Withdraw Share | 0.5 | Share of the evacuators alive when the last load gets out who must withdraw (minimum one). |
| Withdraw Radius | 50 | Metres from the extraction entity a player has to reach to get out. |

### Evacuation - Ending
| Attribute | Default | |
|---|---|---|
| Start Seconds | 3600 | The clock. When it runs out, the attackers win. |
| Wipe Loses | on | A side with nobody left alive loses. |
| End Mission On Victory | on | Records the winner and moves the Lobby to the AAR. Off: the result is announced and an admin ends it. |

---

## Balance notes

- **Time.** The evacuators need at least Loads To Win × Load Seconds of uncontested loading, plus the trips. Example: 5 loads × 60 s, plus a 2.5 km run that takes a Ural about 3 minutes each way. With 4 trucks, that's 4 loads, a truck back, and the 5th load, so about 15 minutes even if nobody interferes. Give the clock at least three times that.
- **Spare loads.** Total Loads minus Loads To Win is how many loaded trucks the evacuators can afford to lose. 8 against 5 allows three burned trucks.
- **Site radius.** This is the main lever on the attackers. Any one of them inside it stops the loading. Too large and a single man in a treeline 140 m out stops everything. Too small and the site is easy to screen.
- **Extraction distance.** Put the extraction point where the attackers can reach the road, but only by going around the site, not through it.
- **Truck count.** Fewer trucks than Loads To Win forces round trips, and each round trip is another run past the attackers.
