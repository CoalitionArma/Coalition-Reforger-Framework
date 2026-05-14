# CRF — Capture the Flag (CTF) Game Mode

Two sides fight over a single shared flag whose position is shown on the map at all times. The first faction to carry the flag to their own designated drop zone (and hold it there briefly) scores a capture. The first side to reach the configured capture count wins the round.

---

## Files

| File | Purpose |
|---|---|
| `Scripts/Game/GameMode/CaptureTheFlag/CRF_CTF_Game.c` | Main game mode component — server logic, scoring, map marker |
| `Scripts/Game/GameMode/CaptureTheFlag/CRF_CTF_PickupFlagAction.c` | `ScriptedUserAction` for picking up the flag |
| `Scripts/Game/GameMode/CaptureTheFlag/CRF_CTF_DropFlagAction.c` | `ScriptedUserAction` for voluntarily dropping the flag |

---

## Mission Editor Setup

### 1. Game Mode Component

Add **`CRF_CTFGamemodeManager`** as a component on your GameMode entity.

| Attribute | Default | Description |
|---|---|---|
| `m_sBluforFactionKey` | `BLUFOR` | Faction key for side 1 |
| `m_sOpforFactionKey` | `OPFOR` | Faction key for side 2 |
| `m_sFlagEntityName` | `ctf_flag` | World entity name of the flag prop |
| `m_sBluforDropzoneName` | `ctf_blufor_dropzone` | World entity name of the BLUFOR capture zone marker |
| `m_sOpforDropzoneName` | `ctf_opfor_dropzone` | World entity name of the OPFOR capture zone marker |
| `m_iCapturesToWin` | `1` | Number of captures to declare victory |
| `m_fCaptureRadius` | `5.0` | Metres from the drop zone centre required to score |
| `m_fCaptureHoldTime` | `3.0` | Seconds the carrier must remain in-zone before scoring |
| `m_bHideMapMarker` | `false` | Hide the flag map marker |
| `m_sFlagMarkerText` | `Flag` | Map marker label |

---

### 2. Flag Entity (`ctf_flag`)

Place any prop or GenericEntity in the world and name it **`ctf_flag`** (or match `m_sFlagEntityName`).

Add the following components to that entity:

#### ActionsManagerComponent
Add a `UserActionContext` (e.g. name `"default"`, reasonable interaction radius).

#### additionalActions
Add **both** of these `ScriptedUserAction` entries inside the context:

| Action Class | UIInfo Name |
|---|---|
| `CRF_CTF_PickupFlagAction` | Pick Up Flag |
| `CRF_CTF_DropFlagAction` | Drop Flag |

> **Multiplayer note:** The game mode syncs the flag's world position to all clients via a replicated property. Each client's engine moves its own local copy of the entity. No `RplComponent` is needed on the flag entity — the pre-placed world entity is always visible to all clients without it.

---

### 3. Drop Zone Entities

Place two marker or prop entities in the world:

- **`ctf_blufor_dropzone`** — at the BLUFOR capture point  
- **`ctf_opfor_dropzone`** — at the OPFOR capture point

These can be any entity (empty, invisible proxy, or distinctive prop). Their world-space origin is used as the centre of the capture radius.

---

## Game Flow

1. Safestart ends → server initialises CTF.
2. The flag appears at its placed world position; a gold map marker tracks it in real time.
3. Any player can walk up to the flag and use **"Pick Up Flag"**.
4. While carrying the flag the game mode syncs its position to all clients every 0.25 s; each client moves its local copy of the entity independently (same pattern as HVT transponders).
5. The carrier's faction must bring the flag to **their own** drop zone and remain inside the capture radius for the configured hold time.
6. On capture the flag resets to its spawn position and the score is broadcast.
7. If the carrier dies the flag drops at their last position and can be picked up again.
8. First faction to hit `m_iCapturesToWin` captures wins; the map marker is removed.

---

## Notes

- Both `PickupCTFFlag` and `DropCTFFlag` RPCs are added to `CRF_PlayerRplToAuthorityManager`. No other framework file changes are required.
- `CAPTURETHEFLAG` has been added to `CRF_EGamemode` in `CRF_Gamemode_Enums.c`.
- The drop zone entities are detected only by proximity; no trigger volume component is needed.
- For missions with INDFOR as a third playable faction, set `m_sBluforFactionKey`/`m_sOpforFactionKey` to whichever two factions are competing and the CTF will work correctly.
