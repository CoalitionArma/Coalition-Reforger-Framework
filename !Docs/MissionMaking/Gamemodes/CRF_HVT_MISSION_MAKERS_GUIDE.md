# CRF High Value Target Gamemode

Track AI, Player, Object, or Element (whole slotting group) targets with periodic transponder marker pings.

> ⚠️ **Player HVT detection matches ANY player using the configured prefab!** Use a unique prefab + faction filter.

---

## Quick Setup

1. Add `CRF_HighValueTargetGamemodeManager` to your **Game Mode Entity**
2. Place empty **transponder entities** in world with unique names
3. Add entries to **HVT Entries** array (one per HVT)
4. Configure each entry's type, prefab, transponder name, and marker text

---

## Global Settings

| Setting | Description | Default |
|---------|-------------|---------|
| Enable Transponder Marker | Enable map markers | ✓ |
| Target Type | Notification text (HVT/VIP) | HVT |
| Disable Damage | Make AI/Player HVTs invulnerable | ✗ |
| Time Between Pings | Seconds between marker updates | 360 |
| Filter Faction | Only show markers to searcher faction (when entry Hunter Faction is NONE) | ✗ |
| Searcher Faction Key | Faction that sees markers (if filtered) | BLUFOR |
| Auto Set Winner On Elimination | Set winner when an ELEMENT is fully eliminated | ✗ |
| Auto End Mission On Elimination | Advance to AAR when auto-win triggers | ✗ |
| Hvt Prefab Yaw | Rotation for spawned AI | 0 0 0 |
| Set Unconcious | Set AI HVTs unconscious on spawn | ✓ |

---

## Entry Configuration (HVT Entries)

| Setting | Description |
|---------|-------------|
| Entry Type | `AI` / `PLAYER` / `OBJECT` / `ELEMENT` |
| Faction | Target faction — marker color, player filter, or ELEMENT target faction |
| Target Callsign | ELEMENT only (prefab not used) |
| Hvt Prefab | AI / PLAYER / OBJECT only — leave empty for ELEMENT |
| Transponder Entity Name | Transponder entity name (spawn location / marker anchor) |
| Marker Text | Map marker text |

**Entry Types:**
| Type | Spawns At | Death Handling | Notes |
|------|-----------|----------------|-------|
| AI | Transponder | Yes | Can set unconscious |
| PLAYER | At normal faction spawn flag | Yes | Matches by prefab+faction |
| OBJECT | Transponder | No | Position tracking only |
| ELEMENT | Slotting group centroid | Yes (whole group) | Eliminated when all occupied slots are dead; all other factions hunt automatically |

**Marker visibility:** By default, every faction except the entry's target faction sees the marker. Enable **Filter Faction** to restrict all markers to a single searcher faction instead (legacy behaviour).

**Faction Colors:** BLUFOR=Blue, OPFOR=Red, INDFOR=Green, CIV=Purple, NONE=Blue

---

## Examples

**AI HVT:**
```
HVT Entries [0]:
  Entry Type: AI
  Hvt Prefab: Character_RUS_Officer.et
  Faction: OPFOR
  Transponder Entity Name: "HVT_Location"
  Marker Text: "Enemy Commander"
```

**Player VIP:**
```
HVT Entries [0]:
  Entry Type: PLAYER
  Hvt Prefab: CRF_VIP.et
  Faction: BLUFOR
  Transponder Entity Name: "VIP_Transponder"
  Marker Text: "VIP Signal"
```

**Object (Radio/Intel):**
```
HVT Entries [0]:
  Entry Type: OBJECT
  Hvt Prefab: Radio_ANPRC77.et
  Faction: NONE
  Transponder Entity Name: "Intel_Transponder"
  Marker Text: "Intel Package"
```

**Element hunt (mirror TVT):**
```
HVT Entries [0]:
  Entry Type: ELEMENT
  Faction: OPFOR
  Target Callsign: 1PLT
  Transponder Entity Name: "OPFOR_PLT_Transponder"
  Marker Text: "OPFOR Platoon"

HVT Entries [1]:
  Entry Type: ELEMENT
  Faction: BLUFOR
  Target Callsign: 1PLT
  Transponder Entity Name: "BLUFOR_PLT_Transponder"
  Marker Text: "BLUFOR Platoon"
```

