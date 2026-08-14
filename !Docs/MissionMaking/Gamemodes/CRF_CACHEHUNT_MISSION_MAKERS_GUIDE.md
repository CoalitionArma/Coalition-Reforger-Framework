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

### Search Narrowing

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Enable Search Narrowing` | Tighten the search areas as the mission runs on | ✓ |
| `Search Narrow Steps` | How many steps to narrow in, each announced | `4` |
| `Search Min Radius Factor` | Final radius as a fraction of the starting radius | `0.25` |
| `Search Narrow Complete At` | Mission fraction by which narrowing is finished | `0.75` |

**Requires a mission time limit** set in the COA gamemode (`Time Limit Minutes`). On an untimed mission there is no clock to narrow against, so narrowing silently does nothing and the areas stay at full size.

Narrowing **does not run during safestart**. The mission clock only starts when safestart ends, so a long briefing does not eat into the attackers' search time. Progress is measured from the moment the round actually goes live.

With the defaults, a 200 m search area on a 60-minute mission steps down to 150 m, 100 m, then 50 m, reaching its tightest at the 45-minute mark and holding there for the last quarter.

See [Why The Areas Narrow](#why-the-areas-narrow) for the reasoning and the trade-off.

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
| `Ammo Costs Supplies` | Charge supplies for ammunition taken from a cache | ✗ |
| `Ammo Supply Cost` | Price per magazine when the faction catalog doesn't price it | `10` |
| `Ammo Classes` | Which gearscript weapon classes stock the caches | Small arms, support, UGL, custom roles |

> Charging supplies needs more than this checkbox — see [Supply Costs](#supply-costs) for the two other things that must be true.

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

## Why The Areas Narrow

Without narrowing, a Cache Hunt round can run its full length with the attackers never finding a single cache — the hunt just quietly fails, and both sides spend an hour on a round that was decided by a dice roll at mission start. Narrowing guarantees the round resolves.

**The trade-off, stated plainly:** narrowing runs on a clock, not on performance, so it partly punishes successful defence. A defending team that holds attackers off for 40 minutes has their cache location progressively handed over as a reward for doing well. That is a real cost. It is accepted because a dead round is worse than an unfair-feeling one, and because the tightest area is still a genuine search rather than a marker on the cache. If your group would rather have the occasional stalled round, turn `Enable Search Narrowing` off — nothing else depends on it.

Two deliberate choices soften it:

- **Stepped, not continuous.** A circle that creeps inward frame by frame is invisible; players just think they misremembered where it was. A step with a notification behind it reads as fresh intel arriving.
- **Announced to both sides.** Defenders learning the net is closing is what makes a stalled round tense instead of merely slow, and it costs them nothing secret.

### The centre moves with the radius

The search area's centre is deliberately offset from the real cache by up to 95% of the radius, so the cache is inside the shape but never at its middle. That makes shrinking less obvious than it looks:

> A 200 m circle offset 150 m from its cache, shrunk to 50 m around a **fixed** centre, is a circle that provably does not contain the cache. Attackers would be searching ground the cache is definitely not on — worse than not narrowing at all.

So the offset scales with the radius rather than staying put. The cache keeps the same *relative* position inside the shape at every size, which guarantees it stays inside. This is why `ComputeSearchCenter()` recomputes from a stored offset vector each time rather than caching a fixed world position.

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

### Never reach for the destination flag's entity

A teleport link spans the map, so **the destination flag is always the one the player is not standing next to** — and Arma Reforger only streams entities near a client. `Replication.FindItem()` returns null for it, and so does any registry lookup: it isn't on that machine at all.

Reading the destination off its entity is therefore guaranteed to fail in multiplayer while working perfectly in Workbench, where one machine holds every entity. The gamemode replicates flag **positions and angles** instead, and the action reads those:

| Instead of | Use |
|---|---|
| `GetHomeFlag() != null` | `gamemode.HasHomeFlagDestination()` |
| `GetCacheFlag(i) != null` | `gamemode.HasCacheFlagDestination(i)` |
| `destinationFlag.GetOwner().GetOrigin()` | `gamemode.GetFlagTransform(i, position, angles)` |
| `destinationFlag.AreEnemiesNear()` | `gamemode.AreEnemiesNearFlag(i)` |

Only the flag the player is **standing at** is resolved as a local entity, which is safe by definition.

### Flag placement follows the cache, not the terrain

Caches get placed indoors — basements especially — and a flag placed by terrain height ends up on the ground *above* the cache, poking up through the building. Terrain height is therefore not used anywhere in flag placement.

Each of 8 candidate bearings around the cache is instead:

1. **Wall-checked** — traced horizontally from the cache at waist height, so a spot on the far side of a basement wall isn't treated as the same room.
2. **Probed vertically** from the cache's own height (+1.5 m down to −3 m), landing on the basement floor indoors and the ground outdoors.
3. **Level-checked** — rejected if the surface found is more than 2.5 m from the cache's height, which is what filters out the terrain above a basement.
4. **Scored by slope** from the trace normal, keeping the flattest.

If no candidate qualifies, the flag goes at the cache's own position and logs a warning — ugly, but reachable, which a flag on the terrain overhead is not. Raising `Flag Spawn Distance` gives the search more room.

Poles are also spawned **upright**, without `SCR_TerrainHelper.OrientToTerrain()`. That builds its basis from the terrain normal and lays the entity flat against the slope — right for a crate, wrong for a pole, which it tilts into the hillside.

### Flag state is replicated by the gamemode

Which cache a flag serves, and whether enemies are near it, are **not** replicated by the flag itself. Cache flags are spawned at runtime, and their component state did not reliably reach clients of a dedicated server — leaving every client's flags unassigned, so no flag reported itself as the home flag and every teleport action hid itself.

Instead the gamemode publishes a **flag roster** — the flags' `RplId`s, their cache indices, and an enemy-proximity bitmask — and each client stamps its own copies from it in the local poll. The gamemode component is world-baked, so its `RplProp`s are dependable; the same channel already carries the cache positions the markers use.

Anything that needs a flag's role should read it through `CRF_CacheHunt_FlagComponent` as normal — the roster keeps that accurate on every machine. Don't add per-flag replication back.

---

## Rearm

Each cache's `SCR_ArsenalComponent` gets its **item list rebuilt from the defending faction's assigned gearscript**. The list is compiled once at mission start by walking every `Magazine Array` in the gearscript config:

- Rifles, Rifle UGLs, Carbines, Pistols, Sniper
- AR, MMG, HMG, AT, MAT, HAT, AA
- Every custom role's primary, secondary, and pistol

Only **ammunition** goes in — no weapons, clothing, medical, or radios. `Ammo Costs Supplies` controls whether it costs anything; the price comes from the faction's entity catalog where it exists, otherwise from `Ammo Supply Cost`.

### Ammo Classes — why Iglas turn up

A launcher's magazine array holds its **rockets or missiles**. An Igla round is a magazine in exactly the way a rifle mag is, so stocking a cache from every magazine array in the gearscript hands every defender unlimited AT and AA.

`Ammo Classes` controls which classes contribute:

| Flag | Covers |
|------|--------|
| `SMALL_ARMS` | Rifles, carbines, pistols, sniper rifles |
| `SUPPORT` | AR, MMG, HMG |
| `GRENADE_LAUNCHER` | Rifle-mounted UGL rounds |
| `ANTI_TANK` | AT, MAT, HAT rockets |
| `ANTI_AIR` | AA missiles |
| `CUSTOM_ROLES` | Whatever the gearscript's custom roles carry |

**`ANTI_TANK` and `ANTI_AIR` are off by default.** Turn them on deliberately — a cache that resupplies AT is a very different objective to one that resupplies rifle mags, and it changes how expensive the cache is for attackers to approach with vehicles.

> Note `CUSTOM_ROLES` pulls whatever those roles carry, which may itself include launchers. If AT rounds still appear with `ANTI_TANK` off, check the gearscript's custom roles.

> **Important:** an arsenal serves a *virtual* item list, not the physical contents of the box's storage. Spawning magazines into the box's inventory does **not** make them appear in the arsenal UI — the list itself has to be replaced. This is why the cache prefab needs an `SCR_ArsenalComponent` and not just an inventory.

### The arsenal is filled on every machine, not replicated

An arsenal's overwrite item list is **plain script state that vanilla never replicates** — it is normally static prefab data, so nothing exists to sync it. Filling it on the server does nothing for anyone else: every client keeps serving the list its own prefab authored.

So each machine fills its own copy. The server does it when the cache spawns; each client does it from the marker poll, resolving its copy of the cache through a replicated `RplId`. Both derive the list from the same replicated gearscript, so they agree without the list ever going over the wire.

> **This class of bug is invisible in Workbench.** There the host *is* the client and reads the server's own objects, so server-only state looks like it works. The same mission on a dedicated server showed the full default arsenal. Anything that mutates script state on a spawned entity needs testing on a real server, not just in Workbench.

### Building an arsenal list in script

Constructing `SCR_ArsenalItemStandalone` entries at runtime rather than authoring them in a prefab hits three traps, and **all three fail the same silent way — an arsenal full of blank tiles, no error**. `CRF_SCR_Arsenal.c` handles them; they're written up here because anything else building an arsenal in script will hit them too.

| Trap | Why it bites | Fix |
|------|--------------|-----|
| `m_ItemResource` never loads | The constructor loads it from the resource name, but a `new`-ed instance has no name yet, so it early-returns. Assigning the name afterwards doesn't re-run it. | Load the `Resource` explicitly after setting the name |
| Type and mode are `0` | `[Attribute("2")]` defaults apply on **config deserialisation**, not on `new`. The list filters with `GetItemType() & typeFilter`, and zero matches nothing. | Set `m_eItemType` and `m_eItemMode` explicitly |
| Stale filter cache | `SCR_ArsenalItemListConfig` memoises filtered results in `m_mArsenalItemsByType`. Refilling the items without clearing it keeps serving the old results. | `m_mArsenalItemsByType.Clear()` after refilling |

Cache ammunition goes in as `EQUIPMENT` / `AMMUNITION`. Both flags must be present in the cache prefab's `m_eSupportedArsenalItemTypes` and `m_eSupportedArsenalItemModes` or the entries are filtered straight back out — the shipped prefab's `493046` / `94` include both. `RefreshArsenal()` is called afterwards to rebuild the served list and notify clients.

Vanilla reference: `scripts/Game/Components/Arsenal/` in the Arma-Reforger-Script-Diff repository.

Because the list is virtual, an arsenal never runs dry — there is no restock timer. Use `Ammo Costs Supplies` if you want resupply rationed rather than unlimited.

### Supply Costs

Two separate things have to be true before a cache charges anything, and **both fail silently**.

**1. The cache must have SUPPLIES enabled.** An arsenal only charges when its owner's `SCR_ResourceComponent` has the SUPPLIES resource type enabled (`SCR_ArsenalComponent.IsArsenalUsingSupplies()`). The shipped `CRF_CacheHunt_Cache.et` ships with SUPPLIES in its *disabled* list, so out of the box every take is free regardless of any price. `Ammo Costs Supplies` flips this at runtime, so you don't need to edit the prefab — but if the prefab also disallows changing that resource type, the toggle no-ops and you get:

```
[CRF_CacheHunt] Could not set supply usage to true on cache prefab '...'.
Its SCR_ResourceComponent disallows changing SUPPLIES - fix it in the prefab instead.
```

**2. The cache must be on the defending faction.** The arsenal UI prices a slot by looking the item up in **the arsenal's assigned faction's** ITEM entity catalog, and returns 0 outright when it isn't there. The shipped cache inherits `ArsenalBox_US`, so left alone it asks the *US* catalog for the defenders' magazines, finds nothing, and prices everything at zero. The gamemode now sets each cache's `FactionAffiliationComponent` to the defending side at spawn to fix this.

**3. Most gearscript magazines aren't priced anywhere.** The arsenal UI prices a slot *only* from the assigned faction's ITEM entity catalog and does a bare `return 0` when the item isn't in it. Gearscript magazines generally are not — no catalog in CRF or COALITION-Lobby prices them — so they display as free even with everything else correct.

`Ammo Supply Cost` is the price used for those. A magazine the faction catalog *does* price keeps the catalog's price, so caches stay consistent with the rest of the mission economy where that economy exists.

**To price a magazine properly**, add an `SCR_ArsenalItem` entry for it in the defending faction's ITEM entity catalog:

```
SCR_ArsenalItem {
  m_eItemMode AMMUNITION
  m_iSupplyCost 5
}
```

The startup log tells you how many fell back:

```
[CRF_CacheHunt] N of M magazine(s) are not priced in the 'OPFOR' ITEM entity catalog;
they use the Ammo Supply Cost of 10.
```

> **Why an override was needed.** Vanilla's *purchase* path already falls back to the arsenal entry's own cost when there's no catalog entry, but the *display* path doesn't — so the price shown and the price charged disagreed. `CRF_SCR_Arsenal.c` overrides `SCR_ArsenalInventorySlotUI.GetTotalResources()` to apply the same fallback, so the number on the slot is the number the player pays.

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
- **Ammo Costs Supplies**: off for a casual op; on if you want cache resupply to draw down a supply pool rather than be unlimited. Tune the price with `Ammo Supply Cost`, or per-magazine in the faction's entity catalog.

---

## Troubleshooting

| Log message | Cause |
|-------------|-------|
| `No cache positions could be resolved` | Spawn point names don't match any entity, or the randomisation centre entity is missing |
| `Cache prefab '…' has no SCR_DamageManagerComponent` | Cache prefab can't be destroyed — pick a destructible prefab |
| `Cache prefab '…' has no SCR_ArsenalComponent` | Rearm is on but the prefab has no arsenal — use a cache prefab with one, or turn rearm off |
| `N of M magazine(s) have no arsenal entry in the '…' ITEM entity catalog` | Those magazines are free because the defending faction's catalog doesn't price them. Add `SCR_ArsenalItem` data for them in that catalog |
| `No ammunition resolved from the '…' gearscript` | The faction's gearscript has no magazine entries, or wasn't loaded — check the COA gamemode's gearscript assignment |
| `Defender home flag '…' was not found` | You didn't place the flag, or its entity name doesn't match the attribute |
| `Flag pole prefab '…' is missing CRF_CacheHunt_FlagComponent` | A custom flag prefab was used without the component |
| `Search marker prefab '…' is not a COA_ShapeMarker` | The marker prefab doesn't inherit `ShapeMarker_Base.et` |
| Teleport actions missing on a dedicated server but fine in Workbench | The flag's cache index isn't reaching clients — see [Flag state is replicated by the gamemode](#flag-state-is-replicated-by-the-gamemode) |
| Caches show the full default arsenal on a dedicated server | The client hasn't filled its own copy — see [The arsenal is filled on every machine](#the-arsenal-is-filled-on-every-machine-not-replicated) |
| `No gearscript assigned to the defending faction` | The faction has no gearscript in the COA gamemode settings |
| `Could not find a spot for randomised cache N` | Min separation is too large for the radius band — caches still spawn, just closer together |
