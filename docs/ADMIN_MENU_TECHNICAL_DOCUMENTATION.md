# Coalition Reforger Framework (CRF) - Admin Menu Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Menu System](#menu-system)
5. [Network Communication](#network-communication)
6. [Ticket System](#ticket-system)
7. [Admin Features](#admin-features)
8. [UI Layout System](#ui-layout-system)
9. [Security & Permissions](#security--permissions)
10. [API Reference](#api-reference)
11. [Development Guidelines](#development-guidelines)

## Overview

The CRF Admin Menu is a comprehensive administrative interface that provides server administrators and moderators with powerful tools for managing players, game state, and server operations in real-time. The system is built on a client-server architecture with secure network communication and role-based access control.

### Key Features
- **Player Management**: Respawning, teleportation, gear reset, healing
- **Ticket System**: Player support requests and admin response system
- **Game State Control**: Timer manipulation, faction ticket management, gear set updates
- **Communication Tools**: Admin messaging, faction-wide hints, broadcasting
- **Real-time Monitoring**: Admin action logging, player status tracking

## Architecture

### System Overview
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Client Side   │    │   Network RPC   │    │   Server Side   │
├─────────────────┤    ├─────────────────┤    ├─────────────────┤
│ CRF_AdminMenu   │◄──►│ RplToAuthority  │◄──►│ AdminMenuManager│
│ UI Components   │    │ RplBroadcast    │    │ GamemodeManager │
│ Input Handling  │    │ Manager         │    │ RespawnManager  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Component Relationships
- **CRF_AdminMenuManager**: Server-side manager handling ticket storage and admin action logging
- **CRF_AdminMenu**: Client-side UI controller managing menu interactions and display
- **CRF_RplToAuthorityManager**: Client-side RPC sender for server requests
- **CRF_RplBroadcastManager**: Server-side RPC handler for client communications

## Core Components

### 1. CRF_AdminMenuManager

**Location**: `scripts/Game/Systems/Core/Managers/CRF_AdminMenuManager.c`

**Purpose**: Server-side component responsible for managing tickets and admin action logs.

#### Key Data Structures:
```cpp
class CRF_TicketMessageData
{
    string sender;      // Player name who sent the message
    string msg;         // Message content
    string timestamp;   // HH:MM:SS formatted timestamp
}

class CRF_AdminActionLog
{
    string timestamp;   // When the action occurred
    string action;      // Description of the admin action
}

class CRF_Ticket
{
    int ticketID;                               // Player ID who created ticket
    ref array<ref CRF_TicketMessageData> messages; // All messages in ticket thread
}
```

#### Core Methods:
- `GetInstance()`: Singleton accessor for server components
- `GetFormattedTimestamp()`: Creates HH:MM:SS timestamp strings
- `StoreAdminLogs(string data)`: Records admin actions for audit trail
- `NewTicketMessage(int ticketID, int senderID, string data)`: Creates/updates tickets
- `CloseTicket(int ticketID)`: Removes ticket from active list
- `RefreshLists()`: Updates client UI when data changes

### 2. CRF_AdminMenu (UI Controller)

**Location**: `scripts/Game/Systems/UI/Menus/CRF_AdminMenuUI.c`

**Purpose**: Client-side menu controller handling user interactions and UI management.

#### Menu Structure:
```
Admin Menu
├── Tickets (Support requests)
├── Respawn (Player revival)
├── Reset Gear (Equipment management)
├── Teleport (Player positioning)
├── Hint (Messaging system)
├── Heal (Health/Vehicle repair)
└── Gamemode (Settings control)
```

#### Core UI Components:
- **Menu Navigation**: Tab-based system with visual feedback
- **List Boxes**: Dynamic population of players, groups, and data
- **Action Buttons**: Context-sensitive operations
- **Input Fields**: Text entry for messages and commands

## Menu System

### Menu Initialization Flow
```cpp
OnMenuOpen()
├── Initialize manager references
├── Setup UI components
├── Register input handlers
├── Initialize chat panel
├── Load default menu (Tickets)
└── Populate admin action logs
```

### Dynamic Menu Loading
Each submenu uses dynamic widget creation:
```cpp
void InitializeTicketMenu()
{
    // Load layout dynamically
    m_wMenuContent = GetGame().GetWorkspace().CreateWidgets(
        "{FD7582ED92D34192}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/TicketMenu.layout"
    );
    
    // Setup components and event handlers
    SetupEventHandlers();
    PopulateData();
}
```

### Menu Navigation System
- **Tab-based Interface**: Each major function has its own tab
- **Color-coded Status**: Active/inactive visual indicators
- **Context Preservation**: Selected items maintained during refreshes
- **Real-time Updates**: Auto-refresh when server data changes

## Network Communication

### RPC Architecture

The admin system uses a dual-RPC approach:

1. **Client → Server**: `CRF_RplToAuthorityManager`
2. **Server → Client**: `CRF_RplBroadcastManager`

### Request Flow Example
```cpp
// Client initiates action
CRF_RplToAuthorityManager.GetInstance().RespawnPlayer(playerId, spawnId);

// Sends RPC to server
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
void RpcAsk_RespawnPlayer(int playerId, RplId SpawnRplID);

// Server processes and broadcasts result
[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
void RpcDo_LogAdminAction(string data, int playerId, bool sendToPlayer);
```

### Security Validation
All server-side RPCs include permission checks:
```cpp
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
{
    // Implicit admin check through RPC restriction
    if (!Replication.IsServer())
        return;
    
    // Process admin action...
}
```

## Ticket System

### Ticket Lifecycle
```
Player Request → Ticket Creation → Admin Assignment → Resolution → Ticket Closure
```

### Message Threading
```cpp
class CRF_Ticket
{
    int ticketID;                               // Unique identifier
    ref array<ref CRF_TicketMessageData> messages; // Message history
    
    void AddMessage(string sender, string msg)
    {
        CRF_TicketMessageData message = new CRF_TicketMessageData;
        message.sender = sender;
        message.msg = msg;
        message.timestamp = CRF_AdminMenuManager.GetFormattedTimestamp();
        messages.Insert(message);
    }
}
```

### Ticket Operations
- **Create**: Player uses `/admin` command to create ticket
- **Reply**: Admins can respond to tickets via menu
- **Assign**: Admins can take ownership of tickets
- **Close**: Tickets are removed when resolved

### Real-time Updates
The ticket system provides instant updates:
- New tickets appear immediately in admin interface
- Message threading maintains conversation history
- Auto-refresh ensures UI stays synchronized

## Admin Features

### 1. Player Respawn System
**Purpose**: Revive dead players at specific locations or with groups

**Capabilities**:
- Individual player respawn at spawn points
- Faction-wide respawn operations
- Group-based spawning for tactical positioning
- Respawn logging for audit trail

### 2. Gear Management
**Purpose**: Reset player equipment and manage faction loadouts

**Features**:
- Individual player gear reset to role defaults
- Faction-wide gear set updates
- Item addition to player inventories
- Equipment validation and role verification

### 3. Teleportation System
**Purpose**: Move players for admin purposes or assistance

**Options**:
- Player-to-player teleportation
- Admin-to-player movement
- Group-based positioning
- Emergency extraction capabilities

### 4. Communication Tools
**Purpose**: Broadcast information and provide player assistance

**Methods**:
- Global server messages
- Faction-specific announcements
- Individual player notifications
- Emergency broadcasting

### 5. Health & Repair System
**Purpose**: Heal players and repair vehicles instantly

**Functions**:
- Full player health restoration
- Vehicle damage repair
- Bulk healing operations
- Emergency medical response

### 6. Game State Control
**Purpose**: Modify game parameters during operation

**Controls**:
- Mission timer adjustment
- Faction ticket manipulation
- Game phase advancement
- Configuration updates

## UI Layout System

### Widget Hierarchy
```
AdminMenu.layout
├── Root Container
│   ├── Navigation Tabs
│   ├── Content Area (Dynamic)
│   ├── Chat Panel
│   └── Action Logs
└── Sub-menu Layouts
    ├── TicketMenu.layout
    ├── RespawnMenu.layout
    ├── GearMenu.layout
    ├── TeleportMenu.layout
    ├── HintMenu.layout
    ├── HealMenu.layout
    └── GameModeMenu.layout
```

### Component Types
- **SCR_ListBoxComponent**: Player/group selection lists
- **SCR_ButtonTextComponent**: Action buttons with event handling
- **MultilineEditBoxWidget**: Message composition areas
- **EditBoxWidget**: Single-line input fields
- **TextWidget**: Status displays and labels

### Dynamic Content Loading
Menus load content on-demand:
```cpp
// Clear previous content
if (m_wMenuContent)
    delete m_wMenuContent;

// Load new menu layout
m_wMenuContent = GetGame().GetWorkspace().CreateWidgets(layoutPath);

// Initialize components
SetupEventHandlers();
PopulateInitialData();
```

## Security & Permissions

### Access Control
Admin menu access is controlled through multiple layers:

1. **Game Engine Level**: `SCR_Global.IsAdmin()`
2. **Framework Level**: `CRF_GamemodeManager.IsModerator()`
3. **Menu Level**: UI visibility based on permissions
4. **RPC Level**: Server-side validation for all actions

### Permission Hierarchy
```
Super Admin (Full Access)
├── Game State Control
├── All Player Management
├── Configuration Changes
└── Emergency Powers

Moderator (Limited Access)
├── Player Support (Tickets)
├── Basic Player Management
├── Communication Tools
└── Healing/Respawn
```

### Audit Trail
All admin actions are logged:
```cpp
void StoreAdminLogs(string data)
{
    CRF_AdminActionLog log = new CRF_AdminActionLog();
    log.action = data;
    log.timestamp = GetFormattedTimestamp();
    m_mAdminActions.Insert(log);
    
    // Notify connected admins
    RefreshLists();
}
```

## API Reference

### Core Manager Methods

#### CRF_AdminMenuManager
```cpp
// Singleton access
static CRF_AdminMenuManager GetInstance();

// Ticket management
void NewTicketMessage(int ticketID, int senderID, string data);
array<int> GetOpenTickets();
array<ref CRF_TicketMessageData> GetTicketMessages(int playerID);
void CloseTicket(int ticketID);
bool TicketExists(int ticketID);

// Admin logging
void StoreAdminLogs(string data);
array<ref CRF_AdminActionLog> GetAdminActionLogs();

// Utility
static string GetFormattedTimestamp();
void RefreshLists();
```

#### CRF_RplToAuthorityManager (Client-side)
```cpp
// Player management
void RespawnPlayer(int playerId, RplId SpawnRplID);
void RespawnFaction(FactionKey faction, bool logAction);
void TeleportPlayers(int playerId1, int playerId2, bool logAction);
void Heal(int playerId, bool logAction, bool isVehicle = false);

// Equipment management
void ResetGear(int playerId, ResourceName prefab, bool logAction);
void UpdateGearSet(string faction, ResourceName path);
void AddItem(int playerId, string prefab, bool logAction);

// Communication
void SendHint(string data, int playerId = -1, string factionKey = "");
void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction);

// Ticket management
void AssignAdminTicket(int ticketID, int adminID, bool logAction);
void CloseAdminTicket(int ticketID, int adminID, bool logAction);

// Game state
void UpdateTimer(int delta);
void UpdateTicket(string action, FactionKey faction, int delta);
void RequestAdvanceGamemodeState(bool overriden);
```

### UI Component Access
```cpp
// Get UI components from current menu
SCR_ListBoxComponent GetListBox(string listbox, Widget widget = null);
SCR_ButtonTextComponent GetMenuButton(string button, Widget widget = null);
MultilineEditBoxWidget GetMultilineEditBox(string multiEditBox, Widget widget = null);
EditBoxWidget GetEditBox(string EditBox, Widget widget = null);
```

### Event Handling
```cpp
// Button click handlers
button.m_OnClicked.Insert(CallbackFunction);

// List selection changes
listBox.m_OnChanged.Insert(SelectionChangedCallback);

// Input validation
if (selectedItem < 0 || selectedItem >= listBox.GetItemCount())
    return;
```

## Development Guidelines

### Adding New Admin Features

1. **Create RPC Methods**:
```cpp
// Client-side request
void NewAdminFeature(int parameter)
{
    Rpc(RpcAsk_NewAdminFeature, parameter);
}

// Server-side handler
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void RpcAsk_NewAdminFeature(int parameter)
{
    // Validate permissions
    // Process request
    // Log action if needed
}
```

2. **Create UI Layout**:
- Design widget layout file
- Implement menu initialization function
- Add navigation button and handler

3. **Add Data Management**:
- Update CRF_AdminMenuManager if persistent data needed
- Implement real-time refresh mechanisms
- Add audit logging for actions
