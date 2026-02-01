/****************************************************************************************
 * CRF_BattleRoyaleComponent.c

 - Battle Royale system, largely inherited from MapStagingComponent
 - Various attempts at eonframe limiting and rpl optimization

 - Has the same timer system as MapStagingComponent, but adapted for BR use
 - Zones and Zone Stages are auto-discovered from entity name prefixes, unused zones are deleted
 - Player Count: Event-driven deaths (instant) + 60s backup poll
 - Winner detection uses group-based alive check, gated by player count threshold

 Comments: (Trist): May need revaluation of optimization logic, most expensive RPL's arent from here

 TODO: - Evaluate further optimizations if it explodes
 - Consider loot spawn/object cleanup/culling similar to CheckPlayersOutside via PolyZoneTrigger

 **************************************************************************************/

//------------------------------------------------------------------------------------------------
// Stage state enum (only INACTIVE, ACTIVE, ACTIVATED needed for activation-only logic)
//------------------------------------------------------------------------------------------------
enum CRF_BattleRoyaleStageState
{
	INACTIVE = 0,	// Not yet triggered/chained
	ACTIVE = 1,		// Started, during timer
	ACTIVATED = 2	// Finished timer or completed
}

//------------------------------------------------------------------------------------------------
// Runtime stage data (generated from discovered boundaries)
//------------------------------------------------------------------------------------------------
class CRF_BattleRoyaleStageData
{
	string m_sStageBoundaryName;
	bool m_bDeletePrevious;
	int m_iStageDuration;
	string m_sStageDisplayText;
	
	void CRF_BattleRoyaleStageData(string boundaryName, int duration, bool deletePrev, string displayText)
	{
		m_sStageBoundaryName = boundaryName;
		m_iStageDuration = duration;
		m_bDeletePrevious = deletePrev;
		m_sStageDisplayText = displayText;
	}
}

//------------------------------------------------------------------------------------------------
// Zone sequence container - uses prefix to auto-discover boundaries
//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sZonePrefix"}, "%1")]
class CRF_BattleRoyaleZoneData
{
	[Attribute("ZoneA", UIWidgets.EditBox, "PREFIX for boundary entities. System auto-finds entities named [Prefix]1, [Prefix]2, etc. Orders HIGHEST→LOWEST (ZoneA5 first stage, ZoneA1 final). Example: 'ZoneA' finds ZoneA1, ZoneA2, ZoneA3...")]
	string m_sZonePrefix;
	
	[Attribute("120", UIWidgets.EditBox, "Default duration (seconds) for each stage in this zone")]
	int m_iDefaultStageDuration;
	
	[Attribute("true", UIWidgets.CheckBox, "Delete previous boundary when each stage activates (recommended for BR)")]
	bool m_bDeletePreviousBoundary;
	
	// Runtime: populated during initialization
	ref array<ref CRF_BattleRoyaleStageData> m_aStages;
}


[ComponentEditorProps(category: "Game Mode Component", description: "Battle Royale Zone System")]
class CRF_BattleRoyaleComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_BattleRoyaleComponent : SCR_BaseGameModeComponent
{
	[Attribute("30", UIWidgets.EditBox, "Initial delay after safestart ends before first stage begins (seconds)", category: "Base Configuration")]
	int m_iInitialDelay;
	
	[Attribute("true", UIWidgets.CheckBox, "Auto-start BR sequence after safestart ends + initial delay", category: "Base Configuration")]
	bool m_bEnableAutoStart;
	
	[Attribute("true", UIWidgets.CheckBox, "Randomly select a zone sequence on init (if false, uses first zone)", category: "Base Configuration")]
	bool m_bRandomZoneSelection;
	
