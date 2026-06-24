# CRF Map Staging Component

A linear based staging component for several one-time staging usage. NOT a dynamic zone control system.

## Overview

The Map Staging Component provides a simple API for mission makers to create dynamic play areas that change over time. Instead of static boundaries, you can create sequences that activates/deactivates new areas, or delete boundaries entirely as the mission progresses. This provides a mini api to work with, and you can mix+match activation types + timers etc. Can be expanded on / salvaged.

ToDo:
- More features
- Integration OR Absorption into Other GM's/Manager
- Persistance Logic (Dynamic zone changing or looping options/logic instead of linear staging, IE: For Frontline/CCO-likes but with zone restrictions that arent contested zone.)

## Quick Setup

1. **Add the Component**: Add `CRF_MapStagingComponent` to CRF_Lobby 
2. **Place Boundaries**: Create GameBoundary entities in your world and name them clearly, or else they wont be identified correctly on init
3. **Add GameBoundrys/Edit the Component**: Set up the polylines for your zones, be sure to set BOTH the GameBoundry/CRF_Polyzone & CRF_PolyzoneTrigger (child object in heirarchy) 's Reversed toggles to off. Does NOT work with REVERSED.
3. **Configure Stages**: Set up your Main Config & Boundary Stage settings in the component inspector. Be sure to set FACTIONKEYS within the CRF_PolyzoneTrigger depending on that boundary's use case.
4. **Test**: Enable debug logging and test your sequence

## Stage Types
_Types of zone execution._
- **ACTIVATION**: Boundary's out of bounds effects NOT PRESENT on polyline area on INIT, then moved away via Stage Timer OR script call
- **DEACTIVATION**: Boundary's out of bounds effects PRESENT on polyline area on INIT, then moved away via Stage Timer OR script call.
- **DELETION**: Boundary gets permanently deleted when stage executes

## Stage States 
_States of Stages carrying `elseifs` that handle what visual & mechanical logic is used_
- **INACTIVE** : Redundant unaffected state, default on init
- **ACTIVE** : Triggered via Stage Timer, if timer parameter set to `False` on `ExecuteStaging` external call. Skipped otherwise. 
- **ACTIVATED** : The end-state of the Stage, after ActivationType is enacted

## Basic Configuration

### Base Settings
- **`Initial Delay`**: Time after safestart ends before auto-staging begins (default: 30s)
- **`Auto-Start`**: Whether staging starts automatically or requires manual triggers
- **`Start/End Sounds`**: Audio feedback for stage transitions
- **`Debug Logging`**: Enable for troubleshooting and testing
- **`Final Stage Messages`**: Message + Sub-Message for when all stages are completely (blank is valid)

### Per-Stage Settings
- **`Boundary Entity Name`**: Exact name of your GameBoundary entity
- **`Stage Type`**: ACTIVATION, DEACTIVATION, or DELETION. For stage timers, it enacts at the END of the timer.
- **`Duration`**: Timer length in seconds
- **`Display Text`**: Message shown under timer during countdown (if shown)
- **`Completion Messages`**: Custom popup announcements when stage finishes (blank is valid)
- **`Visual Colors`**: Boundary colors for ACTIVE and ACTIVATED states. The inital Polygon Color will always be what you set in the boundry prefab component then the script overwrites based on its state. ACTIVE is only present if using STAGE TIMERS. ACTIVATED is the color state in how it will be displayed if its ACTIVATIONTYPE is set to deactivation/activation. Deletion just makes it disappear. (Opacity change for NO fill is valid. IE: Just wanted the polygon fill color to disappear for deactivation type stage.)

## API Reference

### Get Component Instance
```csharp
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
```

### Main External Call Methods

**Execute Stage**
```csharp
staging.ExecuteStaging(stageIndex, useTimer, chainToNext);
// stageIndex: Which stage (0, 1, 2...)
// useTimer: true = countdown timer until activationtype enacts, false = immediate 
// chainToNext: true = make THIS stage's end continue to next stage in the list, false = independent stage execution
```

