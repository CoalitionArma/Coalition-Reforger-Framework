//------------------------------------------------------------------------------------------------
//! CRF Mission Validator Component
//! Validates mission setup and warns mission makers about missing or misconfigured entities
//! Add this to your gamemode entity to get real-time validation feedback
//------------------------------------------------------------------------------------------------

class CRF_MissionValidatorComponentClass : ScriptComponentClass {}

class CRF_MissionValidatorComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------
	// ATTRIBUTES
	//------------------------------------------------------------------------------------
	
	[Attribute(defvalue: "1", desc: "Enable validation on mission start?")]
	bool m_bEnableValidation;
	
	[Attribute(defvalue: "1", desc: "Show warnings in console?")]
	bool m_bShowWarnings;
	
	[Attribute(defvalue: "1", desc: "Show info messages in console?")]
	bool m_bShowInfo;
	
	[Attribute(defvalue: "0", desc: "Block mission start if critical errors found?")]
	bool m_bBlockOnCriticalErrors;
	
	[Attribute(defvalue: "5.0", desc: "Validation delay after mission start (seconds)", params: "0 30 0.1")]
	float m_fValidationDelay;
	
	[Attribute(defvalue: "1", desc: "Show UI popup in Workbench mode?")]
	bool m_bShowWorkbenchUI;
	
	//------------------------------------------------------------------------------------
	// VALIDATION RESULTS
	//------------------------------------------------------------------------------------
	
	protected ref array<string> m_aCriticalErrors = {};
	protected ref array<string> m_aWarnings = {};
	protected ref array<string> m_aInfoMessages = {};
	protected bool m_bValidationComplete = false;
	protected bool m_bHudNotificationShown = false;
	
	//------------------------------------------------------------------------------------
	// INITIALIZATION
	//------------------------------------------------------------------------------------
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if (!Replication.IsServer())
			return;
		
        #ifdef WORKBENCH
        if (m_bEnableValidation)
            GetGame().GetCallqueue().CallLater(ValidateMission, m_fValidationDelay * 1000, false);
        #endif
	}
	
	//------------------------------------------------------------------------------------
	// VALIDATION LOGIC
	//------------------------------------------------------------------------------------
	
	//! Main validation function
	void ValidateMission()
	{
		m_aCriticalErrors.Clear();
		m_aWarnings.Clear();
		m_aInfoMessages.Clear();
		
		// Run all validation checks
		ValidateGamemodeEntity();
		ValidateFactions();
		ValidateSpawnMarkers();
		ValidateSafezones();
		ValidateCVONSetup();
		ValidateSlottingSetup();
		ValidateSpecialGamemodeRequirements();
		
		// Display results
		DisplayValidationResults();
		
		// Show enhanced output in Workbench
		#ifdef WORKBENCH
		if (m_bShowWorkbenchUI)
		{
			ShowWorkbenchOutput();
			
			// Queue HUD notification for when player spawns
			if (m_aCriticalErrors.Count() > 0)
			{
				GetGame().GetCallqueue().CallLater(TryShowHudNotification, 2000, true);
			}
		}
		#endif
		
		m_bValidationComplete = true;
		
		// Block mission if critical errors and blocking enabled
		if (m_bBlockOnCriticalErrors && m_aCriticalErrors.Count() > 0)
		{
			Print("[CRF Mission Validator] CRITICAL ERRORS FOUND - Mission may not function correctly!", LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------
	// VALIDATION CHECKS
	//------------------------------------------------------------------------------------
	
	//! Validate gamemode entity and core components
	protected void ValidateGamemodeEntity()
	{
		IEntity gamemodeEntity = GetOwner();
		
		// Check for required manager components
		if (!gamemodeEntity.FindComponent(CRF_GamemodeManager))
			AddWarning("Missing CRF_GamemodeManager component");
		else
			AddInfo("[OK] CRF_GamemodeManager component found");
		
		if (!gamemodeEntity.FindComponent(CRF_SlottingManager))
			AddWarning("Missing CRF_SlottingManager component");
		else
			AddInfo("[OK] CRF_SlottingManager component found");
		
		if (!gamemodeEntity.FindComponent(CRF_SafestartManager))
			AddWarning("Missing CRF_SafestartManager component");
		else
			AddInfo("[OK] CRF_SafestartManager component found");
		
		if (!gamemodeEntity.FindComponent(CRF_RespawnManager))
			AddWarning("Missing CRF_RespawnManager component");
		else
			AddInfo("[OK] CRF_RespawnManager component found");
		
		// Check for CRF_Gamemode component and validate slot ratio
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(gamemodeEntity.FindComponent(CRF_Gamemode));
		if (gamemode)
		{
			// Validate faction ratios
			if (gamemode.m_iFactionOneRatio <= 0 && gamemode.m_iFactionTwoRatio <= 0)
				AddCriticalError("(If TVT) At least one Faction Ratio must be greater than 0!");
			else if (gamemode.m_iFactionOneRatio <= 0)
				AddWarning("Faction One Ratio is 0 - Set a ratio!");
			else if (gamemode.m_iFactionTwoRatio <= 0)
				AddWarning("Faction Two Ratio is 0 - Set a ratio!");
			else
				AddInfo(string.Format("[OK] Faction ratios set to %1:%2", gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio));
		}
		
		// Check for AIWorld entity
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			AddCriticalError("Missing SCR_AIWorld entity in world! AI will not function.");
		else
			AddInfo("[OK] SCR_AIWorld entity found");
		
		// Check for Map Entity
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			AddCriticalError("Missing SCR_MapEntity in world! Map UI will not function.");
		else
			AddInfo("[OK] SCR_MapEntity found");
		
		// Check for spawn points
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		
		// BLUFOR spawn point check
		if (respawnManager.GetFactionSpawnpoints("BLUFOR").IsEmpty())
			AddCriticalError("Missing BLUFOR Spawn Point(s) in the world! the BLUFOR Faction will not function");
		else
			AddInfo("[OK] BLUFOR Spawn point found");
		
		// OPFOR spawn point check
		if (respawnManager.GetFactionSpawnpoints("OPFOR").IsEmpty())
			AddCriticalError("Missing OPFOR Spawn Point(s) in the world! the OPFOR Faction will not function");
		else
			AddInfo("[OK] OPFOR Spawn point found");
		
		// INDFOR spawn point check
		if (respawnManager.GetFactionSpawnpoints("INDFOR").IsEmpty())
			AddCriticalError("Missing INDFOR Spawn Point(s) in the world! the INDFOR Faction will not function");
		else
			AddInfo("[OK] INDFOR Spawn point found");
		
		// CIV spawn point check
		if (respawnManager.GetFactionSpawnpoints("CIV").IsEmpty())
			AddCriticalError("Missing CIV Spawn Point(s) in the world! the CIV Faction will not function");
		else
			AddInfo("[OK] CIV Spawn point found");
	}
	
	//! Validate faction setup
	protected void ValidateFactions()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		
		if (!factionManager)
		{
			AddCriticalError("FactionManager not found!");
			return;
		}
		
		// Check for standard factions
		array<string> requiredFactions = {"BLUFOR", "OPFOR", "INDFOR", "CIV", "SPEC"};
		
		foreach (string factionKey : requiredFactions)
		{
			Faction faction = factionManager.GetFactionByKey(factionKey);
			if (!faction)
				AddWarning(string.Format("Faction '%1' not found", factionKey));
			else
				AddInfo(string.Format("[OK] Faction '%1' configured", factionKey));
		}
	}
	
	//! Validate spawn markers
	protected void ValidateSpawnMarkers()
	{
		int bluforMarkers = CountEntitiesWithName("BLUFORSpawnMarker");
		int opforMarkers = CountEntitiesWithName("OPFORSpawnMarker");
		int indforMarkers = CountEntitiesWithName("INDFORSpawnMarker");
		
		if (bluforMarkers == 0)
			AddWarning("No BLUFOR spawn markers found (searched for entities named 'BLUFORSpawnMarker')");
		else
			AddInfo(string.Format("[OK] Found %1 BLUFOR spawn marker(s)", bluforMarkers));
		
		if (opforMarkers == 0)
			AddWarning("No OPFOR spawn markers found (searched for entities named 'OPFORSpawnMarker')");
		else
			AddInfo(string.Format("[OK] Found %1 OPFOR spawn marker(s)", opforMarkers));
		
		if (indforMarkers > 0)
			AddInfo(string.Format("[OK] Found %1 INDFOR spawn marker(s)", indforMarkers));
	}
	
	//! Validate safestart zones
	protected void ValidateSafezones()
	{
		int bluforZones = CountEntitiesWithName("BLUFORSafestartBoundry");
		int opforZones = CountEntitiesWithName("OPFORSafestartBoundry");
		int indforZones = CountEntitiesWithName("INDFORSafestartBoundry");
		
		if (bluforZones == 0)
			AddWarning("No BLUFOR safestart boundary found");
		else
			AddInfo(string.Format("[OK] Found %1 BLUFOR safestart boundary/boundaries", bluforZones));
		
		if (opforZones == 0)
			AddWarning("No OPFOR safestart boundary found");
		else
			AddInfo(string.Format("[OK] Found %1 OPFOR safestart boundary/boundaries", opforZones));
		
		if (indforZones > 0)
			AddInfo(string.Format("[OK] Found %1 INDFOR safestart boundary/boundaries", indforZones));
	}
	
	//! Validate CVON setup
	protected void ValidateCVONSetup()
	{
		IEntity gamemodeEntity = GetOwner();
		
		// Check for CVON component
		bool cvonFound = false;
		
		// Search for CVON_VONGameModeComponent (class name might vary)
		array<Managed> components = {};
		gamemodeEntity.FindComponents(ScriptComponent, components);
		
		foreach (Managed comp : components)
		{
			string className = comp.ClassName();
			if (className.Contains("CVON"))
			{
				cvonFound = true;
				break;
			}
		}
		
		if (!cvonFound)
			AddWarning("CVON_VONGameModeComponent not found - voice comms may not work");
		else
			AddInfo("[OK] CVON component found");
	}
	
	//! Validate slotting setup
	protected void ValidateSlottingSetup()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		if (!slottingManager)
		{
			AddWarning("Slotting manager not available for validation");
			return;
		}
		
		// Check if any slots exist
		map<int, ref CRF_SlotDataContainer> slotMap = slottingManager.GetSlotMap();
		
		if (!slotMap || slotMap.IsEmpty())
		{
			AddWarning("No player slots found in mission! Players won't be able to spawn.");
			AddInfo("  -> Place playable AI groups in the world or use CRF_GroupSpawner");
		}
		else
		{
			AddInfo(string.Format("[OK] Found %1 player slots", slotMap.Count()));
			
			// Count slots by faction
			int bluforSlots = 0;
			int opforSlots = 0;
			int indforSlots = 0;
			int civSlots = 0;
			
			foreach (int slotId, CRF_SlotDataContainer slotData : slotMap)
			{
				string factionKey = slotData.GetSlotFactionKey();
				
				switch (factionKey)
				{
					case "BLUFOR": bluforSlots++; break;
					case "OPFOR": opforSlots++; break;
					case "INDFOR": indforSlots++; break;
					case "CIV": civSlots++; break;
				}
			}
			
			AddInfo(string.Format("  -> BLUFOR: %1 slots", bluforSlots));
			AddInfo(string.Format("  -> OPFOR: %1 slots", opforSlots));
			if (indforSlots > 0)
				AddInfo(string.Format("  -> INDFOR: %1 slots", indforSlots));
			if (civSlots > 0)
				AddInfo(string.Format("  -> CIV: %1 slots", civSlots));
		}
	}
	
	//! Validate special gamemode requirements
	protected void ValidateSpecialGamemodeRequirements()
	{
		IEntity gamemodeEntity = GetOwner();
		
		// Check for Rush gamemode
		if (gamemodeEntity.FindComponent(CRF_RushGamemodeManager))
		{
			AddInfo("[OK] Rush gamemode detected");
			
			// Check for required Rush entities
			for (int zone = 1; zone <= 3; zone++)
			{
				for (int mcom = 1; mcom <= 2; mcom++)
				{
					string char;
					if (mcom == 1)
						char = "A";
					else
						char = "B";
					
					string entityName = string.Format("rushZone%1_%2", zone, char);
					
					if (GetGame().GetWorld().FindEntityByName(entityName))
						AddInfo(string.Format("  -> Found Rush MCOM marker: %1", entityName));
				}
			}
		}
		
		// Check for Search & Destroy gamemode
		array<Managed> components = {};
		gamemodeEntity.FindComponents(ScriptComponent, components);
		
		foreach (Managed comp : components)
		{
			string className = comp.ClassName();
			
			if (className.Contains("S&D") || className.Contains("SearchDestroy"))
			{
				AddInfo("[OK] Search & Destroy gamemode detected");
				
				// Check for bomb sites
				if (GetGame().GetWorld().FindEntityByName("aSiteTrigger"))
					AddInfo("  -> Found A site trigger");
				else
					AddWarning("  -> Missing 'aSiteTrigger' entity for S&D");
				
				if (GetGame().GetWorld().FindEntityByName("bSiteTrigger"))
					AddInfo("  -> Found B site trigger");
				else
					AddWarning("  -> Missing 'bSiteTrigger' entity for S&D");
				
				break;
			}
			
			if (className.Contains("Raid"))
			{
				AddInfo("[OK] Raid gamemode detected");
				AddInfo("  -> Ensure you have placed entities with CRF_RaidItemComponent");
				break;
			}
			
			if (className.Contains("GunGame"))
			{
				AddInfo("[OK] Gun Game gamemode detected");
				
				// Check for spawn points
				int spawnCount = 0;
				for (int i = 0; i < 128; i++)
				{
					if (GetGame().GetWorld().FindEntityByName("gunSpawn" + i.ToString()))
						spawnCount++;
					else
						break;
				}
				
				if (spawnCount == 0)
					AddWarning("  -> No gun game spawn points found (gunSpawn0, gunSpawn1, etc.)");
				else
					AddInfo(string.Format("  -> Found %1 gun game spawn points", spawnCount));
				
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------
	// HELPER FUNCTIONS
	//------------------------------------------------------------------------------------
	
	//! Count entities with a specific name
	protected int CountEntitiesWithName(string entityName)
	{
		int count = 0;
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		
		if (entity)
			count = 1;
		
		// Check for numbered variants (e.g., BLUFORSpawnMarker1, BLUFORSpawnMarker2)
		for (int i = 1; i < 10; i++)
		{
			entity = GetGame().GetWorld().FindEntityByName(entityName + i.ToString());
			if (entity)
				count++;
		}
		
		return count;
	}
	
	//! Add a critical error
	protected void AddCriticalError(string message)
	{
		m_aCriticalErrors.Insert(message);
	}
	
	//! Add a warning
	protected void AddWarning(string message)
	{
		m_aWarnings.Insert(message);
	}
	
	//! Add an info message
	protected void AddInfo(string message)
	{
		m_aInfoMessages.Insert(message);
	}
	
	//! Display all validation results
	protected void DisplayValidationResults()
	{
		// In Workbench mode, skip this - ShowWorkbenchOutput() handles the display
		#ifdef WORKBENCH
		if (m_bShowWorkbenchUI)
			return;
		#endif
		
		// Only show summary - errors and warnings will be in Workbench output
		int errorCount = m_aCriticalErrors.Count();
		int warningCount = m_aWarnings.Count();
		
		if (errorCount > 0 || warningCount > 0)
		{
			Print("", LogLevel.NORMAL);
			Print("========================================", LogLevel.NORMAL);
			Print(string.Format("[CRF Mission Validator] Found %1 errors, %2 warnings", errorCount, warningCount), LogLevel.WARNING);
			Print("========================================", LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------
	// PUBLIC API
	//------------------------------------------------------------------------------------
	
	//! Manually trigger validation
	void TriggerValidation()
	{
		if (!m_bValidationComplete)
			ValidateMission();
	}
	
	//! Check if validation passed without critical errors
	bool IsValid()
	{
		return m_aCriticalErrors.Count() == 0;
	}
	
	//! Get critical errors
	array<string> GetCriticalErrors()
	{
		return m_aCriticalErrors;
	}
	
	//! Get warnings
	array<string> GetWarnings()
	{
		return m_aWarnings;
	}
	
	//! Get info messages
	array<string> GetInfoMessages()
	{
		return m_aInfoMessages;
	}
	
	//------------------------------------------------------------------------------------
	// WORKBENCH OUTPUT
	//------------------------------------------------------------------------------------
	
	//! Show enhanced validation output in Workbench
	protected void ShowWorkbenchOutput()
	{
		Print("", LogLevel.NORMAL);
		Print("================================================================", LogLevel.NORMAL);
		Print("         CRF MISSION VALIDATOR - RESULTS", LogLevel.NORMAL);
		Print("================================================================", LogLevel.NORMAL);
		Print("", LogLevel.NORMAL);
		
		int errorCount = m_aCriticalErrors.Count();
		int warningCount = m_aWarnings.Count();
		
		// Show errors if any - ALL as NORMAL log level
		if (errorCount > 0)
		{
			Print("[X] CRITICAL ERRORS FOUND:", LogLevel.NORMAL);
			Print("", LogLevel.NORMAL);
			
			if (errorCount >= 1)
				Print("  [X] " + m_aCriticalErrors[0], LogLevel.NORMAL);
			if (errorCount >= 2)
				Print("  [X] " + m_aCriticalErrors[1], LogLevel.NORMAL);
			if (errorCount >= 3)
				Print("  [X] " + m_aCriticalErrors[2], LogLevel.NORMAL);
			if (errorCount >= 4)
				Print("  [X] " + m_aCriticalErrors[3], LogLevel.NORMAL);
			if (errorCount >= 5)
				Print("  [X] " + m_aCriticalErrors[4], LogLevel.NORMAL);
			if (errorCount > 5)
				PrintFormat("  ... and %1 more errors", errorCount - 5);
			
			Print("", LogLevel.NORMAL);
		}
		
		// Show warnings if any - ALL as NORMAL log level
		if (warningCount > 0)
		{
			Print("[!] WARNINGS:", LogLevel.NORMAL);
			Print("", LogLevel.NORMAL);
			
			if (warningCount >= 1)
				Print("  [!] " + m_aWarnings[0], LogLevel.NORMAL);
			if (warningCount >= 2)
				Print("  [!] " + m_aWarnings[1], LogLevel.NORMAL);
			if (warningCount >= 3)
				Print("  [!] " + m_aWarnings[2], LogLevel.NORMAL);
			if (warningCount >= 4)
				Print("  [!] " + m_aWarnings[3], LogLevel.NORMAL);
			if (warningCount >= 5)
				Print("  [!] " + m_aWarnings[4], LogLevel.NORMAL);
			if (warningCount >= 6)
				Print("  [!] " + m_aWarnings[5], LogLevel.NORMAL);
			if (warningCount >= 7)
				Print("  [!] " + m_aWarnings[6], LogLevel.NORMAL);
			if (warningCount >= 8)
				Print("  [!] " + m_aWarnings[7], LogLevel.NORMAL);
			if (warningCount >= 9)
				Print("  [!] " + m_aWarnings[8], LogLevel.NORMAL);
			if (warningCount >= 10)
				Print("  [!] " + m_aWarnings[9], LogLevel.NORMAL);
			if (warningCount > 10)
				PrintFormat("  ... and %1 more warnings", warningCount - 10);
			
			Print("", LogLevel.NORMAL);
		}
		
		// Final verdict
		Print("================================================================", LogLevel.NORMAL);
		if (errorCount == 0 && warningCount == 0)
		{
			Print("[OK] MISSION IS READY - No issues found!", LogLevel.NORMAL);
		}
		else if (errorCount == 0)
		{
			PrintFormat("[OK] MISSION IS READY - 0 errors, %1 warnings", warningCount);
		}
		else
		{
			PrintFormat("[X] MISSION HAS ERRORS - %1 errors, %2 warnings", errorCount, warningCount);
			Print("    Fix critical errors before deploying this mission!", LogLevel.NORMAL);
		}
		Print("================================================================", LogLevel.NORMAL);
		Print("", LogLevel.NORMAL);
	}
	
	//! Try to show HUD notification (called repeatedly until player is ready)
	protected void TryShowHudNotification()
	{
		// Already shown, stop trying
		if (m_bHudNotificationShown)
		{
			GetGame().GetCallqueue().Remove(TryShowHudNotification);
			return;
		}
		
		// Check if gamemode is in GAME state
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode || gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			return;
		
		// Check if player is in game
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		// Check if player has spawned
		IEntity controlledEntity = pc.GetControlledEntity();
		if (!controlledEntity)
			return;
		
		// Player is ready and game state is GAME, show notification
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
		{
			string message = string.Format("MISSION VALIDATION FAILED - %1 CRITICAL ERRORS - CHECK CONSOLE!", m_aCriticalErrors.Count());
			hintManager.ShowCustomHint(message, "Mission Validator", 10);
			m_bHudNotificationShown = true;
			GetGame().GetCallqueue().Remove(TryShowHudNotification);
		}
	}
}