	[Attribute("false", UIWidgets.CheckBox, "Enable player count tracking and winner detection. POSSIBLE CAUSE OF INSTABILITY ISSUES UNTIL FURTHER STRESS TESTING?", category: "Base Configuration")]
	bool m_bEnablePlayerTracking;
	
	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourceNamePicker, "Audio to play when stage timer starts", "wav", category: "Base Configuration")]
	ResourceName m_sStageStartSound;
	
	[Attribute("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav", UIWidgets.ResourceNamePicker, "Audio to play when stage completes", "wav", category: "Base Configuration")]
	ResourceName m_sStageEndSound;
	
	[Attribute("false", UIWidgets.CheckBox, "Enable debug logging", category: "Base Configuration")]
	bool m_bDebugEnabled;
	
	[Attribute("", UIWidgets.EditBox, "Force a specific zone by PREFIX (e.g. 'ZoneA'). Leave empty for random selection.", category: "Base Configuration")]
	string m_sDebugForceSequence;
	
	[Attribute("Fight Until Death", UIWidgets.EditBox, "Main message shown when all stages complete", category: "Base Configuration")]
	string m_sFinalCompletionMainMessage;
	
	[Attribute("Final zone active", UIWidgets.EditBox, "Sub message shown when all stages complete", category: "Base Configuration")]
	string m_sFinalCompletionSubMessage;
	
	[Attribute("15", UIWidgets.EditBox, "Duration (seconds) to display final completion message", category: "Base Configuration")]
	int m_iFinalCompletionMessageDuration;
	
	//------------------------------------------------------------------------------------------------
	// Zone Config
	//------------------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.Auto, "List of possible zone sequences (only one will be selected at game start)", category: "Zone Configuration")]
	ref array<ref CRF_BattleRoyaleZoneData> m_aZoneSequences;
	
	[Attribute("BATTLE ROYALE DOCUMENTATION\n\n=== ZONE SETUP ===\n\n1. Create zone sequences with a PREFIX (e.g. 'ZoneA', 'ZoneB', 'ZoneC')\n2. Name boundary entities: [Prefix]1, [Prefix]2, [Prefix]3, etc.\n   Example: ZoneA1, ZoneA2, ZoneA3 for prefix 'ZoneA'\n3. Numeric suffix determines stage order:\n   - HIGHEST number = FIRST stage (largest zone)\n   - LOWEST number = FINAL stage (smallest zone)\n   Example: ZoneA5 -> ZoneA4 -> ZoneA3 -> ZoneA2 -> ZoneA1 (final)\n4. System auto-discovers all matching entities at init\n5. System randomly selects ONE zone and DELETES all others\n\n=== BOUNDARY BEHAVIOR ===\n\n- Boundaries start moved 10km away (invisible/inactive)\n- When stage timer completes, boundary moves to original position\n- 'Delete Previous' removes the prior boundary when new one activates\n- Use CRF_PolyZone prefabs with CRF_PolyZoneTrigger for boundaries\n\n=== ZONE SEQUENCE SETTINGS ===\n\n• Zone Name: Display name for identification\n• Zone Prefix: Entity name prefix (e.g. 'ZoneA' finds ZoneA1, ZoneA2...)\n• Default Duration: Timer duration for ALL stages in this zone\n• Delete Previous: Remove prior boundary on activation (recommended)\n\n=== EMERGENCY STOP ===\n\nCRF_BattleRoyaleComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_BattleRoyaleComponent)).StopBattleRoyale();", UIWidgets.EditBoxMultiline, "Battle Royale Setup Instructions", category: "Documentation")]
	string m_sInstructions;
	
	//------------------------------------------------------------------------------------------------
	// Replicated Public State for display.c
	//------------------------------------------------------------------------------------------------
	[RplProp()]
	bool countDownActive = false;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent = "";
	
	[RplProp()]
	string m_sTimerText = "";
	
	[RplProp()]
	string m_sStageText = "";
	
	[RplProp()]
	int m_iCurrentTimer = 0;
	
	[RplProp()]
	int m_iAlivePlayerCount = 0;
	
	// Winner tracking, before was RPLProp'd constantly but that could cause network nonos - rpl sync fix?
	ref array<int> m_aWinnerPlayerIds;
	
	protected bool m_bWinnerCheckActive = false;
	
	// Cached collections
	protected ref array<int> m_aCachedSlotIds; // Cache slots IDs to MAYBE avoid dynamic array issues
	protected ref map<RplId, ref array<int>> m_mGroupPlayers;
	protected ref array<int> m_aCachedWinnerBuffer;

	protected float m_fWinnerCheckTimer = 0;
	protected bool m_bWinnersDeclared = false; // Permanent flag - never check again after winners declared
	protected int m_iLastReplicatedAliveCount = -1; // Track last replicated value to avoid spam
	protected static const float WINNER_CHECK_INTERVAL = 15.0; // Check every X seconds
	protected static const int WINNER_CHECK_THRESHOLD = 8; // Start checking when < X players alive
	

	// Internal State
	[RplProp()]
	protected int m_iSelectedZoneIndex = -1;
	
	[RplProp()]
	protected int m_iCurrentStage = 0;
	
	[RplProp()]
	bool m_bBattleRoyaleActive = false;
	
	protected ref map<string, vector> m_mOriginalPositions;
	protected ref map<string, CRF_BattleRoyaleStageState> m_mStageStates;
	protected string m_sStoredMessageContent;
	protected SCR_PopUpNotification m_PopUpNotification;
	
	// Simplified JIP tracking - only current active boundary
	[RplProp()]
	protected string m_sCurrentActiveBoundary = "";
	
	[RplProp()]
	protected CRF_BattleRoyaleStageState m_iCurrentBoundaryState = CRF_BattleRoyaleStageState.INACTIVE;
	
	// Boundry search limit preventing infinite search 
	protected int m_iInitRetryCount = 0;
	protected static const int MAX_INIT_RETRIES = 3;
	
	// cacheing stage/zone data
	protected ref map<string, ref CRF_BattleRoyaleStageData> m_mStageDataCache;
	protected ref map<string, IEntity> m_mBoundaryEntityCache;
	
	protected bool m_bUpdateStageTimer = false;
	protected float m_fUpdateBuffer = 0;
	protected int m_iPlayerCountUpdateCounter = 0;
	
	// Hardcoded zone colors (can change this to color pickers if wanted)
	protected static const ref Color ACTIVE_POLYGON_COLOR = Color(0.409, 0.092, 0, 0.2);
	protected static const ref Color ACTIVE_POLYGON_BORDER_COLOR = Color(0.409, 0.116, 0.007, 0.9);
	protected static const ref Color ACTIVATED_POLYGON_COLOR = Color(0.293, 0, 0, 0.7);
	protected static const ref Color ACTIVATED_POLYGON_BORDER_COLOR = Color(0.16, 0, 0, 0.85);
	
	//------------------------------------------------------------------------------------------------
	// Initialization
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Initialize cache maps on ALL clients and server
		m_mOriginalPositions = new map<string, vector>();
		m_mStageStates = new map<string, CRF_BattleRoyaleStageState>();
		m_mStageDataCache = new map<string, ref CRF_BattleRoyaleStageData>();
		m_mBoundaryEntityCache = new map<string, IEntity>();
		
		if (!m_aZoneSequences)
			m_aZoneSequences = new array<ref CRF_BattleRoyaleZoneData>();
		
		// Initialize winner array and cached collections
		m_aWinnerPlayerIds = new array<int>();
		m_aCachedSlotIds = new array<int>();
		m_mGroupPlayers = new map<RplId, ref array<int>>();
		m_aCachedWinnerBuffer = new array<int>();
		
		// Server-only initialization
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
		
		// Delay initialization to ensure world is fully loaded
		GetGame().GetCallqueue().CallLater(InitializeBattleRoyale, 5000, false);
		

		GetGame().GetCallqueue().CallLater(MonitorSafestart, 7000, false);
	}
	
	// Called when any controllable entity is destroyed (player death)
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);
		
		// Player tracking disabled
		if (!m_bEnablePlayerTracking)
			return;
		
		// Server only
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		
		if (!m_bBattleRoyaleActive || m_bWinnersDeclared)
			return;
		
		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0)
			return;
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] Player %1 died - scheduling count update", victimId));
		
		// 250ms delay to ensure slot death state is updated
		GetGame().GetCallqueue().CallLater(OnPlayerDeathDelayed, 250, false);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnPlayerDeathDelayed()
	{
		UpdateAlivePlayerCount();
		if (m_bWinnerCheckActive)
			CheckForWinners();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateAlivePlayerCount()
	{
		int aliveCount = 0;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
		{
			if (m_bDebugEnabled)
				Print("[CRF_BattleRoyaleComponent] WARNING: SlottingManager not available for player count");
			return;
		}
		
		// SAFE ITERATION: Copy slot IDs first to prevent crash if player disconnects mid-iteration
		m_aCachedSlotIds.Clear();
		array<int> tempIds = slottingManager.GetAllSlotIds();
		if (!tempIds || tempIds.Count() == 0)
			return;
		
		m_aCachedSlotIds.Copy(tempIds);
		tempIds = null;
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		foreach (int slotId : m_aCachedSlotIds)
		{
			CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
			if (!slotData)
				continue;
			
			// Skip locked or empty
			if (slotData.GetIsLockedSlot() || slotData.GetSlotCurrentPlayerId() == 0)
				continue;
			
			// Skip disconnected players
			if (!pm.IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
				continue;
			
			// Count alive 
			if (!slotData.GetIsDeadSlot())
				aliveCount++;
		}
		
		m_iAlivePlayerCount = aliveCount;
	
	// Check if we should activate winner detection
	// Safety: Only activate after first stage has started AND BR is active AND player count is low
	if (m_bBattleRoyaleActive && m_iCurrentStage > 0 && aliveCount < WINNER_CHECK_THRESHOLD && aliveCount > 0 && !m_bWinnerCheckActive && !m_bWinnersDeclared)
	{
		m_bWinnerCheckActive = true;
		m_fWinnerCheckTimer = 0;
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] Winner detection activated - %1 players alive", aliveCount));
	}
	
	// Only replicate if count actually changed
	if (m_iLastReplicatedAliveCount != aliveCount)
	{
		m_iLastReplicatedAliveCount = aliveCount;
		Replication.BumpMe();
	}
}

//------------------------------------------------------------------------------------------------
override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bUpdateStageTimer)
				UpdateStageTimer();
			
			// Backup poll every Xs (primary detection is OnControllableDestroyed)
			// Gated behind player tracking toggle
			if (m_bEnablePlayerTracking && m_bBattleRoyaleActive)
			{
				m_iPlayerCountUpdateCounter++;
				if (m_iPlayerCountUpdateCounter >= 30)
				{
					UpdateAlivePlayerCount();
					m_iPlayerCountUpdateCounter = 0;
				}
			}
			
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		
		// Winner check loop - only when player tracking enabled
		if (m_bEnablePlayerTracking && m_bWinnerCheckActive)
		{
			m_fWinnerCheckTimer += timeSlice;
			
			if (m_fWinnerCheckTimer >= WINNER_CHECK_INTERVAL)
			{
				m_fWinnerCheckTimer = 0;
				CheckForWinners();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void InitializeBattleRoyale()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Verify world is ready
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] World not ready, retrying in 2s...");
			GetGame().GetCallqueue().CallLater(InitializeBattleRoyale, 2000, false);
			return;
		}
		
		if (m_bDebugEnabled) 
			Print("[CRF_BattleRoyaleComponent] Initializing...");
		
		if (!m_aZoneSequences || m_aZoneSequences.Count() == 0)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] WARNING: No zone sequences configured!");
			return;
		}
		
		// Discover boundaries for each zone by prefix
		foreach (CRF_BattleRoyaleZoneData zoneData : m_aZoneSequences)
		{
			if (!zoneData || zoneData.m_sZonePrefix.IsEmpty())
				continue;
			
			DiscoverZoneBoundaries(zoneData);
		}
		
		// Debug: Show discovered boundaries
		if (m_bDebugEnabled)
		{
			foreach (CRF_BattleRoyaleZoneData zoneData : m_aZoneSequences)
			{
				if (!zoneData)
					continue;
				int stageCount = 0;
				if (zoneData.m_aStages)
					stageCount = zoneData.m_aStages.Count();
				Print(string.Format("[CRF_BattleRoyaleComponent] Zone '%1': %2 stages discovered", zoneData.m_sZonePrefix, stageCount));
			}
		}
		
		// Select zone sequence
		if (m_bRandomZoneSelection)
		{
			SelectRandomZone();
		}
		else
		{
			SelectZone(0);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Searches for boundry entities named [Prefix]1, [Prefix]2, etc. and orders them numerically
	 */
	void DiscoverZoneBoundaries(CRF_BattleRoyaleZoneData zoneData)
	{
		if (!zoneData || zoneData.m_sZonePrefix.IsEmpty())
			return;
		
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		
		// Initialize the stages array
		zoneData.m_aStages = new array<ref CRF_BattleRoyaleStageData>();
		
		// Find all entities matching the prefix pattern (Prefix + number)
		// We'll search for numbers 1-10 each zone prefix 
		ref map<int, string> stageNumbers = new map<int, string>();
		
		for (int i = 1; i <= 10; i++)
		{
			string entityName = zoneData.m_sZonePrefix + i.ToString();
			IEntity entity = world.FindEntityByName(entityName);
			
			if (entity)
			{
				stageNumbers.Set(i, entityName);
				if (m_bDebugEnabled)
					Print(string.Format("[CRF_BattleRoyaleComponent] Found boundary: '%1'", entityName));
			}
		}
		
		// Sort by stage number (highest first = first stage, lowest last = final stage)
		array<int> sortedNumbers = {};
		for (int i = 0; i < stageNumbers.Count(); i++)
		{
			int key = stageNumbers.GetKey(i);
			sortedNumbers.Insert(key);
		}
		sortedNumbers.Sort(true); // true = descending order (highest first)
		
		// Create stage data in order
		foreach (int stageNum : sortedNumbers)
		{
			string entityName;
			if (stageNumbers.Find(stageNum, entityName))
			{
				CRF_BattleRoyaleStageData stageData = new CRF_BattleRoyaleStageData(
					entityName,
					zoneData.m_iDefaultStageDuration,
					zoneData.m_bDeletePreviousBoundary,
					"Zone Closing!"
				);
				
				zoneData.m_aStages.Insert(stageData);
			}
		}
		
		// Clear temporary collections
		sortedNumbers.Clear();
		stageNumbers.Clear();
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyaleComponent] Zone '%1': Discovered %2 boundaries", zoneData.m_sZonePrefix, zoneData.m_aStages.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Select a specific zone sequence and delete all others
	 * @param zoneIndex - Index of the zone sequence to use
	 * @return bool - true if zone was selected successfully
	 */
	bool SelectZone(int zoneIndex)
	{
		if (RplSession.Mode() == RplMode.Client)
			return false;
		
		if (!m_aZoneSequences || zoneIndex < 0 || zoneIndex >= m_aZoneSequences.Count())
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] SelectZone failed - invalid index %1", zoneIndex));
			return false;
		}
		
		m_iSelectedZoneIndex = zoneIndex;
		CRF_BattleRoyaleZoneData selectedZone = m_aZoneSequences[zoneIndex];
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Selected zone sequence: '%1' (index %2)", selectedZone.m_sZonePrefix, zoneIndex));
		
		// Verify the selected zone has discovered boundaries
		if (!selectedZone.m_aStages || selectedZone.m_aStages.Count() == 0)
		{
			Print(string.Format("[CRF_BattleRoyaleComponent] ERROR: Zone '%1' has no boundaries! Check entity names match prefix.", selectedZone.m_sZonePrefix));
			return false;
		}
		
		// FIRST: Verify the selected zone's boundaries exist before deleting anything
		bool allBoundariesExist = true;
		if (selectedZone && selectedZone.m_aStages)
		{
			foreach (CRF_BattleRoyaleStageData stageData : selectedZone.m_aStages)
			{
				if (!stageData || stageData.m_sStageBoundaryName.IsEmpty())
					continue;
				
				IEntity testEntity = GetGame().GetWorld().FindEntityByName(stageData.m_sStageBoundaryName);
				if (!testEntity)
				{
					if (m_bDebugEnabled) 
						Print(string.Format("[CRF_BattleRoyaleComponent] Selected zone boundary '%1' not found - aborting zone selection", stageData.m_sStageBoundaryName));
					allBoundariesExist = false;
					break;
				}
			}
		}
		
		if (!allBoundariesExist)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] Cannot select zone - missing boundaries. Will retry.");
			return false;
		}
		
		// THEN: Delete all boundaries from unused zones (only after verifying selected zone is valid)
		for (int i = 0; i < m_aZoneSequences.Count(); i++)
		{
			if (i == zoneIndex)
				continue;
			
			CRF_BattleRoyaleZoneData otherZone = m_aZoneSequences[i];
			if (!otherZone || !otherZone.m_aStages)
				continue;
			
			foreach (CRF_BattleRoyaleStageData stageData : otherZone.m_aStages)
			{
				if (!stageData || stageData.m_sStageBoundaryName.IsEmpty())
					continue;
				
				// Find entity directly, don't use cache for entities we're about to delete
				IEntity boundaryEntity = GetGame().GetWorld().FindEntityByName(stageData.m_sStageBoundaryName);
				if (boundaryEntity)
				{
					if (m_bDebugEnabled) 
						Print(string.Format("[CRF_BattleRoyaleComponent] Deleting unused boundary: '%1' from zone '%2'", stageData.m_sStageBoundaryName, otherZone.m_sZonePrefix));
					// Delay deletion, maybe will help replication issues
					GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 100, false, boundaryEntity);
				}
			}
		}
		
		// Clear entity cache to ensure we don't have stale references
		if (m_mBoundaryEntityCache)
			m_mBoundaryEntityCache.Clear();
		
		// Initialize the selected zone's boundaries
		InitializeZoneBoundaries(selectedZone);
		
		Replication.BumpMe();
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Randomly select a zone sequence
	 * @return bool - true if zone was selected successfully
	 */
	bool SelectRandomZone()
	{
		if (RplSession.Mode() == RplMode.Client)
			return false;
		
		if (!m_aZoneSequences || m_aZoneSequences.Count() == 0)
			return false;
		
		// Check for forced sequence first (matches by prefix)
		if (!m_sDebugForceSequence.IsEmpty())
		{
			for (int i = 0; i < m_aZoneSequences.Count(); i++)
			{
				CRF_BattleRoyaleZoneData zoneData = m_aZoneSequences[i];
				if (zoneData && zoneData.m_sZonePrefix == m_sDebugForceSequence)
				{
					Print(string.Format("[CRF_BattleRoyaleComponent] DEBUG: Forcing zone sequence '%1' (index %2)", m_sDebugForceSequence, i));
					return SelectZone(i);
				}
			}
			Print(string.Format("[CRF_BattleRoyaleComponent] WARNING: Debug force sequence prefix '%1' not found, falling back to random", m_sDebugForceSequence));
		}
		
		int randomIndex = Math.RandomInt(0, m_aZoneSequences.Count());
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Randomly selected zone index: %1", randomIndex));
		
		return SelectZone(randomIndex);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Initialize boundaries for the selected zone
	 */
	void InitializeZoneBoundaries(CRF_BattleRoyaleZoneData zoneData)
	{
		if (!zoneData || !zoneData.m_aStages)
			return;
		
		bool foundMissingEntity = false;
		
		for (int i = 0; i < zoneData.m_aStages.Count(); i++)
		{
			CRF_BattleRoyaleStageData stageData = zoneData.m_aStages[i];
			if (!stageData || !stageData.m_sStageBoundaryName || stageData.m_sStageBoundaryName.IsEmpty())
			{
				if (m_bDebugEnabled) 
					Print(string.Format("[CRF_BattleRoyaleComponent] WARNING: Invalid stage data at index %1", i));
				continue;
			}
			
			IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sStageBoundaryName);
			if (!boundaryEntity)
			{
				Print(string.Format("[CRF_BattleRoyaleComponent] ERROR: Boundary '%1' not found! Check entity name.", stageData.m_sStageBoundaryName));
				foundMissingEntity = true;
				continue;
			}
			
			vector originalPos = boundaryEntity.GetOrigin();
			m_mOriginalPositions.Set(stageData.m_sStageBoundaryName, originalPos);
			
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Stored original position for '%1': %2", stageData.m_sStageBoundaryName, originalPos));
			
			// Initialize stage state as INACTIVE
			m_mStageStates.Set(stageData.m_sStageBoundaryName, CRF_BattleRoyaleStageState.INACTIVE);
			
			// Cache stage data
			m_mStageDataCache.Set(stageData.m_sStageBoundaryName, stageData);
			
			// ACTIVATION only: boundaries start moved away, get moved back when activated
			vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
			boundaryEntity.SetOrigin(newPos);
			
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Boundary '%1' initialized (moved away)", stageData.m_sStageBoundaryName));
		}
		
		Replication.BumpMe();
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Successfully initialized %1 stages", m_mStageStates.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Fast cached entity lookup
	 */
	IEntity GetCachedBoundaryEntity(string entityName)
	{
		if (!entityName || entityName.IsEmpty())
			return null;
		
		if (!m_mBoundaryEntityCache)
			m_mBoundaryEntityCache = new map<string, IEntity>();
		
		IEntity cachedEntity;
		if (m_mBoundaryEntityCache.Find(entityName, cachedEntity))
		{
			if (cachedEntity)
				return cachedEntity;
			else
				m_mBoundaryEntityCache.Remove(entityName);
		}
		
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;
		
		IEntity entity = world.FindEntityByName(entityName);
		if (entity)
			m_mBoundaryEntityCache.Set(entityName, entity);
		
		return entity;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get the CRF_PolyZoneTrigger child entity from a GameBoundary parent
	 */
	CRF_PolyZoneTrigger GetBoundaryTrigger(string boundaryName)
	{
		if (!boundaryName || boundaryName.IsEmpty())
			return null;
		
		IEntity parentEntity = GetCachedBoundaryEntity(boundaryName);
		if (!parentEntity)
			return null;
		
		IEntity child = parentEntity.GetChildren();
		while (child)
		{
			CRF_PolyZoneTrigger trigger = CRF_PolyZoneTrigger.Cast(child);
			if (trigger)
				return trigger;
			child = child.GetSibling();
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get the currently selected zone stages
	 */
	array<ref CRF_BattleRoyaleStageData> GetSelectedZoneStages()
	{
		if (m_iSelectedZoneIndex < 0 || m_iSelectedZoneIndex >= m_aZoneSequences.Count())
			return null;
		
		CRF_BattleRoyaleZoneData selectedZone = m_aZoneSequences[m_iSelectedZoneIndex];
		if (!selectedZone)
			return null;
		
		return selectedZone.m_aStages;
	}
	
	//------------------------------------------------------------------------------------------------
	// Safestart Monitoring
	//------------------------------------------------------------------------------------------------
	void MonitorSafestart()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		if (!m_bEnableAutoStart)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] Auto-start disabled - waiting for manual trigger");
			GetGame().GetCallqueue().Remove(MonitorSafestart);
			return;
		}
		
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart || safestart.GetSafestartStatus() || CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] Waiting for game to be ready and safestart to end...");
			GetGame().GetCallqueue().CallLater(MonitorSafestart, 15000, false);
			return;
		}
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Game is live, starting in %1 seconds", m_iInitialDelay));
		GetGame().GetCallqueue().CallLater(StartStaging, m_iInitialDelay * 1000, false);
		m_bBattleRoyaleActive = true;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	// Stage Execution
	//------------------------------------------------------------------------------------------------
	void StartStaging()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		array<ref CRF_BattleRoyaleStageData> stages = GetSelectedZoneStages();
		if (!stages || m_iCurrentStage >= stages.Count())
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] StartStaging blocked - no valid stage");
			return;
		}
		
		CRF_BattleRoyaleStageData currentStage = stages[m_iCurrentStage];
		if (!currentStage)
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] ERROR: Stage %1 data is null!", m_iCurrentStage + 1));
			return;
		}
		
		// Clear any existing timer
		if (countDownActive)
		{
			m_bUpdateStageTimer = false;
			countDownActive = false;
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] Cleared existing timer");
		}
		
		m_iCurrentTimer = currentStage.m_iStageDuration;
		m_sTimerText = SCR_FormatHelper.FormatTime(m_iCurrentTimer);
		countDownActive = true;
		m_sStageText = currentStage.m_sStageDisplayText;
		
		// Update boundary visual state to ACTIVE
		Rpc(UpdateBoundaryVisualState, currentStage.m_sStageBoundaryName, CRF_BattleRoyaleStageState.ACTIVE);
		UpdateBoundaryVisualStateLocal(currentStage.m_sStageBoundaryName, CRF_BattleRoyaleStageState.ACTIVE);
		
		// Play stage start sound with 1 second delay
		GetGame().GetCallqueue().CallLater(PlayStageSound, 1000, false, "start");
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Starting stage %1: '%2' (%3s)", m_iCurrentStage + 1, m_sStageText, m_iCurrentTimer));
		
		Replication.BumpMe();
		m_bUpdateStageTimer = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateStageTimer()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		if (!countDownActive)
		{
			m_bUpdateStageTimer = false;
			return;
		}

		// Decrement first
		m_iCurrentTimer--;
		
		// Update display text
		m_sTimerText = SCR_FormatHelper.FormatTime(m_iCurrentTimer);

		if (m_bDebugEnabled)
		{
			if (m_iCurrentTimer % 10 == 0 || m_iCurrentTimer <= 10)
				Print(string.Format("[CRF_BattleRoyaleComponent] Stage %1 timer: %2s remaining", m_iCurrentStage + 1, m_iCurrentTimer));
		}
		
		// Replicate every second for smooth timer display
		Replication.BumpMe();

		if (m_iCurrentTimer <= 0)
		{
			m_bUpdateStageTimer = false;
			countDownActive = false;
			m_sTimerText = "";
			
			ExecuteCurrentStage();
			return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ExecuteCurrentStage()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		array<ref CRF_BattleRoyaleStageData> stages = GetSelectedZoneStages();
		if (!stages || m_iCurrentStage >= stages.Count())
			return;
		
		CRF_BattleRoyaleStageData currentStage = stages[m_iCurrentStage];
		if (!currentStage)
			return;
		
		ApplyBoundaryActivation(currentStage, m_iCurrentStage);
		HandleStageProgression();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Apply the boundary activation (move to original position and check players outside)
	 */
	void ApplyBoundaryActivation(CRF_BattleRoyaleStageData stageData, int stageIndex)
	{
		if (!stageData)
			return;
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sStageBoundaryName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] ApplyBoundaryActivation failed - entity '%1' not found", stageData.m_sStageBoundaryName));
			return;
		}
		
		// Handle "Delete Previous" option first
		if (stageData.m_bDeletePrevious && stageIndex > 0)
		{
			array<ref CRF_BattleRoyaleStageData> stages = GetSelectedZoneStages();
			if (stages)
			{
				CRF_BattleRoyaleStageData prevStageData = stages[stageIndex - 1];
				if (prevStageData)
				{
					IEntity prevEntity = GetCachedBoundaryEntity(prevStageData.m_sStageBoundaryName);
					if (prevEntity)
					{
						if (m_bDebugEnabled) 
							Print(string.Format("[CRF_BattleRoyaleComponent] Deleting previous boundary '%1'", prevStageData.m_sStageBoundaryName));
						m_mBoundaryEntityCache.Remove(prevStageData.m_sStageBoundaryName);
						GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 100, false, prevEntity);
					}
				}
			}
		}
		
		// Move boundary back to original position (ACTIVATION)
		vector originalPos;
		if (m_mOriginalPositions.Find(stageData.m_sStageBoundaryName, originalPos))
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Moving '%1' to original position: %2", stageData.m_sStageBoundaryName, originalPos));
			
			boundaryEntity.SetOrigin(originalPos);
			
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Boundary '%1' activated at %2", stageData.m_sStageBoundaryName, boundaryEntity.GetOrigin()));
			
			// For reversed (INCLUSION) zones, apply effects to players already outside
			CRF_PolyZoneTrigger trigger = GetBoundaryTrigger(stageData.m_sStageBoundaryName);
			if (trigger)
				trigger.CheckPlayersOutside();
		}
		else
		{
			Print(string.Format("[CRF_BattleRoyaleComponent] ERROR: No original position stored for '%1'! Available keys:", stageData.m_sStageBoundaryName));
			for (int i = 0; i < m_mOriginalPositions.Count(); i++)
			{
				string key = m_mOriginalPositions.GetKey(i);
				vector pos = m_mOriginalPositions.Get(key);
				Print(string.Format("  - Key: '%1' = %2", key, pos));
			}
		}
		
		// Update boundary visual state to ACTIVATED
		Rpc(UpdateBoundaryVisualState, stageData.m_sStageBoundaryName, CRF_BattleRoyaleStageState.ACTIVATED);
		UpdateBoundaryVisualStateLocal(stageData.m_sStageBoundaryName, CRF_BattleRoyaleStageState.ACTIVATED);
		
		// Play stage end sound
		PlayStageSound("end");
	}
	
	//------------------------------------------------------------------------------------------------
	void HandleStageProgression()
	{
		m_iCurrentStage++;
		Replication.BumpMe();
		
		array<ref CRF_BattleRoyaleStageData> stages = GetSelectedZoneStages();
		if (stages && m_iCurrentStage < stages.Count())
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Moving to stage %1 in 2s", m_iCurrentStage + 1));
			GetGame().GetCallqueue().CallLater(StartStaging, 2000, false);
		}
		else
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] All stages completed!");
			GetGame().GetCallqueue().CallLater(FinalizeBattleRoyale, 3000, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void FinalizeBattleRoyale()
	{
		if (m_bDebugEnabled) 
			Print("[CRF_BattleRoyaleComponent] Finalizing...");
		
		// Keep m_bBattleRoyaleActive = true so player count stays visible
		m_sStageText = "";
		m_sTimerText = "";
		
		m_sMessageContent = string.Format("%1|%2|%3", m_sFinalCompletionMainMessage, m_iFinalCompletionMessageDuration, m_sFinalCompletionSubMessage);
		Replication.BumpMe();
		ShowMessage();
	}
	
	//------------------------------------------------------------------------------------------------
	void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		m_sStoredMessageContent = m_sMessageContent;
		
		array<string> messageSplitArray = {};
		m_sMessageContent.Split("|", messageSplitArray, false);
		
		if (messageSplitArray.Count() != 3)
		{
			messageSplitArray.Clear();
			return;
		}

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];
		
		messageSplitArray.Clear();
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Popup: '%1' (%2s) - '%3'", mainMessage, time, subMessage));
		m_PopUpNotification.PopupMsg(mainMessage, time.ToFloat(), subMessage);
	}
	
	//------------------------------------------------------------------------------------------------
	// Sound System
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void PlayStageSound(string soundType)
	{
		ResourceName soundToPlay;
		
		if (soundType == "start" && !m_sStageStartSound.IsEmpty())
			soundToPlay = m_sStageStartSound;
		else if (soundType == "end" && !m_sStageEndSound.IsEmpty())
			soundToPlay = m_sStageEndSound;
		else if (soundType == "victory")
		{
			// Cool guy fortnite epic win, yes
			int track = Math.RandomInt(0, 4);
			if (track == 0)
				soundToPlay = "Sounds/RadioBroadcast/Samples/Everon/Music/RadioBroadcast_Music_04_Propis_HappyMondayInAFactory.wav";
			else if (track == 1)
				soundToPlay = "Sounds/RadioBroadcast/Samples/Everon/Music/RadioBroadcast_Music_08_BertDave_Electrolite.wav";
			else if (track == 2)
				soundToPlay = "Sounds/RadioBroadcast/Samples/Everon/Music/RadioBroadcast_Music_06_Havarna_Draty.wav";
			else
				soundToPlay = "Sounds/RadioBroadcast/Samples/Everon/Music/RadioBroadcast_Music_15_OMatejka_Walk(LeonB_Remix).wav";
		}
		
		if (soundToPlay.IsEmpty())
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] PlayStageSound: No sound for type '%1'", soundType));
			return;
		}
		
		if (RplSession.Mode() != RplMode.Client)
		{
			Rpc(PlayStageSound, soundType);
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Playing %1 sound: %2", soundType, soundToPlay));
		}
		
		AudioSystem.PlaySound(soundToPlay);
	}
	
	//------------------------------------------------------------------------------------------------
	// Visual State Updates
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void UpdateBoundaryVisualState(string boundaryName, CRF_BattleRoyaleStageState newState)
	{
		UpdateBoundaryVisualStateLocal(boundaryName, newState);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateBoundaryVisualStateLocal(string boundaryName, CRF_BattleRoyaleStageState newState)
	{
		if (!boundaryName || boundaryName.IsEmpty())
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] UpdateBoundaryVisualStateLocal: Invalid boundary name");
			return;
		}
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(boundaryName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] UpdateBoundaryVisualStateLocal: Boundary '%1' not found", boundaryName));
			return;
		}
		
		CRF_PolyZone polyZone = CRF_PolyZone.Cast(boundaryEntity.FindComponent(CRF_PolyZone));
		if (!polyZone)
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] UpdateBoundaryVisualStateLocal: CRF_PolyZone not found on '%1'", boundaryName));
			return;
		}
		
		// Use hardcoded colors based on state
		Color fillColor;
		Color borderColor;
		string stateString;
		
		if (newState == CRF_BattleRoyaleStageState.INACTIVE)
		{
			if (m_bDebugEnabled) 
				Print(string.Format("[CRF_BattleRoyaleComponent] Boundary '%1' state: INACTIVE (no color change)", boundaryName));
			return;
		}
		else if (newState == CRF_BattleRoyaleStageState.ACTIVE)
		{
			fillColor = ACTIVE_POLYGON_COLOR;
			borderColor = ACTIVE_POLYGON_BORDER_COLOR;
			stateString = "ACTIVE";
		}
		else if (newState == CRF_BattleRoyaleStageState.ACTIVATED)
		{
			fillColor = ACTIVATED_POLYGON_COLOR;
			borderColor = ACTIVATED_POLYGON_BORDER_COLOR;
			stateString = "ACTIVATED";
		}
		
		polyZone.m_cPolygonColor = fillColor;
		polyZone.m_cPolygonBorderColor = borderColor;
		
		// Colors are applied to polyZone properties
		// Widget will use these colors automatically when drawn - no recreation needed
		// Avoid expensive DeleteMapWidget/CreateMapWidget cycle that allocates arrays
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyaleComponent] Colors applied to '%1'", boundaryName));
		
		// Update stored state - server only
		if (Replication.IsServer())
		{
			m_mStageStates.Set(boundaryName, newState);
			
			// Only replicate if this is a new state or different boundary
			if (m_sCurrentActiveBoundary != boundaryName || m_iCurrentBoundaryState != newState)
			{
				m_sCurrentActiveBoundary = boundaryName;
				m_iCurrentBoundaryState = newState;
				Replication.BumpMe();
			}
		}
		
		if (m_bDebugEnabled) 
			Print(string.Format("[CRF_BattleRoyaleComponent] Boundary '%1' visual state: %2", boundaryName, stateString));
	}
	
	//------------------------------------------------------------------------------------------------
	// Winner Detection System
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Check for winning team in Battle Royale, works for any group size
	 * Detects when only ONE group has any alive members, then lists ALL members of that group as winners
	 * Uses SAFE iteration pattern to prevent crashes during player disconnect
	 */
	void CheckForWinners()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
		{
			if (m_bDebugEnabled)
				Print("[CRF_BattleRoyale] CheckForWinners: SlottingManager not found!");
			return;
		}
		
		// SAFE ITERATION: Copy slot IDs first
		m_aCachedSlotIds.Clear();
		array<int> tempIds = slottingManager.GetAllSlotIds();
		if (!tempIds || tempIds.Count() == 0)
			return;
		
		m_aCachedSlotIds.Copy(tempIds);
		tempIds = null;
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] === CheckForWinners called - Total slots: %1 ===", m_aCachedSlotIds.Count()));
		
		m_mGroupPlayers.Clear();
		
		RplId aliveGroupId = RplId.Invalid();
		int aliveGroupCount = 0;
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		foreach (int slotId : m_aCachedSlotIds)
		{
			CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
			if (!slotData)
				continue;
			
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId <= 0)
				continue;
			
			if (!pm.IsPlayerConnected(playerId))
				continue;
			
			RplId groupId = slotData.GetSlotCurrentGroup();
			if (groupId == RplId.Invalid())
				continue;
			
			array<int> groupPlayers;
			if (!m_mGroupPlayers.Find(groupId, groupPlayers))
			{
				groupPlayers = new array<int>();
				m_mGroupPlayers.Insert(groupId, groupPlayers);
			}
			groupPlayers.Insert(playerId);
			
			if (!slotData.GetIsDeadSlot())
			{
				if (aliveGroupId != groupId)
				{
					if (aliveGroupId == RplId.Invalid())
						aliveGroupId = groupId;
					else
						aliveGroupCount++;
				}
				if (aliveGroupCount == 0 && aliveGroupId == groupId)
					aliveGroupCount = 1;
			}
		}
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] Groups with alive players: %1", aliveGroupCount));
		
		if (aliveGroupCount != 1)
		{
			if (m_bDebugEnabled)
				Print(string.Format("[CRF_BattleRoyale] No winner yet - %1 teams with alive players", aliveGroupCount));
			return;
		}
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] WINNER DETECTED! Winning GroupID: %1", aliveGroupId));
		
		m_aWinnerPlayerIds.Clear();
		array<int> winningPlayers;
		if (m_mGroupPlayers.Find(aliveGroupId, winningPlayers))
			m_aWinnerPlayerIds.Copy(winningPlayers);
		
		if (m_bDebugEnabled)
			Print(string.Format("[CRF_BattleRoyale] Total winners: %1", m_aWinnerPlayerIds.Count()));
		
		if (m_aWinnerPlayerIds.Count() > 0)
			DeclareWinners();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Declare the winners and stop the game
	 * Uses RPC broadcast instead of RplProp array
	 */
	void DeclareWinners()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		if (m_bDebugEnabled)
			Print("[CRF_BattleRoyale] ===== DECLARING WINNERS =====");
		
		m_bWinnerCheckActive = false;
		m_bWinnersDeclared = true;
		
		// Broadcast winner IDs via RPC (safer than RplProp array)
		BroadcastWinnersToClients();
		
		// Build winner names string for notification
		string winnerNames = BuildWinnerNamesString();
		
		// Broadcast victory notification via RPC (works for spectators)
		Rpc(RpcDo_ShowVictoryNotification, winnerNames);
		RpcDo_ShowVictoryNotification(winnerNames);
		
		Rpc(PlayStageSound, "victory");
		
		StopBattleRoyale();
		
		if (m_bDebugEnabled)
			Print("[CRF_BattleRoyale] Winner data broadcast via RPC");
	}
	
	//------------------------------------------------------------------------------------------------
	string BuildWinnerNamesString()
	{
		string names = "";
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return "Unknown";
		
		for (int i = 0; i < m_aWinnerPlayerIds.Count(); i++)
		{
			int playerId = m_aWinnerPlayerIds[i];
			string playerName = pm.GetPlayerName(playerId);
			if (playerName.IsEmpty())
				playerName = "Player " + playerId.ToString();
			
			if (i > 0)
				names += ", ";
			names += playerName;
		}
		
		if (names.IsEmpty())
			names = "Unknown";
		
		return names;
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ShowVictoryNotification(string winnerNames)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (popup)
			popup.PopupMsg("VICTORY!", 15, "Winners: " + winnerNames);
	}
	
	//------------------------------------------------------------------------------------------------
	void BroadcastWinnersToClients()
	{
		m_aCachedWinnerBuffer.Clear();
		for (int i = 0; i < 8; i++)
		{
			if (i < m_aWinnerPlayerIds.Count())
				m_aCachedWinnerBuffer.Insert(m_aWinnerPlayerIds[i]);
			else
				m_aCachedWinnerBuffer.Insert(0);
		}
		
		Rpc(RpcDo_ReceiveWinners, 
			m_aCachedWinnerBuffer[0], m_aCachedWinnerBuffer[1], 
			m_aCachedWinnerBuffer[2], m_aCachedWinnerBuffer[3],
			m_aCachedWinnerBuffer[4], m_aCachedWinnerBuffer[5],
			m_aCachedWinnerBuffer[6], m_aCachedWinnerBuffer[7]);
		
		RpcDo_ReceiveWinners(
			m_aCachedWinnerBuffer[0], m_aCachedWinnerBuffer[1], 
			m_aCachedWinnerBuffer[2], m_aCachedWinnerBuffer[3],
			m_aCachedWinnerBuffer[4], m_aCachedWinnerBuffer[5],
			m_aCachedWinnerBuffer[6], m_aCachedWinnerBuffer[7]);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ReceiveWinners(int w1, int w2, int w3, int w4, int w5, int w6, int w7, int w8)
	{
		m_aWinnerPlayerIds.Clear();
		if (w1 > 0) m_aWinnerPlayerIds.Insert(w1);
		if (w2 > 0) m_aWinnerPlayerIds.Insert(w2);
		if (w3 > 0) m_aWinnerPlayerIds.Insert(w3);
		if (w4 > 0) m_aWinnerPlayerIds.Insert(w4);
		if (w5 > 0) m_aWinnerPlayerIds.Insert(w5);
		if (w6 > 0) m_aWinnerPlayerIds.Insert(w6);
		if (w7 > 0) m_aWinnerPlayerIds.Insert(w7);
		if (w8 > 0) m_aWinnerPlayerIds.Insert(w8);
	}
	
	//------------------------------------------------------------------------------------------------
	// Gamemode Stop Method
	//------------------------------------------------------------------------------------------------
	
	void StopBattleRoyale()
	{
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) 
				Print("[CRF_BattleRoyaleComponent] StopBattleRoyale called on client - ignoring");
			return;
		}
		
		if (m_bDebugEnabled) 
			Print("[CRF_BattleRoyaleComponent] Manually stopping battle royale system");
		
		// Stop timers and stage progression, but keep winner display if winners were declared
		countDownActive = false;
		m_bWinnerCheckActive = false;
		m_bUpdateStageTimer = false;
		m_sStageText = "";
		m_sTimerText = "";
		
		// Only deactivate BR and clear winners if no winners were declared
		// This preserves the victory screen when manually stopping after a win
		if (!m_bWinnersDeclared)
		{
			m_bBattleRoyaleActive = false;
			m_aWinnerPlayerIds.Clear();
		}
		
		Replication.BumpMe();
		
		GetGame().GetCallqueue().Remove(UpdateStageTimer);
		GetGame().GetCallqueue().Remove(MonitorSafestart);
		GetGame().GetCallqueue().Remove(StartStaging);
		GetGame().GetCallqueue().Remove(InitializeBattleRoyale);
		GetGame().GetCallqueue().Remove(PlayStageSound);
	}
}