**Execute Full Sequence**
```csharp
staging.ExecuteStagingSequence(startIndex);
// startIndex: Which stage to begin sequence from
```

**Utility Methods**
```csharp
// Manual start (bypasses safestart)
staging.BeginStaging();

// Emergency stop
staging.StopStaging();
```
---

## Details

### IMPORTANT Setup Tips
- Use clear, descriptive names for boundary entities
- Position boundaries at their final locations in the editor
- Test with debug logging enabled
- Configure faction keys in CRF_PolyZoneTrigger components, based on which FACTION can be allowed in that zone. (IE: Progressive OPFOR-Sided Boundries will have OPFOR)
- Only use non-reversed GameBoundary/CRF_PolyZone entities

### Performance
- The system uses entity caching for better performance
- Boundary states are replicated for late-joining players
- RPC system ensures cross-environment compatibility

### Troubleshooting
- Enable debug logging to see detailed execution flow
- Check entity names match exactly (case-sensitive)
- Verify boundaries have CRF_PolyZone components
- Test in both Workbench and dedicated server environments

## Technical Notes

- **Replication**: All critical state is replicated for multiplayer
- **JIP Support**: Late-joining players receive current boundary states
- **Cross-Environment**: Works in both Workbench testing and dedicated servers
- **Performance**: Uses caching and optimization for large boundary counts
- **Safety**: Server-only execution prevents client-side manipulation

## Integration

The component integrates with:
- CRF Safestart Manager (for auto-timing)
- CRF Gamemode system (for state management)
- Standard Arma Reforger GameBoundry entities
- CRF_PolyZone visual system

---

## Example Scenarios

### Ex. 1:  Two-Stage Objective Progression (Destructor Call Based)
**Use Case**: Progressive Objective-Based Zone Unlocking

<img width="380" height="189" alt="image" src="https://github.com/user-attachments/assets/19b9195a-2cb6-4846-8233-d74b01f9055e" />

_Example with timers between as a 'R&R' period, can be used with Rush on destuctor scriptlines or others. 2 MCOMS, 1 Per Stage, 2 BoundryObjects & Stages. First MCOM within/exposed on FACTION safestart area end. Technically just 2 zones_

**Base Configuration**:
- Initial Delay: 0 (not used with manual trigger)
- Auto-Start: false (manual script control only)
- Debug Logging: true (for testing)

**Stage 0 Settings** (First Objective Unlocked):
- Boundary Entity Name: "Boundry_OBJ2"
- Stage Type: DELETION (removes protection around second objective)
- Duration: 180 (3 minute refractory period before zone unlock)
- Display Text: "Second MCOM area opening..."
- Main Completion Message: "Zone 2 Unlocked"
- Sub Completion Message: "Second MCOM Exposed"
- Completion Duration: 8 seconds
- Active Colors: Yellow fill (0.8 0.8 0.2 0.5), Orange border (0.8 0.4 0 1)
- Activated Colors: Not used (boundary deleted)

**First Object Destructor Setup:**
```csharp
// Single MCOMS Example, Deletes zone protecting second MCOM with a timer with NO automatic chaining 
// (Can add normal Rush checks for objects present for multi/sister MCOMS like we already do)
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
staging.ExecuteStaging(0, true, false); // With timer, no chaining
```

**Stage 1 Settings** (Second Objective Unlocked):
- Boundary Entity Name: "Boundry_OBJ3"
- Stage Type: DELETION (reveals second objective area)
- Duration: 180 (3 minute refractory period before final MCOM zone open)
- Display Text: "Final MCOM area opening..."
- Main Completion Message: "Zone 3 Unlocked"
- Sub Completion Message: "Third MCOM Exposed"
- Completion Duration: 10 seconds
- Active Colors: Orange fill (0.8 0.4 0 0.5), Dark border (0.1 0.1 0.1 1)
- Activated Colors: Not used (boundary deleted)

