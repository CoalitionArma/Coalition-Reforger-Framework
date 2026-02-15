/*
	HVT/VIP Gamemode Component
	
	Tracks HVTs/VIPs (PHVT/AI/Object) and syncs their positions to map markers.
	Supports multiple HVTs/VIPs with per-entry faction, marker text, and color configuration.
	
	NOTES:
	- Currently to track HVT JIPs/respawns & reapply vulnerability we use m_fPlayerCheckTimer (HVTs are invulnerable between re-registration. Edge case issue is that someone is respawned and within 60s is attempted to be killed. Shouldnt happen realistically.) Not ideal setup but cant find much else.
	- HVT Prefab selection is what spawns when AI is used, or what Prefab/Slot (optional filter by faction) is used for player-controlled HVTs.

	TODO:
	- Less crappy re-registration of HVTs/VIPs?
	- Edge case where if more than 1 PHVT slots, but unfilled, marker/transponder may be active first minute of game. Race condition not really worth fixing. Unless?
	- Object HVT destruction detection?
*/

enum CRF_HVTFaction
{
	NONE,  
	BLUFOR,
	OPFOR, 
	INDFOR,
	CIV    
}


enum CRF_TargetType
{
	HVT,
	VIP
}

enum CRF_HVTEntryType
{
	AI,     
	PLAYER, 
	OBJECT  
}

// HVT Entry Configuration Class
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTransponderEntityName")]
class CRF_HVTEntry
{
	[Attribute("0", UIWidgets.ComboBox, "Entry type: AI spawns character at transponder, PLAYER detects by prefab match, OBJECT spawns item at transponder.", enums: ParamEnumArray.FromEnum(CRF_HVTEntryType))]
	CRF_HVTEntryType m_eEntryType;
	
