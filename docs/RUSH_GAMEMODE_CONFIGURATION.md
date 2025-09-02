# Rush Gamemode Configuration Guide

## Overview

The Rush gamemode now supports customizable zone and MCOM configurations, allowing developers to create missions with 1-3 zones and 1-2 MCOMs per zone.

## Configuration Options

### Zone Configuration Attributes

These attributes can be configured in the World Editor when placing the `CRF_RushGamemodeManager` component:

#### `m_iNumberOfZones` (Number of Zones)
- **Type:** Integer (1-3)
- **Default:** 3
- **Description:** Sets the total number of zones in the mission
- **UI:** Slider control in the editor

#### `m_iMCOMsPerZone` (MCOMs per Zone) 
- **Type:** Integer (1-2)
- **Default:** 2
- **Description:** Sets how many MCOM sites exist in each zone
- **UI:** Slider control in the editor

## Zone and MCOM Layout Requirements

### Trigger Naming Convention

For the dynamic system to work, trigger entities must follow this naming pattern using **sequential MCOM lettering**:

#### Sequential MCOM Trigger Naming (A, B, C, D, E, F)

The triggers are named sequentially across all zones, not per-zone:

- **MCOM A:** `mcom_a_trigger` (Zone 1, First MCOM)
- **MCOM B:** `mcom_b_trigger` (Zone 1, Second MCOM OR Zone 2, First MCOM if 1 per zone)
- **MCOM C:** `mcom_c_trigger` (Zone 2, First or Second MCOM)
- **MCOM D:** `mcom_d_trigger` (Zone 2, Second MCOM OR Zone 3, First MCOM)
- **MCOM E:** `mcom_e_trigger` (Zone 3, First or Second MCOM)
- **MCOM F:** `mcom_f_trigger` (Zone 3, Second MCOM)

#### MCOM Entity Naming (for entity searching)

Similarly, MCOM entities should be named:
- **MCOM A:** `mcom_a`
- **MCOM B:** `mcom_b`
- **MCOM C:** `mcom_c`
- **MCOM D:** `mcom_d`
- **MCOM E:** `mcom_e`
- **MCOM F:** `mcom_f`

### Example Configurations

#### Configuration 1: Single MCOM per Zone, 3 Zones
```
Number of Zones: 3
MCOMs per Zone: 1

Required Triggers:
- mcom_a_trigger (Zone 1)
- mcom_b_trigger (Zone 2)
- mcom_c_trigger (Zone 3)

MCOM Identifiers:
- MCOMA (Zone 1)
- MCOMB (Zone 2)
- MCOMC (Zone 3)
```

#### Configuration 2: Two MCOMs per Zone, 2 Zones
```
Number of Zones: 2
MCOMs per Zone: 2

Required Triggers:
- mcom_a_trigger (Zone 1, First MCOM)
- mcom_b_trigger (Zone 1, Second MCOM)
- mcom_c_trigger (Zone 2, First MCOM)
- mcom_d_trigger (Zone 2, Second MCOM)

MCOM Identifiers:
- MCOMA, MCOMB (Zone 1)
- MCOMC, MCOMD (Zone 2)
```

#### Configuration 3: Single Zone with Two MCOMs
```
Number of Zones: 1
MCOMs per Zone: 2

Required Triggers:
- mcom_a_trigger (Zone 1, First MCOM)
- mcom_b_trigger (Zone 1, Second MCOM)

MCOM Identifiers:
- MCOMA, MCOMB (Zone 1)
```

#### Configuration 4: Maximum Configuration (3 Zones, 2 MCOMs each)
```
Number of Zones: 3
MCOMs per Zone: 2

Required Triggers:
- mcom_a_trigger (Zone 1, First MCOM)
- mcom_b_trigger (Zone 1, Second MCOM)
- mcom_c_trigger (Zone 2, First MCOM)
- mcom_d_trigger (Zone 2, Second MCOM)
- mcom_e_trigger (Zone 3, First MCOM)
- mcom_f_trigger (Zone 3, Second MCOM)

MCOM Identifiers:
- MCOMA, MCOMB (Zone 1)
- MCOMC, MCOMD (Zone 2)
- MCOME, MCOMF (Zone 3)
```

