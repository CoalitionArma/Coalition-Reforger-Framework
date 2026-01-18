/****************************************************************************************
 * File Name:        CRF_MapStagingComponent
 * Author:           Trist
 * Date Created:     08/03/25
 * Description:      CRF Boundary staging system with refactored, centralized execution methods
 * Comments:        Dont be mad at me I am learning, sars.
 ***************************************************************************************/

/*
 * EXTERNAL API REFERENCE - Simplified to two main methods for outside scripts:
 * 
 * CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
 * 
 * // MAIN EXTERNAL METHODS (use these for all external calls):
 * staging.ExecuteStaging(int stageIndex, bool useTimer, bool chainToNext) - Universal staging execution method
 * staging.ExecuteStagingSequence(int startIndex) - Execute full sequence starting from specific stage
 * 
 * // UTILITY METHODS:
 * staging.BeginStaging()     - Start the full staging sequence from stage 0
 * staging.StopStaging()      - Emergency stop all staging activity and clear timers
 * 
 * Note: All stage indices are 0-based (first stage = 0, second stage = 1, etc.)

 Feel free to modify this or the layout/display, to turn it into something else, etc. Intention for this was to just provide a mini api for people scared
 of code to do a little bit more with boundryzones or just make it a bit faster along with getting rid of using markers.

 */

enum CRF_BoundaryStageType
{
	ACTIVATION = 0,
	DELETION = 1,
	DEACTIVATION = 2
}

enum CRF_BoundaryStageState
{
	INACTIVE = 0,	// Not yet triggered/chained
	ACTIVE = 1,		// Started, during timer
	ACTIVATED = 2	// Finished timer or completed
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sBoundaryEntityName"}, "%1")]
class CRF_BoundaryStageData
{
	[Attribute("", UIWidgets.Object, "Enter name of the GameBoundary entity", params: "et")]
	string m_sBoundaryEntityName;
	
	[Attribute("0", UIWidgets.ComboBox, "Stage behavior type\nACTIVATION: STARTS DISABLED, WILL ENABLE ON TRIGGER/TIMER\nDELETION: STARTS ENABLED, WILL DISABLE ON TRIGGER/TIMER\nDEACTIVATION: STARTS ENABLED, WILL DISABLE ON TRIGGER/TIMER\n", "", ParamEnumArray.FromEnum(CRF_BoundaryStageType))]
	CRF_BoundaryStageType m_eStageType;

	[Attribute("120", UIWidgets.EditBox, "Duration of this stage in seconds, enacts the stage ACTIVATION TYPE at the END of timer")]
	int m_iStageDuration;
	
	[Attribute("", UIWidgets.EditBox, "Text under Stage Timer (If timer for stage is enabled)")]
	string m_sStageDisplayText;
	
	[Attribute("true", UIWidgets.CheckBox, "Show completion message when this stage finishes", category: "Completion Message")]
	bool m_bShowStageCompletionMessage;
	
	[Attribute("Stage Complete", UIWidgets.EditBox, "Main completion message for this stage (leave empty for default)", category: "Completion Message")]
	string m_sStageCompletionMainMessage;
	
	[Attribute("Boundary updated", UIWidgets.EditBox, "Sub completion message for this stage (leave empty for default)", category: "Completion Message")]
	string m_sStageCompletionSubMessage;
	
	[Attribute("10", UIWidgets.EditBox, "Duration (seconds) to display stage completion message", category: "Completion Message")]
	int m_iStageCompletionDuration;
	
	// Visual state colors for boundary zones
	[Attribute("0.8 0.3 0 0.45", UIWidgets.ColorPicker, "ACTIVE state - Polygon fill color (only present when timer running, until activation type completes)")]
	ref Color m_cActivePolygonColor;
	
	[Attribute("0.045 0.045 0.045 1", UIWidgets.ColorPicker, "ACTIVE state - Polygon border color")]
	ref Color m_cActivePolygonBorderColor;
	
	[Attribute("0.5 0.1 0.1 0.45", UIWidgets.ColorPicker, "ACTIVATED state - Polygon fill color (visual state after activation type completes)")]
	ref Color m_cActivatedPolygonColor;
	
	[Attribute("0.045 0.045 0.045 1", UIWidgets.ColorPicker, "ACTIVATED state - Polygon border color")]
	ref Color m_cActivatedPolygonBorderColor;
}

[ComponentEditorProps(category: "Game Mode Component", description: "Map Staging System")]
class CRF_MapStagingComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_MapStagingComponent : SCR_BaseGameModeComponent
{
	[Attribute("30", UIWidgets.EditBox, "Initial delay of auto starting/staging the first stage after safestart ends (seconds)", category: "Base Configuration")]
	int m_iInitialDelay;
	
