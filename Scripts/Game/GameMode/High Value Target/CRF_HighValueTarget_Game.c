/*
	HVT/VIP Gamemode Component
	
	Tracks HVTs/VIPs (PHVT/AI/Object/Vehicle) and syncs their positions to map markers.
	Supports multiple HVTs/VIPs with per-entry faction, marker text, and color configuration.

	FIXES:
	- Fix A: timeDelay parameter for CRF_Mapmarker (now 0) was m_timeBetweenPings, causing CRF_MapMarker to essentially cache the transponder marker 
	  per client based on when they were opening the map. Changed to 0 to skip the cache. The -5 timer offset that
	  pre-staged transponders before the cache expired was also removed.

	NOTES:
	- Currently to track HVT JIPs/respawns & reapply vulnerability we use m_fPlayerCheckTimer (HVTs are invulnerable between re-registration. Edge case issue is that someone is respawned and within 60s is attempted to be killed. Shouldnt happen realistically.) Not ideal setup but cant find much else.
	- HVT Prefab selection is what spawns when AI is used, or what Prefab/Slot (optional filter by faction) is used for player-controlled HVTs.
	
	REQUIREMENTS
	- NEEDS ACE CAPTIVES unless we integrate our own captive/prisoner system. Will cause NULL pointers if not installed.

	TODO:
	- Less crappy re-registration of HVTs/VIPs?
	- Edge case where if more than 1 PHVT slots, but unfilled, marker/transponder may be active first minute of game. Race condition not really worth fixing. Unless?
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
	VEHICLE  // Spawns like OBJECT, tracks destruction like AI/PLAYER
}

enum CRF_AIHVTState
{
	UNCONSCIOUS, // Incapacitated with no regen
	SURRENDERED, // ACE Captives: hands-up surrender animation, requires zipcuffs (ACE Captives required)
	CUFFED       // ACE Captives: surrendered + zip-cuffed via captive system (ACE Captives required)
}

// HVT Entry Configuration Class
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTransponderEntityName")]
class CRF_HVTEntry
{
	[Attribute("0", UIWidgets.ComboBox, "Entry type: AI spawns character at transponder, PLAYER detects by prefab match, OBJECT spawns item at transponder, VEHICLE spawns vehicle at transponder (destruction tracked like AI/PLAYER).", enums: ParamEnumArray.FromEnum(CRF_HVTEntryType))]
	CRF_HVTEntryType m_eEntryType;

	[Attribute("{42A502E3BB727CEB}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliPilot.et", desc: "AI: Character prefab to spawn. PLAYER: Prefab to match the LOBBY SLOT (use a UNIQUE prefab and faction setting for that slot so that the mode detects the PHVT). OBJECT: Item prefab to spawn on the transponder object(e.g. radio). VEHICLE: Vehicle prefab to spawn on the transponder point.", uiwidget: "resourcePickerThumbnail", params: "et")]
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

	string GetLossTitleVerb()
	{
		if (m_eEntryType == CRF_HVTEntryType.VEHICLE)
			return "DESTROYED";
		return "KILLED";
	}

	string GetLossKilledByPhrase()
	{
		if (m_eEntryType == CRF_HVTEntryType.VEHICLE)
			return "Destroyed by";
		return "Killed by";
	}

	string GetLossUnknownCausePhrase()
	{
		if (m_eEntryType == CRF_HVTEntryType.VEHICLE)
			return "been destroyed";
		return "died";
	}
}

// Binds an entity to its destruction callback (GetOnDamageStateChanged doesn't pass the entity itself)
class CRF_HVTDestructionWrapper : Managed
{
	protected CRF_HighValueTargetGamemodeManager m_pManager;
	protected IEntity m_pEntity;

	void CRF_HVTDestructionWrapper(CRF_HighValueTargetGamemodeManager manager, IEntity entity)
	{
		m_pManager = manager;
		m_pEntity = entity;
	}

	void OnDamageStateChanged(EDamageState newState)
	{
		if (newState == EDamageState.DESTROYED && m_pManager)
			m_pManager.OnHVTDestroy(m_pEntity);
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
	
	[Attribute("false", "auto", "Disable damage on AI/Player/Vehicle HVTs (makes them invulnerable). Does not apply to OBJECT entries.", category: "Global Settings")]
	bool m_bDisableDamage;
	
	[Attribute("360", UIWidgets.SpinBox, "The amount of time between marker updates in seconds. Minimum 10s.", "10 3600 1", category: "Global Settings")]
	int m_timeBetweenPings;
	
	[Attribute("false", "auto", "Send an immediate transponder ping when the game starts (before the first regular ping cycle).", category: "Global Settings")]
	bool m_bInitialPing;
	
	[Attribute("false", "auto", "Hide the transponder marker from all factions except the searcher faction.", category: "Global Settings")]
	bool m_filterFaction;

	[Attribute("BLUFOR", "auto", "Faction key for the searching side (only used if Filter Faction is enabled).", category: "Global Settings")]
	string m_searcherFactionKey;

	[Attribute("false", "auto", "Send the HVT's faction a hint notification when the transponder pings. If Faction Filter is enabled, sends to the searcher faction instead.", category: "Global Settings")]
	bool m_updateDefender;
	
	//------------------------------------------------------------------------------------------------
	// AI HVT SETTINGS (Applied to all AI-spawned HVTs)
	//------------------------------------------------------------------------------------------------
	
	[Attribute("0 0 0", "auto", "The rotation (yaw/pitch/roll) applied to spawned AI HVTs.", category: "AI HVT Settings")]
	vector m_hvtPrefabYaw;
	
	[Attribute("0", UIWidgets.ComboBox, "State to apply to AI HVTs on spawn. UNCONSCIOUS: Incapacitated animation, must be drug or carried. SURRENDERED: ACE Captives hands-up animation, NEED ACE zipcuffs. CUFFED: Surrendered and zip-cuffed. The ACE cuff release action is permanently disabled for Surrendered/Cuffed states.", enums: ParamEnumArray.FromEnum(CRF_AIHVTState), category: "AI HVT Settings")]
	CRF_AIHVTState m_eAIHVTState;
	
	//------------------------------------------------------------------------------------------------
	// HVT ENTRIES
	//------------------------------------------------------------------------------------------------
	
	[Attribute("", "auto", "List of HVT entries. Each entry can be configured as player or AI controlled.", category: "HVT Entries")]
	ref array<ref CRF_HVTEntry> m_aHVTEntries;
	
	[Attribute("HVT GAMEMODE SETUP\n\nIMPORTANT:\n• Transponder name is case sensitive\n\n=== BASIC SETUP ===\n1. Add this component to your Game Mode Entity\n2. Add 1 or more 'HVT ENTRIES' within the component\n3. Set Entry Type per entry (AI / PLAYER / OBJECT)\n4. Set Prefab per entry\n\n=== AI/OBJECT ENTRY ===\n• Entry Type = AI/OBJECT\n• Place empty transponder entity in world\n• Set 'Prefab' to AI character or object prefab\n• AI spawns at transponder location\n\n=== PLAYER ENTRY ===\n• Entry Type = PLAYER\n• Place empty transponder entity in world\n• Set 'Prefab' to PLAYER SLOT prefab\n• Set 'Faction' to filter by faction (optional)\nSearches/matches by PREFAB - use UNIQUE prefab!", UIWidgets.EditBoxMultiline, "HVT Setup Instructions", category: "Documentation")]
	string m_sInstructions;
	
	// Replicated HVT positions (server → client)
	[RplProp(onRplName: "SyncTransponderPositions")]
	ref array<vector> m_aHvtPositions = {};
	
	// Replicated dead HVT hint (server → client) - triggers hint display
	[RplProp(onRplName: "OnDeadHVTHintReplicated")]
	string m_sDeadHVTHint;

	// Set alongside m_sDeadHVTHint - lets the client look up the same CRF_HVTEntry for hint info
	[RplProp()]
	int m_iDeadHVTEntryIndex = -1;

	// HVT tracking maps
	ref map<IEntity, int> m_mHVTEntryIndex = new map<IEntity, int>();        // entity → entry index
	ref map<int, IEntity> m_mEntryToHVT = new map<int, IEntity>();           // entry index → entity
	ref map<int, IEntity> m_mEntryToTransponder = new map<int, IEntity>();   // entry index → transponder entity, cached at SetHVTAndState to avoid hot loop FindEntityByName's
	ref map<IEntity, DamageManagerComponent> m_mHVTDamageManagers = new map<IEntity, DamageManagerComponent>(); // entity → damage manager (OBJECT entries usually absent = alive by entity existence)
	ref map<IEntity, ref CRF_HVTDestructionWrapper> m_mHVTDestructionWrappers = new map<IEntity, ref CRF_HVTDestructionWrapper>(); // entity → destruction wrapper (VEHICLE)

	// Client-side: Track which markers have been removed for JIPS/mishaps/desyncs
	ref set<int> m_sRemovedMarkerIndices = new set<int>();
	
	const string MARKER_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
	const int MARKER_SIZE = 50;
	
	bool m_bHVTStateSet = false;
	bool m_bHVTsSpawned = false;
	bool m_bGameInit = false;
	bool m_bHasSafestartBegun = false;  // Latch: Ensures we've seen safestart active before watching for it to end
	
	// ReplicationTimer prevents constant position updates to clients= (m_timeBetweenPings ; Performance good ?)
	float m_fReplicationTimer = 0; // Timer for marker position replication now fires UpdateHVTPositions every m_timeBetweenPings seconds
	float m_fPlayerCheckTimer = 0; // Checks + ReRegisters + Resets Invulnerability if set if player HVTs every 60s to catch JIP/respawns

	//------------------------------------------------------------------------------------------------
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		SetEventMask(owner, EntityEvent.FIXEDFRAME); // Testing event mask for initial clientside 
	}

	override void OnDelete(IEntity owner)
	{
		array<IEntity> trackedEntities = {};
		foreach (IEntity hvtEntity, int entryIndex : m_mHVTEntryIndex)
		{
			if (hvtEntity)
				trackedEntities.Insert(hvtEntity);
		}

		foreach (IEntity hvtEntity : trackedEntities)
			UnregisterHVTEntity(hvtEntity);

		m_mEntryToTransponder.Clear();
		GetGame().GetCallqueue().Remove(ResyncHVTPositions);
		ClearEventMask(owner, EntityEvent.FIXEDFRAME);
#ifdef WORKBENCH
		RemoveAllPreviews();
#endif
		super.OnDelete(owner);
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fUpdateBuffer >= 1)
		{
			m_fUpdateBuffer = 0;
			
			CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
			if (!gamemode || !gamemode.IsRunning())
				return;
			
			if (!m_bHVTStateSet)
			{
				// Gates SpawnHVTEntities to prevent race condition of transponder functions before they are spawned
				if (Replication.IsServer() && !m_bHVTsSpawned)
					return;

				SetHVTAndState();
				return;
			}
			
			// Two-stage safestart check: First ensure we've seen it active, then wait for it to end
			// If safestart is never enabled, skip directly to GameInit
			if (!m_bHasSafestartBegun)
			{
				CRF_SafestartManager safestartManager = CRF_SafestartManager.GetInstance();
				if (safestartManager && safestartManager.GetSafestartStatus())
				{
					m_bHasSafestartBegun = true;
					return;
				}
			}
			
			// Wait for safestart to end before GameInit (skipped if safestart was never active)
			if (!m_bGameInit)
			{
				CRF_SafestartManager safestartManager = CRF_SafestartManager.GetInstance();
				if (m_bHasSafestartBegun && safestartManager && safestartManager.GetSafestartStatus())
					return;
				
				GameInit();
				
				// Clients done - disable loop
				if (!Replication.IsServer())
					ClearEventMask(owner, EntityEvent.FIXEDFRAME);
				
				return;
			}
			
			// Server: Periodic player HVT re-registration + AI captive state repair
			if (++m_fPlayerCheckTimer >= 60)
			{
				m_fPlayerCheckTimer = 0;
				RegisterPlayerHVTs();
				RepairAIHVTStates();
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
	
	// Spawn AI/Object HVTs at transponder locations. SpawnHVTEntities being called inline was too early(?) caused crashes
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(SpawnHVTEntities, 1500, false);
	}

	// Spawn AI/Object entries while transponders still have their editor positions
	void SpawnHVTEntities()
	{
		// Unblock SetHVTAndState (frame loop) even when there is nothing to spawn
		m_bHVTsSpawned = true;

		if (!m_aHVTEntries || m_aHVTEntries.Count() == 0)
			return;

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

			// AIHVT-specific: rotation and initial state
			if (entry.m_eEntryType == CRF_HVTEntryType.AI)
			{
				hvtEntity.SetYawPitchRoll(m_hvtPrefabYaw);

				switch (m_eAIHVTState)
				{
					case CRF_AIHVTState.SURRENDERED:
					{
						SetEntitySurrender(hvtEntity, false);
						break;
					}
					case CRF_AIHVTState.CUFFED:
					{
						SetEntitySurrender(hvtEntity, true);
						break;
					}
					case CRF_AIHVTState.UNCONSCIOUS:
					{
						SetEntityUnconscious(hvtEntity);

						SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterControllerComponent));
						if (characterController)
						{
							characterController.m_OnLifeStateChanged.Remove(OnLifeStateChangedWrapper);
							characterController.m_OnLifeStateChanged.Insert(OnLifeStateChangedWrapper);
						}
						break;
					}
				}
			}
		}
	}

	// Hide transponders until first position sync (AI/Object HVTs already spawned at OnWorldPostProcess)
	void SetHVTAndState()
	{
		m_bHVTStateSet = true;

		if (!m_aHVTEntries || m_aHVTEntries.Count() == 0)
		{
			Print("[HVT] Error: No HVT entries configured!", LogLevel.ERROR);
			return;
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

	// Repair the ACE captive state of AI HVTs if something breaks that state, happens within the 60s loop of re-registration
	// Mishaps/Zeus moving the ACE Captive AI out of its 'vehicle' the ACE system uses, breaks it. With re registering this reapplies that state. Otherwise will leave HVTs inert/broken/immovable like weve seen before.
	// Closely related to the state of ACE Captive. May break on HVT.c ACE changes.
	void RepairAIHVTStates()
	{
		if (m_eAIHVTState != CRF_AIHVTState.SURRENDERED && m_eAIHVTState != CRF_AIHVTState.CUFFED)
			return;

		foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
		{
			if (!entry || entry.m_eEntryType != CRF_HVTEntryType.AI)
				continue;

			if (!m_mEntryToHVT.Contains(index))
				continue;

			IEntity hvtEntity = m_mEntryToHVT.Get(index);
			if (!hvtEntity || !IsHVTAlive(hvtEntity))
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(hvtEntity);
			if (!character)
				continue;

			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
			if (!charController || charController.GetLifeState() != ECharacterLifeState.ALIVE)
				continue;

			// Helper compartment still attached (also true while being escorted) - state intact
			if (ACE_AnimationTools.GetHelperCompartment(character))
				continue;

			// Inside a real vehicle (loaded by players) - helper is intentionally absent, do not re-tie
			// Water bodies also register as vehicle compartments in the engine, so exclude that case
			if (character.IsInVehicle() && !SCR_CharacterControllerComponent.ACE_Carrying_IsCharacterInWater(character))
				continue;

			// Captive/surrendered AI with no helper compartment -> ACE captive state is brokened
			SetEntitySurrender(hvtEntity, m_eAIHVTState == CRF_AIHVTState.CUFFED);
			Print(string.Format("[HVT] Repaired broken ACE captive state for AI HVT entry %1", index), LogLevel.WARNING);
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
				UnregisterHVTEntity(existingHVT);
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
		
		// Hook death callback (characters only - AI/PLAYER)
		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
		{
			characterController.m_OnPlayerDeathWithParam.Remove(OnHVTDeath);
			characterController.m_OnPlayerDeathWithParam.Insert(OnHVTDeath);
		}

		// ALWAYS set damage state to match component setting (overrides prefab default on spawn/respawn)
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(hvtEntity.FindComponent(DamageManagerComponent));
		if (damageManager)
		{
			damageManager.EnableDamageHandling(!m_bDisableDamage);
			m_mHVTDamageManagers.Set(hvtEntity, damageManager); // Cache for IsHVTAlive

			// Non-characters (VEHICLE etc.) have no death callback - hook destruction instead
			if (!characterController)
			{
				SCR_DamageManagerComponent scrDamageManager = SCR_DamageManagerComponent.Cast(damageManager);
				if (scrDamageManager)
				{
					CRF_HVTDestructionWrapper wrapper = new CRF_HVTDestructionWrapper(this, hvtEntity);
					m_mHVTDestructionWrappers.Set(hvtEntity, wrapper);
					scrDamageManager.GetOnDamageStateChanged().Insert(wrapper.OnDamageStateChanged);
				}
			}
		}
	}

	protected void UnregisterHVTEntity(IEntity hvtEntity)
	{
		if (!hvtEntity)
			return;

		SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(hvtEntity.FindComponent(SCR_CharacterControllerComponent));
		if (characterController)
		{
			characterController.m_OnLifeStateChanged.Remove(OnLifeStateChangedWrapper);
			characterController.m_OnPlayerDeathWithParam.Remove(OnHVTDeath);
		}

		// Unhook the destruction wrapper before dropping it
		if (m_mHVTDestructionWrappers.Contains(hvtEntity))
		{
			CRF_HVTDestructionWrapper wrapper = m_mHVTDestructionWrappers.Get(hvtEntity);
			SCR_DamageManagerComponent scrDamageManager = SCR_DamageManagerComponent.Cast(m_mHVTDamageManagers.Get(hvtEntity));
			if (wrapper && scrDamageManager)
				scrDamageManager.GetOnDamageStateChanged().Remove(wrapper.OnDamageStateChanged);

			m_mHVTDestructionWrappers.Remove(hvtEntity);
		}

		if (m_mHVTEntryIndex.Contains(hvtEntity))
		{
			int entryIndex = m_mHVTEntryIndex.Get(hvtEntity);
			if (m_mEntryToHVT.Contains(entryIndex) && m_mEntryToHVT.Get(entryIndex) == hvtEntity)
				m_mEntryToHVT.Remove(entryIndex);
		}

		m_mHVTEntryIndex.Remove(hvtEntity);
		m_mHVTDamageManagers.Remove(hvtEntity);
	}
	
	// HVT death callback (AI/PLAYER) - syncs positions and shows hint
	void OnHVTDeath(SCR_CharacterControllerComponent characterController, IEntity killerEntity, Instigator killer)
	{
		if (!characterController)
			return;

		IEntity hvtEntity = characterController.GetOwner();
		if (!hvtEntity)
			return;

		OnHVTLost(hvtEntity, killer);
	}

	// HVT destruction callback (VEHICLE) - gets the killer from the damage manager instead
	void OnHVTDestroy(IEntity hvtEntity)
	{
		if (!hvtEntity)
			return;

		Instigator killer;
		if (m_mHVTDamageManagers.Contains(hvtEntity))
		{
			DamageManagerComponent damageManager = m_mHVTDamageManagers.Get(hvtEntity);
			if (damageManager)
				killer = damageManager.GetInstigator();
		}

		OnHVTLost(hvtEntity, killer);
	}

	//------------------------------------------------------------------------------------------------
	// HINT STRING HELPERS
	

	string GetTargetLabel()
	{
		if (m_eTargetType == CRF_TargetType.VIP)
			return "VIP";
		return "HVT";
	}

	// Bounds-checked entry lookup
	CRF_HVTEntry GetHVTEntry(int entryIndex)
	{
		if (!m_aHVTEntries || entryIndex < 0 || entryIndex >= m_aHVTEntries.Count())
			return null;
		return m_aHVTEntries[entryIndex];
	}

	//------------------------------------------------------------------------------------------------

	// Shared by OnHVTDeath/OnHVTDestroy: syncs positions, builds the hint, unregisters
	protected void OnHVTLost(IEntity hvtEntity, Instigator killer)
	{
		// Immediately sync positions - zero position will trigger marker removal on clients
		if (m_bEnableTransponderMarker)
			UpdateHVTPositions();

		int entryIndex = -1;
		if (m_mHVTEntryIndex.Contains(hvtEntity))
			entryIndex = m_mHVTEntryIndex.Get(hvtEntity);

		CRF_HVTEntry entry = GetHVTEntry(entryIndex);

		string hvtFactionLabel = "";
		if (entry && entry.m_eFaction != CRF_HVTFaction.NONE)
			hvtFactionLabel = entry.GetFactionKey() + " ";

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
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (entry && factionManager && killer)
		{
			Faction killerFaction = factionManager.GetPlayerFaction(killer.GetInstigatorPlayerID());
			if (killerFaction)
				killerLabel = " - " + entry.GetLossKilledByPhrase() + " " + killerFaction.GetFactionKey();
		}

		// No attributable killer
		if (killerLabel.IsEmpty() && entry)
			killerLabel = " - has " + entry.GetLossUnknownCausePhrase() + " under mysterious circumstances.";

		// Set hint string, replicate the entry
		m_sDeadHVTHint = hvtFactionLabel + GetTargetLabel() + playerNameLabel + killerLabel;
		m_iDeadHVTEntryIndex = entryIndex;
		Replication.BumpMe();
		OnDeadHVTHintReplicated();
		UnregisterHVTEntity(hvtEntity);
	}

	// Client: Show hint when replicated
	void OnDeadHVTHintReplicated()
	{
		if (m_sDeadHVTHint.IsEmpty())
			return;

		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (!hintManager)
			return;

		string verb = "KILLED";
		CRF_HVTEntry entry = GetHVTEntry(m_iDeadHVTEntryIndex);
		if (entry)
			verb = entry.GetLossTitleVerb();

		hintManager.ShowCustomHint(m_sDeadHVTHint, GetTargetLabel() + " " + verb, 10);
	}
	
	void GameInit()
	{
		m_bGameInit = true;
		
		// Server: Register player HVTs now that safestart has ended and players are spawned
		if (Replication.IsServer())
		{
			RegisterPlayerHVTs();
			
			if (m_bEnableTransponderMarker && m_bInitialPing)
				UpdateHVTPositions();
		}
		
		// Client/Listen: Create markers
		if (!m_bEnableTransponderMarker)
			return;
		
		CRF_PlayerScriptedMarkerManager playerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!playerScriptedMarkerManager) 
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
			
			if (!m_mEntryToTransponder.Contains(index))
			{
				Print(string.Format("[HVT] Warning: Transponder entity '%1' not found in world!", entry.m_sTransponderEntityName), LogLevel.WARNING);
				continue;
			}
			
			// Was: playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", m_timeBetweenPings, ...) // Fix A
			playerScriptedMarkerManager.AddScriptedMarker(entry.m_sTransponderEntityName, "0 0 0", 0, entry.m_sMarkerText, MARKER_ICON, MARKER_SIZE, entry.GetMarkerColor());
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

		// Run through minor Hvtposition validation for desync
		GetGame().GetCallqueue().Remove(ResyncHVTPositions);
		GetGame().GetCallqueue().CallLater(ResyncHVTPositions, 1500, false);
	}

	// Desync issues were still present and only noticeable during much longer transponder timer sessions, clears+syncs if missed
	void ResyncHVTPositions()
	{
		if (!Replication.IsServer() || !m_aHvtPositions || m_aHvtPositions.IsEmpty())
			return;

		Replication.BumpMe();
	}
	
	// Check if HVT is alive/valid via damage state (or entity existence for objects)
	bool IsHVTAlive(IEntity hvtEntity)
	{
		if (!hvtEntity)
			return false;
		
		// OBJECT entries usually have no damage manager and are absent from m_mHVTDamageManagers - alive by entity existence
		if (!m_mHVTDamageManagers.Contains(hvtEntity))
			return true;

		DamageManagerComponent damageManager = m_mHVTDamageManagers.Get(hvtEntity);
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
			
			foreach (int index, CRF_HVTEntry entry : m_aHVTEntries)
			{
				if (!entry || !m_mEntryToHVT.Contains(index))
					continue;
				
				IEntity hvtEntity = m_mEntryToHVT.Get(index);
				if (!hvtEntity || !IsHVTAlive(hvtEntity))
					continue;
				
				// If faction-filtered only notify the searching faction otherwise broadcast to all
				string pingFactionKey = "";
				if (m_filterFaction)
					pingFactionKey = m_searcherFactionKey;
				
				CRF_GameplayBroadcastManager.GetInstance().SendHint(targetLabel + " transponder has sent a signal", -1, pingFactionKey);
			}
		}
	}
	
	// Apply ACE Captives state to an AI HVT. Surrendered = hands-up, Cuffed = tied/restrained.
	// Note: Calling SetCaptive and SetSurrender together causes a ACE Captives animation conflict that causes errors
	// The ACE release action is permanently disabled via SetActionEnabled_S in both cases.
	void SetEntitySurrender(IEntity entity, bool cuffed)
	{
		if (!entity)
			return;
		
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (!charController)
			return;
		
		if (cuffed)
			charController.ACE_Captives_SetCaptive(true);
		else
			charController.ACE_Captives_SetSurrender(true);
		
		// Disable the ACE release action for all clients via server-authoritative flag.
		// SetActionEnabled_S replicates to clients — CanBeShown() returns false everywhere.
		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		if (actionsManager)
		{
			array<BaseUserAction> actions = {};
			actionsManager.GetActionsList(actions);
			foreach (BaseUserAction action : actions)
			{
				if (action.IsInherited(ACE_Captives_ReleaseCaptiveUserAction))
				{
					action.SetActionEnabled_S(false);
					break;
				}
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
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT/VEHICLE), or -1 to match any type
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
	
	// Getter bool for checking if entity is AIHVT
	bool IsAIHVT(IEntity entity)
	{
		if (!entity || !m_mHVTEntryIndex.Contains(entity))
			return false;
		
		int entryIndex = m_mHVTEntryIndex.Get(entity);
		if (entryIndex < 0 || entryIndex >= m_aHVTEntries.Count())
			return false;
		
		return m_aHVTEntries[entryIndex].m_eEntryType == CRF_HVTEntryType.AI;
	}
	
	// Count HVTs within range of a position
	// position: Center point to search from
	// range: Search radius in meters (use -1 for unlimited range / count all)
	// faction: Filter by CRF_HVTFaction, or NONE to match any faction
	// entryType: Filter by CRF_HVTEntryType (AI/PLAYER/OBJECT/VEHICLE), or -1 to match any type
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

	//------------------------------------------------------------------------------------------------
	// WORKBENCH-ONLY EDITOR PREVIEW
	// Shows each entry's prefab at its transponder position + a text label, live in the editor.
	// Doodoo'd out by claude since its just a lot of contingency preview refreshes
	//------------------------------------------------------------------------------------------------
#ifdef WORKBENCH

	protected ref map<int, IEntity> m_mPreviewEntities = new map<int, IEntity>();        // entry index -> spawned preview entity
	protected ref map<int, ResourceName> m_mPreviewPrefabs = new map<int, ResourceName>(); // entry index -> its spawned prefab
	protected ref map<int, IEntity> m_mCachedTransponders = new map<int, IEntity>();      // entry index -> resolved transponder entity
	protected ref map<int, vector> m_mLastSyncedPositions = new map<int, vector>();       // entry index -> last position written to its preview

	protected ref map<int, string> m_mPreviewLabels = new map<int, string>(); // entry index -> label text
	protected ref map<int, int> m_mPreviewLabelColors = new map<int, int>();  // entry index -> label color

	protected float m_fPreviewRefreshTimer;

	//=========================================================================
	// WB LIFECYCLE HOOKS
	//=========================================================================

	override event void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		RefreshAllPreviews(owner);
		RefreshAllLabels();
		super._WB_OnInit(owner, mat, src);
	}

	override event void _WB_OnCreate(IEntity owner, IEntitySource src)
	{
		RefreshAllPreviews(owner);
		RefreshAllLabels();
		super._WB_OnCreate(owner, src);
	}

	override int _WB_GetAfterWorldUpdateSpecs(IEntity owner, IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_ALWAYS;
	}

	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (GetGame().InPlayMode())
			return;

		SyncAllPreviewTransforms();
		DrawAllHVTLabels(owner);

		m_fPreviewRefreshTimer += timeSlice;
		if (m_fPreviewRefreshTimer >= 8.0)
		{
			m_fPreviewRefreshTimer = 0;
			RefreshAllPreviews(owner);
		}
	}

	override event bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		RefreshAllPreviews(owner);
		RefreshAllLabels();
		return super._WB_OnKeyChanged(owner, src, key, ownerContainers, parent);
	}

	override event void _WB_OnDelete(IEntity owner, IEntitySource src)
	{
		RemoveAllPreviews();
		super._WB_OnDelete(owner, src);
	}

	void ~CRF_HighValueTargetGamemodeManager()
	{
		RemoveAllPreviews();
	}

	//=========================================================================
	// PREVIEW OBJECT LOGIC
	//=========================================================================

	// Moves each spawned preview to its transponder's current position
	protected void SyncAllPreviewTransforms()
	{
		foreach (int index, IEntity preview : m_mPreviewEntities)
		{
			if (!preview || !m_mCachedTransponders.Contains(index))
				continue;

			IEntity transponder = m_mCachedTransponders.Get(index);
			if (!transponder)
				continue;

			vector pos = transponder.GetOrigin();
			if (m_mLastSyncedPositions.Contains(index) && m_mLastSyncedPositions.Get(index) == pos)
				continue;

			vector transform[4];
			transponder.GetTransform(transform);
			preview.SetTransform(transform);
			preview.Update();

			m_mLastSyncedPositions.Set(index, pos);
		}
	}

	// Resolves each entry's transponder by name and spawns/re-spawns its preview object
	protected void RefreshAllPreviews(IEntity owner)
	{
		if (!m_aHVTEntries)
		{
			RemoveAllPreviews();
			return;
		}

		int count = m_aHVTEntries.Count();

		for (int i = 0; i < count; i++)
		{
			CRF_HVTEntry entry = m_aHVTEntries[i];

			// PLAYER entries have nothing to preview
			if (!entry || entry.m_eEntryType == CRF_HVTEntryType.PLAYER || entry.m_sTransponderEntityName.IsEmpty() || entry.m_hvtPrefab.IsEmpty())
			{
				RemovePreviewForIndex(i);
				m_mCachedTransponders.Remove(i);
				continue;
			}

			IEntity transponder = owner.GetWorld().FindEntityByName(entry.m_sTransponderEntityName);
			m_mCachedTransponders.Set(i, transponder);

			if (!transponder)
			{
				RemovePreviewForIndex(i);
				continue;
			}

			EnsurePreviewForIndex(i, entry.m_hvtPrefab, transponder);
		}

		RemoveStalePreviews(count);
	}

	protected void EnsurePreviewForIndex(int index, ResourceName prefab, IEntity transponder)
	{
		IEntity preview = null;
		if (m_mPreviewEntities.Contains(index))
			preview = m_mPreviewEntities.Get(index);

		ResourceName currentPrefab = "";
		if (m_mPreviewPrefabs.Contains(index))
			currentPrefab = m_mPreviewPrefabs.Get(index);

		// Re-spawn only if the prefab changed or no preview exists yet
		if (!preview || currentPrefab != prefab)
		{
			RemovePreviewForIndex(index);

			Resource previewResource = Resource.Load(prefab);
			if (!previewResource)
				return;

			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			transponder.GetTransform(spawnParams.Transform);

			preview = GetGame().SpawnEntityPrefabLocal(previewResource, transponder.GetWorld(), spawnParams);
			if (!preview)
				return;

			// Ignore terrain-snap raycasts so the mesh doesn't push the transponder up
			preview.ClearFlags(EntityFlags.TRACEABLE, true);

			m_mPreviewEntities.Set(index, preview);
			m_mPreviewPrefabs.Set(index, prefab);
		}

		// Keep it positioned at the transponder
		vector transform[4];
		transponder.GetTransform(transform);
		preview.SetTransform(transform);
		preview.Update();
	}

	protected void RemovePreviewForIndex(int index)
	{
		if (m_mPreviewEntities.Contains(index))
		{
			IEntity preview = m_mPreviewEntities.Get(index);
			if (preview)
				SCR_EntityHelper.DeleteEntityAndChildren(preview);

			m_mPreviewEntities.Remove(index);
		}

		m_mPreviewPrefabs.Remove(index);
		m_mLastSyncedPositions.Remove(index);
	}

	protected void RemoveAllPreviews()
	{
		array<int> indices = {};
		foreach (int index, IEntity preview : m_mPreviewEntities)
			indices.Insert(index);

		foreach (int index : indices)
			RemovePreviewForIndex(index);

		m_mCachedTransponders.Clear();
		m_mLastSyncedPositions.Clear();
	}

	// Removes previews for entries that no longer exist
	protected void RemoveStalePreviews(int currentEntryCount)
	{
		array<int> staleIndices = {};
		foreach (int index, IEntity preview : m_mPreviewEntities)
		{
			if (index >= currentEntryCount)
				staleIndices.Insert(index);
		}

		foreach (int index : staleIndices)
		{
			RemovePreviewForIndex(index);
			m_mCachedTransponders.Remove(index);
		}
	}

	//=========================================================================
	// LABEL TEXT LOGIC
	//=========================================================================

	// Rebuilds the label + color cache for every entry
	protected void RefreshAllLabels()
	{
		m_mPreviewLabels.Clear();
		m_mPreviewLabelColors.Clear();

		if (!m_aHVTEntries)
			return;

		int count = m_aHVTEntries.Count();
		for (int i = 0; i < count; i++)
		{
			CRF_HVTEntry entry = m_aHVTEntries[i];
			if (!entry || entry.m_eEntryType == CRF_HVTEntryType.PLAYER || entry.m_sTransponderEntityName.IsEmpty() || entry.m_hvtPrefab.IsEmpty())
				continue;

			m_mPreviewLabels.Set(i, BuildHVTLabel(entry));
			m_mPreviewLabelColors.Set(i, entry.GetMarkerColor());
		}
	}

	// Builds "[FACTION | TYPE] MarkerText" for one entry
	protected string BuildHVTLabel(CRF_HVTEntry entry)
	{
		string typeName = typename.EnumToString(CRF_HVTEntryType, entry.m_eEntryType);
		string factionName = entry.GetFactionKey();
		if (factionName.IsEmpty())
			factionName = "NONE";

		string prefix = "[" + factionName + " | " + typeName + "]";
		if (entry.m_sMarkerText.IsEmpty())
			return prefix;

		return prefix + " " + entry.m_sMarkerText;
	}

	// Draws each entry's cached label above its transponder
	protected void DrawAllHVTLabels(IEntity owner)
	{
		foreach (int index, IEntity transponder : m_mCachedTransponders)
		{
			if (!transponder || !m_mPreviewLabels.Contains(index))
				continue;

			int color = 0xFFFFFFFF;
			if (m_mPreviewLabelColors.Contains(index))
				color = m_mPreviewLabelColors.Get(index);

			vector pos = transponder.GetOrigin();
			pos[1] = pos[1] + 0.5;

			DebugTextWorldSpace.Create(owner.GetWorld(), m_mPreviewLabels.Get(index),
				DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA | DebugTextFlags.ONCE,
				pos[0], pos[1], pos[2], 14, color, 0x88000000);
		}
	}
#endif
}
