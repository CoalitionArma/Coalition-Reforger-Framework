# CRF Hold and Relieve Gamemode

One side has an element cut off in a zone and a relief force fighting its way to it. The other side sits between them and tries to seize the zone first.

Example mission: `Missions/BelleauWood/tanaKa_1112_TVT80_HoldtheCrossroads.conf` (a US Army platoon cut off at a crossroads near Marigny-en-Orxois, its relief coming up the Paris Farm road, a Russian motor-rifle platoon in the copse between them). Its spec is `CRF-MissionKit/specs/crossroads.toml`.

---

## How a round plays

- **Start.** Everything starts when the round goes live: safe start ends, or the mission enters the game state if it has no safe start. The hold clock starts then.
- **Who is relief.** On the defending faction, a player is in the relief force when their group's squad callsign starts with one of **Relief Callsigns** (not case-sensitive), e.g. `HAMMER` matches `HAMMER 6`, `HAMMER 1` and `HAMMER WPNS`. Everyone else on that side is the cut-off element.
- **Counting.** Every second the mode counts **living players** inside **Zone Radius** of the zone entity (map distance, ignoring height). AI isn't counted.
- **Link-up.** At least **Link Up Players** relief players inside and at most **Link Up Max Attackers** attackers inside, for **Link Up Seconds** in a row. The defenders' task shows the progress, and both sides get a popup when it starts.
- **Seizure.** At least **Seize Players** attackers inside and **no** living defending-faction player inside, for **Seize Seconds** in a row. The attackers' task shows the progress, and both sides get a popup when it starts.
- **Hold time.** **Hold Minutes** after the round goes live (0 = no limit).

**The defenders win** when the relief force links up, or when the hold time runs out without the zone being seized.

**The attackers win** when they seize the zone.

Progress resets as soon as the condition breaks. With **End Mission On Victory** on, the winner is recorded and the Lobby moves to the AAR. Off, the result is announced and an admin ends the mission.

Each side gets one task ("Hold \<zone\>" or "Seize \<zone\>"), and the pause-menu mission info shows the time left and any link-up or seizure in progress. The mode draws no map markers: place your own.

---

## Setup

1. Add `CRF_HoldAndRelieveGamemodeComponent` to the **COA_Lobby** gamemode entity. Don't combine it with another objective mode.
2. Place a named entity at the centre of the zone (any `GenericEntity`) and set **Zone Anchor Name** to its name.
3. Name the relief force's groups so their callsigns start with a **Relief Callsigns** entry, and give the cut-off element different callsigns.
4. Put the defending faction's **static spawn point** with the relief force. Initial group spawns put the cut-off element in the zone, and respawns come back with the relief. Limit the cut-off element's respawns (per-slot respawns of 1 or 0) if being cut off should hurt.
5. Mark the zone on the map for both sides, plus each side's start.

| Attribute | Default | |
|---|---|---|
| Defending Faction | `BLUFOR` | cut-off element and relief force |
| Attacking Faction | `OPFOR` | |
| Relief Callsigns | (empty) | squad-callsign prefixes of the relief force; empty = no link-up possible |
| Zone Anchor Name | `holdrelieve_zone` | |
| Zone Label | `The Crossroads` | name in tasks and popups |
| Zone Radius | 150 m | |
| Link Up Players / Max Attackers / Seconds | 4 / 2 / 30 | |
| Seize Players / Seconds | 3 / 60 | |
| Hold Minutes | 60 | 0 = only link-up or seizure decide it |
| End Mission On Victory | on | |

**Tips**
- Keep the Lobby's mission time limit a little longer than **Hold Minutes**, so the hold result is announced before the mission times out.
- Give the attackers a start between the zone and the relief route, close enough to intercept the relief force once safe start ends.