## Gamemode Behavior

### Zone Progression
- **1 MCOM per Zone:** Zone clears when the single MCOM is destroyed
- **2 MCOMs per Zone:** Zone clears when both MCOMs are destroyed
- Players advance to the next zone only after the current zone is cleared
- Final zone victory occurs when all MCOMs in the last zone are destroyed

### Visual Markers
- 3D markers automatically show correct letters (A for Alpha, B for Beta)
- Map markers adapt to the configured number of zones and MCOMs
- Only active zone markers are visible initially

## Developer API

### Configuration Getters
```c
// Get current configuration
int numberOfZones = rushManager.GetNumberOfZones();      // Returns 1-3
int mcomsPerZone = rushManager.GetMCOMsPerZone();        // Returns 1-2  
int totalMCOMs = rushManager.GetTotalMCOMCount();        // Returns total count

// Validate configuration
bool isValidZone = rushManager.IsZoneValid(zoneNumber);       // 1-based zone number
bool isValidMCOM = rushManager.IsMCOMIndexValid(mcomIndex);   // 0-based MCOM index
```

### Dynamic Status Checking
```c
// Check MCOM status by zone and index
bool isDestroyed = rushManager.GetMCOMDestroyedStatus(zoneNumber, mcomIndex);
bool isPlanted = rushManager.GetMCOMPlantedStatus(zoneNumber, mcomIndex);

// Get MCOM entity references
IEntity mcomEntity = rushManager.GetMCOMEntityByIndex(zoneNumber, mcomIndex);

// Check if entire zone is cleared
bool zoneCleared = rushManager.IsZoneCleared(zoneNumber);
```

### Helper Methods
```c
// Parse MCOM identifiers
int zoneIndex, mcomIndex;
if (rushManager.ParseMCOMIdentifier("MCOMA", zoneIndex, mcomIndex))
{
    // zoneIndex = 0, mcomIndex = 0
}

// Generate identifiers programmatically  
string mcomId = rushManager.GetMCOMIdentifier(1, 0);  // Returns "MCOMA"
string triggerName = rushManager.GetTriggerName(1, 0); // Returns "mcom_a_trigger"
```

## Mission Creation Workflow

1. **Place Triggers:** Create trigger entities with proper naming convention
2. **Configure Component:** Set `m_iNumberOfZones` and `m_iMCOMsPerZone` attributes
3. **Test Configuration:** Verify all required triggers exist for your configuration
4. **Validate Gameplay:** Test zone progression and MCOM destruction mechanics

## Backward Compatibility

The system maintains full backward compatibility with existing missions:
- Legacy zone-based identifiers (Zone1Alpha, Zone1Beta, etc.) are automatically converted to sequential format (MCOMA, MCOMB, etc.)
- Old trigger names (z1_alpha_trigger, z1_beta_trigger, etc.) continue to work alongside new naming
- Legacy hardcoded variables are still updated for network replication
- No changes required for existing missions

## Troubleshooting

### Common Issues

**Missing Triggers Warning**
- Error: `CRF_Rush: Warning - Trigger 'mcom_b_trigger' not found!`
- Solution: Ensure all required trigger entities exist for your configuration

**MCOMs Not Spawning**
- Check trigger naming matches the convention exactly (mcom_a_trigger, mcom_b_trigger, etc.)
- Verify MCOM prefab path is valid: `{A8C69227F4322F20}Prefabs/Structures/CRF_Rush_MCOM.et`

**Zone Not Clearing**
- Ensure configuration matches the number of MCOMs actually placed
- Check that all MCOMs in the zone are properly destroyed

### Debug Information

Enable debug logging to see which triggers are found/missing:
```c
Print(string.Format("CRF_Rush: Searching for trigger '%1'", triggerName), LogLevel.VERBOSE);
```

## Performance Considerations

- Dynamic arrays are only allocated for the configured number of zones/MCOMs
- Legacy compatibility adds minimal overhead
- Memory usage scales with `m_iNumberOfZones * m_iMCOMsPerZone`

## Future Enhancements

Potential future improvements:
- Support for more than 3 zones
- Support for more than 2 MCOMs per zone  
- Custom MCOM letter assignments beyond A-F
- Runtime configuration changes