	[Attribute("{42A502E3BB727CEB}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliPilot.et", desc: "AI: Character prefab to spawn. PLAYER: Prefab to match the LOBBY SLOT (use a UNIQUE prefab and faction setting for that slot so that the mode detects the PHVT). OBJECT: Item prefab to spawn on the transponder object(e.g. radio).", uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_hvtPrefab;
	
	[Attribute("0", UIWidgets.ComboBox, "Faction of this HVT. For PLAYER entries, filters by faction. For AI/OBJECT, sets marker color.", enums: ParamEnumArray.FromEnum(CRF_HVTFaction))]
	CRF_HVTFaction m_eFaction;
	
	[Attribute("", "auto", "Name of the transponder entity in world. AI/OBJECT spawn here, PLAYER marker follows here. Must be UNIQUE per entry.")]
	string m_sTransponderEntityName;
	
	[Attribute("Transponder Signal", "auto", "Text displayed on the map marker for this HVT/Object.")]
	string m_sMarkerText;
	
	string GetFactionKey()
	{
		switch (m_eFaction)
		{
			case CRF_HVTFaction.BLUFOR: return "BLUFOR";
			case CRF_HVTFaction.OPFOR:  return "OPFOR";
			case CRF_HVTFaction.INDFOR: return "INDFOR";
			case CRF_HVTFaction.CIV:    return "CIV";
		}
		return "";
	}
	
	int GetMarkerColor()
	{
		switch (m_eFaction)
		{
			case CRF_HVTFaction.BLUFOR: return ARGB(255, 0, 82, 255);   
			case CRF_HVTFaction.OPFOR:  return ARGB(255, 200, 0, 0);    
			case CRF_HVTFaction.INDFOR: return ARGB(255, 0, 170, 70);   
			case CRF_HVTFaction.CIV:    return ARGB(255, 160, 32, 240); 
		}
		return ARGB(255, 0, 0, 225);
	}
}

//------------------------------------------------------------------------------------------------
// HVT Gamemode Component
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "High Value Target gamemode - track and hunt HVTs")]
class CRF_HighValueTargetGamemodeManagerClass: SCR_BaseGameModeComponentClass
{
	
}
class CRF_HighValueTargetGamemodeManager: SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	// GLOBAL SETTINGS
	//------------------------------------------------------------------------------------------------
	
	[Attribute("true", "auto", "Enable transponder markers on the map that track HVT positions.", category: "Global Settings")]
	bool m_bEnableTransponderMarker;
	
	[Attribute("0", UIWidgets.ComboBox, "Target type for notification text (HVT or VIP).", enums: ParamEnumArray.FromEnum(CRF_TargetType), category: "Global Settings")]
	CRF_TargetType m_eTargetType;
	
	[Attribute("false", "auto", "Disable damage on AI/Player HVTs (makes them invulnerable). Does not apply to OBJECT entries.", category: "Global Settings")]
	bool m_bDisableDamage;
	
	[Attribute("360", UIWidgets.SpinBox, "The amount of time between marker updates in seconds. Minimum 10s.", "10 3600 1", category: "Global Settings")]
	int m_timeBetweenPings;
	
	[Attribute("false", "auto", "Hide the transponder marker from all factions except the searcher faction.", category: "Global Settings")]
	bool m_filterFaction;

	[Attribute("BLUFOR", "auto", "Faction key for the searching side (only used if Filter Faction is enabled).", category: "Global Settings")]
	string m_searcherFactionKey;
	
	//------------------------------------------------------------------------------------------------
	// AI HVT SETTINGS (Applied to all AI-spawned HVTs)
	//------------------------------------------------------------------------------------------------
	
	[Attribute("0 0 0", "auto", "The rotation (yaw/pitch/roll) applied to spawned AI HVTs.", category: "AI HVT Settings")]
	vector m_hvtPrefabYaw;
	
	[Attribute("true", "auto", "Set AI HVTs to unconscious state on spawn.", category: "AI HVT Settings")]
	bool m_setUnconcious;
	
	//------------------------------------------------------------------------------------------------
	// HVT ENTRIES
	//------------------------------------------------------------------------------------------------
	
	[Attribute("", "auto", "List of HVT entries. Each entry can be configured as player or AI controlled.", category: "HVT Entries")]
	ref array<ref CRF_HVTEntry> m_aHVTEntries;
	
	[Attribute("HVT GAMEMODE SETUP\n\n=== BASIC SETUP ===\n1. Add this component to your Game Mode Entity\n2. Add 1 or more 'HVT ENTRIES' within the component\n3. Set Entry Type per entry (AI / PLAYER / OBJECT)\n4. Set Prefab per entry\n\n=== AI ENTRY ===\n• Entry Type = AI\n• Place empty transponder entity in world\n• Set 'Prefab' to AI character prefab\n• AI spawns at transponder location\n\n=== PLAYER ENTRY ===\n• Entry Type = PLAYER\n• Place empty transponder entity in world\n• Set 'Prefab' to PLAYER SLOT prefab\n• Set 'Faction' to filter by faction (optional)\n⚠️ Matches by PREFAB - use UNIQUE prefab!\n\n=== OBJECT ENTRY ===\n• Entry Type = OBJECT\n• Place empty transponder entity in world\n• Set 'Prefab' to object prefab (e.g. radio)\n• Object spawns at transponder location\n• No death handling - just tracks position", UIWidgets.EditBoxMultiline, "HVT Setup Instructions", category: "Documentation")]
	string m_sInstructions;
	
	// Replicated HVT positions (server → client)
	[RplProp(onRplName: "SyncTransponderPositions")]
	ref array<vector> m_aHvtPositions = {};
	
	// Replicated dead HVT hint (server → client) - triggers hint display
	[RplProp(onRplName: "OnDeadHVTHintReplicated")]
	string m_sDeadHVTHint;
	
	// HVT tracking maps
	ref map<IEntity, int> m_mHVTEntryIndex = new map<IEntity, int>();     // entity → entry index
	ref map<int, IEntity> m_mEntryToHVT = new map<int, IEntity>();        // entry index → entity (prevents scope searches)
	
	// Client-side: Track which markers have been removed for JIPS/mishaps/desyncs
	ref set<int> m_sRemovedMarkerIndices = new set<int>();
	
	const string MARKER_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
	const int MARKER_SIZE = 50;
	
	bool m_bHVTStateSet = false;
	bool m_bGameInit = false;
	bool m_bHasSafestartBegun = false;  // Latch: Ensures we've seen safestart active before watching for it to end
	
	// Exchanging per second origin bumpmes for timeslices + performance (good?)
	float m_fReplicationTimer = 0; // Timer for marker position replication (happens -5s before ping to avoid updating per frame)
	float m_fPlayerCheckTimer = 0; // Checks + ReRegisters + Resets Invulnerability if set if player HVTs every 60s to catch JIP/respawns

	//------------------------------------------------------------------------------------------------
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		SetEventMask(owner, EntityEvent.FIXEDFRAME); // Testing event mask for initial clientside 
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fUpdateBuffer >= 1)
		{
			m_fUpdateBuffer = 0;
			
			if (!CRF_Gamemode.GetInstance().IsRunning())
				return;
			
			if (!m_bHVTStateSet)
			{
				SetHVTAndState();
				return;
			}
			
			// Two-stage safestart check: First ensure we've seen it active, then wait for it to end
			if (!m_bHasSafestartBegun)
			{
				m_bHasSafestartBegun = CRF_SafestartManager.GetInstance().GetSafestartStatus();
				return;
			}
			
			// Now wait for safestart to end before GameInit
			if (!m_bGameInit)
			{
				if (CRF_SafestartManager.GetInstance().GetSafestartStatus())
					return;
				
				GameInit();
				
				// Clients done - disable loop
				if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
					ClearEventMask(owner, EntityEvent.FIXEDFRAME);
				
				return;
			}
			
			// Server: Periodic player HVT re-registration
			if (++m_fPlayerCheckTimer >= 60)
			{
				m_fPlayerCheckTimer = 0;
				RegisterPlayerHVTs();
			}
			
			// Server: Sync positions 5s before marker ping to batch RPL
			if (m_bEnableTransponderMarker && ++m_fReplicationTimer >= m_timeBetweenPings - 5)
			{
				m_fReplicationTimer = 0;
				UpdateHVTPositions();
			}
		}
		m_fUpdateBuffer += timeSlice;
	}
	
	// Spawn AI/Object HVTs at transponder locations, then hide transponders until first position sync
	void SetHVTAndState()
	{
		m_bHVTStateSet = true;
		
		if (!m_aHVTEntries || m_aHVTEntries.Count() == 0)
		{
			Print("[HVT] Error: No HVT entries configured!", LogLevel.ERROR);
			return;
		}
		
		// Server: Spawn AI/Object entries FIRST (while transponders still have their editor positions)
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
			{
				if (!entry || entry.m_eEntryType == CRF_HVTEntryType.PLAYER)
					continue;
				
				if (entry.m_sTransponderEntityName.IsEmpty() || entry.m_hvtPrefab.IsEmpty())
				{
					Print(string.Format("[HVT] Warning: Entry %1 missing transponder name or prefab!", index), LogLevel.WARNING);
					continue;
				}
				
				IEntity transponderEntity = GetGame().GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
				if (!transponderEntity)
				{
					Print(string.Format("[HVT] Warning: Entity '%1' not found!", entry.m_sTransponderEntityName), LogLevel.WARNING);
					continue;
				}
				
				EntitySpawnParams spawnParams = new EntitySpawnParams();
				spawnParams.TransformMode = ETransformMode.WORLD;
				spawnParams.Transform[3] = transponderEntity.GetOrigin();
				
				IEntity hvtEntity = GetGame().SpawnEntityPrefab(Resource.Load(entry.m_hvtPrefab), GetGame().GetWorld(), spawnParams);
				if (!hvtEntity)
					continue;
				
				RegisterHVTEntity(hvtEntity, index);
				
				// AI-specific: rotation and unconscious state
				if (entry.m_eEntryType == CRF_HVTEntryType.AI)
				{
					hvtEntity.SetYawPitchRoll(m_hvtPrefabYaw);
					
					if (m_setUnconcious)
					{
						SetEntityUnconscious(hvtEntity);
						
						SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterControllerComponent));
						if (characterController)
							characterController.m_OnLifeStateChanged.Insert(OnLifeStateChangedWrapper);
					}
				}
			}
		}
		
		// All machines: NOW hide transponders underground so markers don't flash at editor positions
		foreach (CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			IEntity transponder = GetGame().GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
			if (transponder)
				transponder.SetOrigin("0 -1000 0");
		}
	}
	
	// Register/re-register player HVTs @ Gameinit and RegisterPlayerHVTs per 60s
	void RegisterPlayerHVTs()
	{
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_eEntryType != CRF_HVTEntryType.PLAYER)
				continue;
			
			if (m_mEntryToHVT.Contains(index))
			{
				IEntity existingHVT = m_mEntryToHVT.Get(index);
				if (existingHVT && IsHVTAlive(existingHVT))
					continue;
			}
			
			FindAndRegisterPlayerHVT(entry, index);
		}
	}
	
	// Find and register player matching HVT entry
	void FindAndRegisterPlayerHVT(CRF_HVTEntry entry, int entryIndex)
	{
		// Clean up dead HVT references if present
		if (m_mEntryToHVT.Contains(entryIndex))
		{
			IEntity existingHVT = m_mEntryToHVT.Get(entryIndex);
			if (existingHVT)
				m_mHVTEntryIndex.Remove(existingHVT);
			
			m_mEntryToHVT.Remove(entryIndex);
		}
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;
			
			// Skip if already registered as an HVT
			if (m_mHVTEntryIndex.Contains(playerEntity))
				continue;
			
			// Check if the player's character prefab matches our HVT prefab
			if (!playerEntity.GetPrefabData())
				continue;
			
			ResourceName playerPrefab = playerEntity.GetPrefabData().GetPrefabName();
			if (playerPrefab != entry.m_hvtPrefab)
				continue;
			
			// If faction filter is set on the entry, check faction
			if (entry.m_eFaction != CRF_HVTFaction.NONE)
			{
				SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				if (factionManager)
				{
					Faction playerFaction = factionManager.GetPlayerFaction(playerId);
					if (!playerFaction || playerFaction.GetFactionKey() != entry.GetFactionKey())
						continue;
				}
			}
			
			// Found matching player - register as new HVT source
			RegisterHVTEntity(playerEntity, entryIndex);
			return;  // One player per entry
		}
	}
	
	// Register HVT entity - component checks handle type differences naturally (Objects skip death/damage hooks)
	void RegisterHVTEntity(IEntity hvtEntity, int entryIndex)
	{
		if (!hvtEntity || m_mHVTEntryIndex.Contains(hvtEntity))
			return;
		
		// Add to tracking (both forward and reverse maps)
		m_mHVTEntryIndex.Set(hvtEntity, entryIndex);
		m_mEntryToHVT.Set(entryIndex, hvtEntity);
		
		// Hook death callback
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
			characterController.m_OnPlayerDeathWithParam.Insert(OnHVTDeath);
		
		// ALWAYS set damage state to match component setting (overrides prefab default on spawn/respawn)
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageManager)
			damageManager.EnableDamageHandling(!m_bDisableDamage);
	}
	
	// HVT death callback - syncs positions and shows hint
	void OnHVTDeath(SCR_CharacterControllerComponent characterController, IEntity killerEntity, Instigator killer)
	{
		IEntity hvtEntity = characterController.GetOwner();
		
		// Immediately sync positions - zero position will trigger marker removal on clients
		if (m_bEnableTransponderMarker)
			UpdateHVTPositions();
		
		// Build dead HVT label
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		// Get target type label
		string targetLabel = "HVT";
		if (m_eTargetType == CRF_TargetType.VIP)
			targetLabel = "VIP";
		
		// Get HVT's faction from entry
		string hvtFactionLabel = "";
		if (m_mHVTEntryIndex.Contains(hvtEntity))
		{
			int entryIndex = m_mHVTEntryIndex.Get(hvtEntity);
			if (entryIndex >= 0 && entryIndex < m_aHVTEntries.Count())
			{
				CRF_HVTEntry entry = m_aHVTEntries[entryIndex];
				if (entry && entry.m_eFaction != CRF_HVTFaction.NONE)
					hvtFactionLabel = entry.GetFactionKey() + " ";
			}
		}
		
		// Get player name if this was a player-controlled HVT
		string playerNameLabel = "";
		int hvtPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(hvtEntity);
		if (hvtPlayerId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(hvtPlayerId);
			if (playerName != "")
				playerNameLabel = " (" + playerName + ")";
		}
		
		// Get killer info
		string killerLabel = "";
		if (factionManager)
		{
			Faction killerFaction = factionManager.GetPlayerFaction(killer.GetInstigatorPlayerID());
			if (killerFaction)
				killerLabel = " - Killed by " + killerFaction.GetFactionKey();
		}
		
		// Set hint string and trigger replication
		m_sDeadHVTHint = hvtFactionLabel + targetLabel + playerNameLabel + killerLabel;
		Replication.BumpMe();
		OnDeadHVTHintReplicated();
	}
	
	// Client: Show hint when replicated
	void OnDeadHVTHintReplicated()
	{
		if (m_sDeadHVTHint.IsEmpty())
			return;
		
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (!hintManager)
			return;
		
		// Get title based on target type
		string hintTitle = "HVT KILLED";
		if (m_eTargetType == CRF_TargetType.VIP)
			hintTitle = "VIP KILLED";
		
		hintManager.ShowCustomHint(m_sDeadHVTHint, hintTitle, 10);
	}
	
	void GameInit()
	{
		m_bGameInit = true;
		
		// Server: Register player HVTs now that safestart has ended and players are spawned
		if (RplSession.Mode() == RplMode.Dedicated || RplSession.Mode() == RplMode.Listen)
		{
			RegisterPlayerHVTs();
			
			if (m_bEnableTransponderMarker)
				UpdateHVTPositions();
		}
		
		// Client/Listen: Create markers
		if (!m_bEnableTransponderMarker)
			return;
		
		CRF_PlayerControllerManager gameModePlayerComponent = CRF_PlayerControllerManager.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		if (m_filterFaction)
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (!factionManager)
				return;
			
			Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
			if (!faction || faction.GetFactionKey() != m_searcherFactionKey)
				return;
		}
		
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			IEntity transponder = GetGame().GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
			if (!transponder)
			{
				Print(string.Format("[HVT] Warning: Transponder entity '%1' not found in world!", entry.m_sTransponderEntityName), LogLevel.WARNING);
				continue;
			}
			
			gameModePlayerComponent.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
		}
	}
	
	// Server: Update and replicate HVT positions
	void UpdateHVTPositions()
	{
		// Ensure entries array is valid before continue
		if (!m_aHVTEntries || m_aHVTEntries.Count() == 0)
			return;
		
		int entryCount = m_aHVTEntries.Count();
		
		// Clear and rebuild positions array
		m_aHvtPositions.Clear();
		
		for (int i = 0; i < entryCount; i++)
		{
			vector pos = vector.Zero;
			
			// O(1) lookup using reverse map
			if (m_mEntryToHVT.Contains(i))
			{
				IEntity hvtEntity = m_mEntryToHVT.Get(i);
				if (hvtEntity && IsHVTAlive(hvtEntity))
					pos = hvtEntity.GetOrigin();
			}
			
			m_aHvtPositions.Insert(pos);
		}
		
		Replication.BumpMe();
		SyncTransponderPositions();
	}
	
	// Check if HVT is alive/valid via damage state (or entity existence for objects)
	bool IsHVTAlive(IEntity hvtEntity)
	{
		if (!hvtEntity)
			return false;
		
		// Check entry type - OBJECT entries don't have character damage managers
		if (m_mHVTEntryIndex.Contains(hvtEntity))
		{
			int entryIndex = m_mHVTEntryIndex.Get(hvtEntity);
			if (entryIndex >= 0 && entryIndex < m_aHVTEntries.Count())
			{
				CRF_HVTEntry entry = m_aHVTEntries[entryIndex];
				if (entry && entry.m_eEntryType == CRF_HVTEntryType.OBJECT)
					return true;  // Objects are always "alive" if entity exists
			}
		}
		
		// For AI/PLAYER - query actual character state via damage manager
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damageManager)
			return false;
		
		// GetState() returns EDamageState - check if not destroyed/dead
		return damageManager.GetState() != EDamageState.DESTROYED;
	}
	
	// Client: Sync transponder positions from replicated array
	void SyncTransponderPositions()
	{
		if (!m_aHvtPositions || m_aHvtPositions.Count() == 0)
			return;
		
		// Guard: Ensure entries array is valid before iterating
		if (!m_aHVTEntries || m_aHVTEntries.Count() == 0)
			return;
		
		CRF_PlayerControllerManager gameModePlayerComponent = CRF_PlayerControllerManager.GetInstance();
		
		// Use min of both arrays to prevent index out of bounds
		int iterCount = Math.Min(m_aHVTEntries.Count(), m_aHvtPositions.Count());
		for (int i = 0; i < iterCount; i++)
		{
			CRF_HVTEntry entry = m_aHVTEntries[i];
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			vector newPos = m_aHvtPositions[i];
			
			// Zero position means HVT is dead - remove marker
			if (newPos == vector.Zero)
			{
				if (gameModePlayerComponent && m_bEnableTransponderMarker)
				{
					gameModePlayerComponent.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
					m_sRemovedMarkerIndices.Insert(i);
				}
				continue;
			}
			
			// Non-zero position - check if marker was previously removed and needs re-creation
			if (m_sRemovedMarkerIndices.Contains(i) && gameModePlayerComponent && m_bEnableTransponderMarker)
			{
				gameModePlayerComponent.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
				gameModePlayerComponent.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
				m_sRemovedMarkerIndices.Remove(i);
			}
			
			IEntity transponder = GetGame().GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
			if (!transponder)
				continue;
			
			transponder.SetOrigin(newPos);
		}
	}
	
	// Set AI HVT entity unconscious with no regen
	void SetEntityUnconscious(IEntity entity)
	{
		if (!entity)
			return;
		
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (!characterController)
			return;
		
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damageManager)
			return;
		
		characterController.SetUnconscious(true);
		damageManager.SetRegenScale(0, true);
	}
	
	// Wrapper for life state changed callback - re-applies unconscious to AI HVTs only
	void OnLifeStateChangedWrapper()
	{
		foreach (IEntity hvtEntity, int entryIndex : m_mHVTEntryIndex)
		{
			// Only apply to AI entries
			if (entryIndex < 0 || entryIndex >= m_aHVTEntries.Count())
				continue;
			
			if (m_aHVTEntries[entryIndex].m_eEntryType != CRF_HVTEntryType.AI)
				continue;
			
			if (hvtEntity && IsHVTAlive(hvtEntity))
				SetEntityUnconscious(hvtEntity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// EXTERNAL QUERY METHODS
	// If you wish to add your own logic, do so below. Hopefully provided good enough external methods to quiery HVT counts or presence
	//------------------------------------------------------------------------------------------------
	
	// Find an HVT within range of a position
	// position: Center point to search from
	// range: Search radius in meters
	// faction: Filter by CRF_HVTFaction, or NONE to match any faction
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT), or -1 to match any type
	// Returns: First matching alive HVT entity, or null if none found
	//
	// Example usage:
	//   CRF_HighValueTargetGamemodeManager hvtManager = CRF_HighValueTargetGamemodeManager.Cast(CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager));
	//   vector searchPosition = ownerEntity.GetOrigin();
	//   if (hvtManager.FindHVTInRange(searchPosition, 50))                                              // Any HVT within 50m
	//   if (hvtManager.FindHVTInRange(searchPosition, 100, CRF_HVTFaction.BLUFOR))                      // BLUFOR HVT within 100m
	//   if (hvtManager.FindHVTInRange(searchPosition, 50, CRF_HVTFaction.NONE, CRF_HVTEntryType.OBJECT)) // Any OBJECT type within 50m
	IEntity FindHVTInRange(vector position, float range, CRF_HVTFaction faction = CRF_HVTFaction.NONE, int entryType = -1)
	{
		foreach (IEntity hvtEntity, int entryIndex : m_mHVTEntryIndex)
		{
			if (!hvtEntity || !IsHVTAlive(hvtEntity))
				continue;
			
			// Validate entry index
			if (entryIndex < 0 || entryIndex >= m_aHVTEntries.Count())
				continue;
			
			CRF_HVTEntry entry = m_aHVTEntries[entryIndex];
			if (!entry)
				continue;
			
			// Filter by faction if specified (NONE = match any)
			if (faction != CRF_HVTFaction.NONE && entry.m_eFaction != faction)
				continue;
			
			// Filter by entry type if specified (-1 = match any)
			if (entryType >= 0 && entry.m_eEntryType != entryType)
				continue;
			
			// Check distance
			if (vector.Distance(position, hvtEntity.GetOrigin()) <= range)
				return hvtEntity;
		}
		
		return null;
	}
	
	// Count HVTs within range of a position
	// position: Center point to search from
	// range: Search radius in meters (use -1 for unlimited range / count all)
	// faction: Filter by CRF_HVTFaction, or NONE to match any faction
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT), or -1 to match any type
	// Returns: Number of matching alive HVTs
	//
	// Example usage:
	//   CRF_HighValueTargetGamemodeManager hvtManager = CRF_HighValueTargetGamemodeManager.Cast(CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager));
	//   vector searchPosition = ownerEntity.GetOrigin();
	//   int hvtCount = hvtManager.CountHVTsInRange(searchPosition, 50);                                              // Any HVT within 50m
	//   int hvtCount = hvtManager.CountHVTsInRange(searchPosition, -1, CRF_HVTFaction.BLUFOR);                       // All BLUFOR HVTs (any range)
	//   int hvtCount = hvtManager.CountHVTsInRange(searchPosition, 100, CRF_HVTFaction.NONE, CRF_HVTEntryType.AI);   // AI HVTs within 100m
	int CountHVTsInRange(vector position, float range, CRF_HVTFaction faction = CRF_HVTFaction.NONE, int entryType = -1)
	{
		int hvtCount = 0;
		
		foreach (IEntity hvtEntity, int entryIndex : m_mHVTEntryIndex)
		{
			if (!hvtEntity || !IsHVTAlive(hvtEntity))
				continue;
			
			// Validate entry index
			if (entryIndex < 0 || entryIndex >= m_aHVTEntries.Count())
				continue;
			
			CRF_HVTEntry entry = m_aHVTEntries[entryIndex];
			if (!entry)
				continue;
			
			// Filter by faction if specified (NONE = match any)
			if (faction != CRF_HVTFaction.NONE && entry.m_eFaction != faction)
				continue;
			
			// Filter by entry type if specified (-1 = match any)
			if (entryType >= 0 && entry.m_eEntryType != entryType)
				continue;
			
			// Check distance (range < 0 means unlimited)
			if (range >= 0 && vector.Distance(position, hvtEntity.GetOrigin()) > range)
				continue;
			
			hvtCount++;
		}
		
		return hvtCount;
	}
}
