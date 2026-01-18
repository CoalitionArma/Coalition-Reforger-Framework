//------------------------------------------------------------------------------------
// CRF_RushGamemodeManager: Rush gamemode implementation for Coalition Reforger Framework
// Manages three zones with two MCOM sites each. When both MCOMs in a zone are destroyed,
// the next zone unlocks for attack.
//
// Features:
// - Progressive zone unlocking (Zone 1 -> Zone 2 -> Zone 3)
// - 2D map markers showing all MCOM locations with color coding
// - 3D HUD markers with sequential letters (A-F) visible through walls
// - Bomb planting/defusing mechanics with countdown timers
// - Configurable attacking/defending sides
// - Dynamic MCOM naming: A, B (Zone 1), C, D (Zone 2), E, F (Zone 3)
// - Optional dynamic respawn point control/spawn teleportation per zone; Same convention as MCOMs
//------------------------------------------------------------------------------------


// RESPAWN CONFIGURATION CLASSES
/**
 * Individual respawn marker entry - defines where to teleport a faction's respawn flag
 */
[BaseContainerProps()]
class CRF_Rush_RespawnPointEntry
{
	[Attribute("", UIWidgets.EditBox, desc: "Name of spawn point entity (e.g., 'Zone1_BLUFOR_Spawn', 'Zone2_OPFOR_Spawn')")]
	string m_sMarkerEntityName;
	
