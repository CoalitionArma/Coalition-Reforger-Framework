# CRF Insurgency Gamemode

Insurgency is a phased cache-destruction mode. Attackers destroy the active phase's caches; when every cache in that phase is destroyed, the next configured phase activates. Completing the last configured phase gives the attackers a victory notification.

## Setup

1. Add `CRF_InsurgencyGamemodeManager` to the Game Mode Entity.
2. Set the attacking and defending faction keys.
3. Place one destructible cache entity for each objective and assign its phase number.
4. Add `CRF_InsDestructiveComponent` and an `RplComponent` to every cache entity. The cache must also have an `SCR_DestructionMultiPhaseComponent`.
5. Set the phase 1 cache or caches to start active. Later-phase caches are activated by the gamemode when the previous phase is cleared.

Every phase number must have at least one cache. Phase numbering should be contiguous, starting at 1; the mode advances to the next integer phase and finishes when it finds no next phase.

## Gamemode Settings

| Setting | Description | Default |
|---|---|---|
| Attacking Side | Faction attacking the caches | `OPFOR` |
| Defending Side | Faction defending the caches | `INDFOR` |
| Phase Buffer Minutes | Delay before the next phase's search area is revealed to attackers; `0` reveals it immediately | `0` |

## Cache Component Settings

| Setting | Description | Default |
|---|---|---|
| Show Marker For Defenders | Show the cache marker to defenders | `true` |
| Cache Phase | Phase/sequence number for this cache | `1` |
| Start Active | Whether the cache is active when the scenario starts | `false` |

When a phase is completed, the mode activates the next phase and triggers a respawn wave for both sides. The configured phase buffer affects when the next search area is revealed, not the cache's phase assignment.

## Implementation

- [`CRF_Insurgency_Game.c`](../../../Scripts/Game/GameMode/Insurgency/CRF_Insurgency_Game.c) — phase management
- [`CRF_InsDesctrucitveComponent.c`](../../../Scripts/Game/GameMode/Insurgency/CRF_InsDesctrucitveComponent.c) — destructible cache setup