**Second Object Destructor Setup:**: 
```csharp
// Single MCOMS Example, Deletes zone protecting third MCOM with a timer with NO automatic chaining 
// (Can add normal Rush checks for objects present for multi/sister MCOMS like we already do)
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
staging.ExecuteStaging(1, true, false); // With timer, no chaining
```

---

### Ex. 2: Single Neutral Zone Unlock In Middle of Map
**Use Case**: Have a neutral zone that unlocks after safestart/delay, zone visually still present and trigger/restriction deactivated.
Can also just have DELETION activation type.

<img width="800" height="450" alt="ex2neutral" src="https://github.com/user-attachments/assets/0de126ca-db80-46be-82fe-89fe29417eb3" />


**Base Configuration**:
- Initial Delay: 180 (3 minutes after safestart)
- Auto-Start: true (autostart)

**Stage 0 Settings**:
- Boundary Entity Name: "Boundry_Neutral"
- Stage Type: DEACTIVATION or Deletion (permanently removes boundary)
- Duration: 300 (5 minute displayed timer)
- Display Text: "Neutral Zone Unlock In.."
- Main Completion Message: "Neutral Zone Open"
- Sub Completion Message: "Kill Each Other"
- Completion Duration: 10 seconds
- Active Colors: Red fill (0.8 0.2 0.2 0.6), Yellow border (0.8 0.8 0.2 1)
- Activated Colors: Any fill (Opacity to 0), Black Border

**Script Usage**: `staging.ExecuteStaging(0, true, false);` // With timer, no chain

### Ex. 3: Multi-Stage Auto Timed Restriction Sequence
**Use Case**: Progressive map shrinking over time to force engagement. Battle royale zone style restriction.

<img width="800" height="450" alt="ex3_br" src="https://github.com/user-attachments/assets/9f2b0f6e-e923-437e-8e9f-970ae85f3162" />



**Base Configuration**:
- Initial Delay: 500 (3 minutes after safestart)
- Auto-Start: true (fully automatic sequence)
- Final Completion Message: "Final Combat Zone Active"
- Final Sub Message: "Fight within the area"
- Final Duration: 15 seconds

**Stage 0 Settings** (Outer Ring Removal):
- Boundary Entity Name: "Boundry_Outer"
- Stage Type: ACTIVATION _(boundary inits as away, activates on time trigger)_
- Duration: 600 (10 minute timer)
- Display Text: "1st Zone Restriction..."
- Main Completion Message: "Outer Zone Closed"
- Sub Completion Message: "Move to inner areas"
- Active Colors: Orange polygon fill, Black Border _The color that indicates impending restriction_
- Activated Colors: Red fill, Black Border

**Stage 1 Settings** (Inner Ring Removal):
- Boundary Entity Name: Boundry_inner"
- Stage Type: ACTIVATION
- Duration: 480 (8 minute timer)
- Display Text: "Final Zone Restriction..."
- Main Completion Message: "Final Zone Closed"
- Sub Completion Message: "Kill everything"
- Active Colors: Orange polygon fill, Black Border _The color that indicates impending restriction_
- Activated Colors: Red fill, Black border


**Script Usage**: Automatic (no script needed) or `staging.ExecuteStagingSequence(0);` for manual start

---

## Usage Examples

### Destructor Script Integration
```csharp
// When object is destroyed, trigger staging with timer
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
staging.ExecuteStaging(0, true, false);
```

### Emergency Controls
```csharp
// Admin commands or fail-safes
if (emergencyCondition) {
    staging.StopStaging(); // Halt all staging
}

// Skip to final stage
staging.ExecuteStaging(lastStageIndex, false, false);
```



## Version Information

- **Author**: Trist
- **Created**: August 2025
- **Component**: CRF_MapStagingComponent
- **Dependencies**: CRF Framework, GameBoundary entities, CRF_PolyZone