**Three-way element hunt (both sides vs INDFOR):**
```
HVT Entries [0]:
  Entry Type: ELEMENT
  Faction: INDFOR
  Target Callsign: 1PLT
  ...
```
Only one entry is needed — BLUFOR and OPFOR both hunt INDFOR automatically. Enable **Auto Set Winner On Elimination** — the faction that scores the final kill wins.

---

## Behavior

- **Markers** sync 5s before each ping cycle
- **AI/Player death** immediately removes marker + broadcasts notification
- **Objects** always "alive" if entity exists (no death detection)
- **Player detection** re-checks every 60s (catches JIP/respawns)
- **Respawned players** get re-detected and marker re-created
- **ELEMENT markers** track the centroid of alive group members
- **ELEMENT elimination** fires when all occupied slots in the group are dead

---

## Scripting API

The `FindHVTInRange()` method allows external scripts to detect nearby HVTs. This is useful for:
- **Objective triggers** - check if HVT reached a location
- **Interaction requirements** - require HVT presence to use an object (e.g., radio)
- **Custom win conditions** - detect HVT in extraction zone

**Method Signature:**
```enforce
IEntity FindHVTInRange(vector position, float range, CRF_HVTFaction faction = NONE, int entryType = -1)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `position` | vector | Center point to search from |
| `range` | float | Search radius in meters |
| `faction` | CRF_HVTFaction | Filter: NONE, BLUFOR, OPFOR, INDFOR, CIV (default: NONE = any) |
| `entryType` | int | Filter: 0=AI, 1=PLAYER, 2=OBJECT, 3=ELEMENT (default: -1 = any) |
| **Returns** | IEntity | First matching **alive** HVT entity, or `null` if none found |

**Usage Examples:**
```enforce
// Get the HVT manager
CRF_HighValueTargetGamemodeManager hvtManager = CRF_HighValueTargetGamemodeManager.Cast(
    CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager)
);

// Any HVT within 50m of this position
IEntity hvt = hvtManager.FindHVTInRange(myPosition, 50);

// Only BLUFOR HVTs within 100m
hvtManager.FindHVTInRange(position, 100, CRF_HVTFaction.BLUFOR);

// Only OBJECT types within 25m (radios, intel, etc.)
hvtManager.FindHVTInRange(position, 25, CRF_HVTFaction.NONE, CRF_HVTEntryType.OBJECT);

// OPFOR AI HVTs within 75m
hvtManager.FindHVTInRange(position, 75, CRF_HVTFaction.OPFOR, CRF_HVTEntryType.AI);
```

**Element query methods:**
```enforce
hvtManager.IsElementAlive(entryIndex);
hvtManager.IsElementAliveByCallsign("OPFOR", "1PLT");
hvtManager.GetElementPosition(entryIndex);
hvtManager.IsElementInRange(entryIndex, searchPos, 100);
```

---

## Example: Radio Requires HVT Nearby

A common use case is requiring the HVT to be near an object before it can be interacted with (e.g., a radio that calls extraction only when VIP is present).

```enforce
// Pseudo-code for a radio interaction script

class MyRadioInteraction : ScriptedUserAction
{
    bool m_bRequireHVT = true;  // Set in editor
    float m_fRequiredRange = 50; // HVT must be within 50m
    
    override bool CanBeShownScript(IEntity user)
    {
        // If HVT requirement is disabled, always show
        if (!m_bRequireHVT)
            return true;
        
        // Get the HVT manager
        CRF_HighValueTargetGamemodeManager hvtManager = CRF_HighValueTargetGamemodeManager.Cast(
            CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager)
        );
        
        if (!hvtManager)
            return false;
        
        // Check if any HVT is within range of this radio
        IEntity radio = GetOwner();
        if (hvtManager.FindHVTInRange(radio.GetOrigin(), m_fRequiredRange))
            return true;  // HVT found nearby - show interaction
        
        return false;  // No HVT nearby - hide interaction
    }
    
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        // Radio triggered - call extraction, respawn faction, etc.
    }
}
```
