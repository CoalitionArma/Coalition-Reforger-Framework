# CRF Mission Marker – README

This guide explains how to set up a Mission Marker object for your scenario using the CRF Objective Marker system.

---

## 📌 1. Placing the Mission Marker

Start by placing **any Objective Marker prefab** from: 

```PrefabsEditable/Markers/ObjectiveMarkers/```

Each marker is labeled as:

```{Faction}ObjectiveMarker.et```

Choose the one appropriate for your mission.  
- **Faction-specific markers** are only visible to players of that faction.  
- **GlobalObjectiveMarker.et** is visible to *all* factions.
---

## 📝 2. Adding a Mission Description

Select your placed Objective Marker in the World Editor.  
Inside its properties you will find:

- **Mission Marker Description**  
- **Mission Marker Images**

- Write your mission briefing text inside the **Mission Marker Description** field.  There is no practical character limit, but avoid making it so long that it extends off-screen.
---

## 🖼️ 3. Adding Briefing Images

Under **Mission Marker Images**, insert any number of images you want to appear in the briefing.

### Image Requirements:
- **Resolution:** 426×240 (maximum)  
  Using larger images wastes memory and provides no visual improvement.
- **Directory:** 
  Save all your briefing images here
  ```CRFImages/BriefingImages/{YourName}/```
---

## ✅ 4. Final Notes

- You may add **unlimited** images and text, but always check the in-game UI to ensure nothing is clipped or cut off. 