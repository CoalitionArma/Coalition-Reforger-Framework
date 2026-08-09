# CRF Cache Hunt Gamemode

Attackers hunt and destroy up to **five hidden ammunition caches**. Defenders know exactly where the caches are, rearm from them, and fast-travel between their main spawn flag and each cache using CRF flag poles.

---

## Quick Setup

1. Add `CRF_CacheHuntGamemodeManager` to your **Game Mode Entity**
2. Set **Attacking Side** and **Defending Side**
3. Place cache locations — either:
   - Named empty entities listed in **Cache Spawn Point Names**, or
   - A single named center entity plus **Randomize Caches**
4. Place `CRF_CacheHunt_FlagPole.et` at the defender main spawn and name it `CacheHunt_HomeFlag`
5. Playtest

Everything else — caches, attacker search circles, defender markers, per-cache teleport flags, and cache ammunition — is spawned and wired up automatically at mission start.

---

## How It Works

1. After **Init Delay Ms**, the server resolves cache positions and spawns the **Cache Prefab** at each one.
2. Each cache's damage manager is hooked, so any destruction is reported straight back to the gamemode — no polling.
3. Attackers get a large **world-scaled search shape** per cache — a `COA_ShapeMarker` circle or square. Its centre is offset a random distance from the real cache, so the cache is always *inside* the shape but never *at* its centre.
4. Defenders get an **exact map marker** on every surviving cache, labelled `Cache A`, `Cache B`, … to match the teleport actions on their flag.
5. A **teleport flag pole** is spawned next to each cache and linked to the defender home flag.
6. Each cache's **arsenal item list is rebuilt from the defending faction's assigned gearscript**, so defenders pull the ammunition their loadouts actually use.
7. When a cache is destroyed, its search circle and teleport flag are removed, a notification goes out, and — if enabled — a **respawn wave fires for both sides**, putting dead players back at their faction's spawn flag.
8. When the last cache falls, an attacker-victory notification is broadcast.

---

## Cache Placement

### Fixed positions (default)

Place empty entities where you want caches and list their names in **Cache Spawn Point Names**. Entries beyond the first five are ignored.

```
Cache Spawn Point Names:
  [0] "Cache_Farmhouse"
  [1] "Cache_Quarry"
  [2] "Cache_Church"
```

### Randomised positions

Tick **Randomize Caches**, place one entity as the centre, and name it to match **Random Center Entity Name**.

| Setting | Meaning |
|---------|---------|
| `Random Cache Count` | How many caches to scatter (1–5) |
| `Random Min Radius` | Closest a cache can spawn to the centre |
| `Random Max Radius` | Furthest a cache can spawn from the centre |
| `Random Min Separation` | Minimum distance between any two caches |

The placer makes 40 attempts per cache to satisfy the separation rule. If it can't, it logs a warning and places the cache anyway rather than dropping it — so a too-tight separation degrades gracefully instead of silently reducing the cache count.

> **Note:** Randomisation happens **per mission start**, on the server. Both sides get a fresh layout every round.

---

## Gamemode Attributes

### Factions

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Attacking Side` | Side hunting the caches | `BLUFOR` |
| `Defending Side` | Side that owns the caches: exact markers, rearm, teleport | `OPFOR` |

### Caches

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Cache Prefab` | Prefab spawned as a cache | `CRF_CacheHunt_Cache.et` |
| `Cache Spawn Point Names` | Names of entities marking cache positions (max 5) | — |
| `Randomize Caches` | Scatter caches around a centre entity instead | ✗ |
| `Random Center Entity Name` | Entity to scatter around | `CacheHunt_Center` |
| `Random Cache Count` | Caches to scatter (1–5) | `3` |
| `Random Min Radius` | Min distance from centre (m) | `100` |
| `Random Max Radius` | Max distance from centre (m) | `600` |
| `Random Min Separation` | Min distance between caches (m) | `150` |
| `Destroy Fuse Seconds` | Delay between the Destroy Cache action finishing and the blast | `6` |

