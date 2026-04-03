# CRF Raid Gamemode

The **CRF Raid Gamemode** is a supply-destruction objective framework for Arma Reforger. Attackers raid a defended site, destroying tagged supply objects to accumulate a destruction percentage toward a configurable win threshold. All tracking is fully event-driven — no periodic sphere queries.

---

## 📌 Overview

- **Gamemode Component** (`CRF_RaidGamemodeComponent`)  
  Add this to your mission's **Game Mode Entity**.  
  Manages the supply pool, win condition, and HUD updates.

- **Raid Item Component** (`CRF_RaidItemComponent`)  
  Add this to any destructible prefab you want to count toward the raid objective.  
  Example targets: ammo caches, comms gear, vehicles, fuel depots.

---

## ⚙️ How It Works

1. On scenario load, each `CRF_RaidItemComponent` calls `RegisterRaidItem()` on the gamemode manager during its `EOnInit`, adding itself to the tracked list.
2. When the **first object is destroyed**, the manager sums all registered supply values to finalize the **total supply pool**. This lazy approach means the total is always accurate regardless of how long the server took to spawn entities.
3. Each subsequent destruction fires `OnItemDestroyed()`, which adds the item's supply value to the running destroyed total, broadcasts a HUD widget update to all clients, and checks the win condition.
4. If the destroyed percentage meets or exceeds `m_fWinThresholdPercent`, a victory notification is broadcast to all clients.

> **Note:** The HUD progress bar is threshold-relative — a full bar always means the win condition has been met, regardless of the configured threshold percentage.

---

## 🎮 GameMode Attributes

| Attribute | Description | Default |
|-----------|-------------|---------|
| `m_fWinThresholdPercent` | Percentage of total supply that must be destroyed for an attacker victory. | `50` |
| `m_sAttackingSide` | Faction key for the attacking side. | `BLUFOR` |
| `m_sDefendingSide` | Faction key for the defending side. | `OPFOR` |

---

## 💥 Raid Item Setup

Attach `CRF_RaidItemComponent` to any prefab you want to act as a raid target.

### Requirements

- Must have a **Damage Manager Component**, e.g.:
  - `SCR_DestructionMultiPhaseComponent`
  - `SCR_DamageManagerComponent`
- Must have an **RplComponent** with:
  - ✅ *Enabled*
  - ✅ *Parent Node From Parent Entity = false*
- Suggested **Base Health**: `2000`
  - Roughly equal to **1 demo charge, 1 AT round, or 5 GP-25 grenades**.

### Attributes

| Attribute | Description | Default |
|-----------|-------------|---------|
| `m_iSupplyValue` | Supply value this object contributes to the total pool. Destroyed supply is compared against the total to determine destruction percentage. | `10` |
| `m_sItemName` | Label displayed on the tactical map marker for this target. | `Target` |

> **Note:** Map markers are placed automatically on `EOnInit` and removed automatically on destruction. No manual marker management needed.

---

## 🚀 How To Use

1. Add `CRF_RaidGamemodeComponent` to your **Game Mode Entity** and set `m_fWinThresholdPercent` to the desired destruction threshold.
2. Place target prefabs in the world and attach `CRF_RaidItemComponent` to each one.
3. Set `m_iSupplyValue` on each item to reflect its relative importance. Higher-value targets influence the destruction percentage more.
4. Set object health appropriate to your explosive balance (recommended: `2000`).
5. Set attacker and defender faction keys on the gamemode component to match your mission factions.
6. Playtest — adjust supply values and win threshold to tune the difficulty curve.

---

## ✅ Recommended Settings

- **Object Health**: `2000` — 1 explosive should destroy 1 target.
- **Win Threshold**: `50%` — attackers must destroy half the total supply pool.
- **Supply Values**: Use varied values (e.g. `10`, `25`, `50`) so high-value targets create meaningful decision-making.
- **Player count**: 80–90 players for best balance.

---

## 📖 Technical Notes

- All tracking is **event-driven** — no periodic sphere queries. Performance scales with destruction events, not world population.
- The supply total is finalized **on the first destruction event**, not at startup. This ensures accuracy on slow-loading servers where entity spawning may not be complete at scenario init.
- If an entity is removed from the world without reaching `EDamageState.DESTROYED` (e.g. scenario reload or cleanup), no supply is counted. This prevents false wins on restart.
- The `WARNING` log `[CRF_Raid] Total supply is 0 after finalization` indicates no `CRF_RaidItemComponent` entities successfully registered — check that they have a valid `SCR_DamageManagerComponent` and that the gamemode component is present in the world.