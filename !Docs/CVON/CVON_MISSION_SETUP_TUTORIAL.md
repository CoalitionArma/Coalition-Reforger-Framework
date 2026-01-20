# CVON Mission Setup Tutorial

This tutorial will guide you through setting up a mission to use CVON (Coalition Voice Over Network) in the Coalition Reforger Framework.

## Table of Contents
1. [Basic Setup](#basic-setup)
2. [Adding CVON Component](#adding-cvon-component)
3. [Faction Configuration](#faction-configuration)
4. [Frequency Configuration](#frequency-configuration)
5. [Testing Your Setup](#testing-your-setup)
6. [Advanced Configuration](#advanced-configuration)
7. [Troubleshooting](#troubleshooting)

## Basic Setup

### Step 1: Use CRF_Lobby Prefab

Your mission must use the `CRF_Lobby` prefab as its base gamemode. This prefab contains all the necessary components for CVON to function.

1. In your mission, ensure you're using: `{5A8E93AF9B6A2DCC}Prefabs/MP/Modes/Lobby/CRF_Lobby.et`
2. The CRF_Lobby prefab already includes the basic CVON components

### Step 2: Verify CVON Component Presence

The `CRF_Lobby` prefab should already contain the `CVON_VONGameModeComponent`. You can verify this by:

1. Opening the `CRF_Lobby` prefab in the mission
2. Looking for `CVON_VONGameModeComponent` in the component list
3. If missing, manually add it to your gamemode entity

## Adding CVON Component

If you need to manually add the CVON component:

### CVON_VONGameModeComponent Configuration

Add the `CVON_VONGameModeComponent` to your gamemode entity with these key settings:

```conf
CVON_VONGameModeComponent "{6624E607CC6D4CC8}" {
  m_sFreqConfig "{8FE52CD7F8502E24}Configs/FreqConfigs/CFCFrequencyConfig.conf"
  m_bUseFactionEncryption true
  m_bTeamspeakChecks true
  m_aSharedFrequencies {
    // Add shared frequency configurations here if needed
  }
}
```

#### Component Properties:

- **m_sFreqConfig**: Path to the frequency configuration file (uses default CRF config)
- **m_bUseFactionEncryption**: When `true`, frequencies are faction-specific; when `false`, frequencies are global
- **m_bTeamspeakChecks**: Enables/disables TeamSpeak integration warnings
- **m_aSharedFrequencies**: Array for defining cross-faction frequency sharing

## Faction Configuration

### Setting Up Group Frequency Overrides

Each faction (BLUFOR, OPFOR, INDFOR) needs proper frequency configuration. In each faction's settings:

1. Navigate to the faction component in your mission
2. Find the **Group Frequency Overrides** section
3. Configure frequency overrides for specific groups that don't follow the standard pattern

#### Example Group Frequency Override:

```conf
CVON_GroupFrequencyContainer {
  m_aGroupNames {
    "MMG 1"        // Name of the group to override
    "Gun Team 1"   // Alternative names for the same group
  }
  m_aSRFrequencies {
    "MMG 1 SR"     // Short Range radio frequencies (if empty, uses group name)
  }
  m_aLRFrequencies {
    "1PLT"         // First LR frequency (primary net)
    "COY"          // Second LR frequency (higher command net)
  }
}
```

#### Key Points:
- **Group Names**: Array of possible group names that this configuration applies to
- **SR Frequencies**: Short Range radio channels (if left blank, defaults to group name)
- **LR Frequencies**: Long Range radio channels (first is primary, additional go to higher command)

### Standard Faction Callsign Structure

Ensure your faction callsigns follow this hierarchy:
```
Company (COY)
├── 1st Platoon (1PLT)
│   ├── 1-1 (Squad)
│   ├── 1-2 (Squad)
│   ├── 1-3 (Squad)
│   └── Support Elements (MMG 1, HAT 1, etc.)
├── 2nd Platoon (2PLT)
│   └── Similar structure...
└── Support Elements
    ├── Engineers
    ├── Mortars
    ├── Medical
    └── Logistics
```

## Frequency Configuration

### Understanding the Frequency System

CVON uses a hierarchical frequency assignment system:

#### Platoon Linked Elements:
Elements are automatically assigned to their corresponding platoon based on numbers:
- **1-1, 1-2, 1-3, 1-4** → Assigned to **1PLT**
- **MMG 1, HAT 1, MAT 1** → Assigned to **1PLT**
- **2-1, 2-2, 2-3, 2-4** → Assigned to **2PLT**

#### Highest HQ Linked Elements:
These elements get assigned to the highest headquarters:
- **Engineers, Mortars, Vehicles, Helicopters, Snipers, Medics**

### Default Frequency Configuration

The system uses `CFCFrequencyConfig.conf` which includes:

```conf
CVON_FreqConfig {
  m_aPresetGroupFrequencyContainers {
    // Company Level
    CVON_GroupFrequencyContainer {
      m_aGroupNames { "COY", "Company" }
      m_aSRFrequencies { "COY SR" }
      m_aLRFrequencies { "COY" }
    }
    
    // Platoon Level
    CVON_GroupFrequencyContainer {
      m_aGroupNames { "1PLT", "1st Platoon" }
      m_aSRFrequencies { "1PLT SR" }
      m_aLRFrequencies { "1PLT", "COY" }
    }
    
    // Squad Level (inherits from platoon)
    CVON_GroupFrequencyContainer {
      m_aGroupNames { "1-1", "Squad 1" }
      m_aLRFrequencies { "1PLT" }
    }
    
    // Support Elements
    CVON_GroupFrequencyContainer {
      m_aGroupNames { "Engineer", "Engi" }
      m_aSRFrequencies { "ENGINEER SR" }
      m_aLRFrequencies { "ENGINEER" }
    }
  }
}
```

### Custom Frequency Configuration

To create a custom frequency configuration:

1. Create a new `.conf` file in `Configs/FreqConfigs/`
2. Copy the structure from `CFCFrequencyConfig.conf`
3. Modify group names and frequencies as needed
4. Update the `m_sFreqConfig` path in your `CVON_VONGameModeComponent`

## Testing Your Setup

### Pre-Mission Testing Checklist

1. **Launch your mission in Workbench**
2. **Check console for CVON warnings**:
   - Missing frequency configurations
   - Invalid group assignments
   - TeamSpeak integration issues

3. **Verify in Slotting Menu**:
   - Players can select slots normally
   - VON indicators show when speaking
   - Push-to-talk functionality works

4. **Test in Spectator Mode**:
   - VON channels are available
   - Channel switching works
   - Cross-faction filtering functions properly

### In-Game Testing

1. **Slot into different roles**
2. **Test radio functionality**:
   - Short Range (SR) communication within squads
   - Long Range (LR) communication between command elements
   - Direct speech in appropriate situations

3. **Verify frequency assignment**:
   - Squad leaders have platoon net access
   - Platoon leaders have company net access
   - Support elements are on correct specialized nets

## Advanced Configuration

### Cross-Faction Communication

To enable communication between factions on specific frequencies:

```conf
CVON_VONGameModeComponent {
  m_aSharedFrequencies {
    CVON_SharedFrequencyContainer {
      m_aFactionIds { "US", "USSR" }  // Factions that can communicate
      m_sFrequency "NEUTRAL"          // Shared frequency name
    }
  }
}
```

### Custom Group Overrides for Vehicles

For vehicle-heavy missions, you might want to override automatic assignment:

```conf
CVON_GroupFrequencyContainer {
  m_aGroupNames { "Armor 1", "Tank 1", "Bradley 1" }
  m_aLRFrequencies { "1PLT", "ARMOR" }  // Assigned to 1PLT + Armor net
}
```

### Specialized Role Configurations

For unique roles that don't fit the standard pattern:

```conf
CVON_GroupFrequencyContainer {
  m_aGroupNames { "JTAC", "Forward Observer" }
  m_aSRFrequencies { "JTAC SR" }
  m_aLRFrequencies { "FIRES", "AIR", "COY" }  // Multiple command nets
}
```

## Troubleshooting

### Common Issues

#### "Missing frequency configuration" Warning
- **Cause**: Group name not found in frequency config or faction overrides
- **Solution**: Add the group to faction overrides or modify frequency config

#### Players Can't Hear Each Other
- **Check**: Faction encryption settings
- **Check**: TeamSpeak integration
- **Check**: Frequency assignments match between players

#### VON Not Working in Spectator
- **Check**: CVON component is properly initialized
- **Check**: Spectator faction is configured correctly
- **Check**: Channel assignment system is functioning

#### Radio Frequencies Not Assigned
- **Check**: Player is properly slotted into a group
- **Check**: Group has valid callsign configuration
- **Check**: Frequency container exists for the group

### Console Error Messages

Monitor the console for CVON-related errors:
- Group assignment failures
- Frequency configuration issues
- TeamSpeak integration problems

## Best Practices

1. **Keep group naming consistent** with the frequency configuration
2. **Test thoroughly** in both Workbench and server environments
3. **Document custom frequencies** for mission briefings
4. **Consider faction balance** when setting up shared frequencies
5. **Regularly update** frequency configurations as mission requirements change