### Attacker Markers

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Enable Search Markers` | Give attackers a search area per cache | ✓ |
| `Search Marker Prefab` | Marker entity spawned as the shape | `ShapeMarker_Base.et` |
| `Search Area Radius` | Shape radius in metres | `200` |
| `Search Offset Factor` | How far the shape's centre drifts from the cache, as a fraction of the radius. `0` centres it exactly on the cache | `0.75` |
| `Search Marker Shape` | `CIRCLE` or `SQUARE` | `CIRCLE` |
| `Search Marker Border Width` | Outline thickness in pixels | `2.5` |
| `Search Marker Color` | Outline colour | red |

See [How The Search Areas Are Drawn](#how-the-search-areas-are-drawn) for why these are spawned per client.

### Defender Markers

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Enable Defender Markers` | Exact cache markers for the defending side | ✓ |
| `Defender Marker Icon` | Icon used for those markers | grenades icon |

### Teleport

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Enable Teleport Flags` | Auto-spawn a flag at each cache | ✓ |
| `Flag Pole Prefab` | Flag prefab used at both ends | `CRF_CacheHunt_FlagPole.et` |
| `Defender Home Flag Name` | Name of the flag you place at the defender main spawn | `CacheHunt_HomeFlag` |
| `Flag Spawn Distance` | How far the flag sits from its cache (m) | `5` |
| `Enemy Proximity Radius` | Enemies this close to either flag disable the teleport (m) | `150` |
| `Flag Check Interval` | How often proximity is re-evaluated (s) | `2` |

### Rearm

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Enable Cache Rearm` | Fill each cache's arsenal with gearscript ammunition | ✓ |
| `Ammo Supply Cost` | Supply cost per magazine taken. `0` makes them free | `0` |