	[Attribute("BLUFOR", UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")}, desc: "Which faction's respawn flag to move")]
	FactionKey m_eFaction;
}

/**
 * Zone respawn configuration - defines where to move respawn flags when a zone is cleared
 * Zone number is determined by position in array (first entry = Zone 1, second = Zone 2, etc.)
 */
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sZoneName")]
class CRF_Rush_ZoneRespawnConfig
{
	[Attribute("Zone", UIWidgets.Auto, desc: "Name of the zone, can be left alone. Does NOT do anything outside of MM organization.")]
	protected string m_sZoneName;
	
	[Attribute("", UIWidgets.Auto, desc: "Marker positions to teleport respawn flags to when the previous zone is cleared. Ensure entities and these marker spawn names are named identically.")]
	ref array<ref CRF_Rush_RespawnPointEntry> m_aRespawnPoints;
}

[ComponentEditorProps(category: "Game Mode Component", description: "Rush gamemode with 3 zones and 6 sequential MCOM sites (A-F)")]
class CRF_RushGamemodeManagerClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RushGamemodeManager: SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	// Faction Settings
	//------------------------------------------------------------------------------------
	[Attribute("BLUFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side attacking the MCOM sites")]
	FactionKey m_AttackingSide;
	
	[Attribute("OPFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side defending the MCOM sites")]
	FactionKey m_DefendingSide;
	
	// MCOM Prefab Settings
	//------------------------------------------------------------------------------------
	[Attribute("{A8C69227F4322F20}Prefabs/Structures/CRF_Rush_MCOM.et", UIWidgets.ResourceNamePicker, desc: "The prefab to spawn as MCOM sites", params: "et")]
	ResourceName m_MCOMPrefab;
	
	// UI Settings
	//------------------------------------------------------------------------------------
	[Attribute("false", desc: "Hide the map markers for MCOM sites")]
	bool m_bHideMapMarkers;
	
	[Attribute("45", desc: "Time in seconds for MCOM destruction countdown")]
	int m_iMCOMTimer;
	
	// Zone Configuration
	//------------------------------------------------------------------------------------
	[Attribute("3", UIWidgets.Slider, desc: "Number of zones in the mission (1-3)", params: "1 3 1")]
	int m_iNumberOfZones;
	
	[Attribute("2", UIWidgets.Slider, desc: "Number of MCOMs per zone (1-2)", params: "1 2 1")]
	int m_iMCOMsPerZone;
	
	// Respawn Point Control
	//------------------------------------------------------------------------------------
	[Attribute("0", UIWidgets.CheckBox, desc: "Enable dynamic respawn point control based on zone progression")]
	bool m_bEnableDynamicRespawns;
	
	[Attribute("", UIWidgets.Auto, desc: "Add zones and respawn points to enable when each zone is cleared. Entry position in list determines zone number (first index = Zone 1, etc.) If wanting to SKIP a zone, add no respawns but maintain the order.")]
	ref array<ref CRF_Rush_ZoneRespawnConfig> m_aZoneRespawnConfigs;
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	// Zone cleared tracking (to prevent re-enabling spawns)
	//------------------------------------------------------------------------------------
	protected ref array<bool> m_aZonesClearedStatus;		// Track which zones have been cleared
	
	// Zone Management
	//------------------------------------------------------------------------------------
	protected int m_iCurrentZone = 1;					// Currently active zone (1-3)
	protected int m_iActiveZoneCount = 1;				// Number of zones that are unlocked
	
	// Dynamic MCOM Status Tracking (supports 1-3 zones, 1-2 MCOMs per zone)
	//------------------------------------------------------------------------------------
	protected ref array<ref array<bool>> m_aMCOMDestroyed;		// [zoneIndex][mcomIndex] - destroyed status
	protected ref array<ref array<bool>> m_aMCOMPlanted;		// [zoneIndex][mcomIndex] - planted status  
	protected ref array<ref array<IEntity>> m_aMCOMEntities;	// [zoneIndex][mcomIndex] - entity references
	
	// Legacy compatibility variables (kept for network replication) - maps to sequential A-F
	//------------------------------------------------------------------------------------
	protected bool m_bZone1Alpha = false;				// Zone 1 Alpha MCOM destroyed (now MCOM A)
	protected bool m_bZone1Beta = false;				// Zone 1 Beta MCOM destroyed (now MCOM B)
	protected bool m_bZone2Alpha = false;				// Zone 2 Alpha MCOM destroyed (now MCOM C)
	protected bool m_bZone2Beta = false;				// Zone 2 Beta MCOM destroyed (now MCOM D)
	protected bool m_bZone3Alpha = false;				// Zone 3 Alpha MCOM destroyed (now MCOM E)
	protected bool m_bZone3Beta = false;				// Zone 3 Beta MCOM destroyed (now MCOM F)
	
	// MCOM Destruction Status (replicated to clients)
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "OnMCOMDestroyedReplicated")]
	protected string m_sReplicatedDestroyedMCOM = "";
	
	[RplProp()]
	protected bool m_bZone1AlphaDestroyed = false;
	
	[RplProp()]
	protected bool m_bZone1BetaDestroyed = false;
	
	[RplProp()]
	protected bool m_bZone2AlphaDestroyed = false;
	
	[RplProp()]
	protected bool m_bZone2BetaDestroyed = false;
	
	[RplProp()]
	protected bool m_bZone3AlphaDestroyed = false;
	
	[RplProp()]
	protected bool m_bZone3BetaDestroyed = false;
	
	// MCOM Planting Status (legacy for replication)
	//------------------------------------------------------------------------------------
	[RplProp()]
	protected bool m_bZone1AlphaPlanted = false;
	
	[RplProp()]
	protected bool m_bZone1BetaPlanted = false;
	
	[RplProp()]
	protected bool m_bZone2AlphaPlanted = false;
	
	[RplProp()]
	protected bool m_bZone2BetaPlanted = false;
	
	[RplProp()]
	protected bool m_bZone3AlphaPlanted = false;
	
	[RplProp()]
	protected bool m_bZone3BetaPlanted = false;
	
	// Countdown Management
	//------------------------------------------------------------------------------------
	[RplProp()]
	protected bool m_bCountdownActive = false;

	protected int m_iCountdownTimeRemaining;
	protected string m_sActiveMCOM = "";				// Which MCOM is currently planted
	
	// Legacy MCOM Entity References (kept for compatibility)
	//------------------------------------------------------------------------------------
	protected IEntity m_Zone1AlphaMCOM;
	protected IEntity m_Zone1BetaMCOM;
	protected IEntity m_Zone2AlphaMCOM;
	protected IEntity m_Zone2BetaMCOM;
	protected IEntity m_Zone3AlphaMCOM;
	protected IEntity m_Zone3BetaMCOM;
	
	// Note: 3D markers are now handled by CRF_Rush_3DMarkerComponent on MCOM entities
	// Each MCOM displays a sequential letter (A, B, C, D, E, F) instead of Alpha/Beta per zone
	// Standalone marker system has been removed to prevent duplicates
	
	// UI and Notification Management
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;
	
	[RplProp(onRplName: "PlaySound")]
	protected string m_sSoundString;
	
	[RplProp(onRplName: "PlayPlantingSoundClient")]
	protected bool m_bPlayPlantingSound;
	
	[RplProp(onRplName: "StopPlantingSoundClient")]
	protected bool m_bStopPlantingSound;
	
	[RplProp(onRplName: "PlayDefuseSoundClient")]
	protected bool m_bPlayDefuseSound;
	
	[RplProp(onRplName: "StopDefuseSoundClient")]
	protected bool m_bStopDefuseSound;
	
	[RplProp(onRplName: "PlayBombSoundClient")]
	protected bool m_bPlayBombSound;
	
	[RplProp(onRplName: "StopBombSoundClient")]
	protected bool m_bStopBombSound;
	
	[RplProp(onRplName: "MCOMDestroyedClient")]
	protected string m_sDestroyedMCOMString;
	
	// 3D Marker Replication
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "Update3DMarkersFromReplication")]
	protected bool m_b3DMarkersInitialized = false;		// Toggle to trigger 3D marker initialization
	
	[RplProp()]
	protected bool m_bZone1AlphaMarkerVisible = false;		// Zone 1 Alpha marker visibility
	
	[RplProp()]
	protected bool m_bZone1BetaMarkerVisible = false;		// Zone 1 Beta marker visibility
	
	[RplProp()]
	protected bool m_bZone2AlphaMarkerVisible = false;		// Zone 2 Alpha marker visibility
	
	[RplProp()]
	protected bool m_bZone2BetaMarkerVisible = false;		// Zone 2 Beta marker visibility
	
	[RplProp()]
	protected bool m_bZone3AlphaMarkerVisible = false;		// Zone 3 Alpha marker visibility
	
	[RplProp()]
	protected bool m_bZone3BetaMarkerVisible = false;		// Zone 3 Beta marker visibility
	
	// Replicated marker positions (for clients that don't have entity references)
	[RplProp()]
	protected vector m_vZone1AlphaPosition;
	
	[RplProp()]
	protected vector m_vZone1BetaPosition;
	
	[RplProp()]
	protected vector m_vZone2AlphaPosition;
	
	[RplProp()]
	protected vector m_vZone2BetaPosition;
	
	[RplProp()]
	protected vector m_vZone3AlphaPosition;
	
	[RplProp()]
	protected vector m_vZone3BetaPosition;
	
	protected SCR_PopUpNotification m_PopUpNotification;
	
	// Sound Management
	//------------------------------------------------------------------------------------
	protected AudioHandle m_CurrentBombSoundHandle;		// Handle for currently playing bomb ticking sound
	protected bool m_bBombSoundPlaying = false;			// Track if bomb sound is currently playing
	protected AudioHandle m_CurrentPlantingSoundHandle;	// Handle for currently playing planting sound
	protected bool m_bPlantingSoundPlaying = false;		// Track if planting sound is currently playing
	protected AudioHandle m_CurrentDefuseSoundHandle;		// Handle for currently playing defuse sound
	protected bool m_bDefuseSoundPlaying = false;			// Track if defuse sound is currently playing
	protected string m_sPlantingMCOM = "";					// Track which MCOM is currently being planted
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	/**
	 * Initialize the Rush gamemode when world is ready
	 * @param world The game world instance
	 */
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		// Enable frame events for marker updates
		SetEventMask(GetOwner(), EntityEvent.FRAME);
		
		InitializeMCOMSites();
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fUpdateBuffer >= 1)
		{
			m_fUpdateBuffer = 0;
			CountdownTimer();
		}
		m_fUpdateBuffer += timeSlice;
	}
	
	/**
	 * Handle player connection events
	 * Ensure each player gets the current marker state
	 */
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		// Delay marker setup to ensure player controller is ready
		GetGame().GetCallqueue().CallLater(SetupMarkersForPlayer, 3000, false, playerId);
	}
	
	/**
	 * Setup markers for a specific player
	 * @param playerId The player ID to setup markers for
	 * NOTE: Public so CRF_JIPSyncManager can call this for centralized JIP sync
	 */
	void SetupMarkersForPlayer(int playerId)
	{
		// For new players connecting, trigger replication update to ensure they get current marker state
		if (Replication.IsServer())
		{
			// Trigger replication update for the new player
			m_b3DMarkersInitialized = !m_b3DMarkersInitialized;
			Replication.BumpMe();
		}
		
		// If markers are not hidden, ensure this player gets the current zone markers
		if (!m_bHideMapMarkers)
		{
			CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
			if (playerControllerManager)
			{
				array<string> currentMarkers = playerControllerManager.GetScriptedMarkersArray();
				if (currentMarkers && currentMarkers.Count() > 0)
				{
					// Player should see existing markers
				}
				else
				{
					// Re-add markers if they're missing
					AddAllMCOMMarkers();
				}
			}
		}
	}
	
	/**
	 * Initialize all MCOM sites and their positions
	 * Spawns MCOMs at predefined trigger positions
	 */
	protected void InitializeMCOMSites()
	{
		// Initialize dynamic arrays based on configuration
		InitializeDynamicArrays();
		
		// Find all MCOM spawn triggers dynamically
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				string triggerName = GetTriggerName(zoneIndex + 1, mcomIndex);
				IEntity trigger = GetGame().GetWorld().FindEntityByName(triggerName);
				
				if (!trigger)
					continue;
				
				string mcomIdentifier = GetMCOMIdentifier(zoneIndex + 1, mcomIndex);
				SpawnMCOMAtPosition(trigger, mcomIdentifier);
			}
		}
		
		// Legacy hardcoded initialization for backward compatibility
		InitializeLegacyMCOMs();
	
		// Initialize map markers if not hidden
		if (!m_bHideMapMarkers)
			GetGame().GetCallqueue().CallLater(InitializeMapMarkers, 1000, false);
		
		// Initialize 3D markers after a delay to ensure UI is ready
		GetGame().GetCallqueue().CallLater(Initialize3DMarkers, 1500, false);
	}
	
	/**
	 * Initialize dynamic arrays for MCOM management
	 */
	protected void InitializeDynamicArrays()
	{
		m_aMCOMDestroyed = new array<ref array<bool>>();
		m_aMCOMPlanted = new array<ref array<bool>>();
		m_aMCOMEntities = new array<ref array<IEntity>>();
		
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			m_aMCOMDestroyed.Insert(new array<bool>());
			m_aMCOMPlanted.Insert(new array<bool>());
			m_aMCOMEntities.Insert(new array<IEntity>());
			
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				m_aMCOMDestroyed[zoneIndex].Insert(false);
				m_aMCOMPlanted[zoneIndex].Insert(false);
				m_aMCOMEntities[zoneIndex].Insert(null);
			}
		}
		
		// Initialize zone cleared tracking only if dynamic respawns are enabled
		if (m_bEnableDynamicRespawns && m_aZoneRespawnConfigs && !m_aZoneRespawnConfigs.IsEmpty())
		{
			m_aZonesClearedStatus = new array<bool>();
			for (int i = 0; i < m_iNumberOfZones; i++)
			{
				m_aZonesClearedStatus.Insert(false);
			}
		}
	}
	/**
	 * Generate trigger name for zone and MCOM index
	 * @param zoneNumber The zone number (1-3)
	 * @param mcomIndex The MCOM index within the zone (0-1)
	 * @return The trigger name
	 */
	protected string GetTriggerName(int zoneNumber, int mcomIndex)
	{
		// Calculate the sequential MCOM letter (a, b, c, d, e, f)
		int globalIndex = (zoneNumber - 1) * m_iMCOMsPerZone + mcomIndex;
		
		switch (globalIndex)
		{
			case 0: return "mcom_a_trigger";
			case 1: return "mcom_b_trigger";
			case 2: return "mcom_c_trigger";
			case 3: return "mcom_d_trigger";
			case 4: return "mcom_e_trigger";
			case 5: return "mcom_f_trigger";
			default: return string.Format("mcom_%1_trigger", globalIndex + 1); // Fallback for more than 6 MCOMs
		}
		
		return ""; // Should never reach here
	}
	
	/**
	 * Generate MCOM identifier for zone and MCOM index
	 * @param zoneNumber The zone number (1-3)  
	 * @param mcomIndex The MCOM index within the zone (0-1)
	 * @return The MCOM identifier
	 */
	protected string GetMCOMIdentifier(int zoneNumber, int mcomIndex)
	{
		// Calculate the sequential MCOM letter (A, B, C, D, E, F)
		int globalIndex = (zoneNumber - 1) * m_iMCOMsPerZone + mcomIndex;
		
		switch (globalIndex)
		{
			case 0: return "MCOMA";
			case 1: return "MCOMB";
			case 2: return "MCOMC";
			case 3: return "MCOMD";
			case 4: return "MCOME";
			case 5: return "MCOMF";
			default: return string.Format("MCOM%1", globalIndex + 1); // Fallback for more than 6 MCOMs
		}
		
		return ""; // Should never reach here
	}
	
	/**
	 * Initialize MCOMs at trigger positions
	 */
	protected void InitializeLegacyMCOMs()
	{
		// Find all MCOM spawn triggers
		IEntity zone1AlphaTrigger = GetGame().GetWorld().FindEntityByName("z1_alpha_trigger");
		IEntity zone1BetaTrigger = GetGame().GetWorld().FindEntityByName("z1_beta_trigger");
		IEntity zone2AlphaTrigger = GetGame().GetWorld().FindEntityByName("z2_alpha_trigger");
		IEntity zone2BetaTrigger = GetGame().GetWorld().FindEntityByName("z2_beta_trigger");
		IEntity zone3AlphaTrigger = GetGame().GetWorld().FindEntityByName("z3_alpha_trigger");
		IEntity zone3BetaTrigger = GetGame().GetWorld().FindEntityByName("z3_beta_trigger");
		
		Print("[SpawnMCOMs] Configuration: " + m_iNumberOfZones + " zones, " + m_iMCOMsPerZone + " MCOMs per zone");
		
		// Spawn MCOMs using configuration-aware logic
		if (zone1AlphaTrigger && m_iNumberOfZones >= 1 && m_iMCOMsPerZone >= 1)
		{
			Print("[SpawnMCOMs] Spawning MCOMA at zone1AlphaTrigger (Zone 1)");
			SpawnMCOMAtPosition(zone1AlphaTrigger, "MCOMA");
		}
		
		// MCOMB spawning depends on configuration
		if (m_iMCOMsPerZone == 1)
		{
			// 1 MCOM per zone: MCOMB goes to Zone 2 (zone2AlphaTrigger = mcom_b_trigger)
			if (zone2AlphaTrigger && m_iNumberOfZones >= 2)
			{
				Print("[SpawnMCOMs] Spawning MCOMB at zone2AlphaTrigger (Zone 2) - 1 MCOM per zone config");
				SpawnMCOMAtPosition(zone2AlphaTrigger, "MCOMB");
			}
		}
		else
		{
			// 2 MCOMs per zone: MCOMB goes to Zone 1 Beta (zone1BetaTrigger = mcom_b_trigger)
			if (zone1BetaTrigger && m_iNumberOfZones >= 1)
			{
				Print("[SpawnMCOMs] Spawning MCOMB at zone1BetaTrigger (Zone 1) - 2 MCOMs per zone config");
				SpawnMCOMAtPosition(zone1BetaTrigger, "MCOMB");
			}
		}
		
		// MCOMC spawning depends on configuration
		if (m_iMCOMsPerZone == 1)
		{
			// 1 MCOM per zone: MCOMC goes to Zone 3 (zone3AlphaTrigger = mcom_c_trigger)
			if (zone3AlphaTrigger && m_iNumberOfZones >= 3)
			{
				Print("[SpawnMCOMs] Spawning MCOMC at zone3AlphaTrigger (Zone 3) - 1 MCOM per zone config");
				SpawnMCOMAtPosition(zone3AlphaTrigger, "MCOMC");
			}
		}
		else
		{
			// 2 MCOMs per zone: MCOMC goes to Zone 2 Alpha (zone2AlphaTrigger = mcom_c_trigger)
			if (zone2AlphaTrigger && m_iNumberOfZones >= 2)
			{
				Print("[SpawnMCOMs] Spawning MCOMC at zone2AlphaTrigger (Zone 2) - 2 MCOMs per zone config");
				SpawnMCOMAtPosition(zone2AlphaTrigger, "MCOMC");
			}
		}
		
		// MCOMD always goes to Zone 2 Beta when there are 2 MCOMs per zone
		if (zone2BetaTrigger && m_iNumberOfZones >= 2 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone2BetaTrigger, "MCOMD");
		
		// MCOME and MCOMF only used in 2 MCOMs per zone configuration for Zone 3
		if (zone3AlphaTrigger && m_iNumberOfZones >= 3 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone3AlphaTrigger, "MCOME");
		if (zone3BetaTrigger && m_iNumberOfZones >= 3 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone3BetaTrigger, "MCOMF");
		
		// Initialize map markers if not hidden
		if (!m_bHideMapMarkers)
			GetGame().GetCallqueue().CallLater(InitializeMapMarkers, 1000, false);
		
		// Initialize 3D markers after a delay to ensure UI is ready
		GetGame().GetCallqueue().CallLater(Initialize3DMarkers, 1500, false);
	}
	
	/**
	 * Spawn an MCOM at a specific trigger position
	 * @param trigger The trigger entity defining spawn position
	 * @param mcomIdentifier Unique identifier for this MCOM
	 */
	protected void SpawnMCOMAtPosition(IEntity trigger, string mcomIdentifier)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		trigger.GetWorldTransform(spawnParams.Transform);
		
		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);
		
		IEntity mcomEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_MCOMPrefab), GetGame().GetWorld(), spawnParams);
		if (!mcomEntity)
		{
			return;
		}
		
		// Configure the 3D marker component on the spawned MCOM
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (markerComponent)
		{
			// Set the correct letter based on sequential MCOM identifier
			string markerLetter = "A";
			switch (mcomIdentifier)
			{
				case "MCOMA":
				case "Zone1Alpha": // Legacy compatibility
					markerLetter = "A";
					break;
				case "MCOMB": 
				case "Zone1Beta": // Legacy compatibility
					markerLetter = "B";
					break;
				case "MCOMC":
				case "Zone2Alpha": // Legacy compatibility
					markerLetter = "C";
					break;
				case "MCOMD":
				case "Zone2Beta": // Legacy compatibility
					markerLetter = "D";
					break;
				case "MCOME":
				case "Zone3Alpha": // Legacy compatibility
					markerLetter = "E";
					break;
				case "MCOMF":
				case "Zone3Beta": // Legacy compatibility
					markerLetter = "F";
					break;
			}
			
			// Update the component's letter using the proper method
			markerComponent.SetMarkerLetter(markerLetter);
			
			// Determine zone and set color based on zone
			int zoneIndex, mcomIndex;
			Color markerColor;
			bool shouldShow = true; // Always show all MCOM markers
			
			if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
			{
				int zoneNumber = zoneIndex + 1; // Convert from 0-based to 1-based
				
				// Set color based on zone status
				if (zoneNumber == m_iCurrentZone)
				{
					markerColor = Color.FromInt(ARGB(255, 255, 0, 0)); // Red for active zone
				}
				else if (zoneNumber < m_iCurrentZone)
				{
					markerColor = Color.FromInt(ARGB(255, 128, 128, 128)); // Gray for completed zones
				}
				else
				{
					markerColor = Color.FromInt(ARGB(255, 255, 255, 0)); // Yellow for future zones
				}
				
				string colorName;
				if (zoneNumber == m_iCurrentZone)
				{
					colorName = "Red";
				}
				else if (zoneNumber < m_iCurrentZone)
				{
					colorName = "Gray";
				}
				else
				{
					colorName = "Yellow";
				}
			}
			else
			{
				// Fallback color if parsing fails
				markerColor = Color.FromInt(ARGB(255, 255, 0, 0)); // Red
			}
			
			markerComponent.SetMarkerColor(markerColor);
			markerComponent.SetVisible(shouldShow);
		}
		else
		{
			return;
		}
		
		// Store entity in dynamic arrays
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && mcomIndex < m_aMCOMEntities[zoneIndex].Count())
				m_aMCOMEntities[zoneIndex][mcomIndex] = mcomEntity;
		}
		
		// Store entity references for legacy compatibility (only entity references, not IDs)
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				m_Zone1AlphaMCOM = mcomEntity;
				break;
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				m_Zone1BetaMCOM = mcomEntity;
				break;
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				m_Zone2AlphaMCOM = mcomEntity;
				break;
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				m_Zone2BetaMCOM = mcomEntity;
				break;
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				m_Zone3AlphaMCOM = mcomEntity;
				break;
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				m_Zone3BetaMCOM = mcomEntity;
				break;
		}
	}
	
	/**
	 * Initialize map markers for all MCOMs
	 */
	protected void InitializeMapMarkers()
	{
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerManager) 
		{
			GetGame().GetCallqueue().CallLater(InitializeMapMarkers, 2000, false);
			return;
		}
		
		// Add markers for all MCOMs
		AddAllMCOMMarkers();
		
		// Force a client-side update for all connected players
		GetGame().GetCallqueue().CallLater(ForceMarkerUpdate, 1000, false);
	}
	
	/**
	 * Initialize 3D markers for all MCOMs
	 */
	protected void Initialize3DMarkers()
	{
		// The CRF_Rush_3DMarkerComponent handles marker display on MCOM entities
		// This prevents duplicate markers when running locally (listen server)
		
		// BUT we still need to initialize positions and visibility states for replication
		if (Replication.IsServer())
			InitializeAllMCOMMarkers(); // This sets positions and visibility states but doesn't create UI
		
		// We still need to update marker colors based on zone state for replication
		Update3DMarkerColors();
	}
	
	/**
	 * Initialize all MCOM markers on server and replicate to clients
	 * NOTE: Marker creation disabled, but visibility state still needed for replication
	 */
	protected void InitializeAllMCOMMarkers()
	{
		// Set replicated visibility properties - only show current zone for 3D markers
		m_bZone1AlphaMarkerVisible = ShouldMarkerBeVisible("MCOMA") || ShouldMarkerBeVisible("Zone1Alpha");
		m_bZone1BetaMarkerVisible = ShouldMarkerBeVisible("MCOMB") || ShouldMarkerBeVisible("Zone1Beta");
		m_bZone2AlphaMarkerVisible = ShouldMarkerBeVisible("MCOMC") || ShouldMarkerBeVisible("Zone2Alpha");
		m_bZone2BetaMarkerVisible = ShouldMarkerBeVisible("MCOMD") || ShouldMarkerBeVisible("Zone2Beta");
		m_bZone3AlphaMarkerVisible = ShouldMarkerBeVisible("MCOME") || ShouldMarkerBeVisible("Zone3Alpha");
		m_bZone3BetaMarkerVisible = ShouldMarkerBeVisible("MCOMF") || ShouldMarkerBeVisible("Zone3Beta");
		
		// Set replicated positions for clients
		if (m_Zone1AlphaMCOM) m_vZone1AlphaPosition = GetMarkerPosition(m_Zone1AlphaMCOM);
		if (m_Zone1BetaMCOM) m_vZone1BetaPosition = GetMarkerPosition(m_Zone1BetaMCOM);
		if (m_Zone2AlphaMCOM) m_vZone2AlphaPosition = GetMarkerPosition(m_Zone2AlphaMCOM);
		if (m_Zone2BetaMCOM) m_vZone2BetaPosition = GetMarkerPosition(m_Zone2BetaMCOM);
		if (m_Zone3AlphaMCOM) m_vZone3AlphaPosition = GetMarkerPosition(m_Zone3AlphaMCOM);
		if (m_Zone3BetaMCOM) m_vZone3BetaPosition = GetMarkerPosition(m_Zone3BetaMCOM);
		
		// Force replication update for clients
		Replication.BumpMe();
	}
	
	/**
	 * Get the marker position for an MCOM entity
	 * @param entity The MCOM entity
	 * @return The world position for the marker
	 */
	protected vector GetMarkerPosition(IEntity entity)
	{	
		if (!entity)
			return "0 0 0";
			
		vector position = entity.GetOrigin();
		
		// Try to get center position using bounds
		vector mins, maxs;
		entity.GetBounds(mins, maxs);
		vector localCenter = (mins + maxs) * 0.5;
		position = entity.CoordToParent(localCenter);
		
		// Offset above the MCOM for better visibility
		position[1] = position[1] + 2.0;
		position[0] = position[0] + 1.0;
		
		return position;
	}
	
	/**
	 * Initialize a single MCOM marker locally (server-side)
	 * NOTE: Disabled in favor of player-based component system
	 */
	protected void InitializeMCOMMarkerLocal(string mcomIdentifier, string letter, Color color, bool visible)
	{
		// Find the MCOM entity and add/update its 3D marker component
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		if (!mcomEntity)
			return;
		
		// Get or create the 3D marker component on the entity
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (!markerComponent)
		{
			// Component doesn't exist, this means the MCOM entity doesn't have the marker component configured
			return;
		}
		
		// Update the marker properties
		markerComponent.SetVisible(visible);
		markerComponent.SetMarkerColor(color);
	}
	
	/**
	 * Initialize all MCOM markers for a specific player
	 * NOTE: Disabled in favor of player-based component system
	 */
	protected void InitializeAllMCOMMarkersForPlayer(PlayerController playerController)
	{
		// Re-enabled to work with 3D marker components on MCOM entities
		// Initialize all MCOM markers with current visibility states and sequential letters
		InitializeMCOMMarkerLocal("Zone1Alpha", "A", Color.FromInt(ARGB(255, 255, 100, 100)), m_bZone1AlphaMarkerVisible);
		InitializeMCOMMarkerLocal("Zone1Beta", "B", Color.FromInt(ARGB(255, 255, 100, 100)), m_bZone1BetaMarkerVisible);
		InitializeMCOMMarkerLocal("Zone2Alpha", "C", Color.FromInt(ARGB(255, 100, 255, 100)), m_bZone2AlphaMarkerVisible);
		InitializeMCOMMarkerLocal("Zone2Beta", "D", Color.FromInt(ARGB(255, 100, 255, 100)), m_bZone2BetaMarkerVisible);
		InitializeMCOMMarkerLocal("Zone3Alpha", "E", Color.FromInt(ARGB(255, 100, 100, 255)), m_bZone3AlphaMarkerVisible);
		InitializeMCOMMarkerLocal("Zone3Beta", "F", Color.FromInt(ARGB(255, 100, 100, 255)), m_bZone3BetaMarkerVisible);
	}
	
	/**
	 * Initialize a single MCOM marker on server and replicate to clients
	 * @param mcomIdentifier The MCOM identifier
	 * @param letter The marker letter (A or B)
	 * @param color The marker color
	 */
	protected void InitializeMCOMMarker(string mcomIdentifier, string letter, Color color)
	{
		// Determine visibility based on current zone
		bool visible = ShouldMarkerBeVisible(mcomIdentifier);
		
		// Initialize locally on server
		Initialize3DMarkerLocal(mcomIdentifier, letter, color, visible);
		
		// Replicate to all clients
		if (Replication.IsServer())
			Rpc(RpcDo_Initialize3DMarker, mcomIdentifier, letter, color.R(), color.G(), color.B(), color.A(), visible);
	}
	
	/**
	 * Initialize a single MCOM marker for a specific player
	 * @param mcomIdentifier The MCOM identifier
	 * @param letter The marker letter (A or B)
	 * @param color The marker color
	 * @param playerController The player to send to
	 */
	protected void InitializeMCOMMarkerForPlayer(string mcomIdentifier, string letter, Color color, PlayerController playerController)
	{
		// Determine visibility based on current zone
		bool visible = ShouldMarkerBeVisible(mcomIdentifier);
		
		// For now, just use broadcast RPC since targeting specific players is complex
		// The client-side filtering will handle this appropriately
		if (Replication.IsServer())
			Rpc(RpcDo_Initialize3DMarker, mcomIdentifier, letter, color.R(), color.G(), color.B(), color.A(), visible);
	}
	
	/**
	 * Determine if a marker should be visible based on current game state
	 * @param mcomIdentifier The MCOM identifier
	 * @return True if the marker should be visible
	 */
	protected bool ShouldMarkerBeVisible(string mcomIdentifier)
	{
		// Parse the zone index from the MCOM identifier
		int zoneIndex, mcomIndex;
		if (!ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			return false;
		}
		
		// Check if this MCOM is destroyed
		if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && 
			mcomIndex < m_aMCOMDestroyed[zoneIndex].Count())
		{
			if (m_aMCOMDestroyed[zoneIndex][mcomIndex])
			{
				return false; // Hide if destroyed
			}
		}
		
		// For 3D markers: only show current active zone
		int zoneNumber = zoneIndex + 1; // Convert from 0-based index to 1-based zone number
		bool isVisible = zoneNumber == m_iCurrentZone;
		return isVisible;
	}
	
	/**
	 * Add markers for all MCOMs with zone-specific styling
	 */
	protected void AddAllMCOMMarkers()
	{
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerManager)
		{
			return;
		}
		
		// Add markers for ALL MCOMs with color coding based on zone status
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			int zoneNumber = zoneIndex + 1;
			
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				// Check if this MCOM is already destroyed
				if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && 
					mcomIndex < m_aMCOMDestroyed[zoneIndex].Count() && 
					m_aMCOMDestroyed[zoneIndex][mcomIndex])
				{
					continue;
				}
				
				// Get the trigger name and MCOM identifier
				string triggerName = GetTriggerName(zoneNumber, mcomIndex);
				string mcomIdentifier = GetMCOMIdentifier(zoneNumber, mcomIndex);
				
				// Get trigger position
				IEntity trigger = GetGame().GetWorld().FindEntityByName(triggerName);
				if (!trigger)
					continue;
				
				vector pos = trigger.GetOrigin();
				string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
				
				// Get the marker letter and icon
				string markerLetter = GetMarkerLetterFromIdentifier(mcomIdentifier);
				string iconPath = GetMarkerIconPath(markerLetter);
				string markerName = string.Format("MCOM %1", markerLetter);
				
				// Set color based on zone status
				int markerColor;
				if (zoneNumber == m_iCurrentZone)
				{
					markerColor = ARGB(255, 255, 0, 0); // Red for active zone
				}
				else if (zoneNumber < m_iCurrentZone)
				{
					markerColor = ARGB(255, 128, 128, 128); // Gray for completed zones
				}
				else
				{
					markerColor = ARGB(255, 255, 255, 0); // Yellow for future zones
				}
				
				// Add the marker with appropriate color
				playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, markerName, iconPath, 50, markerColor);
			}
		}
		
		// Check how many markers are stored
		array<string> storedMarkers = playerControllerManager.GetScriptedMarkersArray();
	}
	
	/**
	 * Get the marker letter from MCOM identifier
	 * @param mcomIdentifier The MCOM identifier (MCOMA, MCOMB, etc.)
	 * @return The single letter for the marker (A, B, C, etc.)
	 */
	protected string GetMarkerLetterFromIdentifier(string mcomIdentifier)
	{
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				return "A";
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				return "B";
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				return "C";
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				return "D";
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				return "E";
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				return "F";
			default:
				return "A"; // Fallback
		}
		
		return "";
	}
	
	/**
	 * Get the icon path for a marker letter
	 * @param letter The marker letter (A, B, C, D, E, F)
	 * @return The resource path for the icon
	 */
	protected string GetMarkerIconPath(string letter)
	{
		switch (letter)
		{
			case "A": return "{21A2A457BD0E42C1}UI/Objectives/A.edds";
			case "B": return "{7F4A8D140283CCCE}UI/Objectives/B.edds";
			case "C": return "{8B42CA8C0F5EA4BA}UI/Objectives/C.edds";
			case "D": return "{C29ADF937D98D0D0}UI/Objectives/D.edds";
			case "E": return "{3692980B7045B8A4}UI/Objectives/E.edds";
			case "F": return "{687AB148CFC836AB}UI/Objectives/F.edds";
			default: return "{21A2A457BD0E42C1}UI/Objectives/A.edds";
		}
		
		return "";
	}
	
	/**
	 * Update map markers to show zone progression
	 */
	protected void UpdateAllMCOMMarkers()
	{
		if (m_bHideMapMarkers)
			return;
		
		// Remove all existing markers first
		RemoveAllMCOMMarkers();
		
		// Add all markers with updated colors
		AddAllMCOMMarkers();
		
		// Update 3D marker colors as well
		Update3DMarkerColors();
	}
	
	/**
	 * Update map markers to show only the currently active zone
	 * This is now the primary method for managing zone-based marker visibility
	 */
	protected void UpdateActiveZoneMarkers()
	{
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerManager || m_bHideMapMarkers) 
			return;
		
		// Remove all existing MCOM markers
		RemoveAllMCOMMarkers();
		
		// Add markers for the current active zone using dynamic system
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			int zoneNumber = zoneIndex + 1;
			
			// Only show markers for current active zone
			if (zoneNumber != m_iCurrentZone)
				continue;
			
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				// Check if this MCOM is already destroyed
				if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && 
					mcomIndex < m_aMCOMDestroyed[zoneIndex].Count() && 
					m_aMCOMDestroyed[zoneIndex][mcomIndex])
				{
					continue;
				}
				
				// Get trigger and identifier for this MCOM
				string triggerName = GetTriggerName(zoneNumber, mcomIndex);
				IEntity trigger = GetGame().GetWorld().FindEntityByName(triggerName);
				if (!trigger)
					continue;
				
				// Get position and create marker
				vector pos = trigger.GetOrigin();
				string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
				
				// Get the sequential letter for this MCOM
				string mcomIdentifier = GetMCOMIdentifier(zoneNumber, mcomIndex);
				string markerLetter = GetMarkerLetterFromIdentifier(mcomIdentifier);
				string iconPath = GetMarkerIconPath(markerLetter);
				string markerName = string.Format("MCOM %1", markerLetter);
				
				// Add marker with red color for active zone
				playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, markerName, iconPath, 50, ARGB(255, 255, 0, 0));
			}
		}
	}
	
	/**
	 * Remove all MCOM markers from the map
	 */
	protected void RemoveAllMCOMMarkers()
	{
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerManager) 
			return;
		
		// Clear all markers and re-add non-MCOM markers if needed
		// This is more reliable than trying to match exact position strings
		playerControllerManager.RemoveALLScriptedMarkers();
	}
	
	/**
	 * Force marker update for all connected players
	 * This helps ensure markers appear properly on the client side
	 */
	protected void ForceMarkerUpdate()
	{
		// Notify all players that markers have been updated
		// This will trigger the client-side map marker system to refresh
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerManager)
			array<string> markers = playerControllerManager.GetScriptedMarkersArray();
	}
	
	/**
	 * Refresh map markers by removing all and re-initializing
	 * Used when MCOMs are destroyed to clean up their markers
	 */
	void RefreshMapMarkers()
	{
		Print("[CRF_RushGamemodeManager] Refreshing map markers...");
		
		// Only refresh on clients and if markers are enabled
		if (Replication.IsServer() || m_bHideMapMarkers)
		{
			Print("[CRF_RushGamemodeManager] Skipping marker refresh - server or markers hidden");
			return;
		}
		
		// Remove all existing markers and re-initialize only active ones
		RemoveAllMCOMMarkers();
		InitializeMapMarkers();
		
		Print("[CRF_RushGamemodeManager] Map markers refreshed");
	}
	
	//===================================================================================
	// 3D MARKER MANAGEMENT
	//===================================================================================
	
	/**
	 * Update 3D marker colors based on current zone status
	 */
	protected void Update3DMarkerColors()
	{
		// On server, update replicated visibility properties
		if (Replication.IsServer())
		{
			m_bZone1AlphaMarkerVisible = ShouldMarkerBeVisible("MCOMA") || ShouldMarkerBeVisible("Zone1Alpha");
			m_bZone1BetaMarkerVisible = ShouldMarkerBeVisible("MCOMB") || ShouldMarkerBeVisible("Zone1Beta");
			m_bZone2AlphaMarkerVisible = ShouldMarkerBeVisible("MCOMC") || ShouldMarkerBeVisible("Zone2Alpha");
			m_bZone2BetaMarkerVisible = ShouldMarkerBeVisible("MCOMD") || ShouldMarkerBeVisible("Zone2Beta");
			m_bZone3AlphaMarkerVisible = ShouldMarkerBeVisible("MCOME") || ShouldMarkerBeVisible("Zone3Alpha");
			m_bZone3BetaMarkerVisible = ShouldMarkerBeVisible("MCOMF") || ShouldMarkerBeVisible("Zone3Beta");
			
			// Toggle replication trigger to notify clients
			m_b3DMarkersInitialized = !m_b3DMarkersInitialized;
			Replication.BumpMe();
			
			// Also send RPC to ensure immediate client update
			Rpc(RpcDo_Update3DMarkerColors, m_iCurrentZone);
			
		}
		
		// Execute locally (both server and client)
		Update3DMarkerColorsLocal();
	}
	
	/**
	 * Local implementation to update 3D marker colors
	 */
	protected void Update3DMarkerColorsLocal()
	{
		// Hide all 3D markers first
		HideAll3DMarkers();
		
		// Show 3D markers only for the current active zone using dynamic system
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			int zoneNumber = zoneIndex + 1;
			
			// Only show markers for the current active zone
			if (zoneNumber != m_iCurrentZone)
				continue;
			
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				// Check if we have this MCOM in our dynamic arrays
				if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && 
					mcomIndex < m_aMCOMEntities[zoneIndex].Count())
				{
					IEntity mcomEntity = m_aMCOMEntities[zoneIndex][mcomIndex];
					if (mcomEntity)
					{
						// Check if this MCOM is destroyed
						bool isDestroyed = m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && 
										   mcomIndex < m_aMCOMDestroyed[zoneIndex].Count() && 
										   m_aMCOMDestroyed[zoneIndex][mcomIndex];
						
						// Show all markers unless destroyed
						bool shouldShow = !isDestroyed;
						
						// Set color for active zone (always red)
						Color markerColor = Color.FromInt(ARGB(255, 255, 0, 0)); // Red for active zone
						
						Update3DMarkerForMCOM(mcomEntity, zoneNumber, shouldShow, markerColor);
					}
				}
			}
		}
		
		// Also update legacy markers for backward compatibility - use configuration-aware zone numbers
		if (m_Zone1AlphaMCOM)
		{
			int actualZone = GetZoneNumber("MCOMA");
			UpdateLegacyMarkerWithColor(m_Zone1AlphaMCOM, actualZone, m_bZone1AlphaMarkerVisible);
		}
		if (m_Zone1BetaMCOM)
		{
			int actualZone = GetZoneNumber("MCOMB");
			UpdateLegacyMarkerWithColor(m_Zone1BetaMCOM, actualZone, m_bZone1BetaMarkerVisible);
		}
		if (m_Zone2AlphaMCOM)
		{
			int actualZone = GetZoneNumber("MCOMC");
			UpdateLegacyMarkerWithColor(m_Zone2AlphaMCOM, actualZone, m_bZone2AlphaMarkerVisible);
		}
		if (m_Zone2BetaMCOM)
		{
			int actualZone = GetZoneNumber("MCOMD");
			UpdateLegacyMarkerWithColor(m_Zone2BetaMCOM, actualZone, m_bZone2BetaMarkerVisible);
		}
		if (m_Zone3AlphaMCOM)
		{
			int actualZone = GetZoneNumber("MCOME");
			UpdateLegacyMarkerWithColor(m_Zone3AlphaMCOM, actualZone, m_bZone3AlphaMarkerVisible);
		}
		if (m_Zone3BetaMCOM)
		{
			int actualZone = GetZoneNumber("MCOMF");
			UpdateLegacyMarkerWithColor(m_Zone3BetaMCOM, actualZone, m_bZone3BetaMarkerVisible);
		}
	}
	
	/**
	 * Hide all 3D markers
	 */
	protected void HideAll3DMarkers()
	{
		// Hide all markers using dynamic system
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && 
					mcomIndex < m_aMCOMEntities[zoneIndex].Count())
				{
					IEntity mcomEntity = m_aMCOMEntities[zoneIndex][mcomIndex];
					if (mcomEntity)
					{
						CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
						if (marker)
							marker.SetVisible(false);
					}
				}
			}
		}
		
		// Also hide legacy markers for backward compatibility
		if (m_Zone1AlphaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone1AlphaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) 
				marker.SetVisible(false);
		}
		if (m_Zone1BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone1BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) 
				marker.SetVisible(false);
		}
		if (m_Zone2AlphaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone2AlphaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) 
				marker.SetVisible(false);
		}
		if (m_Zone2BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone2BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) 
				marker.SetVisible(false);
		}
		if (m_Zone3AlphaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone3AlphaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker)
				marker.SetVisible(false);
		}
		if (m_Zone3BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone3BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) 
				marker.SetVisible(false);
		}
	}
	
	/**
	 * Update 3D marker for a specific MCOM
	 * @param mcomEntity The MCOM entity
	 * @param zoneNumber Zone number (1-3)
	 * @param isVisible Whether the marker should be visible
	 */
	protected void Update3DMarkerForMCOM(IEntity mcomEntity, int zoneNumber, bool isVisible, Color markerColor)
	{
		if (!mcomEntity)
			return;
		
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (!markerComponent)
			return;
		
		// Set the marker color and visibility
		markerComponent.SetMarkerColor(markerColor);
		markerComponent.SetVisible(isVisible);
	}
	
	/**
	 * Update legacy marker with color based on zone status
	 * @param mcomEntity The MCOM entity
	 * @param zoneNumber The zone number this MCOM belongs to
	 * @param baseVisibility The base visibility state
	 */
	protected void UpdateLegacyMarkerWithColor(IEntity mcomEntity, int zoneNumber, bool baseVisibility)
	{
		if (!mcomEntity)
			return;
		
		// Only show markers for current active zone
		bool shouldShow = (zoneNumber == m_iCurrentZone) && baseVisibility;
		
		// Determine color based on zone status
		Color markerColor;
		if (zoneNumber == m_iCurrentZone)
		{
			markerColor = Color.FromInt(ARGB(255, 255, 0, 0)); // Red for active zone
		}
		else if (zoneNumber < m_iCurrentZone)
		{
			markerColor = Color.FromInt(ARGB(255, 128, 128, 128)); // Gray for completed zones
		}
		else
		{
			markerColor = Color.FromInt(ARGB(255, 255, 255, 0)); // Yellow for future zones
		}
		
		Update3DMarkerForMCOM(mcomEntity, zoneNumber, shouldShow, markerColor);
	}
	
	//===================================================================================
	// MCOM PLANTING AND DEFUSING
	//===================================================================================
	
	/**
	 * Handle MCOM planting/defusing
	 * Called from client actions via RPC
	 * @param mcomIdentifier Which MCOM is being interacted with
	 * @param isPlanted True for planting, false for defusing
	 */
	void ToggleMCOMPlanted(string mcomIdentifier, bool isPlanted) 
	{
		// Only execute countdown time logic on the server
		if (Replication.IsServer())
		{
			if (isPlanted)
			{
				// Safety check: Don't allow planting if another bomb is already active
				if (m_bCountdownActive)
					return;
				
				// Update planted status
				SetMCOMPlantedStatus(mcomIdentifier, isPlanted);
				
				// Start countdown
				m_bCountdownActive = true;
				m_sActiveMCOM = mcomIdentifier;
				Print("[CRF_RushGamemodeManager] ===== COUNTDOWN STARTED ===== for MCOM: " + mcomIdentifier + " Timer: " + m_iMCOMTimer + " seconds");
				
				// Always use full timer (no time saving)
				m_iCountdownTimeRemaining = m_iMCOMTimer;
				
				// Play plant sound
				m_sSoundString = "{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav";
				
				// Start global bomb ticking sound
				StartBombTickingSound();
				
				// Start flashing red marker for planted MCOM
				StartFlashingMarker(mcomIdentifier);
				
				// Show plant message
				string zoneName = GetZoneDisplayName(mcomIdentifier);
				string siteName = GetSiteDisplayName(mcomIdentifier);
				m_sMessageContent = string.Format("Attackers have armed %1 MCOM %2!|15|", zoneName, siteName);
			}
			else
			{
				// Update planted status
				SetMCOMPlantedStatus(mcomIdentifier, isPlanted);
				
				// Stop countdown
				m_bCountdownActive = false;
				m_sActiveMCOM = "";
				
				// Stop global bomb ticking sound
				StopBombTickingSound();
				
				// Stop flashing marker for defused MCOM
				StopFlashingMarker(mcomIdentifier);
				
				// Show defuse message
				string zoneName = GetZoneDisplayName(mcomIdentifier);
				string siteName = GetSiteDisplayName(mcomIdentifier);
				m_sMessageContent = string.Format("Defenders have defused the bomb at %1 %2!|15|", zoneName, siteName);
			}
		}
		else
		{
			// Client-side: Just update planted status for UI purposes
			SetMCOMPlantedStatus(mcomIdentifier, isPlanted);
			
			if (isPlanted)
			{
				// Show simple plant message for client
				string zoneName = GetZoneDisplayName(mcomIdentifier);
				string siteName = GetSiteDisplayName(mcomIdentifier);
				m_sMessageContent = string.Format("Attackers have armed %1 MCOM %2!|15|", zoneName, siteName);
			}
			else
			{
				// Show simple defuse message for client
				string zoneName = GetZoneDisplayName(mcomIdentifier);
				string siteName = GetSiteDisplayName(mcomIdentifier);
				m_sMessageContent = string.Format("Defenders have defused the bomb at %1 %2!|15|", zoneName, siteName);
			}
		}
		
		Replication.BumpMe();
		ShowMessage();
		PlaySound();
	}
	
	/**
	 * Set the planted status for a specific MCOM
	 * @param mcomIdentifier The MCOM identifier
	 * @param isPlanted The planted status
	 */
	protected void SetMCOMPlantedStatus(string mcomIdentifier, bool isPlanted)
	{
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				m_bZone1AlphaPlanted = isPlanted; 
				break;
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				m_bZone1BetaPlanted = isPlanted; 
				break;
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				m_bZone2AlphaPlanted = isPlanted; 
				break;
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				m_bZone2BetaPlanted = isPlanted; 
				break;
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				m_bZone3AlphaPlanted = isPlanted; 
				break;
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				m_bZone3BetaPlanted = isPlanted; 
				break;
		}
	}
	
	/**
	 * Get the planted status for a specific MCOM
	 * @param mcomIdentifier The MCOM identifier
	 * @return True if planted, false otherwise
	 */
	bool GetMCOMPlantedStatus(string mcomIdentifier)
	{
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				return m_bZone1AlphaPlanted;
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				return m_bZone1BetaPlanted;
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				return m_bZone2AlphaPlanted;
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				return m_bZone2BetaPlanted;
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				return m_bZone3AlphaPlanted;
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				return m_bZone3BetaPlanted;
		}
		return false;
	}
	
	//===================================================================================
	// COUNTDOWN AND DESTRUCTION
	//===================================================================================
	
	/**
	 * Countdown timer for MCOM destruction
	 * Called every second when a bomb is planted
	 */
	protected void CountdownTimer()
	{
		// Check if countdown should continue
		if (!m_bCountdownActive || m_sActiveMCOM.IsEmpty()) 
			return;
		
		// Decrement timer
		m_iCountdownTimeRemaining--;
		
		// Update countdown display
		string zoneName = GetZoneDisplayName(m_sActiveMCOM);
		string siteName = GetSiteDisplayName(m_sActiveMCOM);
		m_sMessageContent = string.Format("%1 %2: %3", zoneName, siteName, SCR_FormatHelper.FormatTime(m_iCountdownTimeRemaining));
		
		// Check if time is up
		if (m_iCountdownTimeRemaining <= 0) 
		{
			Print("[CRF_RushGamemodeManager] ===== COUNTDOWN TIMER EXPIRED ===== Calling MCOMDestroyed for: " + m_sActiveMCOM);
			MCOMDestroyed(m_sActiveMCOM);
			
			// Stop countdown
			m_bCountdownActive = false;
			m_sActiveMCOM = "";
		}
		
		Replication.BumpMe();
		ShowMessage();
	}
	
	/**
	 * Handle MCOM destruction
	 * @param mcomIdentifier The destroyed MCOM identifier
	 */
	protected void MCOMDestroyed(string mcomIdentifier) 
	{
		// This method should only run on the server
		if (!Replication.IsServer())
			return;
		
		Print("[CRF_RushGamemodeManager] ===== MCOMDestroyed START ===== for: " + mcomIdentifier);
		
		// DEBUG: Test entity lookup immediately
		IEntity testEntity = GetMCOMEntity(mcomIdentifier);
		if (testEntity)
		{
			Print("[CRF_RushGamemodeManager] DEBUG: Entity lookup successful - ID: " + testEntity.GetID());
		}
		else
		{
			Print("[CRF_RushGamemodeManager] DEBUG: Entity lookup FAILED for identifier: " + mcomIdentifier);
			
			// Try all possible identifiers as debug
			array<string> testIdentifiers = {"MCOMA", "MCOMB", "MCOMC", "MCOMD", "MCOME", "MCOMF", "Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
			for (int i = 0; i < testIdentifiers.Count(); i++)
			{
				IEntity debugEntity = GetMCOMEntity(testIdentifiers[i]);
				if (debugEntity)
				{
					Print("[CRF_RushGamemodeManager] DEBUG: Found entity for identifier: " + testIdentifiers[i] + " ID: " + debugEntity.GetID());
				}
			}
		}
		
		// Mark MCOM as destroyed
		SetMCOMDestroyedStatus(mcomIdentifier, true);
		
		// Stop bomb ticking sound since MCOM is destroyed
		StopBombTickingSound();
		
		// Stop flashing marker since MCOM is destroyed
		StopFlashingMarker(mcomIdentifier);
		
		// Get the MCOM entity before we start the deletion process
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		if (mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] Found MCOM entity to destroy: " + mcomIdentifier + " ID: " + mcomEntity.GetID());
			
			// Hide the 3D marker immediately
			CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
			if (markerComponent)
			{
				markerComponent.SetVisible(false);
			}
		}
		else
		{
			Print("[CRF_RushGamemodeManager] MCOM entity not found: " + mcomIdentifier, LogLevel.WARNING);
		}
		
		// Set replicated property to trigger client-side deletion via OnMCOMDestroyedReplicated
		m_sReplicatedDestroyedMCOM = mcomIdentifier;
		Print("[CRF_RushGamemodeManager] Set replicated property m_sReplicatedDestroyedMCOM = " + mcomIdentifier);
		
		// Set legacy property for backward compatibility
		m_sDestroyedMCOMString = mcomIdentifier;
		
		// Play destruction sound
		m_sSoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		
		// Show destruction message
		string zoneName = GetZoneDisplayName(mcomIdentifier);
		string siteName = GetSiteDisplayName(mcomIdentifier);
		m_sMessageContent = string.Format("%1 %2 DESTROYED!|15|", zoneName, siteName);
		
		// Check if zone is cleared
		CheckZoneCleared(GetZoneNumber(mcomIdentifier));
		
		// Force immediate replication of destruction status
		Replication.BumpMe();
		MCOMDestroyedClient();
		ShowMessage();
		PlaySound();
		
		// Schedule server-side entity deletion with delay to ensure clients process effects first
		if (mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] Scheduling server entity deletion for: " + mcomIdentifier + " in 3 seconds");
			GetGame().GetCallqueue().CallLater(DeleteMCOMEntityServer, 3000, false, mcomEntity, mcomIdentifier);
		}
		else
		{
			Print("[CRF_RushGamemodeManager] WARNING: Cannot schedule entity deletion - entity not found for: " + mcomIdentifier);
		}
		
		Print("[CRF_RushGamemodeManager] ===== MCOMDestroyed END ===== for: " + mcomIdentifier);
	}
	
	//------------------------------------------------------------------------------------
	// OnMCOMDestroyedReplicated - Called when MCOM destruction is replicated to clients
	//------------------------------------------------------------------------------------
	void OnMCOMDestroyedReplicated()
	{
		if (m_sReplicatedDestroyedMCOM == "") return;
		
		Print(string.Format("[CRF_Rush_Game] OnMCOMDestroyedReplicated - Processing destruction of: %1 (Server: %2)", m_sReplicatedDestroyedMCOM, Replication.IsServer()));
		
		// Only process deletion on clients - server handles its own deletion
		if (Replication.IsServer())
		{
			Print("[CRF_Rush_Game] OnMCOMDestroyedReplicated - Skipping on server");
			return;
		}
		
		// Delay the deletion slightly to ensure the entity is ready for deletion
		GetGame().GetCallqueue().CallLater(ProcessClientMCOMDeletion, 1000, false, m_sReplicatedDestroyedMCOM);
	}
	
	//------------------------------------------------------------------------------------
	// ProcessClientMCOMDeletion - Handle the actual client-side entity deletion
	//------------------------------------------------------------------------------------
	protected void ProcessClientMCOMDeletion(string mcomIdentifier)
	{
		Print(string.Format("[CRF_Rush_Game] ProcessClientMCOMDeletion - Starting deletion process for: %1", mcomIdentifier));
		Print(string.Format("[CRF_Rush_Game] ProcessClientMCOMDeletion - Running on client: %1", !Replication.IsServer()));
		
		// Find the entity using the proper MCOM entity lookup method
		IEntity entity = GetMCOMEntity(mcomIdentifier);
		if (entity)
		{
			Print(string.Format("[CRF_Rush_Game] Client found MCOM entity: %1 (ID: %2), deleting...", mcomIdentifier, entity.GetID()));
			
			// Hide 3D marker first
			CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(entity.FindComponent(CRF_Rush_3DMarkerComponent));
			if (markerComponent)
				markerComponent.SetVisible(false);
			
			// Clean up references
			CleanupMCOMReference(mcomIdentifier);
			
			// Delete all children
			DeleteMCOMChildren(entity);
			
			// Delete the entity
			delete entity;
			Print(string.Format("[CRF_Rush_Game] Successfully deleted MCOM entity: %1", mcomIdentifier));
		}
		else
		{
			Print(string.Format("[CRF_Rush_Game] Client could not find MCOM entity: %1 - may already be deleted", mcomIdentifier), LogLevel.WARNING);
			
			// Even if entity is not found, still clean up references
			CleanupMCOMReference(mcomIdentifier);
			
			// Try alternative entity lookup methods as debugging
			string entityName = GetMCOMEntityName(mcomIdentifier);
			Print(string.Format("[CRF_Rush_Game] Tried entity name: %1", entityName));
			
			IEntity altEntity = GetGame().GetWorld().FindEntityByName(entityName);
			if (altEntity)
			{
				Print(string.Format("[CRF_Rush_Game] Found entity by name: %1 (ID: %2)", entityName, altEntity.GetID()));
			}
			else
			{
				Print(string.Format("[CRF_Rush_Game] No entity found by name: %1", entityName));
			}
		}
	}
	
	protected void DeleteMCOMChildren(IEntity mcomEntity)
	{
		int num = 0;
		IEntity child = mcomEntity.GetChildren();
		while (child)
		{
			IEntity childToDelete = child;
			num++;
			DeleteMCOMChildren(child); //Recursivity is fun
			child = child.GetSibling();
			delete childToDelete;
		}
	}
	
	/**
	 * Server-authoritative entity deletion
	 * @param mcomEntity The MCOM entity to delete
	 * @param mcomIdentifier The MCOM identifier for logging
	 */
	protected void DeleteMCOMEntityServer(IEntity mcomEntity, string mcomIdentifier)
	{
		if (!Replication.IsServer())
		{
			Print("[CRF_RushGamemodeManager] DeleteMCOMEntityServer called on non-server", LogLevel.WARNING);
			return;
		}
		
		if (!mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] DeleteMCOMEntityServer: Entity is null for " + mcomIdentifier, LogLevel.WARNING);
			return;
		}
		
		Print("[CRF_RushGamemodeManager] ===== DeleteMCOMEntityServer START ===== for " + mcomIdentifier + " ID: " + mcomEntity.GetID());
		
		// Send RPC to all clients to handle entity and marker deletion FIRST
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
		{
			Print("[CRF_RushGamemodeManager] Sending RPC to clients for MCOM deletion: " + mcomIdentifier);
			broadcastManager.DeleteRushMCOMEntity(mcomIdentifier);
			Print("[CRF_RushGamemodeManager] RPC sent successfully to clients for MCOM deletion: " + mcomIdentifier);
		}
		else
		{
			Print("[CRF_RushGamemodeManager] CRITICAL ERROR: Could not find RplBroadcastManager!", LogLevel.ERROR);
		}
		
		// Set replicated property to trigger client-side deletion as backup
		m_sReplicatedDestroyedMCOM = mcomIdentifier;
		Replication.BumpMe(); // Force immediate replication
		Print("[CRF_RushGamemodeManager] Set replicated property and forced replication for: " + mcomIdentifier);
		
		// Clean up server-side references
		CleanupMCOMReference(mcomIdentifier);
		
		// Delay server entity deletion to give clients time to process RPC
		Print("[CRF_RushGamemodeManager] Scheduling final server entity deletion in 2 seconds for: " + mcomIdentifier);
		GetGame().GetCallqueue().CallLater(DeleteMCOMEntityFinal, 2000, false, mcomEntity, mcomIdentifier);
		
		Print("[CRF_RushGamemodeManager] ===== DeleteMCOMEntityServer END ===== for " + mcomIdentifier);
	}
	
	//------------------------------------------------------------------------------------
	// Final entity deletion on server after client RPC processing
	//------------------------------------------------------------------------------------
	protected void DeleteMCOMEntityFinal(IEntity mcomEntity, string mcomIdentifier)
	{
		if (!mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] DeleteMCOMEntityFinal: Entity already deleted for " + mcomIdentifier);
			return;
		}

		// Delete all children
		DeleteMCOMChildren(mcomEntity);
		
		// Delete the entity
		delete mcomEntity;
		
		Print("[CRF_RushGamemodeManager] Server entity deletion completed for: " + mcomIdentifier);
	}

	/**
	 * Set the destroyed status for a specific MCOM
	 * @param mcomIdentifier The MCOM identifier
	 * @param isDestroyed The destroyed status
	 */
	protected void SetMCOMDestroyedStatus(string mcomIdentifier, bool isDestroyed)
	{
		// Update dynamic arrays (primary system)
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && mcomIndex < m_aMCOMDestroyed[zoneIndex].Count())
				m_aMCOMDestroyed[zoneIndex][mcomIndex] = isDestroyed;
		}
		
		// Update legacy variables for replication (for clients that expect these)
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				m_bZone1Alpha = isDestroyed; 
				break;
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				m_bZone1Beta = isDestroyed; 
				break;
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				m_bZone2Alpha = isDestroyed; 
				break;
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				m_bZone2Beta = isDestroyed; 
				break;
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				m_bZone3Alpha = isDestroyed; 
				break;
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				m_bZone3Beta = isDestroyed; 
				break;
		}
	}
	
	/**
	 * Public wrapper to set MCOM destroyed status from RPC handlers
	 * @param mcomIdentifier The MCOM identifier
	 * @param isDestroyed The destroyed status
	 */
	void SetMCOMDestroyedStatusFromRPC(string mcomIdentifier, bool isDestroyed)
	{
		SetMCOMDestroyedStatus(mcomIdentifier, isDestroyed);
		Print("[CRF_RushGamemodeManager] SetMCOMDestroyedStatusFromRPC called for: " + mcomIdentifier + " destroyed: " + isDestroyed);
	}
	
	/**
	 * Parse MCOM identifier to get zone and MCOM indices
	 * @param mcomIdentifier The MCOM identifier (e.g., "Zone1Alpha")
	 * @param zoneIndex Output zone index (0-based)
	 * @param mcomIndex Output MCOM index (0-based)
	 * @return True if parsing was successful
	 */
	protected bool ParseMCOMIdentifier(string mcomIdentifier, out int zoneIndex, out int mcomIndex)
	{
		// Handle sequential identifiers (preferred format)
		switch (mcomIdentifier)
		{
			case "MCOMA":
				zoneIndex = 0; mcomIndex = 0; 
				return true;
			case "MCOMB":
				if (m_iMCOMsPerZone == 1) { zoneIndex = 1; mcomIndex = 0; }
				else { zoneIndex = 0; mcomIndex = 1; }
				return true;
			case "MCOMC":
				if (m_iMCOMsPerZone == 1) { zoneIndex = 2; mcomIndex = 0; } // Zone 3, Index 0 for 1 MCOM per zone
				else { zoneIndex = 1; mcomIndex = 0; } // Zone 2, Index 0 for 2 MCOMs per zone
				return true;
			case "MCOMD":
				zoneIndex = 1; mcomIndex = 1; 
				return true;
			case "MCOME":
				if (m_iMCOMsPerZone == 1) { zoneIndex = 2; mcomIndex = 0; }
				else { zoneIndex = 2; mcomIndex = 0; }
				return true;
			case "MCOMF":
				zoneIndex = 2; mcomIndex = 1; 
				return true;
		}
		
		// Legacy compatibility - only for backward compatibility with existing missions
		if (mcomIdentifier.Contains("Zone1"))
			zoneIndex = 0;
		else if (mcomIdentifier.Contains("Zone2"))
			zoneIndex = 1;
		else if (mcomIdentifier.Contains("Zone3"))
			zoneIndex = 2;
		else
			return false;
		
		if (mcomIdentifier.Contains("Alpha"))
			mcomIndex = 0;
		else if (mcomIdentifier.Contains("Beta"))
			mcomIndex = 1;
		else
			return false;
		
		return true;
	}
	
	/**
	 * Check if a zone is completely cleared (all MCOMs destroyed)
	 * @param zoneNumber The zone number (1-3)
	 */
	protected void CheckZoneCleared(int zoneNumber)
	{
		bool zoneCleared = IsZoneCleared(zoneNumber);
		
		if (zoneCleared)
		{
			// Set appropriate message and handle zone progression
			if (zoneNumber < m_iNumberOfZones)
			{
				int oldZone = m_iCurrentZone;
				m_iCurrentZone = zoneNumber + 1;
				Print("[CheckZoneCleared] Zone transition: " + oldZone + " → " + m_iCurrentZone);
				m_sMessageContent = string.Format("Zone %1 Cleared! Zone %2 is now unlocked.|20|Attackers advance!", zoneNumber, zoneNumber + 1);
			}
			else
			{
				m_sMessageContent = "All zones cleared! Attackers have won!|60|Attacker Victory!";
			}
		}
		
		if (zoneCleared && zoneNumber < m_iNumberOfZones)
		{
			// First teleport the flags if dynamic respawns enabled
			if (m_bEnableDynamicRespawns && m_aZoneRespawnConfigs && !m_aZoneRespawnConfigs.IsEmpty())
			{
				Print(string.Format("[CRF_Rush] Zone %1 fully cleared, scheduling respawn point changes for zone %2", zoneNumber, zoneNumber));
				GetGame().GetCallqueue().CallLater(EnableZoneRespawnPoints, 250, false, zoneNumber);
			}
			
			// Update both map markers and 3D markers for new active zone
			Print("[CheckZoneCleared] Updating markers for new active zone: " + m_iCurrentZone);
			GetGame().GetCallqueue().CallLater(UpdateAllMCOMMarkers, 500, false);
			
			// Delay zone status update (respawn wave) until after teleport completes
			GetGame().GetCallqueue().CallLater(UpdateZoneStatus, 500, false);
		}
		
		Replication.BumpMe();
		ShowMessage();
	}
	
	/**
	 * Check if a zone is completely cleared (all configured MCOMs destroyed)
	 * @param zoneNumber The zone number (1-3)
	 * @return True if all MCOMs in the zone are destroyed
	 */
	protected bool IsZoneCleared(int zoneNumber)
	{
		int zoneIndex = zoneNumber - 1;
		
		// Check dynamic arrays first
		if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count())
		{
			bool allDestroyed = true;
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone && mcomIndex < m_aMCOMDestroyed[zoneIndex].Count(); mcomIndex++)
			{
				if (!m_aMCOMDestroyed[zoneIndex][mcomIndex])
				{
					allDestroyed = false;
					break;
				}
			}
			return allDestroyed;
		}
		
		// Fallback to legacy checks for backward compatibility
		switch (zoneNumber)
		{
			case 1:
			{
				if (m_iMCOMsPerZone == 1)
					return m_bZone1Alpha;
				else
					return m_bZone1Alpha && m_bZone1Beta;
				break;
			}
			case 2:
			{
				if (m_iMCOMsPerZone == 1)
					return m_bZone2Alpha;
				else
					return m_bZone2Alpha && m_bZone2Beta;
				break;
			}
			case 3:
			{
				if (m_iMCOMsPerZone == 1)
					return m_bZone3Alpha;
				else
					return m_bZone3Alpha && m_bZone3Beta;
				break;
			}
		}
		
		return false;
	}
	
	/**
	 * Updates the current zone and active zone count
	 */
	void UpdateZoneStatus()
	{
		// Update active zone count
		m_iActiveZoneCount = m_iCurrentZone;
			
		// Update map markers for new zone
		if (!m_bHideMapMarkers)
			GetGame().GetCallqueue().CallLater(UpdateAllMCOMMarkers, 500, false);
		
		// Update 3D markers for new zone
		GetGame().GetCallqueue().CallLater(Update3DMarkerColors, 600, false);
		
		// Respawn players for zone advance
		RespawnPlayersForZoneAdvance();
	}
	
	/**
	 * Respawn players when a zone is cleared and the next zone opens
	 */
	protected void RespawnPlayersForZoneAdvance()
	{
		// Get the respawn manager
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (!respawnManager)
			return;

		// Trigger respawn waves for both attacking and defending sides
		respawnManager.RespawnSide(m_AttackingSide);
		respawnManager.RespawnSide(m_DefendingSide);
		
	}
	
	//===================================================================================
	// CLIENT-SIDE EFFECTS
	//===================================================================================
	
	/**
	 * Handle MCOM destruction effects on client
	 * Called via RPC replication
	 */
	protected void MCOMDestroyedClient() 
	{
		IEntity destroyedMCOMEntity = GetMCOMEntity(m_sDestroyedMCOMString);
		if (!destroyedMCOMEntity)
			return;
		
		// Store the position before entity deletion occurs
		vector explosionPosition = destroyedMCOMEntity.GetOrigin();
		
		// Remove map marker for destroyed MCOM
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerManager && !m_bHideMapMarkers)
		{
			// Refresh all MCOM markers to reflect the new state
			GetGame().GetCallqueue().CallLater(UpdateAllMCOMMarkers, 100, false);
		}
		
		// Spawn explosion effects using stored position
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = explosionPosition;
		
		// Delayed explosion effects for better synchronization - use position instead of entity
		GetGame().GetCallqueue().CallLater(CreateExplosionEffectsAtPosition, 385, false, spawnParams);
		
		// Initial explosion
		GetGame().SpawnEntityPrefab(Resource.Load("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et"), GetGame().GetWorld(), spawnParams);
	}
	
	/**
	 * Create delayed explosion effects at a specific position
	 * @param spawnParams Spawn parameters for effects
	 */
	protected void CreateExplosionEffectsAtPosition(EntitySpawnParams spawnParams)
	{
		// Spawn rubble
		GetGame().SpawnEntityPrefab(Resource.Load("{5A81BD9171FC3B07}Prefabs/Structures/Ruins/HouseRuins/HouseRuin_01/HouseRuin_01_BrickPile_Big.et"), GetGame().GetWorld(), spawnParams);
		
		// Secondary explosion
		GetGame().SpawnEntityPrefab(Resource.Load("{BCE4E0823FCFBCB7}Prefabs/Weapons/Warheads/Explosions/Explosion_AmmoRack_Large.et"), GetGame().GetWorld(), spawnParams);
		
		// Fire effects
		GetGame().SpawnEntityPrefab(Resource.Load("{4BE47BA2B7E3877E}Prefabs/Systems/Fire/Wrapper_Fire_Large_Damage.et"), GetGame().GetWorld(), spawnParams);
		
		// NOTE: Entity deletion is now handled by RPC system via CRF_RplBroadcastManager.DeleteRushMCOMEntity()
		// This ensures proper client-server synchronization of entity deletion
	}
	
	//===================================================================================
	// UI AND MESSAGING
	//===================================================================================
	
	/**
	 * Display popup message to players
	 * Called via RPC replication
	 */
	protected void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		if (!m_PopUpNotification)
			return;
		
		m_sStoredMessageContent = m_sMessageContent;
		
		// Parse message format: "message|duration|submessage"
		array<string> messageParts = {};
		m_sMessageContent.Split("|", messageParts, false);
		
		if (messageParts.Count() < 2)
			return;
		
		string mainMessage = messageParts[0];
		float duration = messageParts[1].ToFloat();
		string subMessage;
		if (messageParts.Count() > 2)
			subMessage = messageParts[2];
		else
			subMessage = "";
		
		m_PopUpNotification.PopupMsg(mainMessage, duration, subMessage);
	}
	
	/**
	 * Play sound effect
	 * Called via RPC replication
	 */
	protected void PlaySound()
	{
		if (!m_sSoundString.IsEmpty())
		{
			// Use AudioSystem for non-3D game event sounds (explosions, notifications, etc.)
			AudioSystem.PlaySound(m_sSoundString);
		}
	}
	
	/**
	 * Update 3D markers from replicated data
	 * Called when replicated marker data changes
	 */
	protected void Update3DMarkersFromReplication()
	{
		// Don't process on server (server manages the data)
		if (Replication.IsServer())
		{
			return;
		}
		
		// Initialize markers with replicated visibility data
		InitializeMarkersFromReplicatedData();
		
		// Notify player marker component about the update
		NotifyPlayerMarkerComponents();
	}
	
	/**
	 * Initialize markers based on replicated data (client-side only)
	 */
	protected void InitializeMarkersFromReplicatedData()
	{
		// Create markers for each zone based on replicated visibility and positions
		// Now uses sequential letters A-F instead of Alpha/Beta per zone
		array<string> zones = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
		array<string> letters = {"A", "B", "C", "D", "E", "F"};
		array<ref Color> colors = {
			Color.FromRGBA(1.0, 0.0, 0.0, 1.0), Color.FromRGBA(1.0, 0.0, 0.0, 1.0),
			Color.FromRGBA(1.0, 0.5, 0.0, 1.0), Color.FromRGBA(1.0, 0.5, 0.0, 1.0),
			Color.FromRGBA(1.0, 1.0, 0.0, 1.0), Color.FromRGBA(1.0, 1.0, 0.0, 1.0)
		};
		array<bool> visibility = {
			m_bZone1AlphaMarkerVisible, m_bZone1BetaMarkerVisible,
			m_bZone2AlphaMarkerVisible, m_bZone2BetaMarkerVisible,
			m_bZone3AlphaMarkerVisible, m_bZone3BetaMarkerVisible
		};
		array<vector> positions = {
			m_vZone1AlphaPosition, m_vZone1BetaPosition,
			m_vZone2AlphaPosition, m_vZone2BetaPosition,
			m_vZone3AlphaPosition, m_vZone3BetaPosition
		};
		
		for (int i = 0; i < zones.Count(); i++)
		{
			string zoneId = zones[i];
			string letter = letters[i];
			Color color = colors[i];
			bool visible = visibility[i];
			vector position = positions[i];
			
			// Note: Marker creation is now handled by CRF_Rush_3DMarkerComponent on MCOM entities
			// No need to create markers here - they're created by the component system
		}
	}
	
	/**
	 * Notify all player marker components that data has been updated
	 */
	protected void NotifyPlayerMarkerComponents()
	{
		// Skip on server
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Update visibility of 3D marker components on MCOM entities
		Update3DMarkerColorsLocal();
	}
	
	//===================================================================================
	// UTILITY FUNCTIONS
	//===================================================================================
	
	/**
	 * Get MCOM entity by identifier
	 * @param mcomIdentifier The MCOM identifier
	 * @return The MCOM entity or null
	 */
	IEntity GetMCOMEntity(string mcomIdentifier)
	{
		Print("[CRF_RushGamemodeManager] GetMCOMEntity searching for: " + mcomIdentifier);
		IEntity mcomEntity;
		
		// Try to get from dynamic arrays first
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			Print("[CRF_RushGamemodeManager] GetMCOMEntity parsed identifier - zone: " + zoneIndex + " mcom: " + mcomIndex);
			if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && mcomIndex < m_aMCOMEntities[zoneIndex].Count())
			{
				mcomEntity = m_aMCOMEntities[zoneIndex][mcomIndex];
				if (mcomEntity)
				{
					Print("[CRF_RushGamemodeManager] GetMCOMEntity found in dynamic array: " + mcomIdentifier + " ID: " + mcomEntity.GetID());
					return mcomEntity;
				}
			}
		}
		
		// Handle sequential MCOM identifiers (A-F system)
		Print("[CRF_RushGamemodeManager] GetMCOMEntity trying sequential lookup for: " + mcomIdentifier);
		switch (mcomIdentifier)
		{
			case "MCOMA":
				mcomEntity = m_Zone1AlphaMCOM;
				break;
			case "MCOMB":
				mcomEntity = m_Zone1BetaMCOM;
				break;
			case "MCOMC":
				mcomEntity = m_Zone2AlphaMCOM;
				break;
			case "MCOMD":
				mcomEntity = m_Zone2BetaMCOM;
				break;
			case "MCOME":
				mcomEntity = m_Zone3AlphaMCOM;
				break;
			case "MCOMF":
				mcomEntity = m_Zone3BetaMCOM;
				break;
		}
		
		if (mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] GetMCOMEntity found via sequential lookup: " + mcomIdentifier + " ID: " + mcomEntity.GetID());
			return mcomEntity;
		}
		
		// Fallback to legacy member variables for backward compatibility
		Print("[CRF_RushGamemodeManager] GetMCOMEntity trying legacy lookup for: " + mcomIdentifier);
		if (!mcomEntity)
		{
			switch (mcomIdentifier)
			{
				case "Zone1Alpha":
				case "zone1_alpha":
					mcomEntity = m_Zone1AlphaMCOM;
					break;
				case "Zone1Beta":
				case "zone1_beta":
					mcomEntity = m_Zone1BetaMCOM;
					break;
				case "Zone2Alpha":
				case "zone2_alpha":
					mcomEntity = m_Zone2AlphaMCOM;
					break;
				case "Zone2Beta":
				case "zone2_beta":
					mcomEntity = m_Zone2BetaMCOM;
					break;
				case "Zone3Alpha":
				case "zone3_alpha":
					mcomEntity = m_Zone3AlphaMCOM;
					break;
				case "Zone3Beta":
				case "zone3_beta":
					mcomEntity = m_Zone3BetaMCOM;
					break;
			}
		}
		
		if (mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] GetMCOMEntity found via legacy lookup: " + mcomIdentifier + " ID: " + mcomEntity.GetID());
			return mcomEntity;
		}
		
		// If still null, try to find by entity name
		Print("[CRF_RushGamemodeManager] GetMCOMEntity trying FindEntityByName for: " + mcomIdentifier);
		if (!mcomEntity)
		{
			string entityName = GetMCOMEntityName(mcomIdentifier);
			Print("[CRF_RushGamemodeManager] GetMCOMEntity searching for entity name: " + entityName);
			if (!entityName.IsEmpty())
			{
				mcomEntity = GetGame().GetWorld().FindEntityByName(entityName);
				if (mcomEntity)
				{
					Print("[CRF_RushGamemodeManager] GetMCOMEntity found via FindEntityByName: " + entityName + " ID: " + mcomEntity.GetID());
				}
			}
		}
		
		if (!mcomEntity)
		{
			Print("[CRF_RushGamemodeManager] GetMCOMEntity FAILED to find entity for: " + mcomIdentifier);
			
			// Debug all member variables
			Print("[CRF_RushGamemodeManager] DEBUG member variables:");
			if (m_Zone1AlphaMCOM) Print("  m_Zone1AlphaMCOM = " + m_Zone1AlphaMCOM.GetID());
			if (m_Zone1BetaMCOM) Print("  m_Zone1BetaMCOM = " + m_Zone1BetaMCOM.GetID());
			if (m_Zone2AlphaMCOM) Print("  m_Zone2AlphaMCOM = " + m_Zone2AlphaMCOM.GetID());
			if (m_Zone2BetaMCOM) Print("  m_Zone2BetaMCOM = " + m_Zone2BetaMCOM.GetID());
			if (m_Zone3AlphaMCOM) Print("  m_Zone3AlphaMCOM = " + m_Zone3AlphaMCOM.GetID());
			if (m_Zone3BetaMCOM) Print("  m_Zone3BetaMCOM = " + m_Zone3BetaMCOM.GetID());
		}
		
		return mcomEntity;
	}
	
	/**
	 * Get MCOM entity name for finding by name
	 * @param mcomIdentifier The MCOM identifier
	 * @return The entity name to search for
	 */
	protected string GetMCOMEntityName(string mcomIdentifier)
	{
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
			case "zone1_alpha":
				return "mcom_a";
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
			case "zone1_beta":
				return "mcom_b";
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
			case "zone2_alpha":
				return "mcom_c";
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
			case "zone2_beta":
				return "mcom_d";
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
			case "zone3_alpha":
				return "mcom_e";
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
			case "zone3_beta":
				return "mcom_f";
		}
		
		return "";
	}
	
	/**
	 * Get MCOM entity ID by identifier
	 * @param mcomIdentifier The MCOM identifier
	 * @return The MCOM entity ID
	 */
	EntityID GetMCOMEntityID(string mcomIdentifier)
	{
		IEntity entity = GetMCOMEntity(mcomIdentifier);
		if (entity)
			return entity.GetID();
		return EntityID.INVALID;
	}
	
	/**
	 * Get zone number from MCOM identifier
	 * @param mcomIdentifier The MCOM identifier
	 * @return Zone number (1-3)
	 */
	protected int GetZoneNumber(string mcomIdentifier)
	{
		// Handle MCOMB special case based on configuration
		if (mcomIdentifier == "MCOMB")
		{
			int zone;
			if (m_iMCOMsPerZone == 1)
				zone = 2;
			else
				zone = 1;
			return zone;
		}
		
		// Handle MCOMC special case based on configuration
		if (mcomIdentifier == "MCOMC")
		{
			int zone;
			if (m_iMCOMsPerZone == 1)
				zone = 3;
			else
				zone = 2;
			return zone;
		}
		
		// Handle new sequential naming
		switch (mcomIdentifier)
		{
			case "MCOMA":
				return 1;
			case "MCOMD":
				return 2;
			case "MCOME":
			case "MCOMF":
				return 3;
		}
		
		// Legacy compatibility
		if (mcomIdentifier.StartsWith("Zone1")) return 1;
		if (mcomIdentifier.StartsWith("Zone2")) return 2;
		if (mcomIdentifier.StartsWith("Zone3")) return 3;
		return 1;
	}
	
	/**
	 * Get display name for zone
	 * @param mcomIdentifier The MCOM identifier
	 * @return Zone display name
	 */
	protected string GetZoneDisplayName(string mcomIdentifier)
	{
		int zoneNum = GetZoneNumber(mcomIdentifier);
		return string.Format("Zone %1", zoneNum);
	}
	
	/**
	 * Get display name for site (A, B, C, D, E, F)
	 * @param mcomIdentifier The MCOM identifier
	 * @return Site display name (single letter)
	 */
	protected string GetSiteDisplayName(string mcomIdentifier)
	{
		// Handle new sequential identifiers
		switch (mcomIdentifier)
		{
			case "MCOMA": return "A";
			case "MCOMB": return "B";
			case "MCOMC": return "C";
			case "MCOMD": return "D";
			case "MCOME": return "E";
			case "MCOMF": return "F";
		}
		
		// Legacy compatibility - map old Alpha/Beta to sequential letters
		if (mcomIdentifier.EndsWith("Alpha"))
		{
			if (mcomIdentifier.StartsWith("Zone1")) return "A";
			if (mcomIdentifier.StartsWith("Zone2")) return "C";
			if (mcomIdentifier.StartsWith("Zone3")) return "E";
		}
		if (mcomIdentifier.EndsWith("Beta"))
		{
			if (mcomIdentifier.StartsWith("Zone1")) return "B";
			if (mcomIdentifier.StartsWith("Zone2")) return "D";
			if (mcomIdentifier.StartsWith("Zone3")) return "F";
		}
		
		return "A"; // Default fallback
	}

	/**
	 * Get trigger name from MCOM identifier
	 * @param mcomIdentifier The MCOM identifier
	 * @return Trigger entity name
	 */
	protected string GetTriggerName(string mcomIdentifier)
	{
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				return "mcom_a_trigger";
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				return "mcom_b_trigger";
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				return "mcom_c_trigger";
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				return "mcom_d_trigger";
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				return "mcom_e_trigger";
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				return "mcom_f_trigger";
		}
		return "";
	}
	
	/**
	 * Check if a specific MCOM is in the currently active zone
	 * @param mcomIdentifier The MCOM identifier
	 * @return True if in active zone
	 */
	bool IsMCOMInActiveZone(string mcomIdentifier)
	{
		int mcomZone = GetZoneNumber(mcomIdentifier);
		return (mcomZone == m_iCurrentZone);
	}
	
	/**
	 * Get all MCOM identifiers in the currently active zone
	 * @param activeMCOMs Array to fill with active MCOM identifiers
	 */
	void GetActiveMCOMIdentifiers(out array<string> activeMCOMs)
	{
		activeMCOMs.Clear();
		
		// Use dynamic system to get MCOMs in the current active zone
		for (int zoneIndex = 0; zoneIndex < m_iNumberOfZones; zoneIndex++)
		{
			int zoneNumber = zoneIndex + 1;
			
			// Only include MCOMs from the current active zone
			if (zoneNumber != m_iCurrentZone)
				continue;
			
			for (int mcomIndex = 0; mcomIndex < m_iMCOMsPerZone; mcomIndex++)
			{
				string mcomIdentifier = GetMCOMIdentifier(zoneNumber, mcomIndex);
				activeMCOMs.Insert(mcomIdentifier);
			}
		}
		
		// Legacy fallback - if no dynamic MCOMs found, use legacy system
		if (activeMCOMs.Count() == 0)
		{
			array<string> allMCOMs = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
			
			for (int i = 0; i < allMCOMs.Count(); i++)
			{
				if (IsMCOMInActiveZone(allMCOMs[i]))
				{
					activeMCOMs.Insert(allMCOMs[i]);
				}
			}
		}
	}
	
	//===================================================================================
	// SOUND MANAGEMENT - Using MCOM 3D SoundComponent with Client Replication
	//===================================================================================
	
	/**
	 * Start playing the 3D bomb ticking sound on specific MCOM (server-side with replication)
	 */
	void StartBombTickingSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		if (m_sActiveMCOM.IsEmpty())
		{
			return;
		}
		
		// Get the MCOM entity that has the bomb planted
		IEntity mcomEntity = GetMCOMEntity(m_sActiveMCOM);
		if (!mcomEntity)
		{
			return;
		}
		
		// Get the SoundComponent from the MCOM entity (server-side only)
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (soundComponent)
		{
			// Stop any existing bomb sound first
			StopBombTickingSound();
			
			// Play the RUSH_BEEP event on server
			m_CurrentBombSoundHandle = soundComponent.SoundEvent("RUSH_BEEP");
			m_bBombSoundPlaying = true;
		}
		
		// Replicate the sound to all clients using 3D positioning
		vector mcomPosition = mcomEntity.GetOrigin();
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
		{
			broadcastManager.PlayRushMCOMSound("RUSH_BEEP", mcomPosition);
		}
		
		// Replicate state to clients for UI purposes
		m_bPlayBombSound = !m_bPlayBombSound;
		Replication.BumpMe();
	}
	
	/**
	 * Stop playing the 3D bomb ticking sound (server-side with replication)
	 */
	void StopBombTickingSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		// Always try to stop the sound, even if state variables suggest it's not playing
		// This ensures sounds are properly cleaned up in all scenarios
		
		// Try to stop sound on current active MCOM
		if (!m_sActiveMCOM.IsEmpty())
		{
			IEntity mcomEntity = GetMCOMEntity(m_sActiveMCOM);
			if (mcomEntity)
			{
				// Get the SoundComponent from the MCOM entity (server-side only)
				SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
				if (soundComponent)
				{
					// Terminate the bomb sound on server
					if (soundComponent.IsHandleValid(m_CurrentBombSoundHandle))
					{
						soundComponent.Terminate(m_CurrentBombSoundHandle);
					}
					else
					{
						// Try to terminate all sounds as fallback
						soundComponent.TerminateAll();
					}
				}
				
				// Replicate the sound stop to all clients
				vector mcomPosition = mcomEntity.GetOrigin();
				CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
				if (broadcastManager)
				{
					broadcastManager.StopRushMCOMSound("RUSH_BEEP", mcomPosition);
				}
			}
		}
		
		// Reset state variables
		m_CurrentBombSoundHandle = AudioHandle.Invalid;
		m_bBombSoundPlaying = false;
		
		// Replicate state to clients for UI purposes
		m_bStopBombSound = !m_bStopBombSound;
		Replication.BumpMe();
	}
	
	/**
	 * Play the 3D planting sound effect on specific MCOM (server-side with replication)
	 */
	void PlayPlantingSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		// Find MCOM entity where planting is happening
		IEntity mcomEntity = GetPlantingMCOMEntity();
		if (!mcomEntity)
		{
			return;
		}
		
		// Get the SoundComponent from the MCOM entity (server-side only)
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (soundComponent)
		{
			// Stop any existing planting sound first
			StopPlantingSound();
			
			// Play the RUSH_PLANTING event on server
			m_CurrentPlantingSoundHandle = soundComponent.SoundEvent("RUSH_PLANTING");
			m_bPlantingSoundPlaying = true;
		}
		
		// Store which MCOM is playing the planting sound
		m_sPlantingMCOM = GetMCOMIdentifierFromEntity(mcomEntity);
		
		// Replicate the sound to all clients using 3D positioning
		vector mcomPosition = mcomEntity.GetOrigin();
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
		{
			broadcastManager.PlayRushMCOMSound("RUSH_PLANTING", mcomPosition);
		}
		
		// Replicate state to clients for UI purposes
		m_bPlayPlantingSound = !m_bPlayPlantingSound;
		Replication.BumpMe();
	}
	
	/**
	 * Stop the 3D planting sound effect (server-side with replication)
	 */
	void StopPlantingSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		// Always try to stop the sound, even if state variables suggest it's not playing
		// This ensures sounds are properly cleaned up in all scenarios
		
		// Try to stop sound on planting MCOM
		if (!m_sPlantingMCOM.IsEmpty())
		{
			IEntity mcomEntity = GetMCOMEntity(m_sPlantingMCOM);
			if (mcomEntity)
			{
				// Get the SoundComponent from the MCOM entity (server-side only)
				SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
				if (soundComponent)
				{
					// Terminate the planting sound on server
					if (soundComponent.IsHandleValid(m_CurrentPlantingSoundHandle))
					{
						soundComponent.Terminate(m_CurrentPlantingSoundHandle);
					}
					else
					{
						// Try to stop all RUSH_PLANTING sounds as fallback
						soundComponent.TerminateAll();
					}
				}
				
				// Replicate the sound stop to all clients
				vector mcomPosition = mcomEntity.GetOrigin();
				CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
				if (broadcastManager)
				{
					broadcastManager.StopRushMCOMSound("RUSH_PLANTING", mcomPosition);
				}
			}
		}
		
		// Reset state variables
		m_CurrentPlantingSoundHandle = AudioHandle.Invalid;
		m_bPlantingSoundPlaying = false;
		m_sPlantingMCOM = "";
		
		// Replicate state to clients for UI purposes
		m_bStopPlantingSound = !m_bStopPlantingSound;
		Replication.BumpMe();
	}
	
	/**
	 * Helper method to get the MCOM entity being planted (for sound placement)
	 */
	protected IEntity GetPlantingMCOMEntity()
	{
		// If we have the planting MCOM identifier set, use that
		if (!m_sPlantingMCOM.IsEmpty())
		{
			return GetMCOMEntity(m_sPlantingMCOM);
		}
		
		// If we already have an active MCOM set, use that
		if (!m_sActiveMCOM.IsEmpty())
		{
			return GetMCOMEntity(m_sActiveMCOM);
		}
		
		// Otherwise, find the MCOM in the active zone that's most likely being planted
		// This is a fallback - in practice, this should be set properly by the action
		array<string> activeMCOMs = {};
		GetActiveMCOMIdentifiers(activeMCOMs);
		
		for (int i = 0; i < activeMCOMs.Count(); i++)
		{
			if (!GetMCOMPlantedStatus(activeMCOMs[i]))
			{
				return GetMCOMEntity(activeMCOMs[i]);
			}
		}
		
		return null;
	}
	
	/**
	 * Helper method to get MCOM identifier from entity
	 */
	protected string GetMCOMIdentifierFromEntity(IEntity mcomEntity)
	{
		if (!mcomEntity)
			return "";
		
		// Check against all stored MCOM entities using entity reference comparison
		if (mcomEntity == m_Zone1AlphaMCOM) return "MCOMA";
		if (mcomEntity == m_Zone1BetaMCOM) return "MCOMB";
		if (mcomEntity == m_Zone2AlphaMCOM) return "MCOMC";
		if (mcomEntity == m_Zone2BetaMCOM) return "MCOMD";
		if (mcomEntity == m_Zone3AlphaMCOM) return "MCOME";
		if (mcomEntity == m_Zone3BetaMCOM) return "MCOMF";
		
		// Also check dynamic arrays for robust lookup
		for (int zoneIndex = 0; zoneIndex < m_aMCOMEntities.Count(); zoneIndex++)
		{
			for (int mcomIndex = 0; mcomIndex < m_aMCOMEntities[zoneIndex].Count(); mcomIndex++)
			{
				if (m_aMCOMEntities[zoneIndex][mcomIndex] == mcomEntity)
				{
					// Convert zone/mcom indices back to identifier
					if (zoneIndex == 0 && mcomIndex == 0) return "MCOMA";
					if (zoneIndex == 0 && mcomIndex == 1) return "MCOMB";
					if (zoneIndex == 1 && mcomIndex == 0) return "MCOMC";
					if (zoneIndex == 1 && mcomIndex == 1) return "MCOMD";
					if (zoneIndex == 2 && mcomIndex == 0) return "MCOME";
					if (zoneIndex == 2 && mcomIndex == 1) return "MCOMF";
				}
			}
		}
		
		return "";
	}
	
	/**
	 * Set which MCOM is currently being planted (called from plant action)
	 */
	void SetPlantingMCOM(string mcomIdentifier)
	{
		m_sPlantingMCOM = mcomIdentifier;
	}
	
	/**
	 * Client-side planting sound stopping
	 * NOTE: This method is now redundant since we use server-side SoundComponent + RPC replication
	 */
	protected void StopPlantingSoundClient()
	{
		// Client-side sound handling is now managed by RPC replication from server
		// The server manages sounds using SoundComponent and replicates to clients
	}
	
	/**
	 * Play the 3D defuse sound effect on specific MCOM (server-side with replication)
	 */
	void PlayDefuseSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		if (m_sActiveMCOM.IsEmpty())
			return;
		
		// Get the MCOM entity that has the bomb planted (being defused)
		IEntity mcomEntity = GetMCOMEntity(m_sActiveMCOM);
		if (!mcomEntity)
			return;
		
		// Get the SoundComponent from the MCOM entity (server-side only)
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (soundComponent)
		{
			// Stop any existing defuse sound first
			StopDefuseSound();
			
			// Play the RUSH_PLANTING event for defuse (reusing planting sound for defuse action)
			m_CurrentDefuseSoundHandle = soundComponent.SoundEvent("RUSH_PLANTING");
			m_bDefuseSoundPlaying = true;
		}
		
		// Replicate the sound to all clients using 3D positioning
		vector mcomPosition = mcomEntity.GetOrigin();
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
		{
			broadcastManager.PlayRushMCOMSound("RUSH_PLANTING", mcomPosition);
		}
		
		// Replicate state to clients for UI purposes
		m_bPlayDefuseSound = !m_bPlayDefuseSound;
		Replication.BumpMe();
		
		// Automatically stop defuse sound after 3 seconds
		GetGame().GetCallqueue().CallLater(StopDefuseSound, 3000, false);
	}
	
	/**
	 * Stop the 3D defuse sound effect (server-side with replication)
	 */
	void StopDefuseSound()
	{
		// Only execute on server (SoundComponent exists only on server)
		if (!Replication.IsServer())
			return;
		
		if (!m_bDefuseSoundPlaying || m_sActiveMCOM.IsEmpty())
			return;
		
		// Get the MCOM entity that was playing the defuse sound
		IEntity mcomEntity = GetMCOMEntity(m_sActiveMCOM);
		if (!mcomEntity)
			return;
		
		// Get the SoundComponent from the MCOM entity (server-side only)
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (soundComponent)
		{
			// Terminate the defuse sound on server
			if (soundComponent.IsHandleValid(m_CurrentDefuseSoundHandle))
			{
				soundComponent.Terminate(m_CurrentDefuseSoundHandle);
			}
		}
		
		// Replicate the sound stop to all clients
		vector mcomPosition = mcomEntity.GetOrigin();
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
		{
			broadcastManager.StopRushMCOMSound("RUSH_PLANTING", mcomPosition);
		}
		
		m_CurrentDefuseSoundHandle = AudioHandle.Invalid;
		m_bDefuseSoundPlaying = false;
		
		// Replicate state to clients for UI purposes
		m_bStopDefuseSound = !m_bStopDefuseSound;
		Replication.BumpMe();
	}
	
	/**
	 * Client-side defuse sound playback
	 * NOTE: This method is now redundant since we use server-side SoundComponent + RPC replication
	 */
	protected void PlayDefuseSoundClient()
	{
		// Client-side sound handling is now managed by RPC replication from server
		// The server plays sounds using SoundComponent.SoundEvent() and replicates to clients
	}
	
	/**
	 * Client-side defuse sound stopping
	 */
	protected void StopDefuseSoundClient()
	{
		if (!m_bDefuseSoundPlaying)
			return;
		
		// Reset the defuse sound handle (sound will stop automatically)
		AudioSystem.TerminateSound(m_CurrentDefuseSoundHandle);
		m_CurrentDefuseSoundHandle = AudioHandle.Invalid;
		m_bDefuseSoundPlaying = false;
	}
	
	/**
	 * Client-side bomb sound playing (RPC handler)
	 * NOTE: This method is now redundant since we use server-side SoundComponent + RPC replication
	 */
	protected void PlayBombSoundClient()
	{
		// Client-side sound handling is now managed by RPC replication from server
		// The server plays sounds using SoundComponent.SoundEvent() and replicates to clients
		m_bBombSoundPlaying = true;
	}
			
			
	protected void PlayPlantingSoundClient()
	{
		
	}
	
	/**
	 * Client-side bomb sound stopping (RPC handler)
	 */
	protected void StopBombSoundClient()
	{
		if (!m_bBombSoundPlaying)
			return;
		
		// Reset the bomb sound handle
		AudioSystem.TerminateSound(m_CurrentBombSoundHandle);
		m_CurrentBombSoundHandle = AudioHandle.Invalid;
		m_bBombSoundPlaying = false;
	}
	
	//===================================================================================
	// 3D MARKER FLASHING MANAGEMENT
	//===================================================================================
	
	/**
	 * Start flashing red marker for a planted MCOM
	 * @param mcomIdentifier The MCOM identifier to flash
	 */
	protected void StartFlashingMarker(string mcomIdentifier)
	{
		// On server, replicate to all clients
		if (Replication.IsServer())
			Rpc(RpcDo_StartFlashing3DMarker, mcomIdentifier);
		
		// Execute locally (both server and client)
		StartFlashing3DMarkerLocal(mcomIdentifier);
		
		// Update map marker to show planted status
		UpdatePlantedMapMarker(mcomIdentifier, true);
	}
	
	/**
	 * Stop flashing marker for a defused MCOM
	 * @param mcomIdentifier The MCOM identifier to stop flashing
	 */
	protected void StopFlashingMarker(string mcomIdentifier)
	{
		// On server, replicate to all clients
		if (Replication.IsServer())
			Rpc(RpcDo_StopFlashing3DMarker, mcomIdentifier);
		
		// Execute locally (both server and client)
		StopFlashing3DMarkerLocal(mcomIdentifier);
		
		// Update map marker to show normal status
		UpdatePlantedMapMarker(mcomIdentifier, false);
	}
	
	/**
	 * Update map marker to show planted/normal status
	 * @param mcomIdentifier The MCOM identifier
	 * @param isPlanted True if planted, false for normal
	 */
	protected void UpdatePlantedMapMarker(string mcomIdentifier, bool isPlanted)
	{
		if (m_bHideMapMarkers)
			return;
		
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerManager)
			return;
		
		// Get trigger name and position for this MCOM
		string triggerName = GetTriggerName(mcomIdentifier);
		IEntity trigger = GetGame().GetWorld().FindEntityByName(triggerName);
		if (!trigger)
			return;
		
		vector pos = trigger.GetOrigin();
		string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
		
		// Get the letter for this MCOM
		string markerLetter = GetSiteDisplayName(mcomIdentifier);
		string iconPath = GetMarkerIconPath(markerLetter);
		
		// Determine marker text and color
		string markerText;
		int markerColor;
		
		if (isPlanted)
		{
			markerText = string.Format("MCOM %1 (PLANTED!)", markerLetter);
			markerColor = ARGB(200, 255, 50, 50); // Bright red for planted
		}
		else
		{
			markerText = string.Format("MCOM %1", markerLetter);
			markerColor = ARGB(255, 255, 0, 0); // Red for active zone
		}
		
		// Remove existing marker first
		playerControllerManager.RemoveALLScriptedMarkers();
		
		// Add updated marker with planted status
		playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, markerText, iconPath, 50, markerColor);
		
		// Re-add other active zone markers
		GetGame().GetCallqueue().CallLater(UpdateActiveZoneMarkers, 100, false);
	}
	
	//===================================================================================
	// 3D MARKER REPLICATION
	//===================================================================================
	
	/**
	 * RPC method to start flashing 3D marker on all clients
	 * @param mcomIdentifier The MCOM identifier to flash
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_StartFlashing3DMarker(string mcomIdentifier)
	{
		StartFlashing3DMarkerLocal(mcomIdentifier);
	}
	
	/**
	 * RPC method to stop flashing 3D marker on all clients
	 * @param mcomIdentifier The MCOM identifier to stop flashing
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_StopFlashing3DMarker(string mcomIdentifier)
	{
		StopFlashing3DMarkerLocal(mcomIdentifier);
	}
	
	/**
	 * RPC method to update 3D marker colors on all clients
	 * @param currentZone The current zone to display markers for
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_Update3DMarkerColors(int currentZone)
	{
		// Ensure client has the correct current zone
		if (!Replication.IsServer())
			m_iCurrentZone = currentZone;
			
		Update3DMarkerColorsLocal();
	}
	
	/**
	 * RPC method to initialize 3D marker on all clients
	 * NOTE: Disabled in favor of player-based component system
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_Initialize3DMarker(string mcomIdentifier, string letter, float colorR, float colorG, float colorB, float colorA, bool visible)
	{
		// Re-enabled to work with 3D marker components on MCOM entities
		Color color = Color.FromRGBA(colorR, colorG, colorB, colorA);
		Initialize3DMarkerLocal(mcomIdentifier, letter, color, visible);
	}
	
	/**
	 * RPC method to initialize 3D marker on specific client
	 * NOTE: Disabled in favor of player-based component system
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_Initialize3DMarkerForPlayer(string mcomIdentifier, string letter, float colorR, float colorG, float colorB, float colorA, bool visible)
	{
		// Re-enabled to work with 3D marker components on MCOM entities
		Color color = Color.FromRGBA(colorR, colorG, colorB, colorA);
		Initialize3DMarkerLocal(mcomIdentifier, letter, color, visible);
	}
	
	/**
	 * Local implementation to start flashing 3D marker
	 * @param mcomIdentifier The MCOM identifier to flash
	 */
	protected void StartFlashing3DMarkerLocal(string mcomIdentifier)
	{
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		if (!mcomEntity)
			return;
		
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (!markerComponent)
			return;
		
		// Start flashing with bright red color
		Color flashColor = Color.FromInt(ARGB(255, 255, 0, 0)); // Bright red
		markerComponent.StartFlashing(flashColor);
	}
	
	/**
	 * Local implementation to stop flashing 3D marker
	 * @param mcomIdentifier The MCOM identifier to stop flashing
	 */
	protected void StopFlashing3DMarkerLocal(string mcomIdentifier)
	{
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		if (!mcomEntity)
			return;
		
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (!markerComponent)
			return;
		
		// Stop flashing and return to normal color
		markerComponent.StopFlashing();
	}
	
	/**
	 * Local implementation to initialize 3D marker
	 * NOTE: Disabled in favor of player-based component system
	 */
	protected void Initialize3DMarkerLocal(string mcomIdentifier, string letter, Color color, bool visible)
	{
		// Initialize 3D marker component on MCOM entity
	}
	
	//===================================================================================
	// STANDALONE MARKER METHODS (DEPRECATED - REMOVED TO PREVENT DUPLICATE MARKERS)
	// 3D markers are now handled by CRF_Rush_3DMarkerComponent on MCOM entities
	//===================================================================================
	
	//===================================================================================
	// PUBLIC GETTERS
	//===================================================================================
	
	/**
	 * Get the attacking faction key
	 * @return Attacking faction key
	 */
	FactionKey GetAttackingSide() { return m_AttackingSide; }
	
	/**
	 * Get the defending faction key
	 * @return Defending faction key
	 */
	FactionKey GetDefendingSide() { return m_DefendingSide; }
	
	/**
	 * Get current active zone number
	 * @return Current zone (1-3)
	 */
	int GetCurrentZone() { return m_iCurrentZone; }
	
	/**
	 * Check if countdown is currently active
	 * @return True if countdown active
	 */
	bool IsCountdownActive() { return m_bCountdownActive; }
	
	/**
	 * Get remaining countdown time
	 * @return Seconds remaining
	 */
	int GetCountdownTimeRemaining() { return m_iCountdownTimeRemaining; }
	
	/**
	 * Get saved countdown time for a specific MCOM (always 0 - saved times removed)
	 * @param mcomIdentifier The MCOM identifier
	 * @return Always 0 (saved times no longer supported)
	 */
	int GetMCOMSavedTime(string mcomIdentifier) 
	{ 
		return 0; // Saved times no longer supported - timer restarts on defuse
	}
	
	/**
	 * Get the planting sound effect resource (hardcoded .acp file)
	 * @return Planting sound effect resource name
	 */
	ResourceName GetPlantingSoundEffect() { return "{1D6C7E5479081CAF}Sounds/Rush/planting_3D.acp"; }
	
	/**
	 * Get the bomb ticking sound effect resource (hardcoded .acp file)
	 * @return Bomb sound effect resource name
	 */
	ResourceName GetBombSoundEffect() { return "{A6BBE7DBD7C64EE6}Sounds/Rush/beep_3D.acp"; }
	
	//===================================================================================
	// POSITION GETTERS (For Player Marker Component)
	//===================================================================================
	
	/**
	 * Get Zone 1 Alpha marker position
	 * @return World position vector
	 */
	vector GetZone1AlphaPosition() { return m_vZone1AlphaPosition; }
	
	/**
	 * Get Zone 1 Beta marker position
	 * @return World position vector
	 */
	vector GetZone1BetaPosition() { return m_vZone1BetaPosition; }
	
	/**
	 * Get Zone 2 Alpha marker position
	 * @return World position vector
	 */
	vector GetZone2AlphaPosition() { return m_vZone2AlphaPosition; }
	
	/**
	 * Get Zone 2 Beta marker position
	 * @return World position vector
	 */
	vector GetZone2BetaPosition() { return m_vZone2BetaPosition; }
	
	/**
	 * Get Zone 3 Alpha marker position
	 * @return World position vector
	 */
	vector GetZone3AlphaPosition() { return m_vZone3AlphaPosition; }
	
	/**
	 * Get Zone 3 Beta marker position
	 * @return World position vector
	 */
	vector GetZone3BetaPosition() { return m_vZone3BetaPosition; }
	
	/**
	 * Check if markers are currently visible
	 * @return True if any markers should be visible
	 */
	bool AreMarkersVisible() { 
		return m_bZone1AlphaMarkerVisible || m_bZone1BetaMarkerVisible || 
		       m_bZone2AlphaMarkerVisible || m_bZone2BetaMarkerVisible || 
		       m_bZone3AlphaMarkerVisible || m_bZone3BetaMarkerVisible; 
	}
	
	/**
	 * Clean up MCOM reference after entity deletion (called from broadcast manager on clients)
	 * @param mcomIdentifier The MCOM identifier to clean up
	 */
	void CleanupMCOMReference(string mcomIdentifier)
	{
		Print("[CRF_RushGamemodeManager] CleanupMCOMReference called for: " + mcomIdentifier);
		
		// Get the MCOM entity before clearing references for marker cleanup
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		
		// Hide 3D marker component if entity still exists
		if (mcomEntity)
		{
			CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
			if (markerComponent)
			{
				markerComponent.SetVisible(false);
			}
		}
		
		// Clear references from legacy member variables
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha":
				m_Zone1AlphaMCOM = null;
				// Hide marker visibility flags
				m_bZone1AlphaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone1Alpha reference");
				break;
			case "MCOMB":
			case "Zone1Beta":
				m_Zone1BetaMCOM = null;
				m_bZone1BetaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone1Beta reference");
				break;
			case "MCOMC":
			case "Zone2Alpha":
				m_Zone2AlphaMCOM = null;
				m_bZone2AlphaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone2Alpha reference");
				break;
			case "MCOMD":
			case "Zone2Beta":
				m_Zone2BetaMCOM = null;
				m_bZone2BetaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone2Beta reference");
				break;
			case "MCOME":
			case "Zone3Alpha":
				m_Zone3AlphaMCOM = null;
				m_bZone3AlphaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone3Alpha reference");
				break;
			case "MCOMF":
			case "Zone3Beta":
				m_Zone3BetaMCOM = null;
				m_bZone3BetaMarkerVisible = false;
				Print("[CRF_RushGamemodeManager] Cleared Zone3Beta reference");
				break;
		}
		
		// Clear references from dynamic arrays
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && mcomIndex < m_aMCOMEntities[zoneIndex].Count())
			{
				m_aMCOMEntities[zoneIndex][mcomIndex] = null;
				Print(string.Format("[CRF_RushGamemodeManager] Cleared dynamic array reference at zone %1, mcom %2", zoneIndex, mcomIndex));
			}
		}
		
		// Force map marker refresh on clients to remove destroyed MCOM markers
		if (!Replication.IsServer())
		{
			// Only refresh if markers are enabled
			if (!m_bHideMapMarkers)
			{
				GetGame().GetCallqueue().CallLater(RefreshMapMarkers, 500, false);
				Print("[CRF_RushGamemodeManager] Scheduled map marker refresh for: " + mcomIdentifier);
			}
		}
		
		Print("[CRF_RushGamemodeManager] CleanupMCOMReference completed for: " + mcomIdentifier);
	}
	
	//===================================================================================
	// CONFIGURATION METHODS
	//===================================================================================
	
	/**
	 * Get the configured number of zones
	 * @return Number of zones (1-3)
	 */
	int GetNumberOfZones() { return m_iNumberOfZones; }
	
	/**
	 * Get the configured number of MCOMs per zone
	 * @return Number of MCOMs per zone (1-2)
	 */
	int GetMCOMsPerZone() { return m_iMCOMsPerZone; }
	
	/**
	 * Get the total number of MCOMs in the mission
	 * @return Total MCOM count
	 */
	int GetTotalMCOMCount() { return m_iNumberOfZones * m_iMCOMsPerZone; }
	
	/**
	 * Check if a zone number is valid for the current configuration
	 * @param zoneNumber The zone number to check (1-based)
	 * @return True if the zone is valid
	 */
	bool IsZoneValid(int zoneNumber) { return zoneNumber >= 1 && zoneNumber <= m_iNumberOfZones; }
	
	/**
	 * Check if an MCOM index is valid for the current configuration
	 * @param mcomIndex The MCOM index to check (0-based)
	 * @return True if the MCOM index is valid
	 */
	bool IsMCOMIndexValid(int mcomIndex) { return mcomIndex >= 0 && mcomIndex < m_iMCOMsPerZone; }
	
	/**
	 * Get MCOM destroyed status from dynamic arrays
	 * @param zoneNumber Zone number (1-based)
	 * @param mcomIndex MCOM index (0-based)
	 * @return True if MCOM is destroyed
	 */
	bool GetMCOMDestroyedStatus(int zoneNumber, int mcomIndex)
	{
		int zoneIndex = zoneNumber - 1;
		if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && mcomIndex < m_aMCOMDestroyed[zoneIndex].Count())
			return m_aMCOMDestroyed[zoneIndex][mcomIndex];
		return false;
	}
	
	/**
	 * Get MCOM planted status from dynamic arrays
	 * @param zoneNumber Zone number (1-based)
	 * @param mcomIndex MCOM index (0-based)
	 * @return True if MCOM is planted
	 */
	bool GetMCOMPlantedStatus(int zoneNumber, int mcomIndex)
	{
		int zoneIndex = zoneNumber - 1;
		if (m_aMCOMPlanted && zoneIndex < m_aMCOMPlanted.Count() && mcomIndex < m_aMCOMPlanted[zoneIndex].Count())
			return m_aMCOMPlanted[zoneIndex][mcomIndex];
		return false;
	}
	
	//------------------------------------------------------------------------------------
	// EnableZoneRespawnPoints - Enable configured respawn points when a zone is cleared
	//------------------------------------------------------------------------------------
	protected void EnableZoneRespawnPoints(int zoneNumber)
	{
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		// Check if dynamic respawns are enabled
		if (!m_bEnableDynamicRespawns)
			return;
		
		// If no respawn configs, exit early
		if (!m_aZoneRespawnConfigs || m_aZoneRespawnConfigs.IsEmpty())
			return;
		
		// Check if tracking array exists (safety check)
		if (!m_aZonesClearedStatus)
			return;
		
		// Check if this zone was already cleared (prevent duplicate lookups)
		int zoneIndex = zoneNumber - 1;
		if (zoneIndex < 0 || zoneIndex >= m_aZonesClearedStatus.Count())
			return;
		
		if (m_aZonesClearedStatus[zoneIndex])
		{
			Print(string.Format("[CRF_Rush] Zone %1 already cleared, skipping respawn activation", zoneNumber));
			return;
		}
		
		// Mark this zone as cleared
		m_aZonesClearedStatus[zoneIndex] = true;
		
		// Check if respawn flags need to be teleported for this zone (optional)
		GetGame().GetCallqueue().CallLater(EnableCurrentZoneRespawns, 100, false, zoneNumber);
	}
	
	//------------------------------------------------------------------------------------
	// EnableCurrentZoneRespawns - Teleport respawn flags to new zone positions (if configured)
	//------------------------------------------------------------------------------------
	protected void EnableCurrentZoneRespawns(int zoneNumber)
	{
		// Find config for this zone by array index (zone number - 1)
		int configIndex = zoneNumber - 1;
		
		if (configIndex < 0 || configIndex >= m_aZoneRespawnConfigs.Count())
		{
			Print(string.Format("[CRF_Rush] No respawn configuration at index %1 for Zone %2", configIndex, zoneNumber));
			return;
		}
		
		CRF_Rush_ZoneRespawnConfig zoneConfig = m_aZoneRespawnConfigs[configIndex];
		
		// Skip if no respawn points configured (optional)
		if (!zoneConfig || !zoneConfig.m_aRespawnPoints || zoneConfig.m_aRespawnPoints.IsEmpty())
		{
			Print(string.Format("[CRF_Rush] Zone %1: No respawn flag movement configured (optional)", zoneNumber));
			return;
		}
		
		// Get respawn manager
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (!respawnManager)
		{
			Print("[CRF_Rush] ERROR: RespawnManager not found!", LogLevel.ERROR);
			return;
		}
		
		Print(string.Format("[CRF_Rush] Zone %1: Moving %2 respawn flag(s) to new positions", zoneNumber, zoneConfig.m_aRespawnPoints.Count()));
		
		// Move each faction's respawn flag to its marker position
		foreach (CRF_Rush_RespawnPointEntry entry : zoneConfig.m_aRespawnPoints)
		{
			if (!entry || entry.m_sMarkerEntityName == "")
			{
				Print("[CRF_Rush] ERROR: Empty marker entity name in config!", LogLevel.ERROR);
				continue;
			}
			
			// Find the marker entity by name
			IEntity markerEntity = GetGame().GetWorld().FindEntityByName(entry.m_sMarkerEntityName);
			if (!markerEntity)
			{
				Print(string.Format("[CRF_Rush] ERROR: Marker entity '%1' not found in world!", entry.m_sMarkerEntityName), LogLevel.ERROR);
				continue;
			}
			
			// Get the first respawn flag for this faction
			array<IEntity> factionSpawns = respawnManager.GetFactionSpawnpoints(entry.m_eFaction);
			if (!factionSpawns || factionSpawns.IsEmpty())
			{
				Print(string.Format("[CRF_Rush] No spawns found for faction %1", entry.m_eFaction), LogLevel.WARNING);
				continue;
			}
			
			IEntity respawnFlag = factionSpawns[0];
			
			// Get marker position and orientation
			vector markerPos = markerEntity.GetOrigin();
			vector markerAngles = markerEntity.GetAngles();
			
			// Move respawn flag (Note: visual flag mesh may not update immediately)
			respawnFlag.SetOrigin(markerPos);
			respawnFlag.SetAngles(markerAngles);
			
			Print(string.Format("[CRF_Rush] Teleported %1 respawn flag to zone %2 at %3", entry.m_eFaction, zoneNumber, markerPos));
		}
	}
	
	/**
	 * Get MCOM entity from dynamic arrays
	 * @param zoneNumber Zone number (1-based)
	 * @param mcomIndex MCOM index (0-based)
	 * @return The MCOM entity or null
	 */
	IEntity GetMCOMEntityByIndex(int zoneNumber, int mcomIndex)
	{
		int zoneIndex = zoneNumber - 1;
		if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && mcomIndex < m_aMCOMEntities[zoneIndex].Count())
			return m_aMCOMEntities[zoneIndex][mcomIndex];
		return null;
	}
}