	[Attribute("true", UIWidgets.CheckBox, "Auto-start staging sequence after safestart ends + initial delay (if false, staging only triggers via manual scripting)", category: "Base Configuration")]
	bool m_bEnableAutoStart;
	
	// Global audio settings for all stages
	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourceNamePicker, "Audio to play when any stage timer starts", "wav", category: "Base Configuration")]
	ResourceName m_sStageStartSound;
	
	[Attribute("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav", UIWidgets.ResourceNamePicker, "Audio to play when any stage completes", "wav", category: "Base Configuration")]
	ResourceName m_sStageEndSound;
	
	[Attribute("false", UIWidgets.CheckBox, "Enable debug logging", category: "Base Configuration")]
	bool m_bDebugEnabled;
	
	[Attribute("All Stages Complete", UIWidgets.EditBox, "Main message shown when all stages are completed(blank is valid)", category: "Base Configuration")]
	string m_sFinalCompletionMainMessage;
	
	[Attribute("Final boundaries active", UIWidgets.EditBox, "Sub message shown when all stages are completed(blank is valid)", category: "Base Configuration")]
	string m_sFinalCompletionSubMessage;
	
	[Attribute("15", UIWidgets.EditBox, "Duration (seconds) to display final completion message", category: "Base Configuration")]
	int m_iFinalCompletionMessageDuration;
	
	[Attribute("", UIWidgets.Auto, "List of boundary stages", category: "Stage Configuration")]
	ref array<ref CRF_BoundaryStageData> m_aBoundaryStages;
	
 	[Attribute("STAGING DOCUMENTATION\n\n=== Stage Execution ===\nGet Line:\n\nCRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));\n\nCall Types:\n\nstaging.ExecuteStaging(stageIndex, useTimer, chainToNext)\n• stageIndex: Stage to execute (your first non main-gameboundry in the list is index0)\n• useTimer: true = countdown timer, false = immediate\n• chainToNext: true = auto progress to next stages, false = single stage only\n\nstaging.ExecuteStagingSequence(startIndex)\n• Start full sequence from specified stage\n\n=== USAGE EXAMPLES ===\n\n// Execute stage 3 in list immediately without timer, no chaining after execution\nstaging.ExecuteStaging(2, false, false);\n\n// Execute stage 2 in list with timer, chain to next stage in list after execution\nstaging.ExecuteStaging(1, true, true);\n\n// Execute stage 1 in list with stage timer but no chaining to next stage in list\nstaging.ExecuteStaging(0, true, false);\n\n// Start full sequence from stage 3 in list\nstaging.ExecuteStagingSequence(2);\n\n=== SETUP ===\n\n- Place & rename Gameboundry prefabs\n- Position them where you want the final boundary areas\n- Be sure to edit faction keys within the boundrys' CRF_Polyzonetriggers based on what factions youre excluding and on your activation type\n- Enable debug if problems arise\n- Only for Non-Reversed gameboundries/crf_polyzones\n\n=== STAGE TYPES ===\nACTIVATION/DEACTIVATION: Boundary either is removed or placed at the END of the timer/manual trigger\nDELETION: Boundary exists at normal position, gets deleted when timer completes", UIWidgets.EditBoxMultiline, "Staging Setup Instructions & API Examples", category: "Documentation")]
	string m_sInstructions;
	
	// Public state for display
	[RplProp()]
	bool countDownActive = false;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent = "";
	
	[RplProp()]
	string m_sTimerText = ""; // Dedicated property for timer display
	
	[RplProp()]
	string m_sSoundToPlay = "";
	
	[RplProp()]
	string m_sStageText = "";
	
	// Internal states for jippers
	[RplProp()]
	protected int m_iCurrentStage = 0;
	
	[RplProp()]
	int m_iCurrentTimer = 0; // Made public
	
	[RplProp()]
	protected bool m_bStagingActive = false;
	protected bool m_bChainToNext = true; // Track chaining mode for current execution
	protected ref map<string, vector> m_mOriginalPositions;
	protected ref map<string, CRF_BoundaryStageState> m_mStageStates; // Track state of each boundary
	protected string m_sStoredMessageContent;
	protected SCR_PopUpNotification m_PopUpNotification;
	
	// rpc for jippers
	[RplProp()]
	protected ref array<string> m_aReplicatedBoundaryNames = {};
	
	[RplProp(onRplName: "OnBoundaryStatesChanged")]
	protected ref array<int> m_aReplicatedBoundaryStates = {};
	
	// Performance safeguards
	protected int m_iInitRetryCount = 0;
	protected static const int MAX_INIT_RETRIES = 3;
	
	// Performance~ cache stage data lookups
	protected ref map<string, ref CRF_BoundaryStageData> m_mStageDataCache;
	
	// Performance: cache entity references to avoid repeated FindEntityByName calls
	protected ref map<string, IEntity> m_mBoundaryEntityCache;
	
	protected bool m_bUpdateStageTimer = false;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Initialize cache maps on ALL clients and server (needed for RPC calls)
		m_mOriginalPositions = new map<string, vector>();
		m_mStageStates = new map<string, CRF_BoundaryStageState>();
		m_mStageDataCache = new map<string, ref CRF_BoundaryStageData>();
		m_mBoundaryEntityCache = new map<string, IEntity>();
		if (!m_aBoundaryStages)
			m_aBoundaryStages = new array<ref CRF_BoundaryStageData>();
		
		// Initialize replicated arrays
		m_aReplicatedBoundaryNames = new array<string>();
		m_aReplicatedBoundaryStates = new array<int>();
		
		// Server-only initialization
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
		// Delay initialization to ensure world is fully loaded
		GetGame().GetCallqueue().CallLater(InitializeBoundaries, 3000, false);
		// Start monitoring safestart (single call, not repeating)
		GetGame().GetCallqueue().CallLater(MonitorSafestart, 5000, false);
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bUpdateStageTimer)
				UpdateStageTimer();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
	}
	
	//-----------------------------------------------------------------------------------------------
	void InitializeBoundaries()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Initializing boundaries...");
		
		if (!m_aBoundaryStages || m_aBoundaryStages.Count() == 0)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] WARNING: No boundary stages configured!");
			return;
		}
		
		bool foundMissingEntity = false;
		
		for (int i = 0; i < m_aBoundaryStages.Count(); i++)
		{
			CRF_BoundaryStageData stageData = m_aBoundaryStages[i];
			if (!stageData || !stageData.m_sBoundaryEntityName || stageData.m_sBoundaryEntityName.IsEmpty())
			{
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] WARNING: Invalid stage data at index %1", i));
				continue;
			}
			
			// Try to find boundary entity (use cached lookup for performance)
			IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sBoundaryEntityName);
			if (!boundaryEntity)
			{
				Print(string.Format("[CRF_MapStagingComponent] ERROR: Boundary '%1' not found! Check entity name in world.", stageData.m_sBoundaryEntityName));
				foundMissingEntity = true;
				continue; // Continue processing other entities instead of immediate return
			}
			
			vector originalPos = boundaryEntity.GetOrigin();
			m_mOriginalPositions.Set(stageData.m_sBoundaryEntityName, originalPos);
			
			// Initialize stage state as INACTIVE but don't set initial colors
			m_mStageStates.Set(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.INACTIVE);
			
			// Add to replicated arrays for late-joining players
			m_aReplicatedBoundaryNames.Insert(stageData.m_sBoundaryEntityName);
			m_aReplicatedBoundaryStates.Insert(CRF_BoundaryStageState.INACTIVE);
			
			// Cache stage data for performance optimization
			m_mStageDataCache.Set(stageData.m_sBoundaryEntityName, stageData);
			
			// Position boundaries for ACTIVATION/DEACTIVATION types (move them away from their final position initially)
			if (stageData.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
			{
				// ACTIVATION boundaries start moved away and get moved back to original position when activated
				vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
				boundaryEntity.SetOrigin(newPos);
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ACTIVATION boundary '%1' moved away initially", stageData.m_sBoundaryEntityName));
			}
			else if (stageData.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
			{
				// DEACTIVATION boundaries start at their normal position and get moved away when deactivated
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] DEACTIVATION boundary '%1' starts at normal position", stageData.m_sBoundaryEntityName));
			}
			
			// Debug logging for boundary initialization
			if (m_bDebugEnabled)
			{
				if (stageData.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
					Print(string.Format("[CRF_MapStagingComponent] ACTIVATION boundary '%1' ready (moved away initially)", stageData.m_sBoundaryEntityName));
				else if (stageData.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
					Print(string.Format("[CRF_MapStagingComponent] DEACTIVATION boundary '%1' ready (at normal position)", stageData.m_sBoundaryEntityName));
				else
					Print(string.Format("[CRF_MapStagingComponent] DELETION boundary '%1' ready for removal", stageData.m_sBoundaryEntityName));
			}
		}
		
		// Handle missing entities with retry logic (performance safeguard)
		if (foundMissingEntity && m_iInitRetryCount < MAX_INIT_RETRIES)
		{
			m_iInitRetryCount++;
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Retrying boundary initialization in 5 seconds (attempt %1/%2)", m_iInitRetryCount, MAX_INIT_RETRIES));
			GetGame().GetCallqueue().CallLater(InitializeBoundaries, 5000, false);
			return;
		}
		else if (foundMissingEntity)
		{
			Print("[CRF_MapStagingComponent] ERROR: Maximum initialization retries exceeded! Some boundaries will not function properly.");
		}
		
		// Replicate the initial boundary states to all clients
		Replication.BumpMe();
		
		int successfullyInitialized = m_aReplicatedBoundaryNames.Count();
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Successfully initialized %1/%2 boundary stages", successfullyInitialized, m_aBoundaryStages.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Fast cached entity lookup to avoid expensive FindEntityByName calls
	 * @param entityName - Name of the entity to find
	 * @return IEntity - The found entity or null
	 */
	IEntity GetCachedBoundaryEntity(string entityName)
	{
		// Null/empty check
		if (!entityName || entityName.IsEmpty())
			return null;
		
		// Keep cache initialized
		if (!m_mBoundaryEntityCache)
		{
			m_mBoundaryEntityCache = new map<string, IEntity>();
		}
		
		// Check cache first
		IEntity cachedEntity;
		if (m_mBoundaryEntityCache.Find(entityName, cachedEntity))
		{
			// Verify entity still exists
			if (cachedEntity)
				return cachedEntity;
			else
			{
				// Clean up invalid cache entry
				m_mBoundaryEntityCache.Remove(entityName);
			}
		}
		
		// big dick scary get when getting cached entities fails, me practicing optimization lul
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (entity)
		{
			m_mBoundaryEntityCache.Set(entityName, entity);
		}
		
		return entity;
	}
	
	//------------------------------------------------------------------------------------------------
	void MonitorSafestart()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		// No auto staging if auto start is disabled
		if (!m_bEnableAutoStart)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Auto-start disabled - waiting for manual script trigger");
			GetGame().GetCallqueue().Remove(MonitorSafestart);
			return;
		}
		
		// Safestart check
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart || safestart.GetSafestartStatus() || CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Waiting for game to be ready and safestart to end...");
			GetGame().GetCallqueue().CallLater(MonitorSafestart, 15000, false);
			return;
		}
		
		// Safestart has ended and game is live
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Game is live, starting staging in %1 seconds", m_iInitialDelay));
		GetGame().GetCallqueue().CallLater(StartStaging, m_iInitialDelay * 1000, false);
		m_bStagingActive = true;
		m_bChainToNext = true; // Auto staging always chains
		Replication.BumpMe(); // Replicate staging activation
	}
	
	//------------------------------------------------------------------------------------------------
	void StartStaging()
	{
		// rpl to prevent client exec
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!m_aBoundaryStages || m_iCurrentStage >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] StartStaging blocked - no valid stage");
			return;
		}
		
		// For manual calls, m_bStagingActive might not be set yet, so check if we have a valid current stage - Mid stage firing of StartStaging protection
		if (!m_bStagingActive && m_iCurrentStage == 0)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] StartStaging blocked - staging not active and at stage 0");
			return;
		}
		
		CRF_BoundaryStageData currentStage = m_aBoundaryStages[m_iCurrentStage];
		if (!currentStage)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ERROR: Stage %1 data is null!", m_iCurrentStage + 1));
			return;
		}
		
		// Clear any existing timer before starting new one
		if (countDownActive)
		{
			m_bUpdateStageTimer = false;
			countDownActive = false;
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Cleared existing timer in StartStaging");
		}
		
		m_iCurrentTimer = currentStage.m_iStageDuration;
		m_sTimerText = SCR_FormatHelper.FormatTime(m_iCurrentTimer); // Set initial display text
		countDownActive = true;
		m_sStageText = currentStage.m_sStageDisplayText; // Set stage text for HUD display
		
		// Update boundary visual state to ACTIVE (timer running) - both RPC and local for Workbench compatibility
		Rpc(UpdateBoundaryVisualState, currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
		UpdateBoundaryVisualStateLocal(currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
		
		// Play stage start sound using centralized system with 1 second delay
		GetGame().GetCallqueue().CallLater(PlayStageSound, 1000, false, "start");
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Starting stage %1: '%2' (%3s)", 
			m_iCurrentStage + 1, m_sStageText, m_iCurrentTimer));
		
		Replication.BumpMe();
		m_bUpdateStageTimer = true;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Simplified timer update method - handles countdown only
	 */
	void UpdateStageTimer()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!countDownActive)
		{
			m_bUpdateStageTimer = false;
			return;
		}

		m_sTimerText = SCR_FormatHelper.FormatTime(m_iCurrentTimer);
		Replication.BumpMe();

		if (m_bDebugEnabled && (m_iCurrentTimer % 10 == 0 || m_iCurrentTimer <= 10))
		{
			Print(string.Format("[CRF_MapStagingComponent] Stage %1 timer: %2s remaining", m_iCurrentStage + 1, m_iCurrentTimer));
		}

		if (m_iCurrentTimer <= 0)
		{
			m_bUpdateStageTimer = false;
			countDownActive = false;
			m_sTimerText = ""; // Clear timer text when stage completes
			Replication.BumpMe();
			
			// Timer completed - let ExecuteStage handle the progression logic
			ExecuteStage(m_bChainToNext); // Use stored chaining mode
			return;
		}

		m_iCurrentTimer--;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Enhanced stage execution method - handles all execution logic based on parameters
	 * @param isChainedExecution - Whether this should chain to next stages (true for sequences, false for single stages)
	 * @param fromTimer - Whether this execution came from a timer completion (true) or immediate call (false)
	 */
	void ExecuteStage(bool isChainedExecution = true, bool fromTimer = true)
	{
		// Server-only operation for mp
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!m_aBoundaryStages || m_iCurrentStage >= m_aBoundaryStages.Count())
			return;
		
		CRF_BoundaryStageData currentStage = m_aBoundaryStages[m_iCurrentStage];
		if (!currentStage)
			return;
		
		// Handle immediate execution setup (for non-timer calls)
		if (!fromTimer)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Executing stage %1 immediately (no timer)", m_iCurrentStage + 1));
			
			// Play stage start sound for immediate execution with 1 second delay
			GetGame().GetCallqueue().CallLater(PlayStageSound, 1000, false, "start");
			
			// Set stage text for display
			m_sStageText = currentStage.m_sStageDisplayText;
			
			// Update boundary visual state to ACTIVE briefly, then execute
			Rpc(UpdateBoundaryVisualState, currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
			UpdateBoundaryVisualStateLocal(currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
			Replication.BumpMe();
			
			// Delay execution by 1 second to allow start sound to play properly
			GetGame().GetCallqueue().CallLater(ApplyBoundaryAction, 1000, false, currentStage, m_iCurrentStage, isChainedExecution);
			GetGame().GetCallqueue().CallLater(StageProgressHandler, 1500, false, isChainedExecution); // Handle progression after boundary action
			return;
		}
		
		// Timer-based execution (existing logic)
		ApplyBoundaryAction(currentStage, m_iCurrentStage, isChainedExecution);
		StageProgressHandler(isChainedExecution);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Handle stage progression logic - separated from ExecuteStage for cleaner flow
	 * @param isChainedExecution - Whether this should chain to next stages
	 */
	void StageProgressHandler(bool isChainedExecution)
	{
		// Handle progression based on chaining mode
		if (isChainedExecution)
		{
			// Chained execution - move to next stage or finish sequence
			m_iCurrentStage++;
			Replication.BumpMe(); // Replicate stage progression
			
			if (m_aBoundaryStages && m_iCurrentStage < m_aBoundaryStages.Count())
			{
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Moving to stage %1 in 2s", m_iCurrentStage + 1));
				GetGame().GetCallqueue().CallLater(StartStaging, 2000, false);
			}
			else
			{
				if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] All stages completed! Finalizing in 3 seconds...");
				
				// Delay finalization to allow final stage end sound to play
				GetGame().GetCallqueue().CallLater(FinalizeStagingSystem, 3000, false);
			}
		}
		else
		{
			// Single stage execution - clear staging state and stop
			m_sStageText = ""; // Clear stage text for non-chained execution
			m_bStagingActive = false; // Clear staging active state since we're not chaining
			Replication.BumpMe();
			
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Single stage %1 execution completed", m_iCurrentStage + 1));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Finalize staging system after delay (allows final sound to play)
	 */
	void FinalizeStagingSystem()
	{
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Finalizing staging system");
		
		m_bStagingActive = false;
		m_sStageText = ""; // Clear stage text when finished
		m_sTimerText = ""; // Clear timer text when finished
		
		// Use customizable final completion message
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
			return;

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Popup: '%1' (%2s) - '%3'", mainMessage, time, subMessage));
		m_PopUpNotification.PopupMsg(mainMessage, time.ToFloat(), subMessage);
	}
	
	//-----------------------------------------------------------------------------------------------
	/**
	 * Simple sound system - handles all staging sound effects
	 * @param soundType - "start" or "end" to determine which sound to play
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void PlayStageSound(string soundType)
	{
		ResourceName soundToPlay;
		
		if (soundType == "start" && !m_sStageStartSound.IsEmpty())
			soundToPlay = m_sStageStartSound;
		else if (soundType == "end" && !m_sStageEndSound.IsEmpty())
			soundToPlay = m_sStageEndSound;
		
		if (soundToPlay.IsEmpty())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] PlayStageSound: No sound configured for type '%1'", soundType));
			return;
		}
		
		// Server: Send to all clients via RPC
		if (RplSession.Mode() != RplMode.Client)
		{
			Rpc(PlayStageSound, soundType);
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Playing %1 sound: %2", soundType, soundToPlay));
		}
	
		// Both server and clients: Play locally
		m_sSoundToPlay = soundToPlay;
		Replication.BumpMe();
		AudioSystem.PlaySound(soundToPlay);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * RPC method to update boundary colors on all clients (including server)
	 * @param boundaryName - Name of the boundary entity to update
	 * @param newState - The new visual state to apply
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void UpdateBoundaryVisualState(string boundaryName, CRF_BoundaryStageState newState)
	{
		// Call the local implementation
		UpdateBoundaryVisualStateLocal(boundaryName, newState);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Local implementation of boundary visual state updates (works in both dedicated server and Workbench)
	 * @param boundaryName - Name of the boundary entity to update
	 * @param newState - The new visual state to apply
	 */
	void UpdateBoundaryVisualStateLocal(string boundaryName, CRF_BoundaryStageState newState)
	{
		// Safety checks
		if (!boundaryName || boundaryName.IsEmpty())
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] UpdateBoundaryVisualStateLocal: Invalid boundary name");
			return;
		}
		
		// Find the boundary entity (use cached lookup for performance)
		IEntity boundaryEntity = GetCachedBoundaryEntity(boundaryName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualStateLocal: Boundary '%1' not found", boundaryName));
			return;
		}
		
		// Find the corresponding stage data for color information (use cache for performance)
		CRF_BoundaryStageData stageData = null;
		if (m_mStageDataCache)
		{
			m_mStageDataCache.Find(boundaryName, stageData);
		}
		
		// Fallback to linear search if cache missed or not initialized
		if (!stageData && m_aBoundaryStages)
		{
			for (int i = 0; i < m_aBoundaryStages.Count(); i++)
			{
				if (m_aBoundaryStages[i] && m_aBoundaryStages[i].m_sBoundaryEntityName == boundaryName)
				{
					stageData = m_aBoundaryStages[i];
					break;
				}
			}
		}
		
		if (!stageData)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualStateLocal: Stage data not found for boundary '%1'", boundaryName));
			return;
		}
		
		// Get the CRF_PolyZone component from the boundary entity
		CRF_PolyZone polyZone = CRF_PolyZone.Cast(boundaryEntity.FindComponent(CRF_PolyZone));
		if (!polyZone)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualStateLocal: CRF_PolyZone component not found on '%1'", boundaryName));
			return;
		}
		
		// Update colors based on the new state (skip INACTIVE state)
		Color fillColor, borderColor;
		string stateString;
		
		switch (newState)
		{
			case CRF_BoundaryStageState.INACTIVE:
				// No color changes for INACTIVE state
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Boundary '%1' visual state updated to: INACTIVE (no color change)", boundaryName));
				return;
				
			case CRF_BoundaryStageState.ACTIVE:
				fillColor = stageData.m_cActivePolygonColor;
				borderColor = stageData.m_cActivePolygonBorderColor;
				stateString = "ACTIVE";
				break;
				
			case CRF_BoundaryStageState.ACTIVATED:
				fillColor = stageData.m_cActivatedPolygonColor;
				borderColor = stageData.m_cActivatedPolygonBorderColor;
				stateString = "ACTIVATED";
				break;
		}
		
		// Apply the colors to the poly zone
		if (fillColor)
		{
			polyZone.m_cPolygonColor = fillColor;
		}
		if (borderColor)
		{
			polyZone.m_cPolygonBorderColor = borderColor;
		}
		
		// Update existing widgets if active
		if (polyZone.m_MapEntity && polyZone.m_wCanvasWidget)
		{
			MapConfiguration mapConfig = polyZone.m_MapEntity.GetMapConfig();
			if (mapConfig)
			{
				// Real-time update: delete and recreate widget with new colors
				polyZone.DeleteMapWidget(mapConfig);
				polyZone.CreateMapWidget(mapConfig);
				
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Widget updated in real-time for '%1' with new colors", boundaryName));
			}
		}
		else if (m_bDebugEnabled)
		{
			Print(string.Format("[CRF_MapStagingComponent] Colors applied to '%1' - widget will update when map is opened", boundaryName));
		}
		
		// Update the stored state - only server
		if (Replication.IsServer())
		{
			m_mStageStates.Set(boundaryName, newState);
			
			// Update replicated arrays for late-joining players
			int boundaryIndex = m_aReplicatedBoundaryNames.Find(boundaryName);
			if (boundaryIndex != -1)
			{
				m_aReplicatedBoundaryStates[boundaryIndex] = newState;
				Replication.BumpMe(); // Notify replication system of change
			}
		}
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Boundary '%1' visual state updated to: %2", boundaryName, stateString));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * JIP handling
	 */
	void OnBoundaryStatesChanged()
	{
		// Skip on server - server already has the correct states
		if (Replication.IsServer())
			return;
		
		// Safety checks for replicated arrays
		if (!m_aReplicatedBoundaryNames || !m_aReplicatedBoundaryStates)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] OnBoundaryStatesChanged: Replicated arrays not initialized");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Applying replicated boundary states for late-joining client");
		
		// Apply all replicated boundary states
		for (int i = 0; i < m_aReplicatedBoundaryNames.Count() && i < m_aReplicatedBoundaryStates.Count(); i++)
		{
			string boundaryName = m_aReplicatedBoundaryNames[i];
			CRF_BoundaryStageState state = m_aReplicatedBoundaryStates[i];
			
			// Skip empty/null boundary names
			if (!boundaryName || boundaryName.IsEmpty())
				continue;
			
			// Update the visual state without sending RPC (we're receiving one) - use local method
			UpdateBoundaryVisualStateLocal(boundaryName, state);
		}
	}
	
	//-------------------------------------------------------------------------------------------------
	/**
	 * Centralized boundary action execution with integrated sound and completion handling
	 * @param stageData - The stage data to execute
	 * @param stageIndex - Index of the stage (for completion message)
	 * @param isChainedExecution - Whether this is part of a sequence (affects completion message and progression)
	 * @return string - The stage type string for completion message
	 */
	string ApplyBoundaryAction(CRF_BoundaryStageData stageData, int stageIndex, bool isChainedExecution = false)
	{
		if (!stageData)
			return "";
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sBoundaryEntityName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ApplyBoundaryAction failed - boundary entity '%1' not found", stageData.m_sBoundaryEntityName));
			return "";
		}

		string stageTypeString;

		// Execute the boundary action
		if (stageData.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
		{
			vector originalPos;
			if (m_mOriginalPositions.Find(stageData.m_sBoundaryEntityName, originalPos))
			{
				boundaryEntity.SetOrigin(originalPos);
				stageTypeString = "ACTIVATED";
				string logPrefix;
				if (isChainedExecution)
					logPrefix = "";
				else
					logPrefix = "Single execution: ";
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1ACTIVATION boundary '%2' activated", logPrefix, stageData.m_sBoundaryEntityName));
			}
		}
		else if (stageData.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
		{
			vector originalPos;
			if (m_mOriginalPositions.Find(stageData.m_sBoundaryEntityName, originalPos))
			{
				vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
				boundaryEntity.SetOrigin(newPos);
				stageTypeString = "DEACTIVATED";
				string logPrefix;
				if (isChainedExecution)
					logPrefix = "";
				else
					logPrefix = "Single execution: ";
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1DEACTIVATION boundary '%2' moved away and deactivated", logPrefix, stageData.m_sBoundaryEntityName));
			}
		}
		else
		{
			SCR_EntityHelper.DeleteEntityAndChildren(boundaryEntity);
			stageTypeString = "DELETED";
			string logPrefix;
			if (isChainedExecution)
				logPrefix = "";
			else
				logPrefix = "Single execution: ";
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1DELETION boundary '%2' removed", logPrefix, stageData.m_sBoundaryEntityName));
		}

		// Update boundary visual state to ACTIVATED - both RPC and local for Workbench compatibility
		Rpc(UpdateBoundaryVisualState, stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVATED);
		UpdateBoundaryVisualStateLocal(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVATED);

		// Play stage end sound using centralized system
		PlayStageSound("end");

		// Show completion popup with custom message (if enabled)
		if (stageData.m_bShowStageCompletionMessage)
		{
			string completionMessage;
			if (!stageData.m_sStageCompletionMainMessage.IsEmpty())
			{
				// Use custom stage completion message with full format support
				string subMessage;
				if (stageData.m_sStageCompletionSubMessage.IsEmpty())
					subMessage = string.Format("Play area %1", stageTypeString);
				else
					subMessage = stageData.m_sStageCompletionSubMessage;
					
				completionMessage = string.Format("%1|%2|%3", 
					stageData.m_sStageCompletionMainMessage, 
					stageData.m_iStageCompletionDuration, 
					subMessage);
			}
			else
			{
				// Use default format
				completionMessage = string.Format("Stage %1 Complete|10|Play area %2", stageIndex + 1, stageTypeString);
			}

			m_sMessageContent = completionMessage;
			Replication.BumpMe();
			ShowMessage();
		}
		else if (m_bDebugEnabled)
		{
			Print(string.Format("[CRF_MapStagingComponent] Stage %1 completion message suppressed (toggle disabled)", stageIndex + 1));
		}

		return stageTypeString;
	}

	//-------------------------------------------------------------------------------------------------
	// EXTERNAL SCRIPT ACCESS METHODS
	// These methods allow other scripts to manually control the staging system
	//-------------------------------------------------------------------------------------------------
	
	//-------------------------------------------------------------------------------------------------
	// REFACTORED EXTERNAL API - Two main methods for all external staging control
	//-------------------------------------------------------------------------------------------------
	
	/**
	 * Universal staging execution method - external API for parameter handling
	 * 
	 * @param stageIndex - Index of the stage to execute (0-based)
	 * @param useTimer - Whether to use a countdown timer before execution
	 * @param chainToNext - Whether to automatically chain to next stages after completion
	 * @return bool - true if stage was triggered successfully
	 * 
	 * USAGE EXAMPLES:
	 * 
	 * // Execute stage immediately without timer, no chaining (old TriggerStageChainless)
	 * staging.ExecuteStaging(2, false, false);
	 * 
	 * // Execute stage with timer, chain to next stages (old TriggerStage)
	 * staging.ExecuteStaging(1, true, true);
	 * 
	 * // Execute stage with timer but no chaining (old TriggerStageTimedNoChain)
	 * staging.ExecuteStaging(0, true, false);
	 */
	bool ExecuteStaging(int stageIndex, bool useTimer = true, bool chainToNext = true)
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] ExecuteStaging called on client - ignoring");
			return false;
		}
		
		// Validation - keep existing validation logic
		if (!m_aBoundaryStages || stageIndex < 0 || stageIndex >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ExecuteStaging failed - invalid stage index %1", stageIndex));
			return false;
		}
		
		CRF_BoundaryStageData stageData = m_aBoundaryStages[stageIndex];
		if (!stageData)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ExecuteStaging failed - stage data is null at index %1", stageIndex));
			return false;
		}
		
		// Set up execution context - pass parameters to ExecuteStage
		m_iCurrentStage = stageIndex;
		m_bChainToNext = chainToNext;
		m_bStagingActive = true;
		
		if (m_bDebugEnabled) 
		{
			string timerText = "immediate";
			if (useTimer) timerText = "with timer";
			string chainText = "no chain";
			if (chainToNext) chainText = "chaining";
			Print(string.Format("[CRF_MapStagingComponent] ExecuteStaging: stage %1 (%2, %3)", stageIndex + 1, timerText, chainText));
		}
		
		// Pass execution to ExecuteStage with parameters
		if (useTimer)
		{
			// Timer-based execution - let StartStaging handle the timer, then ExecuteStage handles logic
			StartStaging();
		}
		else
		{
			// Immediate execution - let ExecuteStage handle all logic
			ExecuteStage(chainToNext, false); // false = immediate execution
		}
		
		return true;
	}
	
	/**
	 * Execute a full staging sequence starting from a specific stage
	 * 
	 * @param startIndex - Index of the stage to start the sequence from (0-based)
	 * @return bool - true if sequence was started successfully
	 * 
	 * USAGE EXAMPLES:
	 * 
	 * // Start full sequence from beginning
	 * staging.ExecuteStagingSequence(0);
	 * 
	 * // Start sequence from stage 3
	 * staging.ExecuteStagingSequence(2);
	 */
	bool ExecuteStagingSequence(int startIndex = 0)
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] ExecuteStagingSequence called on client - ignoring");
			return false;
		}
		
		if (!m_aBoundaryStages || startIndex < 0 || startIndex >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ExecuteStagingSequence failed - invalid start index %1", startIndex));
			return false;
		}
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Starting staging sequence from stage %1", startIndex + 1));
		
		m_iCurrentStage = startIndex;
		m_bStagingActive = true;
		m_bChainToNext = true; // Sequences always chain
		Replication.BumpMe();
		StartStaging();
		
		return true;
	}
	
	/**
	 * Begin the staging system manually
	 * 
	 * Bypasses safestart monitoring and immediately begins the first stage.
	 * Useful for custom game mode initialization or testing.
	 * 
	 * USAGE EXAMPLE:
	 * 
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.BeginStaging();
	 */
	void BeginStaging()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] BeginStaging called on client - ignoring");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Manually starting staging system");
		
		// Use the new unified method to start the sequence
		ExecuteStagingSequence(0);
	}
	
	/**
	 * Stop the staging system and clear all timers
	 * 
	 * Emergency stop function that halts all staging activity,
	 * clears active timers, and resets the system state.
	 * 
	 * USAGE EXAMPLE:
	 * 
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.StopStaging();
	 */
	void StopStaging()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] StopStaging called on client - ignoring");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Manually stopping staging system");
		m_bStagingActive = false;
		countDownActive = false;
		m_sStageText = "";
		m_sTimerText = ""; // Clear timer display
		Replication.BumpMe(); // Replicate staging stop
		GetGame().GetCallqueue().Remove(UpdateStageTimer);
		GetGame().GetCallqueue().Remove(MonitorSafestart);
		GetGame().GetCallqueue().Remove(StartStaging); // Prevent pending StartStaging calls
		GetGame().GetCallqueue().Remove(InitializeBoundaries); // Prevent pending initialization
		GetGame().GetCallqueue().Remove(PlayStageSound); // Clear any pending sound calls
	}
}
