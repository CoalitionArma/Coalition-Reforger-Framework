# Coalition Reforger Framework (CRF) - Technical Documentation

## Overview

The Coalition Reforger Framework (CRF) is a comprehensive game framework for Arma Reforger developed by the COALITION gaming community. It provides a modular, extensible foundation for creating tactical multiplayer missions with advanced player management, role-based equipment systems, and multiple game modes.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Systems](#core-systems)
3. [Game Modes](#game-modes)
4. [Configuration System](#configuration-system)
5. [Network Architecture](#network-architecture)
6. [Development Setup](#development-setup)
7. [API Reference](#api-reference)
8. [Contributing](#contributing)

## Architecture Overview

### Project Structure

```
Coalition-Reforger-Framework/
├── addon.gproj                     # Main project configuration
├── scripts/                        # Core framework scripts
│   ├── Game/
│   │   ├── GameMode/               # Game mode implementations
│   │   ├── Systems/                # Core framework systems
│   │   └── UserActions/            # Custom user interactions
│   └── helper/                     # Utility libraries
├── Configs/                        # Configuration files
│   ├── Gearscripts/               # Role and equipment configurations
│   ├── Core/                      # Core system configurations
│   └── Resources/                 # Resource definitions
├── Prefabs/                       # Entity prefabs
├── Assets/                        # Game assets (models, textures)
├── UI/                           # User interface layouts
├── Worlds/                       # Mission worlds
└── Missions/                     # Mission configurations
```

### Core Framework Components

The CRF framework is built around several key manager components that handle different aspects of gameplay:

- **CRF_Gamemode**: Main game mode controller
- **CRF_GamemodeManager**: Central manager coordination
- **CRF_SlottingManager**: Player slot assignment and management
- **CRF_GearscriptManager**: Role-based equipment system
- **CRF_RespawnManager**: Player respawn logic
- **CRF_SafestartManager**: Mission preparation phase management
- **CRF_LoggingManager**: Server event logging
- **CRF_AdminMenuManager**: Administrative controls

## Core Systems

### 1. Mission State Management

The framework operates through four distinct phases:

```cpp
enum CRF_EGamemodeState
{
    BRIEFING,    // Mission briefing and information display
    SLOTTING,    // Player role selection and team assignment
    GAME,        // Active gameplay phase
    AAR          // After Action Report and statistics
}
```

Each state transition triggers specific UI changes and system activations:

```cpp
void AdvanceGamemodeState(bool overriden = false)
{
    if ((m_GamemodeState == CRF_EGamemodeState.AAR || 
         m_GamemodeState == CRF_EGamemodeState.GAME) && !overriden)
        return;

    m_GamemodeState += 1;
    Replication.BumpMe();
    OnGamemodeStateChanged();
}
```

### 2. Player Slotting System

The slotting system manages player assignment to roles and groups:

#### Slotting States
- **LEADERSANDMEDICS**: Priority roles (Squad Leaders, Medics)
- **EVERYONE**: All players can select roles
- **LOCKED**: No further slot changes allowed

#### Role Categories
- **SQUAD_LEADER**: Command roles with leadership equipment
- **TEAM_LEADER**: Sub-unit leadership
- **MEDIC**: Medical roles with healing equipment
- **SPECIALTY**: Specialized roles (Engineers, AT specialists, etc.)
- **ASSISTANT**: Support roles for specialty weapons
- **SPECIALTY_ASSISTANT**: Dedicated assistant roles

### 3. Gearscript System

The gearscript system provides role-based equipment loadouts:

```cpp
class CRF_GearscriptManager : ScriptComponent
{
    void SetEntityGear(IEntity entity, ResourceName resourceNameToScan)
    {
        // Determine faction and role from entity
        FactionKey factionKey = DetermineFactionKey(resourceNameToScan);
        CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceNameToScan);
        
        // Apply appropriate gear configuration
        ApplyGearConfiguration(entity, factionKey, role);
    }
}
```

#### Equipment Categories
- **Weapons**: Primary, secondary, and specialty weapons
- **Magazines**: Ammunition loadouts per role
- **Clothing**: Faction-specific uniforms and armor
- **Items**: Role-specific equipment (radios, medical supplies, tools)

### 4. Network Replication

The framework uses Arma Reforger's RPC system for client-server communication:

```cpp
[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
void RPC_UpdateGameState(int newState)
{
    m_GamemodeState = newState;
    OnGamemodeStateChanged();
}
```

## Game Modes

### 1. Rush Mode

A linear attack/defend mode with progressive objectives:

**Features:**
- 3 zones with 2 MCOM sites each
- Progressive unlocking (Zone 1 → Zone 2 → Zone 3)
- Bomb planting/defusing mechanics
- Dynamic map markers and 3D HUD indicators

**Implementation:**
```cpp
class CRF_RushGamemodeManager: SCR_BaseGameModeComponent
{
    [Attribute("BLUFOR")] FactionKey m_AttackingSide;
    [Attribute("OPFOR")] FactionKey m_DefendingSide;
    [Attribute("45")] int m_MCOMTimer;
}
```

### 2. Search and Destroy

Classic tactical mode with bomb sites:

**Features:**
- Two bomb sites (A and B)
- Single round elimination
- Configurable attacking/defending sides

### 3. Frontline

Territory control with advancing front lines:

**Features:**
- Linear zone capture
- Front line advancement
- Time-based victory conditions

### 4. High Value Target

VIP escort/elimination missions:

**Features:**
- Target tracking with periodic updates
- Faction-specific visibility options
- Configurable transponder entities

## Configuration System

### Mission Configuration

Base mission settings are defined in `CRF_Lobby` Component:

```cpp
// Mission timing
[Attribute("45")] int m_iTimeLimitMinutes;

// Faction settings
[Attribute("1")] int m_iFactionOneRatio;
[Attribute("BLU")] string m_sFactionOneKey;

// Respawn configuration
[Attribute("0")] bool m_bRespawnEnabled;
[Attribute("60")] int m_iTimeToRespawn;
```

### Role Configuration

Roles are defined in `CRF_Global_Roles_Config.conf` with faction-specific variants:

```cpp
CRF_RoleConfig {
    m_Role SQUAD_LEAD
    m_sRoleName "Squad Leader"
    m_SlottingType SQUAD_LEADER
    m_BluforVariant "{CC8EDD051CB0C1CE}Prefabs/Characters/..."
    m_OpforVariant "{9CB623C7B62F4955}Prefabs/Characters/..."
    m_aWeapons { 1, 4 }      // Rifle + Leadership Radio
    m_aMagazines { 1, 4 }    // Corresponding magazines
    m_aItems { 1, 4 }        // Leadership items
}
```

### Gearscript Configuration

Equipment loadouts are configured per faction:

```cpp
CRF_GearScriptContainer {
    m_rGearScript "{...}Configs/Gearscripts/..."
    m_bEnableGIRadios true
    m_bEnableLeadershipRadios true
    m_rGIRadiosPrefab "{...}Prefabs/Items/Equipment/Radios/..."
}
```

**NOTE**: When adding/removing a gearscript config you may need to run "Gear Script Config Generator" under the plugins menu to regenerate the config file used by the admin menu to allow hot swapping of gearscripts. This plugin does automatically trigger when a new resource is created in the /Config/GearScripts/Standard but will need running manually when one is removed.

## Network Architecture

### Replication System

The framework uses Arma Reforger's replication system with `[RplProp()]` attributes:

```cpp
class CRF_Gamemode : SCR_BaseGameMode
{
    [RplProp(onRplName: "OnGamemodeStateChanged")]
    int m_GamemodeState = CRF_EGamemodeState.BRIEFING;
    
    [RplProp()]
    vector m_vGenericSpawn[4];  // Spectator spawn point
}
```

### RPC Communication

Remote procedure calls handle real-time events:

```cpp
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
void RPC_RequestSlotChange(int slotID, int playerID)
{
    if (Replication.IsServer())
        ProcessSlotRequest(slotID, playerID);
}
```

### Authority Management

Server-side validation ensures game state integrity:

```cpp
if (!Replication.IsServer())
    return;

// Server-only logic here
UpdateGameState();
Replication.BumpMe();  // Trigger replication
```

## Development Setup

### Prerequisites

1. **Arma Reforger Workbench** - Latest version
2. **Git** - For version control
3. **Text Editor** - VS Code recommended with Enforce Script extension

### Project Setup

1. Clone the repository:
```bash
git clone https://github.com/CoalitionArma/Coalition-Reforger-Framework.git
```

2. Open in Arma Reforger Workbench:
   - File → Open Project
   - Navigate to `addon.gproj`

3. Configure workspace settings:
   - Set script editor preferences
   - Configure build settings for target platforms

### Building and Testing

1. **Script Compilation**: Workbench automatically compiles scripts
2. **Asset Processing**: Use Resource Manager for asset imports
3. **Mission Testing**: Use built-in mission editor and multiplayer testing

### Adding New Game Modes

1. Create new game mode class extending `SCR_BaseGameModeComponent`:

```cpp
class MyCustomGamemodeManager: SCR_BaseGameModeComponent
{
    // Game mode specific attributes
    [Attribute("Default Value")] string m_MyAttribute;
    
    // Initialization
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        // Custom initialization logic
    }
    
    // Game mode logic
    void MyGamemodeFunction()
    {
        // Implementation
    }
}
```

2. Add component to gamemode entity in mission
3. Configure attributes in entity properties
4. Implement game state management integration

## API Reference

### Core Manager Access

```cpp
// Get framework managers
CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
CRF_SlottingManager slotting = CRF_SlottingManager.GetInstance();
CRF_GearscriptManager gearscript = CRF_GearscriptManager.GetInstance();
```

### Player Management

```cpp
// Get player information
int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);

// Check player roles
bool isSpectator = CRF_GamemodeManager.IsSpectator(entity);
bool isModerator = CRF_GamemodeManager.GetInstance().IsPlayerModerator(playerId);
```

### Equipment System

```cpp
// Apply gear to entity
CRF_GearscriptManager gearManager = CRF_GearscriptManager.GetInstance();
gearManager.SetEntityGear(entity, prefabResourceName);

// Get role information
CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceName);
CRF_RoleConfig roleConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);
```

### UI Management

```cpp
// Open specific menus
CRF_PlayerControllerManager playerController = CRF_PlayerControllerManager.GetInstance();
playerController.OpenSlottingMenu();
playerController.OpenBriefingMenu();

// Display notifications
playerController.ShowHint("Message to display", 5000); // 5 second duration
```

## Logging and Debugging

### Server Logging

The framework includes comprehensive logging:

```cpp
class CRF_LoggingManager: SCR_BaseGameModeComponent
{
    void LogPlayerAction(string action, int playerId, string details)
    {
        string logEntry = string.Format("%1,%2,%3,%4", 
            GetServerTime(), action, playerId, details);
        WriteToLogFile(logEntry);
    }
}
```

### Debug Features

- **Admin Menu**: Real-time game state monitoring
- **Console Commands**: Administrative controls
- **Network Debugging**: RPC call monitoring

## Performance Considerations

### Optimization Guidelines

1. **Entity Management**: Use object pooling for frequently spawned entities
2. **Network Traffic**: Minimize RPC frequency and payload size
3. **Script Performance**: Avoid expensive operations in frequently called functions
4. **Memory Usage**: Clean up unused references and arrays

### Monitoring Tools

- **Resource Manager**: Asset loading monitoring
- **Script Profiler**: Performance bottleneck identification
- **Network Monitor**: Replication overhead analysis

## Contributing

### Code Standards

1. **Naming Conventions**:
   - Classes: `CRF_ClassName`
   - Methods: `PascalCase`
   - Variables: `m_camelCase` (member), `camelCase` (local)

2. **Documentation**:
   - Document all public methods
   - Include parameter descriptions
   - Provide usage examples for complex features

3. **Testing**:
   - Test in both singleplayer and multiplayer environments
   - Verify network synchronization
   - Check performance impact

### Pull Request Process

1. Fork the repository
2. Create feature branch: `feature/your-feature-name`
3. Implement changes with tests
4. Submit pull request with detailed description
5. Address review feedback

### Issue Reporting

Report bugs and feature requests through:
- **GitHub Issues**: Technical problems and feature requests
- **Discord**: Community discussion and support
- **Email**: Security vulnerabilities

## License

This project is licensed under the Arma Public License. See the license file for details.

## Support

- **Discord**: [https://discord.gg/the-coalition](https://discord.gg/the-coalition)
- **GitHub**: [https://github.com/CoalitionArma/Coalition-Reforger-Framework](https://github.com/CoalitionArma/Coalition-Reforger-Framework)
- **Ko-fi**: [Support the developers](https://ko-fi.com/I2I1VTOXS)

---

*Last updated: August 2025*
*Framework Version: Latest*
*Arma Reforger Compatibility: Current stable version*
