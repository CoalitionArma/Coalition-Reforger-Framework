# CRF Interdiction Gamemode

The hunting side (players) has to find an AI convoy, stop it and take the officer riding with it. Taken alive, he gives up the exact location of the insurgent HQ. Killed first, his documents only point to a large area around it. The hunters then secure the HQ, and the operation ends when everyone still alive is back at base.

Example mission: `Missions/Chernarus/tanaKa_1108_COTVT52_OperationSideswipe.conf` (US special forces in Little Birds against the ChDKz, NWAF to Pusta).

Needs ACE Captives: the officer surrenders through ACE, and the hunters zip-tie him with `ACE_Captives_ZipCuffs`.

---

## How a round plays

- **Start.** Everything starts when the round goes live (safe start ends, or the mission enters the game state if it has no safe start). The clock starts then too.
- **The officer.** He's the member of the convoy group spawned from **Officer Prefab**. The mode finds him as soon as the group has spawned its members.
- **Surrender.** A living hunter within **Surrender Radius** (3D, default 40 m) while the officer is conscious and either on foot or in a vehicle that has stopped (under 2 m/s) or is wrecked, held for **Surrender Delay** seconds:
  - he leaves the convoy group and his AI is switched off, so he neither shoots nor runs;
  - if he's in a vehicle he climbs out (teleported out after 6 s if he can't);
  - he drops the weapon in his hands and raises his hands (ACE surrender);
  - the hunters get *"THE OFFICER IS SURRENDERING"*.
  If he's knocked out while surrendered, his hands go back up a few seconds after he comes round.
- **Captured.** The officer is ACE-captive (zip-tied) while alive. This counts however it happens, including tying him while he's unconscious without surrendering first. The hunters get the exact HQ position: a red marker on their map, a popup with its grid, and the HQ task moves there.
- **Killed.** If he dies before he's captured, the hunters get a red circle of **Hint Radius** on their map. The HQ is always inside it, and its centre is up to 60% of the radius away from the HQ, chosen once per mission. If he dies after he was captured, nothing changes.
- **Securing the HQ.** Checked every half second, revealed or not. The HQ is a circle of **HQ Radius** around the HQ entity. It's cleared while at least one living hunter is inside and at most **HQ Cleared Max Opfor** conscious insurgents are (AI and players alike; unconscious and captive ones don't count). Cleared for **HQ Secure Seconds** in a row, it's secured for good. The hunters' HQ task shows the progress.
- **Extraction.** Once the HQ is secured, the hunters win when every living hunter (at least one) is within **Base Radius** of the base entity, on foot or in a vehicle. The "Return to base" task counts them.

**The hunters win** when the HQ is secured and everyone still alive is home.

**The insurgents win** when:
- **Time Limit Minutes** runs out (0 = no limit); or
- no hunter has been alive for 60 s and none can respawn (respawn off, past the Lobby's respawn cutoff, no hunter spawn points, no tickets left, or in per-slot respawn mode no hunter slot with respawns left).

With **End Mission On Victory** on (the default) the winner is recorded and the Lobby moves to the AAR. Off, the result is announced and an admin ends the mission.

---

## Setup

1. Add `CRF_InterdictionGamemodeComponent` to the **COA_Lobby** gamemode entity. Don't combine it with another objective mode.
2. Place the convoy as **one** `SCR_AIGroup` named `interdiction_convoy`:
   - the insurgent faction, Lobby gearscript characters so their gear applies;
   - one unit prefab slot is the officer prefab, and **Officer Prefab** is set to exactly that prefab (it's matched by GUID). Use a prefab no other convoy member uses;
   - its vehicles in `m_aStaticVehicles`, a get-in waypoint, then Move waypoints along the route and an `AIWaypointCycle` so it keeps driving.
3. Draw the route on the hunters' map (for example a `DrawLinePrefab` visible to them only). The mode doesn't draw it.
4. Place two named entities. Empty `GameEntity` objects are fine, because only their position matters:

| Name (default) | What it is |
|---|---|
| `interdiction_hq` | The centre of the HQ. Don't put a hunter-visible marker on it. |
| `interdiction_base` | The centre of the hunters' base. |

5. Give the hunters `ACE_Captives_ZipCuffs` in their gearscript.
6. Match the Lobby time limit to **Time Limit Minutes**. Both start when the round goes live, but an admin extending the Lobby timer doesn't extend this one.

The mode makes its own vanilla tasks (map task list):
- hunters: *Intercept the convoy*, *Take the officer alive*, *Find the insurgent HQ* (no position until something is revealed), and *Return to base* once the HQ is secured;
- insurgents: *Protect the convoy* and *Defend the HQ* (no positions).

The pause menu's mission info shows the officer's state and what's known about the HQ.

If the convoy group or the officer can't be found, the log says so after a minute and the mode keeps looking. The HQ and the extraction still work without him.

---

## Attributes (category "Interdiction")

| Attribute | Default | |
|---|---|---|
| Blufor Faction | `BLUFOR` | The hunting side. |
| Opfor Faction | `OPFOR` | The insurgents, AI and players. |
| Convoy Group Name | `interdiction_convoy` | The placed `SCR_AIGroup`. |
| Officer Prefab | empty | The convoy member spawned from this prefab is the officer. |
| Surrender Radius | 40 | Metres (3D) from the officer a living hunter has to be. |
| Surrender Delay | 3 | Seconds the surrender condition has to hold. |
| HQ Anchor Name | `interdiction_hq` | |
| HQ Radius | 150 | Metres. |
| HQ Cleared Max Opfor | 2 | Conscious insurgents allowed inside a cleared HQ. |
| HQ Secure Seconds | 30 | Seconds it has to stay cleared. |
| Hint Radius | 900 | Metres: the area the hunters get if the officer dies first. |
| Base Anchor Name | `interdiction_base` | |
| Base Radius | 350 | Metres. |
| Time Limit Minutes | 120 | From round live. 0: no limit. |
| End Mission On Victory | on | Record the winner and go to the AAR. |

---

## Notes

- **No secrets reach the client.** The HQ position is only sent to clients once it's revealed, and the hint circle only once the officer is dead. Only hunters draw the markers. The `interdiction_hq` entity is still part of the world file, like any placed object.
- **Hint radius.** The HQ can be up to 60% of the radius from the circle's centre. 900 m gives a circle nearly 2 km across: a real search, but one that ends.
- **HQ radius.** Cover the whole garrisoned area. Insurgents beyond the radius don't stop the HQ being secured, and hunters beyond it don't count as inside.
- **Base radius.** Cover the pads and the spawn, so returning helicopters can land anywhere on base.
