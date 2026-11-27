# CRF Hostage Rescue Gamemode

A rescue force has to get player hostages out alive. The captors choose where to hold them.

- **Rescue force wins** when enough hostages reach the extraction point alive (a share of the hostages slotted when the round goes live).
- **Captors win** when the clock runs out, when so many hostages have died that the share can't be reached any more, or when the rescue force has nobody left alive.
- **Both lose** when the captors execute a hostage (default; see Execution Outcome).
- **The mode never ends the round.** The first result is final: it is announced to everyone (popup), logged to the admins, and the tasks are closed. Play goes on, and rescues and deaths are still annotated, until an admin ends the mission.

## Hints

Hostage events show as hint cards, only to the sides that would know about them:

| Event | Rescue force | Captors | Hostages |
|---|---|---|---|
| Hostages moved (safe start) | | ✓ | ✓ |
| Hostages restrained (round live) | | ✓ | ✓ |
| Hostage freed | ✓ | | ✓ |
| Hostage recaptured (tied up again) | | ✓ | ✓ |
| Hostage down / conscious again | ✓ | ✓ | ✓ |
| Hostage rescued, killed, executed | ✓ | ✓ | ✓ |
| Intel (1 km grid square) | ✓ | | |

Long Night (`Missions/Chernarus/tanaKa_1127_TVT60_LongNight.conf`) is the reference mission.

## How a round plays

- **Safe start: picking the site.** Hostages are player slots on their own faction (normally CIV) and start with the captors. A captor group leader can forward deploy their group from the map ("Forward Deploy Element") anywhere inside the captors' forward deploy zone. When the leader of a **keeper group** does it, every hostage goes along to the same spot. Hostages can't forward deploy on their own.
- **Round live.** Every hostage is tied up (ACE Captives). The rescue force frees them with the ACE release action or escorts them. Captors can re-tie and move them.
- **Rescue.** A living hostage inside the extraction radius is rescued and leaves the round alive (spectator).
- **Executions.** A hostage killed by the captors is handled by **Execution Outcome**. By default (`BOTH_LOSE`) the round is decided at once: both sides lose. `LOST` treats it like any other death; `RESCUED` counts it as a rescue.
- **Other deaths** (the rescue force, a fall, no killer) lose that hostage. The captors win when so many are lost that the share can't be reached.
- **Intel.** Optionally, the rescue force is told which 1 km grid square the hostages are in, first after a delay and then at an interval.

Only living players count; AI is ignored.

## Setup

1. Add `CRF_HostageRescueGamemodeComponent` to the `COA_Lobby` gamemode entity.
2. Give the hostage faction player slots (role `UNARMED`), a spawn and a safe start zone at the captors' start.
3. Place a named entity at the extraction point (`hostage_extract`).
4. Place a forward deploy zone (`PrefabsMissionMaking/GameBorders/ForwardDeploy/OPFOR_ForardDeployZone.et`) over the area the captors may choose from, and set its **Visible For Factions** to the captors **and** the hostage faction. A hostage moved into it has to be inside a zone of their own faction, or the border check kills them for leaving their safe start zone.
5. Name the keeper group so its squad callsign starts with an entry in **Keeper Callsigns**.

In the mission kit: `[[forward_deploy]]` with `faction` and `factions = ["OPFOR", "CIV"]`.

## Attributes

| Setting | Description | Default |
|---|---|---|
| Attacking Faction | The rescue force. | `BLUFOR` |
| Defending Faction | The captors. | `OPFOR` |
| Hostage Faction | Every living player of this faction is a hostage. | `CIV` |
| Keeper Callsigns | Captor groups whose leader's forward deploy takes the hostages along (squad callsign prefix, not case-sensitive). | — |
| Restrain At Round Live | Tie every hostage up when the round goes live. | on |
| Execution Outcome | A hostage killed by the captors: `BOTH_LOSE` (decides the round: both sides defeated), `LOST` (like any other death) or `RESCUED` (counts as a rescue). | `BOTH_LOSE` |
| Extraction Name | Entity at the extraction point. | `hostage_extract` |
| Extraction Radius | Metres from it, on the map, a living hostage has to reach. | `60` |
| Rescue Share | Share of the hostages at round live that must be rescued, rounded up (at least one). 0.6 of 5 = 3. | `0.6` |
| Time Limit Minutes | From round live until the captors win (announced; the round goes on). | `60` |
| Wipe Loses | The captors win when the rescue force has nobody left alive. | on |
| Intel First Minutes | Minutes after round live of the first grid-square report to the rescue force. 0: none. | `10` |
| Intel Interval Minutes | Minutes between later reports. 0: only the first. | `15` |

Set the Lobby time limit longer than safe start plus **Time Limit Minutes**.

## Tasks

One task per side (rescue / hold), with no map position: tasks reach every client, and the hostage site is a secret.

## Implementation

- [`CRF_HostageRescueGamemodeComponent.c`](../../../Scripts/Game/GameMode/HostageRescue/CRF_HostageRescueGamemodeComponent.c): the round, rescues, deaths, intel, tasks.
- [`CRF_COA_ForwardDeployManager.c`](../../../Scripts/Game/!Systems/ModdedOverrides/Coalition%20Lobby/Managers/CRF_COA_ForwardDeployManager.c): moves the hostages with the keeper group and hides forward deploy from the hostages.
