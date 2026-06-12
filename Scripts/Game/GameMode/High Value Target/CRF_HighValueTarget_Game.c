/*
	HVT/VIP Gamemode Component
	
	Tracks HVTs/VIPs (PHVT/AI/Object) and slotting-group elements, syncing positions to map markers.
	Supports multiple HVTs/VIPs with per-entry faction, marker text, and color configuration.
	Also supports ELEMENT entries (whole slotting groups) — all factions except the target automatically hunt them.

	Entry types:
	- AI / PLAYER / OBJECT: single-entity targets (original HVT behaviour)
	- ELEMENT: whole slotting group identified by faction + callsign (centroid tracking, elimination objectives)

	FIXES:
	- Fix A: timeDelay parameter for CRF_Mapmarker (now 0) was m_timeBetweenPings, causing CRF_MapMarker to essentially cache the transponder marker 
	  per client based on when they were opening the map. Changed to 0 to skip the cache. The -5 timer offset that
	  pre-staged transponders before the cache expired was also removed.

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
	OBJECT,
	ELEMENT
}

// HVT Entry Configuration Class
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTransponderEntityName")]
class CRF_HVTEntry
{
	[Attribute("0", UIWidgets.ComboBox, "Entry type: AI spawns character at transponder, PLAYER detects by prefab match, OBJECT spawns item at transponder, ELEMENT tracks a slotting group by callsign.", enums: ParamEnumArray.FromEnum(CRF_HVTEntryType))]
	CRF_HVTEntryType m_eEntryType;
	
	[Attribute("0", UIWidgets.ComboBox, "Faction of this HVT/element. For PLAYER entries, filters by faction. For AI/OBJECT/ELEMENT, sets marker color and identifies the target side.", enums: ParamEnumArray.FromEnum(CRF_HVTFaction))]
	CRF_HVTFaction m_eFaction;
	
	[Attribute("1PLT", "auto", "ELEMENT only: Callsign substring matched against the slotting group name (e.g. 1PLT, PLT, 1-1, MAT). Case-insensitive. Ignored for other entry types.", category: "Element")]
	string m_sTargetCallsign;
	
	[Attribute("", "auto", "Not used for ELEMENT entries. AI: character prefab to spawn. PLAYER: prefab to match the LOBBY SLOT (use a UNIQUE prefab and faction setting for that slot so that the mode detects the PHVT). OBJECT: item prefab to spawn on the transponder (e.g. radio).", uiwidget: "resourcePickerThumbnail", params: "et", category: "Single Target (AI / PLAYER / OBJECT)")]
	ResourceName m_hvtPrefab;
	
	[Attribute("", "auto", "Name of the transponder entity in world. AI/OBJECT spawn here, PLAYER/ELEMENT marker follows here. Must be UNIQUE per entry.")]
	string m_sTransponderEntityName;
	
	[Attribute("Transponder Signal", "auto", "Text displayed on the map marker for this HVT/Object/Element.")]
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
[ComponentEditorProps(category: "Game Mode Component", description: "High Value Target gamemode - track and hunt HVTs and elements")]
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
	
	[Attribute("false", "auto", "Disable damage on AI/Player HVTs (makes them invulnerable). Does not apply to OBJECT or ELEMENT entries.", category: "Global Settings")]
	bool m_bDisableDamage;
	
	[Attribute("360", UIWidgets.SpinBox, "The amount of time between marker updates in seconds. Minimum 10s.", "10 3600 1", category: "Global Settings")]
	int m_timeBetweenPings;
	
	[Attribute("false", "auto", "Send an immediate transponder ping when the game starts (before the first regular ping cycle).", category: "Global Settings")]
	bool m_bInitialPing;
	
	[Attribute("false", "auto", "Hide the transponder marker from all factions except the searcher faction. When disabled, each entry is visible to every faction except the entry's target faction.", category: "Global Settings")]
	bool m_filterFaction;

	[Attribute("BLUFOR", "auto", "Faction key for the searching side (only used if Filter Faction is enabled).", category: "Global Settings")]
	string m_searcherFactionKey;

	[Attribute("false", "auto", "Send the HVT's faction a hint notification when the transponder pings. If Faction Filter is enabled, sends to the searcher faction instead.", category: "Global Settings")]
	bool m_updateDefender;
	
	[Attribute("false", "auto", "Set the mission winner when an ELEMENT entry is fully eliminated. All factions except the target are hunters; multi-hunter objectives credit the faction that scored the final kill.", category: "Global Settings")]
	bool m_bAutoSetWinnerOnElimination;
	
	[Attribute("false", "auto", "Advance to AAR when an ELEMENT elimination triggers Auto Set Winner.", category: "Global Settings")]
	bool m_bAutoEndMissionOnElimination;
	
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
	
	[Attribute("", "auto", "List of HVT entries. Each entry can be configured as player, AI, object, or element.", category: "HVT Entries")]
	ref array<ref CRF_HVTEntry> m_aHVTEntries;
	
	[Attribute("HVT GAMEMODE SETUP\n\nIMPORTANT:\n• Transponder name is case sensitive\n\n=== BASIC SETUP ===\n1. Add this component to your Game Mode Entity\n2. Add 1 or more 'HVT ENTRIES' within the component\n3. Set Entry Type per entry (AI / PLAYER / OBJECT / ELEMENT)\n\n=== AI/OBJECT ENTRY ===\n• Entry Type = AI/OBJECT\n• Set Prefab (Single Target category)\n• Place empty transponder entity in world\n• AI spawns at transponder location\n\n=== PLAYER ENTRY ===\n• Entry Type = PLAYER\n• Set Prefab (Single Target category) to PLAYER SLOT prefab\n• Place empty transponder entity in world\n• Set Faction to filter by faction (optional)\nSearches/matches by PREFAB - use UNIQUE prefab!\n\n=== ELEMENT ENTRY ===\n• Entry Type = ELEMENT\n• Set Faction to the target element's faction\n• Set Target Callsign (e.g. 1PLT, 1-1, MAT)\n• Place empty transponder entity in world\n• Prefab is NOT used — leave it empty\n• All other factions are automatically assigned to hunt this element\n\n=== EXAMPLES ===\nMirror TVT element hunt:\n  Entry 0: ELEMENT, OPFOR, 1PLT\n  Entry 1: ELEMENT, BLUFOR, 1PLT\n\nThree-way element hunt:\n  Entry 0: ELEMENT, INDFOR, 1PLT\n  (BLUFOR and OPFOR both hunt INDFOR automatically)", UIWidgets.EditBoxMultiline, "HVT Setup Instructions", category: "Documentation")]
	string m_sInstructions;
	
	// Replicated HVT positions (server → client)
	[RplProp(onRplName: "SyncTransponderPositions")]
	ref array<vector> m_aHvtPositions = {};
	
	// Replicated dead HVT hint (server → client) - triggers hint display
	[RplProp(onRplName: "OnDeadHVTHintReplicated")]
	string m_sDeadHVTHint;
	
	// HVT tracking maps
	ref map<IEntity, int> m_mHVTEntryIndex = new map<IEntity, int>();        // entity → entry index
	ref map<int, IEntity> m_mEntryToHVT = new map<int, IEntity>();           // entry index → entity
	ref map<int, IEntity> m_mEntryToTransponder = new map<int, IEntity>();   // entry index → transponder entity, cached at SetHVTAndState to avoid hot loop FindEntityByName's
	ref map<IEntity, SCR_CharacterDamageManagerComponent> m_mHVTDamageManagers = new map<IEntity, SCR_CharacterDamageManagerComponent>(); // entity → damage manager (OBJECT entries absent = alive by entity existence)
	
	// ELEMENT entry tracking
	ref map<int, RplId> m_mEntryToTargetGroup = new map<int, RplId>();
	ref map<IEntity, RplId> m_mElementMemberToGroup = new map<IEntity, RplId>();
	ref map<RplId, string> m_mLastEliminatorFactionByGroup = new map<RplId, string>();
	ref set<int> m_sEliminatedEntryIndices = new set<int>();
	ref set<RplId> m_sEliminatedGroupIds = new set<RplId>();
	
	// Client-side: Track which markers have been removed for JIP/mishaps/desyncs
	ref set<int> m_sRemovedMarkerIndices = new set<int>();
	
	const string MARKER_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
	const int MARKER_SIZE = 50;
	
	bool m_bHVTStateSet = false;
	bool m_bGameInit = false;
	bool m_bHasSafestartBegun = false;  // Latch: Ensures we've seen safestart active before watching for it to end
	
	// ReplicationTimer prevents constant position updates to clients= (m_timeBetweenPings ; Performance good ?)
	float m_fReplicationTimer = 0; // Timer for marker position replication now fires UpdateHVTPositions every m_timeBetweenPings seconds
	float m_fPlayerCheckTimer = 0; // Checks + ReRegisters + Resets Invulnerability if set if player HVTs every 60s to catch JIP/respawns
	float m_fUpdateBuffer = 0;

	//------------------------------------------------------------------------------------------------
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		SetEventMask(owner, EntityEvent.FIXEDFRAME); // Testing event mask for initial clientside 
	}
	
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
			// If safestart is never enabled, skip directly to GameInit
			if (!m_bHasSafestartBegun)
			{
				if (CRF_SafestartManager.GetInstance().GetSafestartStatus())
				{
					m_bHasSafestartBegun = true;
					return;
				}
			}
			
			// Wait for safestart to end before GameInit (skipped if safestart was never active)
			if (!m_bGameInit)
			{
				if (m_bHasSafestartBegun && CRF_SafestartManager.GetInstance().GetSafestartStatus())
					return;
				
				GameInit();
				
				// Clients done - disable loop
				if (!Replication.IsServer())
					ClearEventMask(owner, EntityEvent.FIXEDFRAME);
				
				return;
			}
			
			if (Replication.IsServer())
			{
				// Server: Periodic player HVT re-registration and ELEMENT member re-registration
				if (++m_fPlayerCheckTimer >= 60)
				{
					m_fPlayerCheckTimer = 0;
					RegisterPlayerHVTs();
					RegisterElementMembers();
				}
				
				CheckElementEliminations();
			}
			
			// Server: Sync positions at the configured interval, replication fires only after m_timeBetweenPings for performance reasons
			if (m_bEnableTransponderMarker && ++m_fReplicationTimer >= m_timeBetweenPings)
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
		if (Replication.IsServer())
		{
			foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
			{
				if (!entry || entry.m_eEntryType == CRF_HVTEntryType.PLAYER || entry.m_eEntryType == CRF_HVTEntryType.ELEMENT)
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
		// Cache transponder entity references here - avoids repeated FindEntityByName in SyncTransponderPositions
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			IEntity transponder = GetGame().GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
			if (!transponder)
				continue;
			
			transponder.SetOrigin("0 -1000 0");
			m_mEntryToTransponder.Set(index, transponder);
		}
		
		if (!m_aHvtPositions.IsEmpty())
			SyncTransponderPositions();
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
			{
				m_mHVTEntryIndex.Remove(existingHVT);
				m_mHVTDamageManagers.Remove(existingHVT);
			}
			
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
		{
			damageManager.EnableDamageHandling(!m_bDisableDamage);
			m_mHVTDamageManagers.Set(hvtEntity, damageManager); // Cache for IsHVTAlive
		}
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
		if (m_sDeadHVTHint.IndexOf("element eliminated") != -1)
			hintTitle = "ELEMENT ELIMINATED";
		else if (m_eTargetType == CRF_TargetType.VIP)
			hintTitle = "VIP KILLED";
		
		hintManager.ShowCustomHint(m_sDeadHVTHint, hintTitle, 10);
	}
	
	void GameInit()
	{
		m_bGameInit = true;
		
		// Server: Register player HVTs now that safestart has ended and players are spawned
		if (Replication.IsServer())
		{
			RegisterPlayerHVTs();
			ResolveElementGroups();
			RegisterElementMembers();
			
			if (m_bEnableTransponderMarker && m_bInitialPing)
				UpdateHVTPositions();
		}
		
		// Client/Listen: Create markers
		if (!m_bEnableTransponderMarker)
			return;
		
		CRF_PlayerScriptedMarkerManager playerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!playerScriptedMarkerManager) 
			return;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction localFaction;
		if (factionManager)
			localFaction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			if (!CanLocalPlayerSeeEntryMarker(entry, localFaction))
				continue;
			
			if (!m_mEntryToTransponder.Contains(index))
			{
				Print(string.Format("[HVT] Warning: Transponder entity '%1' not found in world!", entry.m_sTransponderEntityName), LogLevel.WARNING);
				continue;
			}
			
			// Was: playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, ...) // Fix A
			playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", 0, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
		}
	}
	
	// Returns true if the local player should see this entry's transponder marker
	protected bool CanLocalPlayerSeeEntryMarker(CRF_HVTEntry entry, Faction localFaction)
	{
		if (m_filterFaction)
		{
			if (!localFaction)
				return false;
			return localFaction.GetFactionKey() == m_searcherFactionKey;
		}
		
		// Default: every faction except the entry's target faction can see the marker
		string targetFactionKey = entry.GetFactionKey();
		if (targetFactionKey.IsEmpty())
			return true;
		
		if (!localFaction)
			return false;
		
		return localFaction.GetFactionKey() != targetFactionKey;
	}
	
	protected ref array<string> GetMissionFactionKeys()
	{
		ref array<string> factionKeys = new array<string>();
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return factionKeys;
		
		foreach (int slotId, CRF_SlotData slotData : slottingManager.GetSlotMap())
		{
			if (!slotData)
				continue;
			
			string factionKey = slotData.GetSlotFactionKey();
			if (factionKey.IsEmpty())
				continue;
			
			if (factionKeys.Find(factionKey) == -1)
				factionKeys.Insert(factionKey);
		}
		
		return factionKeys;
	}
	
	protected ref array<string> CollectHunterFactionsForTarget(string targetFactionKey)
	{
		ref array<string> hunterFactions = new array<string>();
		
		foreach (string factionKey : GetMissionFactionKeys())
		{
			if (factionKey == targetFactionKey)
				continue;
			
			hunterFactions.Insert(factionKey);
		}
		
		return hunterFactions;
	}
	
	void ResolveElementGroups()
	{
		if (!m_aHVTEntries)
			return;
		
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_eEntryType != CRF_HVTEntryType.ELEMENT)
				continue;
			
			if (m_mEntryToTargetGroup.Contains(index))
				continue;
			
			if (entry.GetFactionKey().IsEmpty() || entry.m_sTargetCallsign.IsEmpty())
			{
				Print(string.Format("[HVT] Warning: ELEMENT entry %1 missing faction or callsign!", index), LogLevel.WARNING);
				continue;
			}
			
			SCR_AIGroup group = FindGroupByFactionAndCallsign(entry.GetFactionKey(), entry.m_sTargetCallsign);
			if (!group)
			{
				Print(string.Format("[HVT] Warning: ELEMENT entry %1 could not resolve group '%2' (%3).", index, entry.m_sTargetCallsign, entry.GetFactionKey()), LogLevel.WARNING);
				continue;
			}
			
			m_mEntryToTargetGroup.Set(index, Replication.FindItemId(group));
		}
	}
	
	SCR_AIGroup FindGroupByFactionAndCallsign(string factionKey, string callsignFilter)
	{
		if (factionKey.IsEmpty() || callsignFilter.IsEmpty())
			return null;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return null;
		
		string filterLower = callsignFilter;
		filterLower.ToLower();
		
		array<SCR_AIGroup> groups = slottingManager.GetAllGroups(factionKey);
		foreach (SCR_AIGroup group : groups)
		{
			if (!group)
				continue;
			
			string groupName = group.GetCustomNameWithOriginal();
			groupName.ToLower();
			if (groupName.IndexOf(filterLower) != -1)
				return group;
		}
		
		return null;
	}
	
	void RegisterElementMembers()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		ref set<RplId> groupsRegistered = new set<RplId>();
		
		foreach (int index, RplId groupId : m_mEntryToTargetGroup)
		{
			if (groupId == RplId.Invalid() || m_sEliminatedGroupIds.Contains(groupId))
				continue;
			
			if (groupsRegistered.Contains(groupId))
				continue;
			
			groupsRegistered.Insert(groupId);
			
			array<int> slotIds = slottingManager.GetAllSlotIDsForGroup(groupId);
			foreach (int slotId : slotIds)
			{
				CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
				if (!slotData || slotData.GetIsDeadSlot())
					continue;
				
				IEntity memberEntity = GetLivingEntityFromSlot(slotData, playerManager);
				if (memberEntity)
					RegisterElementMember(memberEntity, groupId);
			}
		}
	}
	
	void RegisterElementMember(IEntity memberEntity, RplId groupId)
	{
		if (!memberEntity || m_mElementMemberToGroup.Contains(memberEntity))
			return;
		
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(memberEntity.FindComponent(SCR_CharacterControllerComponent));
		if (!characterController)
			return;
		
		m_mElementMemberToGroup.Set(memberEntity, groupId);
		characterController.m_OnPlayerDeathWithParam.Insert(OnElementMemberDeath);
	}
	
	void OnElementMemberDeath(SCR_CharacterControllerComponent characterController, IEntity killerEntity, Instigator killer)
	{
		if (!Replication.IsServer())
			return;
		
		IEntity memberEntity = characterController.GetOwner();
		if (!memberEntity || !m_mElementMemberToGroup.Contains(memberEntity))
			return;
		
		RplId groupId = m_mElementMemberToGroup.Get(memberEntity);
		m_mElementMemberToGroup.Remove(memberEntity);
		
		string killerFactionKey = ResolveKillerFactionKey(killer, killerEntity);
		if (!killerFactionKey.IsEmpty())
			m_mLastEliminatorFactionByGroup.Set(groupId, killerFactionKey);
		
		CheckElementEliminations();
		
		if (m_bEnableTransponderMarker)
			UpdateHVTPositions();
	}
	
	protected string ResolveKillerFactionKey(Instigator killer, IEntity killerEntity)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return "";
		
		int killerPlayerId = killer.GetInstigatorPlayerID();
		if (killerPlayerId > 0)
		{
			Faction killerFaction = factionManager.GetPlayerFaction(killerPlayerId);
			if (killerFaction)
				return killerFaction.GetFactionKey();
		}
		
		if (killerEntity)
			return CRF_EntityHelper.DetermineFactionKey(killerEntity);
		
		return "";
	}
	
	void CheckElementEliminations()
	{
		if (!m_aHVTEntries)
			return;
		
		ResolveElementGroups();
		
		ref set<RplId> groupsChecked = new set<RplId>();
		
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_eEntryType != CRF_HVTEntryType.ELEMENT)
				continue;
			
			if (m_sEliminatedEntryIndices.Contains(index))
				continue;
			
			if (!m_mEntryToTargetGroup.Contains(index))
				continue;
			
			RplId groupId = m_mEntryToTargetGroup.Get(index);
			if (groupId == RplId.Invalid() || m_sEliminatedGroupIds.Contains(groupId))
				continue;
			
			if (groupsChecked.Contains(groupId))
				continue;
			
			groupsChecked.Insert(groupId);
			
			if (IsTargetGroupAlive(groupId))
				continue;
			
			OnTargetGroupEliminated(groupId);
		}
	}
	
	void OnTargetGroupEliminated(RplId groupId)
	{
		m_sEliminatedGroupIds.Insert(groupId);
		
		string targetLabel = BuildGroupLabel(groupId);
		ref array<string> hunterFactions = CollectHunterFactionsForGroup(groupId);
		string eliminatorFaction = "";
		if (m_mLastEliminatorFactionByGroup.Contains(groupId))
			eliminatorFaction = m_mLastEliminatorFactionByGroup.Get(groupId);
		
		string hunterLabel = BuildHunterLabel(hunterFactions, eliminatorFaction);
		m_sDeadHVTHint = targetLabel + " eliminated" + hunterLabel;
		Replication.BumpMe();
		OnDeadHVTHintReplicated();
		
		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_eEntryType != CRF_HVTEntryType.ELEMENT)
				continue;
			
			if (!m_mEntryToTargetGroup.Contains(index))
				continue;
			
			if (m_mEntryToTargetGroup.Get(index) != groupId)
				continue;
			
			m_sEliminatedEntryIndices.Insert(index);
		}
		
		if (m_bEnableTransponderMarker)
			UpdateHVTPositions();
		
		if (m_bAutoSetWinnerOnElimination)
		{
			string winningFaction = ResolveEliminationWinner(groupId, hunterFactions);
			if (!winningFaction.IsEmpty())
			{
				CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
				if (loggingManager)
					loggingManager.SetWinningFaction(winningFaction, "hvt_element_elimination");
				
				if (m_bAutoEndMissionOnElimination)
					CRF_Gamemode.GetInstance().AdvanceGamemodeState(true);
			}
		}
	}
	
	protected string BuildGroupLabel(RplId groupId)
	{
		SCR_AIGroup group = CRF_EntityHelper.GetGroupFromRplId(groupId);
		if (!group)
			return "Target element";
		
		Faction faction = group.GetFaction();
		string factionKey = "";
		if (faction)
			factionKey = faction.GetFactionKey() + " ";
		
		return factionKey + group.GetCustomNameWithOriginal() + " element";
	}
	
	protected ref array<string> CollectHunterFactionsForGroup(RplId groupId)
	{
		SCR_AIGroup group = CRF_EntityHelper.GetGroupFromRplId(groupId);
		string targetFactionKey = "";
		if (group)
		{
			Faction faction = group.GetFaction();
			if (faction)
				targetFactionKey = faction.GetFactionKey();
		}
		
		return CollectHunterFactionsForTarget(targetFactionKey);
	}
	
	protected string BuildHunterLabel(array<string> hunterFactions, string eliminatorFaction = "")
	{
		if (!hunterFactions || hunterFactions.Count() == 0)
			return "";
		
		if (hunterFactions.Count() == 1)
			return " - " + hunterFactions[0] + " objective complete";
		
		if (!eliminatorFaction.IsEmpty())
			return " - Eliminated by " + eliminatorFaction;
		
		string label = " - Objectives complete for ";
		for (int i = 0; i < hunterFactions.Count(); i++)
		{
			if (i > 0)
				label += ", ";
			label += hunterFactions[i];
		}
		
		return label;
	}
	
	protected string ResolveEliminationWinner(RplId groupId, array<string> hunterFactions)
	{
		if (!hunterFactions || hunterFactions.Count() == 0)
			return "";
		
		if (hunterFactions.Count() == 1)
			return hunterFactions[0];
		
		if (!m_mLastEliminatorFactionByGroup.Contains(groupId))
			return "";
		
		string eliminatorFaction = m_mLastEliminatorFactionByGroup.Get(groupId);
		if (eliminatorFaction.IsEmpty())
			return "";
		
		if (hunterFactions.Find(eliminatorFaction) != -1)
			return eliminatorFaction;
		
		return "";
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
			CRF_HVTEntry entry = m_aHVTEntries[i];
			
			if (!entry)
			{
				m_aHvtPositions.Insert(pos);
				continue;
			}
			
			if (entry.m_eEntryType == CRF_HVTEntryType.ELEMENT)
			{
				if (!m_sEliminatedEntryIndices.Contains(i) && m_mEntryToTargetGroup.Contains(i))
				{
					RplId groupId = m_mEntryToTargetGroup.Get(i);
					if (groupId != RplId.Invalid() && !m_sEliminatedGroupIds.Contains(groupId))
						pos = GetElementCentroid(groupId);
				}
			}
			// O(1) lookup using reverse map
			else if (m_mEntryToHVT.Contains(i))
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
	
	vector GetElementCentroid(RplId groupId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return vector.Zero;
		
		array<int> slotIds = slottingManager.GetAllSlotIDsForGroup(groupId);
		vector sum = vector.Zero;
		int count = 0;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		
		foreach (int slotId : slotIds)
		{
			CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (!slotData || slotData.GetIsDeadSlot())
				continue;
			
			IEntity livingEntity = GetLivingEntityFromSlot(slotData, playerManager);
			if (!livingEntity)
				continue;
			
			sum += livingEntity.GetOrigin();
			count++;
		}
		
		if (count == 0)
			return vector.Zero;
		
		return Vector(sum[0] / count, sum[1] / count, sum[2] / count);
	}
	
	protected IEntity GetLivingEntityFromSlot(CRF_SlotData slotData, PlayerManager playerManager)
	{
		if (!slotData)
			return null;
		
		int playerId = slotData.GetSlotCurrentPlayerId();
		if (playerId > 0 && playerManager)
		{
			IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
			if (controlled && IsCharacterAlive(controlled))
				return controlled;
		}
		
		SCR_ChimeraCharacter character = CRF_EntityHelper.GetCharacterFromRplId(slotData.GetSlotCurrentCharacter());
		if (character && IsCharacterAlive(character))
			return character;
		
		return null;
	}
	
	protected bool IsCharacterAlive(IEntity entity)
	{
		if (!entity)
			return false;
		
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damageManager)
			return true;
		
		return damageManager.GetState() != EDamageState.DESTROYED;
	}
	
	bool IsTargetGroupAlive(RplId groupId)
	{
		if (groupId == RplId.Invalid() || m_sEliminatedGroupIds.Contains(groupId))
			return false;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;
		
		array<int> slotIds = slottingManager.GetAllSlotIDsForGroup(groupId);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		bool anyOccupiedSlot = false;
		
		foreach (int slotId : slotIds)
		{
			CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (!slotData)
				continue;
			
			bool occupied = slotData.GetSlotCurrentPlayerId() > 0 || slotData.GetSlotCurrentCharacter() != RplId.Invalid();
			if (!occupied)
				continue;
			
			anyOccupiedSlot = true;
			
			if (slotData.GetIsDeadSlot())
				continue;
			
			if (GetLivingEntityFromSlot(slotData, playerManager))
				return true;
		}
		
		if (!anyOccupiedSlot)
			return true;
		
		return false;
	}
	
	// Check if HVT is alive/valid via damage state (or entity existence for objects)
	bool IsHVTAlive(IEntity hvtEntity)
	{
		if (!hvtEntity)
			return false;
		
		// OBJECT entries have no damage manager and are absent from m_mHVTDamageManagers - alive by entity existence
		if (!m_mHVTDamageManagers.Contains(hvtEntity))
			return true;
		
		SCR_CharacterDamageManagerComponent damageManager = m_mHVTDamageManagers.Get(hvtEntity);
		if (!damageManager)
			return false;
		
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
		
		CRF_PlayerScriptedMarkerManager playerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction localFaction;
		if (factionManager)
			localFaction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
		// Use min of both arrays to prevent index out of bounds
		int iterCount = Math.Min(m_aHVTEntries.Count(), m_aHvtPositions.Count());
		for (int i = 0; i < iterCount; i++)
		{
			CRF_HVTEntry entry = m_aHVTEntries[i];
			if (!entry || entry.m_sTransponderEntityName.IsEmpty())
				continue;
			
			if (!CanLocalPlayerSeeEntryMarker(entry, localFaction))
				continue;
			
			vector newPos = m_aHvtPositions[i];
			
			// Zero position means HVT is dead - remove marker
			if (newPos == vector.Zero)
			{
				if (playerScriptedMarkerManager && m_bEnableTransponderMarker)
				{
					// Was: playerScriptedMarkerManager.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, ...) // Fix A
					playerScriptedMarkerManager.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", 0, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
					m_sRemovedMarkerIndices.Insert(i);
				}
				continue;
			}
			
			// Non-zero position - check if marker was previously removed and needs re-creation
			if (m_sRemovedMarkerIndices.Contains(i) && playerScriptedMarkerManager && m_bEnableTransponderMarker)
			{
				// Was: playerScriptedMarkerManager.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, ...) // Fix A
				playerScriptedMarkerManager.RemoveScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", 0, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
				// Was: playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, ...) // Fix A
				playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", 0, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
				m_sRemovedMarkerIndices.Remove(i);
			}
			
			if (!m_mEntryToTransponder.Contains(i))
				continue;
			
			IEntity transponder = m_mEntryToTransponder.Get(i);
			if (!transponder)
				continue;
			
			transponder.SetOrigin(newPos);
		}
		
		// Server: Notify the relevant faction that the transponder has pinged
		if (m_updateDefender && Replication.IsServer())
		{
			string targetLabel = "HVT";
			if (m_eTargetType == CRF_TargetType.VIP)
				targetLabel = "VIP";
			
			ref set<RplId> pingedGroups = new set<RplId>();
			
			foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
			{
				if (!entry)
					continue;
				
				if (entry.m_eEntryType == CRF_HVTEntryType.ELEMENT)
				{
					if (m_sEliminatedEntryIndices.Contains(index))
						continue;
					
					if (!m_mEntryToTargetGroup.Contains(index))
						continue;
					
					RplId groupId = m_mEntryToTargetGroup.Get(index);
					if (groupId == RplId.Invalid() || m_sEliminatedGroupIds.Contains(groupId))
						continue;
					
					if (pingedGroups.Contains(groupId))
						continue;
					
					if (!IsTargetGroupAlive(groupId))
						continue;
					
					pingedGroups.Insert(groupId);
					
					SCR_AIGroup group = CRF_EntityHelper.GetGroupFromRplId(groupId);
					string groupName = entry.m_sTargetCallsign;
					if (group)
						groupName = group.GetCustomNameWithOriginal();
					
					// If faction-filtered only notify the searching faction, otherwise notify the target faction
					string pingFactionKey = entry.GetFactionKey();
					if (m_filterFaction)
						pingFactionKey = m_searcherFactionKey;
					
					CRF_RplBroadcastManager.GetInstance().SendHint(entry.GetFactionKey() + " " + groupName + " transponder ping", -1, pingFactionKey);
					continue;
				}
				
				if (!m_mEntryToHVT.Contains(index))
					continue;
				
				IEntity hvtEntity = m_mEntryToHVT.Get(index);
				if (!hvtEntity || !IsHVTAlive(hvtEntity))
					continue;
				
				// If faction-filtered only notify the searching faction otherwise broadcast to all
				string pingFactionKey = "";
				if (m_filterFaction)
					pingFactionKey = m_searcherFactionKey;
				
				CRF_RplBroadcastManager.GetInstance().SendHint(targetLabel + " transponder has sent a signal", -1, pingFactionKey);
			}
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
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT/ELEMENT), or -1 to match any type
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
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT/ELEMENT), or -1 to match any type
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
	
	// ELEMENT query methods — use entry index from HVT Entries array, or callsign lookup
	bool IsElementAlive(int entryIndex)
	{
		if (m_sEliminatedEntryIndices.Contains(entryIndex))
			return false;
		
		if (!m_mEntryToTargetGroup.Contains(entryIndex))
			return false;
		
		return IsTargetGroupAlive(m_mEntryToTargetGroup.Get(entryIndex));
	}
	
	bool IsElementAliveByCallsign(string factionKey, string callsignFilter)
	{
		SCR_AIGroup group = FindGroupByFactionAndCallsign(factionKey, callsignFilter);
		if (!group)
			return false;
		
		RplId groupId = Replication.FindItemId(group);
		if (m_sEliminatedGroupIds.Contains(groupId))
			return false;
		
		return IsTargetGroupAlive(groupId);
	}
	
	vector GetElementPosition(int entryIndex)
	{
		if (!m_mEntryToTargetGroup.Contains(entryIndex))
			return vector.Zero;
		
		return GetElementCentroid(m_mEntryToTargetGroup.Get(entryIndex));
	}
	
	bool IsElementInRange(int entryIndex, vector position, float range)
	{
		vector elementPos = GetElementPosition(entryIndex);
		if (elementPos == vector.Zero)
			return false;
		
		return vector.Distance(position, elementPos) <= range;
	}
}
