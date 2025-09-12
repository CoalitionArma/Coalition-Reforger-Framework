# Coalition Reforger Framework (CRF) - Vehicle Depot User Guide

## Overview

The CRF Vehicle Depot system allows players to spawn vehicles using various cost systems. Includes collision detection, role restrictions, and configurable spawn patterns.

![Vehicle Depot Example](images/vehicle_depot_example.png)

### Features:
- Easy vehicle depot setup spawn alternative
- 3 Use Cases (Custom Use count, Side Tickets, & Nearby Storage Detection)
- Customizable spawn locations

## Quick Setup Guide (Prefab Use)

| Step | Action | Details |
|------|--------|---------|
| 1 | **Place Prefab** | Place prefab "**VehicleDepot.et**" or "**VehicleDepot_Cache.et**" (Comes with supply cache) → Place in mission → Orient toward spawn direction |
| 2 | **Basic Config** | Set Depot Name and Interaction Range (default: 5m) |
| 4 | **Add Vehicles** | Vehicle Configuration → Add vehicles with Name, Prefab, Cost Type, Cost Amount |
| 5 | **Configure Spawning** | Set Spawn Pattern, Distance, Spacing, Point Count or desired settings for spawning. |

### ⚠️ **Critical**: 
- If Exceeding 10+ vehicles (The prefab default amount), It will no longer auto-detect vehicle additions <br> Add/Duplicate more **AdditionalActions**:**CRF_DepotSpawnActions** within the **ActionsManager** and change their vehicle indexes <br> (IE: 15 vehicles in **VehicleDepot** -> 15 **CRF_DepotSpawnActions** -> All with indices 0-14)

## Configuration Settings

### Cost Types
| Type | Source | Usage | Best For |
|------|--------|-------|----------|
| **TICKETS** | CRF Side Ticket System | Spawns Vs Assets Balancing | High-value vehicles, spam prevention |
| **SUPPLIES** | Nearby supply storage | Shared resource pool | Logistics gameplay, resource management |
| **USES** | Global depot pool | Shared use count | Limited spawns, event vehicles |

### Basic Configuration
| Setting | Default | Description |
|---------|---------|-------------|
| **Depot Name** | "Vehicle Depot" | Display name for players |
| **Interaction Range** | 5m | Distance players can use depot |

### Spawn Configuration  
| Setting | Default | Description |
|---------|---------|-------------|
| **Spawn Distance** | 6m | Distance from depot to spawn area |
| **Spawn Spacing** | 6m | Space between spawn points |
| **Spawn Point Count** | 4 | Number of spawn positions |
| **Collision Radius** | 4m | Detection radius for obstacles |

### Grid Pattern (when using GRID)
| Setting | Default | Description |
|---------|---------|-------------|
| **Grid Columns** | 3 | Vehicles per row |
| **Grid Row Spacing** | 8m | Distance between rows |

### Resource Configuration
| Setting | Default | Description |
|---------|---------|-------------|
| **Supply Search Radius** | 50m | Range to find supply storage |
| **Global Uses Pool** | 20 | Total uses for USES-type vehicles |

### Other Options
| Setting | Description |
|---------|-------------|
| **Restrict to Leadership** | Limit access to Squad/Team/Platoon Leaders only |
| **Enable Debug Logging** | Console output for troubleshooting |
| **Show Debug Visuals** | Spawn point visualization (Workbench only) |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **No vehicles in menu** | Check CRF_DepotSpawnAction entries have correct indices (0, 1, 2...) |
| **"Vehicle index X not found"** | Verify vehicle indices match array positions starting from 0 |
| **Vehicles spawn in objects/land on people** | Increase Collision Radius, Spawn Point Count, or Spawn Distance |
| **"No space available"** | Clear obstacles, increase Spawn Points, try different Spawn Pattern, Distances, or smaller Collision Radius |
| **Can't afford vehicles** | Check ticket/supply availability or global uses pool |
| **It doesnt spawn vehicles** | Toggle debug OR reload Workbench |

## Custom Depot Creation (Non-Prefab Use)
| Step | Action | Details |
|------|--------|---------|
| 1 | **Add CRF_VehicleDepot & ActionsManagerComponent To Object** | Set Depot Name and Interaction Range (default: 5m) |
| 2 | **Set Up Vehicle Depot** | Add vehicles, set up desired settings |
| 3.A | **Set Up ActionsManager Component** | Create an Action Context. Name it anything. <br><br> Under ActionsManager, Add a **PointInfo** to the **Position** class, and customize PointInfo offset for menu location. <br><br> **Set a Radius and Height for menu distance interactibility**   |
| 3.B | **Set Up ActionsManager Component** | Under **Additional Actions**, add the **CRF_DepotSpawnAction** to it. <br><br> If you set up **3.A** correctly, you should be able to set the **ParentContextList** of the **DepotSpawnAction** to the **Action Context**->**Context Name** (This links any **Additional Actions**->**CRF_DepotSpawnActions** you add together.) <br><br> Add as many indexes to it as there are vehicles in **CRF_VehicleDepot**. <img width="150" height="300" alt="image" src="https://github.com/user-attachments/assets/8dbd334f-103a-4081-8b6d-4217e2fb1450" />|
| 4 | **Done?** | If done correctly, should now be functional. If wanting a good example or to copy properties, use prefabs. |


