# CRF Rush Gamemode Component

## Overview
The CRF Rush Gamemode Component implements a Rush-style gamemode for the Coalition Reforger Framework. It features three zones with two MCOM (M-COM) sites each. When both MCOMs in a zone are destroyed, the next zone unlocks for attack.

## Features
- **3 Zones**: Each zone contains two MCOM sites (Alpha and Beta)
- **Progressive Unlocking**: Zones unlock sequentially as the previous zone is cleared
- **Faction-Based Gameplay**: One side attacks, one side defends
- **Countdown System**: Planted bombs have a configurable countdown timer
- **Map Markers**: Dynamic markers show active MCOM sites
- **Explosion Effects**: Visual and audio effects when MCOMs are destroyed
- **Player Respawning**: Automatic respawn when zones advance

## Setup Requirements

### 1. Entity Placement
For the Rush gamemode to work, you must place the following entities in your world:

#### Zone 1 Triggers:
- Entity name: `z1_alpha_trigger` (Zone 1 Alpha MCOM spawn point)
- Entity name: `z1_beta_trigger` (Zone 1 Beta MCOM spawn point)

#### Zone 2 Triggers:
- Entity name: `z2_alpha_trigger` (Zone 2 Alpha MCOM spawn point)
- Entity name: `z2_beta_trigger` (Zone 2 Beta MCOM spawn point)

#### Zone 3 Triggers:
- Entity name: `z3_alpha_trigger` (Zone 3 Alpha MCOM spawn point)
- Entity name: `z3_beta_trigger` (Zone 3 Beta MCOM spawn point)

**Important**: All six trigger entities must be present in the world or the gamemode will fail to initialize.

### 2. Component Configuration
Add the `CRF_RushGamemodeManager` component to your gamemode and configure the following attributes:

#### Faction Settings:
- **Attacking Side**: Choose which faction attacks the MCOMs (BLU/OPF/IND)
- **Defending Side**: Choose which faction defends the MCOMs (BLU/OPF/IND)

#### MCOM Settings:
- **MCOM Prefab**: The prefab to spawn as MCOM sites (default: CRF_Rush_MCOM.et)
- **MCOM Timer**: Countdown time in seconds before planted bombs explode (default: 45)

#### UI Settings:
- **Hide Map Markers**: Option to hide MCOM markers on the map

## Gameplay Flow

### Phase 1: Zone 1
- Only Zone 1 MCOMs are active and can be interacted with
- Attackers must plant bombs on either Alpha or Beta MCOM
- Defenders can defuse planted bombs
- When both Zone 1 MCOMs are destroyed, Zone 2 unlocks

### Phase 2: Zone 2
- Zone 2 MCOMs become active
- Same mechanics as Zone 1
- When both Zone 2 MCOMs are destroyed, Zone 3 unlocks

### Phase 3: Zone 3
- Final zone with the last two MCOMs
- When both Zone 3 MCOMs are destroyed, attackers win
- Game advances to After Action Report (AAR) state

## Player Actions

### For Attackers:
- **Plant Bomb**: Available on active zone MCOMs when no countdown is active
- Action takes 5 seconds to complete
- Only one bomb can be active at a time across all MCOMs

### For Defenders:
- **Defuse Bomb**: Available on planted MCOMs during countdown
- Action takes 7 seconds to complete
- Can only defuse bombs that are currently planted

## Technical Details

### Files:
1. `scripts/Game/GameMode/Rush/CRF_Rush_Game.c` - Main gamemode component
2. `scripts/Game/GameMode/Rush/CRF_Rush_PlantBombAction.c` - Plant bomb action
3. `scripts/Game/GameMode/Rush/CRF_Rush_DefuseBombAction.c` - Defuse bomb action
4. `Prefabs/Structures/CRF_Rush_MCOM.et` - MCOM prefab with Rush actions
5. `cripts/Game/GameMode/Rush/CRF_Rush_3dMarkerComponent.c` - Creates the 3d marker widgets

### Network Architecture:
- Uses RPC system for client-server communication
- Client actions trigger server-side gamemode logic
- Server replicates game state to all clients
- Supports proper network synchronization

### Integration Points:
- Extends `SCR_BaseGameModeComponent`
- Integrates with CRF manager systems
- Uses CRF notification and marker systems
- Leverages CRF respawn management
