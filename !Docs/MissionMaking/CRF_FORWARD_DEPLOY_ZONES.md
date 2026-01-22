# Faction Forward Deployment – Mission Maker Guide

**Faction Forward Deployment** allows mission makers to create zone-based forward insertion areas for each faction. Squad leaders can reposition their groups during SafeStart using the map radial menu, keeping early-game flow clean and controlled.

---

## 📦 Overview

Faction Forward Deployment works using world-placed prefabs named:

- **`BLUFORForwardDeployZone`**
- **`OPFORForwardDeployZone`**
- **`INDFORForwardDeployZone`**

Once placed and drawn, these zones allow faction leaders to right-click on the map and forward deploy their entire squad into the zone—**but only during SafeStart**.

After SafeStart ends, all zones are deleted and the option disappears.

---

## 🎨 Setting Up Zones (Mission Maker Steps)

### 1. **Place the Prefabs**
Drag any `{FactionKey}ForwardDeployZone` prefab into the world.

You can place multiple zones per faction if needed.

### 2. **Draw the Actual Zone**
In the editor:

1. Press **V** to enter the edit/drawing mode  
2. Hold **Ctrl + Left Mouse Button** and click to draw the polyline point
3. Continue placing points until you are satisfied with the shape

This polyline defines **where leaders may teleport their squad**.

---

## 🧭 How Leaders Use Forward Deployment

During SafeStart:

1. Open the **Map**
2. **Right-click** inside your faction’s forward deploy zone
3. Select **Forward Deploy Element** from the radial menu
4. The system teleports the entire group to the clicked location

**The target location *must* be inside the zone**, or the request is rejected.

---

## 🔒 SafeStart Behavior

- Forward Deployment is only available **while SafeStart is active**
- Once a squad teleports into the zone, they **cannot leave it** until SafeStart ends
- When SafeStart ends:
  - Forward deploy zones are **deleted**
  - The radial menu **Forward Deploy** option is disabled

This prevents early rushing, map exploitation, or grief starts while still letting leaders stage their teams.

---

## 📝 Notes & Behaviors

- Mixed groups in vehicles will only teleport with the group that has the most units in that vehicle.
- Players inside vehicles **teleport with their group**    
- Leaders can use the feature multiple times *until* SafeStart ends  

---

## ❓ FAQ

**Q: Can players teleport individually?**  
No. Only group leaders can trigger it, and it always moves the **whole group**.

**Q: Can factions start outside the zone?**  
Yes—but once they forward deploy in, they cannot leave until SafeStart ends.

---

## 👍 Why Use Forward Deployment?

- Keeps early match phases safe and organized  
- Prevents premature combat before SafeStart ends  
- Lets leaders stage their groups tactically  

---