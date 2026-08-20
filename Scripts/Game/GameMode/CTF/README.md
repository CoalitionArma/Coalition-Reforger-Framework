# CRF Capture The Flag Gamemode Component

## Overview
The CRF Capture The Flag (CTF) gamemode implements a physical, carryable flag objective for the Coalition Reforger Framework. Players pick up a flag, it attaches to their body, and they must carry it to a base to score. If the carrier dies or disconnects, the flag drops where they fell and can be picked up by anyone.

Two variants are supported, chosen purely by how many flags the mission maker places:

- **Neutral Flag**: One ownerless flag. Either faction may pick it up and must carry it to the **opposing** faction's base to score.
- **Double Flag**: Each faction has its own home flag. The opposing faction must steal it from its home base and carry it back to their **own** base to score.

## Features
- Physical flag entity that visually attaches to the carrier (follows a configurable skeleton bone)
- Automatic drop-at-death/disconnect, pickable up by anyone afterward
- Configurable auto-return timer for dropped flags left untouched
- Auto-spawned map markers at each base, sized to the real capture radius (no manual placement needed)
- Full replication support (RplProp + RplSave/RplLoad for JIP clients)
- Server-authoritative pickup/capture validation routed through the framework's central RPC authority manager
- Score tracking with a one-time "reached the score limit" popup per team - the mission never ends automatically
- Every popup is paired with a configurable sound on every machine - a distinct one for captures/score limit vs. routine pickup/drop/return

## Setup Requirements

### 1. Flag Prefab
Build (or clone an existing banner/flag-pole prefab into) a flag prefab with:
- **RplComponent** - required for replication and for the pickup action to resolve the flag over RPC.
- **CRF_CTF_FlagComponent** - the flag's core script component. Configure:
  - **Owning Faction**: leave empty for the Neutral Flag mode flag, or set BLUFOR/OPFOR for a Double Flag mode home flag.
  - **Carry Bone Name**: skeleton bone the flag follows while carried (default `Spine2`).
  - **Carry Offset**: local offset from that bone.
- **UserActionsComponent** with a **CRF_CTF_PickUpFlagAction** entry, so players get a "Pick Up" prompt.
- (Optional) **Physics** component so a dropped flag settles naturally on uneven terrain.

### 2. Entity Placement
Place your flag prefab(s) directly in the world:

- **Neutral Flag mode**: place exactly one flag entity, `CRF_CTF_FlagComponent.Owning Faction` left empty.
- **Double Flag mode**: place exactly two flag entities, one with Owning Faction `BLUFOR`, one with `OPFOR` (or whichever two factions you configure as Team A/Team B).