### Misc

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Respawn On Cache Destroyed` | Fire a respawn wave for both sides on each cache kill | ✓ |
| `Init Delay Ms` | Delay before caches spawn, letting the world finish loading | `2000` |

---

## Cache Prefab Requirements

The default is `{66F430BAD8FD6BB7}Prefabs/Props/Military/Arsenal/AmmoBoxes/CRF_CacheHunt_Cache.et`. Any replacement must have:

- ✅ A damage manager — `SCR_DestructionMultiPhaseComponent` or `SCR_DamageManagerComponent`.
  Without one the cache can never be destroyed and the objective can't complete. The gamemode logs an `ERROR` if this is missing.
- ✅ An `RplComponent` with *Rpl State Override = Runtime*
- ✅ An `SCR_ArsenalComponent` — only needed if **Enable Cache Rearm** is on. Missing it logs an `ERROR` and skips the arsenal; everything else still works.

Suggested base health: **2000** — roughly one demo charge, one AT round, or five GP-25 grenades.

### The shipped cache prefab

`CRF_CacheHunt_Cache.et` inherits `ArsenalBox_US.et` and keeps `SCR_ArsenalComponent` **enabled** — the gamemode replaces its authored item list at runtime, so whatever is listed in the prefab is only a fallback for when rearm is disabled.

It also needs a **`CRF_CacheHunt_DestroyAction`** in its `ActionsManagerComponent`. Without it, attackers can only destroy caches with explosives.

If you roll your own, make it in Workbench rather than hand-editing the `.et`: prefab inheritance keys overrides off the base prefab's component GUIDs, so a hand-written override either duplicates components or silently fails.

---

## Destroying Caches

Attackers have two routes:

1. **Explosives** — any damage that drives the cache's damage manager to `EDamageState.DESTROYED`.
2. **The Destroy Cache action** — `CRF_CacheHunt_DestroyAction` on the cache prefab, shown only to the attacking faction. Set its `Duration` in the prefab to taste (10–20 s makes it a contested objective rather than a free tap).

Both funnel into the same bookkeeping, and both are idempotent — a cache already down is ignored.

### The demolition fuse

The action doesn't detonate the cache on the spot. Finishing it lights a fuse of **`Destroy Fuse Seconds`**, and only then does the cache blow — otherwise the attacker who set the charge is standing inside their own explosion.

Two timers, doing different jobs:

| Timer | Set in | What it is |
|-------|--------|------------|
| `Duration` | The action, on the cache prefab | How long the attacker stands there planting the charge — the window defenders get to interrupt them |
| `Destroy Fuse Seconds` | The gamemode | How long they have to run once it's planted |

During the fuse the cache still counts as **alive**: it stays on the defenders' map, keeps its search shape, and its teleport flag still works. Markers, the respawn wave, and the destroyed count all fire at detonation, not at planting. A second attacker triggering the action on a cache that's already fusing is ignored.

Set `Destroy Fuse Seconds` to `0` for the old instant behaviour. Caches killed with explosives are unaffected — that blast is the attacker's own charge and already has its own timing.

The action is a **request, not an order**. The client sends only the cache's `RplId`; the server then re-checks that the requesting player is on the attacking side and is within 8 m of the cache before anything happens. Rejections are logged:

```
[CRF_CacheHunt] Rejected a Destroy Cache request from player N - not on the attacking side.
[CRF_CacheHunt] Rejected a Destroy Cache request from player N - too far from the cache.
```

---

## How The Search Areas Are Drawn

The search areas are COALITION-Lobby `COA_ShapeMarker` entities (`ShapeMarker_Base.et`), drawn as real vector outlines on the map rather than stretched icons, so they stay crisp at any zoom.

`COA_ShapeMarker` carries no `RplComponent`, so it can't be a server-spawned replicated entity. The gamemode works with that rather than around it:

1. The **server** picks each search area's centre once (`ComputeSearchCenter`) and publishes the list as a replicated property.
2. Each **client** spawns its own local shape marker at those centres.

Because the server owns the centres, every attacker searches the same ground. And because only attackers ever spawn one, non-attackers don't have the entity at all — there's no faction-visibility filter to get wrong, and nothing on a defender's machine to accidentally reveal.

Local markers are reconciled on a 2-second poll, so switching slots or respawning onto the other side adds or removes the shapes correctly, and a destroyed cache's shape disappears when its centre drops off the replicated list.

Size, shape, and border need setters `COA_ShapeMarker` doesn't ship with, so the CRF adds them in `Scripts/Game/!Systems/ModdedOverrides/Coalition Lobby/UI/CRF_COA_ShapeMarker.c`:

| Method | Purpose |
|--------|---------|
| `SetShapeRadius(float)` | Circle radius in metres (sets both axes to the diameter) |
| `SetShapeSize(float x, float y)` | Independent X/Y extents |
| `SetShapeType(COA_EMapShapeMarkerType)` | `CIRCLE` or `SQUARE` |
| `SetShapeBorderWidth(float)` | Outline thickness |

These are **local only** — they set fields on the calling machine and do not replicate. That's correct for an unreplicated marker; don't add `Rpc()` calls to them.

> **Note:** the base `COA_ManualMarker.SetSize()` sets `m_fWorldSize`, which the shape marker's draw path ignores in favour of `m_fWorldSizeX` / `m_fWorldSizeY`. Use `SetShapeRadius()` / `SetShapeSize()` instead.

---

## Teleport Flags

### Setup

Place **one** `CRF_CacheHunt_FlagPole.et` at the defender main spawn and set its entity name to match **Defender Home Flag Name**. That's the only flag you place by hand; the rest are auto-spawned next to each cache.

### Behaviour

| From | Action shown | Leads to |
|------|--------------|----------|
| Home flag | `Teleport to Cache A` … `Teleport to Cache E` | The flag at that cache |
| Cache flag | `Teleport to Main Spawn` | The home flag |

Cache letters are **stable for the whole mission** and shared with the defenders' map markers, so `Cache B` on the map is the same cache as `Teleport to Cache B` on the flag. Destroying cache A does not renumber the others — B stays B, and the now-dead A simply disappears from both the map and the flag.

- Actions are **defender-only**. Attackers never see them.
- A home-flag action **hides itself** once its cache is destroyed.
- If attackers are within **Enemy Proximity Radius** of *either* end, the action greys out with a reason:
  - `Enemies nearby - teleport disabled` — enemies at your end
  - `Enemies at the destination - teleport disabled` — enemies at the far end

Proximity is evaluated **on the server** and only the resulting boolean is replicated, so a client can never mine the flag state for enemy positions.

The flag prefab ships with five teleport actions (one per possible cache). On a cache flag, only slot 0 is used, so the other four stay hidden. You don't need to configure them.

---

## Rearm

Each cache's `SCR_ArsenalComponent` gets its **item list rebuilt from the defending faction's assigned gearscript**. The list is compiled once at mission start by walking every `Magazine Array` in the gearscript config:

- Rifles, Rifle UGLs, Carbines, Pistols, Sniper
- AR, MMG, HMG, AT, MAT, HAT, AA
- Every custom role's primary, secondary, and pistol

Only **ammunition** goes in — no weapons, clothing, medical, or radios. Each entry costs `Ammo Supply Cost` supplies (0 by default, i.e. free).

> **Important:** an arsenal serves a *virtual* item list, not the physical contents of the box's storage. Spawning magazines into the box's inventory does **not** make them appear in the arsenal UI — the list itself has to be replaced. This is why the cache prefab needs an `SCR_ArsenalComponent` and not just an inventory.

Because the list is virtual, an arsenal never runs dry — there is no restock timer. Use `Ammo Supply Cost` if you want resupply to be rationed rather than unlimited.

> **Note:** the caches are not faction-locked. An attacker who has already found a cache can use it, but at that point they're there to destroy it anyway.

---

## Scripting API

```enforce
CRF_CacheHuntGamemodeManager cacheHunt = CRF_CacheHuntGamemodeManager.GetInstance();
if (cacheHunt)
{
    int remaining = cacheHunt.GetCachesRemaining();
    int destroyed = cacheHunt.GetCachesDestroyed();
    int total     = cacheHunt.GetCacheTotal();

    bool isDefender = cacheHunt.IsDefender(someEntity);
}
```

| Method | Returns |
|--------|---------|
| `GetInstance()` | The active gamemode manager, or `null` |
| `GetCacheTotal()` | How many caches spawned |
| `GetCachesDestroyed()` | How many are down |
| `GetCachesRemaining()` | How many are left |
| `GetAttackingSide()` / `GetDefendingSide()` | Faction keys |
| `IsDefender(IEntity)` | Whether an entity is on the defending side |
| `IsLocalPlayerDefender()` | Whether the local player is on the defending side |
| `GetCacheLabel(int)` | `0` → `"A"`, `1` → `"B"`, … |

---

## Recommended Settings

- **Search Area Radius**: `200` m with **Search Offset Factor** `0.75` — attackers get a ~12 hectare box to sweep, and the centre is a poor guess.
- **Enemy Proximity Radius**: `150` m — close enough that a probing attacker shuts down reinforcement, far enough that defenders aren't locked out by a stray scout.
- **Random Min Separation**: at least `2 ×` the search radius, so the circles don't overlap into one blob.
- **Cache count**: 3 for a 60–90 minute session, 5 for a long op.
- **Ammo Supply Cost**: `0` for a casual op; raise it if you want cache resupply to draw down a supply pool rather than be unlimited.

---

## Troubleshooting

| Log message | Cause |
|-------------|-------|
| `No cache positions could be resolved` | Spawn point names don't match any entity, or the randomisation centre entity is missing |
| `Cache prefab '…' has no SCR_DamageManagerComponent` | Cache prefab can't be destroyed — pick a destructible prefab |
| `Cache prefab '…' has no SCR_ArsenalComponent` | Rearm is on but the prefab has no arsenal — use a cache prefab with one, or turn rearm off |
| `No ammunition resolved from the '…' gearscript` | The faction's gearscript has no magazine entries, or wasn't loaded — check the COA gamemode's gearscript assignment |
| `Defender home flag '…' was not found` | You didn't place the flag, or its entity name doesn't match the attribute |
| `Flag pole prefab '…' is missing CRF_CacheHunt_FlagComponent` | A custom flag prefab was used without the component |
| `Search marker prefab '…' is not a COA_ShapeMarker` | The marker prefab doesn't inherit `ShapeMarker_Base.et` |
| `No gearscript assigned to the defending faction` | The faction has no gearscript in the COA gamemode settings |
| `Could not find a spot for randomised cache N` | Min separation is too large for the radius band — caches still spawn, just closer together |
