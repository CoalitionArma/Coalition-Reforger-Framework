# CRF Raid Gamemode

The **CRF Raid Gamemode** is a TvT-style mission framework for Arma Reforger.  
It features attackers raiding a defended site, dynamic phase transitions, and a fallback **independent militia faction** made from dead attackers.

---

## 📌 Overview

- **Gamemode Component** (`CRF_RaidGamemodeComponent`)  
  Add this to your mission’s **Game Mode Entity**.  
  Configure points to win, retreat thresholds, and attacker/defender factions.

- **Raid Item Component** (`CRF_RaidItemComponent`)  
  Add this to any world prefab you want to be destructible and tied to raid victory conditions.  
  Example targets: ammo caches, comms gear, vehicles, fuel depots.

---

## ⚔️ How It Works

1. **Phase 1 – Raid**
   - Attackers destroy objectives (`CRF_RaidItemComponent`) to earn points.
   - If total destroyed points ≥ **Points To Win**, phase ends.
   - If attackers’ dead ratio exceeds **Retreat Threshold**, phase ends.

2. **Phase 2 – Counterattack**
   - Defenders receive a **respawn wave** at a designated location.
   - Dead attackers respawn as **Independent faction militia**, weak and lightly armed.
   - Attackers must withdraw or fight against both defenders + militia.

---

## 🎮 GameMode Attributes

| Attribute | Description | Default |
|-----------|-------------|---------|
| `m_iPointsToWin` | Total points attackers must destroy to win Phase 1. | `100` |
| `m_fPercentAttackersRetreat` | % of attacker casualties that trigger retreat. | `50` |
| `m_sDefendingSide` | Faction key for defenders. | `OPFOR` |
| `m_sAttackingSide` | Faction key for attackers. | `BLUFOR` |
| `m_sIndependentFaction` | Faction key for militia. | `INDFOR` |

---

## 💥 Raid Item Setup

Attach `CRF_RaidItemComponent` to any prefab you want to act as a raid target.  

### Requirements
- Must have a **Damage Manager Component**, e.g.:
  - `SCR_DestructionMultiPhaseComponent`
  - `SCR_DestructionDamageManagerComponent`
- Must have an **RplComponent** with:
  - ✅ *Enabled*  
  - ✅ *Parent Node From Parent Entity = false*
- Suggested **Base Health**: `2000`
  - Roughly equal to **1 demo charge, 1 AT round, or 5 GP-25 grenades**.

### Attribute
| Attribute | Description | Default |
|-----------|-------------|---------|
| `m_iPointsEarnedWhenDestroyed` | Points awarded when destroyed. | `10` |

---

## 🪖 Faction Balance

- **Attackers (BLUFOR)**  
  - Superior in firepower, explosives, and vehicles.  
  - Must achieve destruction quickly before reinforcements.

- **Defenders (OPFOR)**  
  - Fewer numbers, but fortified positions and respawn reinforcement in Phase 2.  

- **Independent (INDFOR)**  
  - Formed from **dead attackers** during Phase 2.  
  - Weak militia force, rear guard style.  
  - Used to harass retreating attackers.  

---

## 🚀 How To Use

1. Add `CRF_RaidGamemodeComponent` to your **Game Mode Entity**.
2. Place target prefabs (ammo caches, vehicles, comms towers) and attach `CRF_RaidItemComponent`.
3. Configure health and points per object.
4. Set attacker/defender spawn points (`DefenderRespawn`, `IndependentRespawn`).
5. Playtest with around **80–90 players** for best balance.

---

## ✅ Recommended Settings

- **Object Health**: `2000`
- **Demo/AT balance**: 1 explosive should destroy 1 target.
- **Points to Win**: ~`100` (tweak depending on number of targets).
- **Retreat Threshold**: ~`50%` of attackers dead.

---

## 📖 Notes

- The gamemode dynamically sorts militia groups for INDFOR from dead attackers.
- Squad sizes are capped at **8 players per squad**; groups auto-balance evenly.
- Attackers **cannot respawn** after Phase 1; their only second chance is as INDFOR militia.

---

### Example Workflow

1. Attackers raid depot → destroy enough points.  
2. Defenders reinforce with respawn wave.  
3. Dead attackers return as INDFOR militia.  
4. Surviving attackers must withdraw under pressure from both sides.  

---