Place two entities for the capture zones:
- Entity name: `ctf_teamA_base` (Team A's capture/return point)
- Entity name: `ctf_teamB_base` (Team B's capture/return point)

(Names are configurable on the gamemode component if you'd rather use different entity names.)

**Players get a map marker on each base automatically** - nothing to place by hand. `CRF_CTFGamemodeManager` spawns a **COA_ShapeMarker** on each base at mission start, sized to exactly match the **Capture Radius** attribute (see Base Settings below), so the circle on the map is always the true capture zone, never a second number that can drift out of sync.

The default **Base Marker Prefab** attribute already points at CRF's shared generic shape marker (`PrefabsMissionMaking/Markers/ShapeMarkers/ShapeMarker_Base.et`, the same one Cache Hunt uses for its search-area circles), so this works out of the box with no prefab authoring required. Swap it for a different `COA_ShapeMarker` prefab if you want, or clear the attribute to disable the marker entirely.

Since `COA_ShapeMarker` carries no `RplComponent`, it can't be a server-spawned replicated entity - every client (and listen-server host) spawns its own local copy independently, all sized identically from the same `Capture Radius` attribute. A dedicated server has no map UI, so it skips spawning.

Each marker's zone name (e.g. "BLUFOR Base") is stored via `COA_ShapeMarker.SetShapeName()`, but does not currently show as a hover tooltip: `COA_ShapeMarker`'s default rendering mode (a vector-drawn outline) never builds the widget that owns the hover label, and switching that mode off to fix it was tried and crashes `CreateMapWidget()` with a null instance - that fallback path isn't safe to use from a runtime-spawned marker yet. The circle itself, sized to Capture Radius, still renders correctly either way.

### 3. Component Configuration
Add the `CRF_CTFGamemodeManager` component to your gamemode entity and configure:

#### Mode:
- **Game Mode Type**: `Neutral` or `Double` - purely diagnostic, logs a warning on mission start if the number of placed flags doesn't match.

#### Faction Settings:
- **Team A / Team B**: the two competing factions.

#### Base Settings:
- **Team A/B Base Name**: entity names for each team's capture/return point.
- **Base Marker Prefab**: `COA_ShapeMarker` prefab spawned on each base to mark the capture zone on the map (see the visible marker setup above). Defaults to CRF's shared shape marker; clear to disable.
- **Base Marker Shape**: circle or square outline for the marker.
- **Base Marker Border Width**: outline thickness in pixels.
- **Team A/B Marker Color**: color of each base's marker.
- **Capture Radius**: distance from a base a carrier must be within to capture or return a flag. Also the radius used for the map marker.
- **Enable Debug Visuals**: shows a translucent wireframe sphere sized to Capture Radius on each base entity while the gamemode entity is selected in Workbench - a 3D-space view of the zone against the terrain, complementing the 2D map marker. Editor only, no cost in the shipped game.

#### Scoring:
- **Score To Win**: captures needed for a team to trigger the "reached the score limit" popup. This is an announcement only - capturing keeps working afterward and the mission does not end. `0` disables the announcement entirely. If you want the mission to actually end at some point, wire that up separately (a timer, an admin action, etc.) - this gamemode intentionally never forces it.
- **Require Own Flag At Home To Capture**: Double mode only - blocks scoring with the enemy flag unless your own flag is currently at home.

#### Flag Behavior:
- **Auto Return Seconds**: how long a dropped, untouched flag waits before returning to base on its own. `0` disables this.
- **Tick Interval**: how often (seconds) capture/return/auto-return conditions are checked.

#### Notifications:
- **Notification Sound**: plays on every machine alongside pickup/drop/return popups. Leave empty to disable.
- **Capture Sound**: plays on every machine alongside capture and score-limit popups instead of Notification Sound, so scoring stands out from routine pickup/drop noise. Leave empty to disable.

## Gameplay Flow

1. A player walks up to an eligible flag and picks it up (docked home flags can only be stolen by the opposing faction; dropped flags can be picked up by anyone).
2. The flag attaches to the carrier's back and follows them.
3. If the carrier dies or disconnects, the flag drops at that location for anyone to retrieve.
4. The carrier brings the flag to the correct base (their own, in Double mode; the enemy's, in Neutral mode) to score.
5. When a team reaches Score To Win, everyone sees a "reached the score limit" popup - once per team. Play continues; nothing else changes.

## Technical Details

### Files:
1. `Scripts/Game/GameMode/CTF/CRF_CTF_Game.c` - Main gamemode component (`CRF_CTFGamemodeManager`)
2. `Scripts/Game/GameMode/CTF/CRF_CTF_FlagComponent.c` - Physical flag component (pickup/carry/drop/return state)
3. `Scripts/Game/GameMode/CTF/CRF_CTF_PickUpFlagAction.c` - Pickup interaction
4. `Scripts/Game/!Systems/ModdedOverrides/Coalition Lobby/Replication/CRF_COA_PlayerRplToAuthorityManager.c` - Adds `RequestCTFFlagPickup` / `RpcAsk_RequestCTFFlagPickup`, the client -> server request path for pickups
5. `Scripts/Game/!Systems/ModdedOverrides/Coalition Lobby/UI/CRF_COA_ShapeMarker.c` - Adds `SetShapeName()` (alongside the existing `SetShapeSize`/`SetShapeType`/`SetShapeBorderWidth`), local-only setters for gamemodes that spawn shape markers at runtime

### Network Architecture:
- Flags are placed in the world (not runtime-spawned) and self-register into a static roster in `OnPostInit`.
- Pickup requests go client -> `COA_PlayerRplToAuthorityManager` -> server -> `CRF_CTFGamemodeManager.TryPickUpFlag()`, matching every other CRF gamemode's authority pattern.
- While carried, the flag's position is **not** replicated every frame - every machine (including the server) independently computes the same bone-follow transform from the carrier's own already-replicated position, keeping network cost near zero.
- Drop/return positions and carry state (`m_iFlagState`, `m_iCarrierPlayerId`, `m_vDroppedPosition`) are `[RplProp()]`, with `RplSave`/`RplLoad` for JIP clients.
- Capture, return, and auto-return conditions are evaluated on a server-side timer (distance checks against cached base positions), the same proximity-sweep approach used by the Cache Hunt gamemode - no physical trigger volumes required.
- Each base's `COA_ShapeMarker` is spawned locally on every machine in `OnWorldPostProcess` via `SpawnBaseMarkers()`/`SpawnShapeMarker()`, sized directly from the `Capture Radius` attribute via `SetShapeRadius()` and labelled via `SetShapeName()` - the same local-spawn, no-RplComponent pattern Cache Hunt uses for its attacker search area markers. Skipped on dedicated servers (no map UI to show it on).

### Integration Points:
- Extends `SCR_BaseGameModeComponent`
- Hooks `OnPlayerKilled` / `OnPlayerDisconnected` to drop a carried flag
- Uses `SCR_PopUpNotification` for pickup/drop/capture/score-limit messages, plus `AudioSystem.PlaySound()` alongside every one of them - `BroadcastMessage(message, sound)` takes an optional per-call sound (defaulting to Notification Sound), which `CaptureFlag()`/`CheckScoreLimit()` override with Capture Sound. `BroadcastMessage()` calls `RpcDo_BroadcastMessage()` directly *and* via `Rpc()`, since a Broadcast RPC does not loop back to the calling machine on a listen server - same pattern `CRF_CommunityTagManager.RpcDo_PlayerInfoUpdated` documents. The sound plays independent of the popup, so it's never silent even without a `COA_Gamemode` present.
- Does **not** call `CRF_LoggingManager.SetWinningFaction()` or `COA_Gamemode.AdvanceGamemodeState()` - reaching Score To Win is announced only; the mission never ends automatically.
