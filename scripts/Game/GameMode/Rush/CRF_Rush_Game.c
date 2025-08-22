//------------------------------------------------------------------------------------
// CRF_RushGamemodeManager: Rush gamemode implementation for Coalition Reforger Framework
// Manages three zones with two MCOM sites each. When both MCOMs in a zone are destroyed,
// the next zone unlocks for attack.
//
// Features:
// - Progressive zone unlocking (Zone 1 -> Zone 2 -> Zone 3)
// - 2D map markers showing all MCOM locations with color coding
// - 3D HUD markers with letters (A/B) visible through walls
// - Bomb planting/defusing mechanics with countdown timers
// - Configurable attacking/defending sides
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Rush gamemode with 3 zones and 6 MCOM sites")]
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
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	// Zone Management
	//------------------------------------------------------------------------------------
	protected int m_iCurrentZone = 1;					// Currently active zone (1-3)
	protected int m_iActiveZoneCount = 1;				// Number of zones that are unlocked
	
	// Dynamic MCOM Status Tracking (supports 1-3 zones, 1-2 MCOMs per zone)
	//------------------------------------------------------------------------------------
	protected ref array<ref array<bool>> m_aMCOMDestroyed;		// [zoneIndex][mcomIndex] - destroyed status
	protected ref array<ref array<bool>> m_aMCOMPlanted;		// [zoneIndex][mcomIndex] - planted status  
	protected ref array<ref array<IEntity>> m_aMCOMEntities;	// [zoneIndex][mcomIndex] - entity references
	
	// Legacy compatibility variables (kept for network replication)
	//------------------------------------------------------------------------------------
	protected bool m_bZone1Alpha = false;				// Zone 1 Alpha MCOM destroyed
	protected bool m_bZone1Beta = false;				// Zone 1 Beta MCOM destroyed
	protected bool m_bZone2Alpha = false;				// Zone 2 Alpha MCOM destroyed
	protected bool m_bZone2Beta = false;				// Zone 2 Beta MCOM destroyed
	protected bool m_bZone3Alpha = false;				// Zone 3 Alpha MCOM destroyed
	protected bool m_bZone3Beta = false;				// Zone 3 Beta MCOM destroyed
	
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
	// Standalone marker system has been removed to prevent duplicates
	
	// Entity IDs for network synchronization
	protected EntityID m_Zone1AlphaID;
	protected EntityID m_Zone1BetaID;
	protected EntityID m_Zone2AlphaID;
	protected EntityID m_Zone2BetaID;
	protected EntityID m_Zone3AlphaID;
	protected EntityID m_Zone3BetaID;
	
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
	 */
	protected void SetupMarkersForPlayer(int playerId)
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
		{
			continue;
		}				string mcomIdentifier = GetMCOMIdentifier(zoneIndex + 1, mcomIndex);
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
	 * Legacy MCOM initialization for backward compatibility
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
		
		// Only spawn legacy MCOMs if they exist and are within our configuration
		if (zone1AlphaTrigger && m_iNumberOfZones >= 1 && m_iMCOMsPerZone >= 1)
			SpawnMCOMAtPosition(zone1AlphaTrigger, "Zone1Alpha");
		if (zone1BetaTrigger && m_iNumberOfZones >= 1 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone1BetaTrigger, "Zone1Beta");
		if (zone2AlphaTrigger && m_iNumberOfZones >= 2 && m_iMCOMsPerZone >= 1)
			SpawnMCOMAtPosition(zone2AlphaTrigger, "Zone2Alpha");
		if (zone2BetaTrigger && m_iNumberOfZones >= 2 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone2BetaTrigger, "Zone2Beta");
		if (zone3AlphaTrigger && m_iNumberOfZones >= 3 && m_iMCOMsPerZone >= 1)
			SpawnMCOMAtPosition(zone3AlphaTrigger, "Zone3Alpha");
		if (zone3BetaTrigger && m_iNumberOfZones >= 3 && m_iMCOMsPerZone >= 2)
			SpawnMCOMAtPosition(zone3BetaTrigger, "Zone3Beta");
		
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
		
		// Store entity references for legacy compatibility
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
			{
				m_Zone1AlphaMCOM = mcomEntity;
				m_Zone1AlphaID = mcomEntity.GetID();
				break;
			}
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
			{
				m_Zone1BetaMCOM = mcomEntity;
				m_Zone1BetaID = mcomEntity.GetID();
				break;
			}
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
			{
				m_Zone2AlphaMCOM = mcomEntity;
				m_Zone2AlphaID = mcomEntity.GetID();
				break;
			}
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
			{
				m_Zone2BetaMCOM = mcomEntity;
				m_Zone2BetaID = mcomEntity.GetID();
				break;
			}
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
			{
				m_Zone3AlphaMCOM = mcomEntity;
				m_Zone3AlphaID = mcomEntity.GetID();
				break;
			}
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
			{
				m_Zone3BetaMCOM = mcomEntity;
				m_Zone3BetaID = mcomEntity.GetID();
				break;
			}
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
			return false;
		
		// Check if this MCOM is destroyed
		if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && 
			mcomIndex < m_aMCOMDestroyed[zoneIndex].Count())
		{
			if (m_aMCOMDestroyed[zoneIndex][mcomIndex])
				return false; // Hide if destroyed
		}
		
		// For 3D markers: only show current active zone
		int zoneNumber = zoneIndex + 1; // Convert from 0-based index to 1-based zone number
		return zoneNumber == m_iCurrentZone;
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
				playerControllerManager.AddScriptedMarker(triggerName, "0 0 0", 1, markerName, iconPath, 50, markerColor);
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
		
		// Add markers for the current active zone using static positions
		switch (m_iCurrentZone)
		{
			case 1:
				if (!m_bZone1Alpha)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z1_alpha_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Alpha", "{21A2A457BD0E42C1}UI/Objectives/A.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				if (!m_bZone1Beta)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z1_beta_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Beta", "{7F4A8D140283CCCE}UI/Objectives/B.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				break;
			case 2:
				if (!m_bZone2Alpha)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z2_alpha_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Alpha", "{21A2A457BD0E42C1}UI/Objectives/A.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				if (!m_bZone2Beta)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z2_beta_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Beta", "{7F4A8D140283CCCE}UI/Objectives/B.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				break;
			case 3:
				if (!m_bZone3Alpha)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z3_alpha_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Alpha", "{21A2A457BD0E42C1}UI/Objectives/A.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				if (!m_bZone3Beta)
				{
					IEntity trigger = GetGame().GetWorld().FindEntityByName("z3_beta_trigger");
					if (trigger)
					{
						vector pos = trigger.GetOrigin();
						string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
						playerControllerManager.AddScriptedMarker("Static Marker", posStr, 1, "MCOM Beta", "{7F4A8D140283CCCE}UI/Objectives/B.edds", 50, ARGB(255, 255, 0, 0));
					}
				}
				break;
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
		
		// Also update legacy markers for backward compatibility - only show current zone
		UpdateLegacyMarkerWithColor(m_Zone1AlphaMCOM, 1, m_bZone1AlphaMarkerVisible);
		UpdateLegacyMarkerWithColor(m_Zone1BetaMCOM, 1, m_bZone1BetaMarkerVisible);
		UpdateLegacyMarkerWithColor(m_Zone2AlphaMCOM, 2, m_bZone2AlphaMarkerVisible);
		UpdateLegacyMarkerWithColor(m_Zone2BetaMCOM, 2, m_bZone2BetaMarkerVisible);
		UpdateLegacyMarkerWithColor(m_Zone3AlphaMCOM, 3, m_bZone3AlphaMarkerVisible);
		UpdateLegacyMarkerWithColor(m_Zone3BetaMCOM, 3, m_bZone3BetaMarkerVisible);
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
			if (marker) marker.SetVisible(false);
		}
		if (m_Zone1BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone1BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) marker.SetVisible(false);
		}
		if (m_Zone2AlphaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone2AlphaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) marker.SetVisible(false);
		}
		if (m_Zone2BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone2BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) marker.SetVisible(false);
		}
		if (m_Zone3AlphaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone3AlphaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) marker.SetVisible(false);
		}
		if (m_Zone3BetaMCOM)
		{
			CRF_Rush_3DMarkerComponent marker = CRF_Rush_3DMarkerComponent.Cast(m_Zone3BetaMCOM.FindComponent(CRF_Rush_3DMarkerComponent));
			if (marker) marker.SetVisible(false);
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
				{
					return;
				}
				
				// Update planted status
				SetMCOMPlantedStatus(mcomIdentifier, isPlanted);
				
				// Start countdown
				m_bCountdownActive = true;
				m_sActiveMCOM = mcomIdentifier;
				
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
				
				// Start countdown timer
				GetGame().GetCallqueue().CallLater(CountdownTimer, 1000, true);
			}
			else
			{
				// Update planted status
				SetMCOMPlantedStatus(mcomIdentifier, isPlanted);
				
				// Stop countdown
				m_bCountdownActive = false;
				m_sActiveMCOM = "";
				
				// Remove countdown timer
				GetGame().GetCallqueue().Remove(CountdownTimer);
				
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
		{
			GetGame().GetCallqueue().Remove(CountdownTimer);
			return;
		}
		
		// Decrement timer
		m_iCountdownTimeRemaining--;
		
		// Update countdown display
		string zoneName = GetZoneDisplayName(m_sActiveMCOM);
		string siteName = GetSiteDisplayName(m_sActiveMCOM);
		m_sMessageContent = string.Format("%1 %2: %3", zoneName, siteName, SCR_FormatHelper.FormatTime(m_iCountdownTimeRemaining));
		
		// Check if time is up
		if (m_iCountdownTimeRemaining <= 0) 
		{
			MCOMDestroyed(m_sActiveMCOM);
			
			// Stop countdown
			m_bCountdownActive = false;
			m_sActiveMCOM = "";
			GetGame().GetCallqueue().Remove(CountdownTimer);
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
		// Mark MCOM as destroyed
		SetMCOMDestroyedStatus(mcomIdentifier, true);
		
		// Stop bomb ticking sound since MCOM is destroyed
		StopBombTickingSound();
		
		// Stop flashing marker since MCOM is destroyed
		StopFlashingMarker(mcomIdentifier);
		
		// Hide the 3D marker and delete the MCOM entity
		IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
		if (mcomEntity)
		{
			CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
			if (markerComponent)
				markerComponent.SetVisible(false);
			
			// Actually delete the MCOM entity from the world
			GetGame().GetCallqueue().CallLater(DeleteMCOMEntity, 1000, false, mcomEntity);
		}
		
		// Set destroyed MCOM for client replication
		m_sDestroyedMCOMString = mcomIdentifier;
		
		// Play destruction sound
		m_sSoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		
		// Show destruction message
		string zoneName = GetZoneDisplayName(mcomIdentifier);
		string siteName = GetSiteDisplayName(mcomIdentifier);
		m_sMessageContent = string.Format("%1 %2 DESTROYED!|15|", zoneName, siteName);
		
		// Check if zone is cleared
		CheckZoneCleared(GetZoneNumber(mcomIdentifier));
		
		Replication.BumpMe();
		MCOMDestroyedClient();
		ShowMessage();
		PlaySound();
	}
	
	/**
	 * Set the destroyed status for a specific MCOM
	 * @param mcomIdentifier The MCOM identifier
	 * @param isDestroyed The destroyed status
	 */
	protected void SetMCOMDestroyedStatus(string mcomIdentifier, bool isDestroyed)
	{
		// Update dynamic arrays
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			if (m_aMCOMDestroyed && zoneIndex < m_aMCOMDestroyed.Count() && mcomIndex < m_aMCOMDestroyed[zoneIndex].Count())
				m_aMCOMDestroyed[zoneIndex][mcomIndex] = isDestroyed;
		}
		
		// Update legacy variables for backward compatibility and replication
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
	 * Parse MCOM identifier to get zone and MCOM indices
	 * @param mcomIdentifier The MCOM identifier (e.g., "Zone1Alpha")
	 * @param zoneIndex Output zone index (0-based)
	 * @param mcomIndex Output MCOM index (0-based)
	 * @return True if parsing was successful
	 */
	protected bool ParseMCOMIdentifier(string mcomIdentifier, out int zoneIndex, out int mcomIndex)
	{
		// Handle new sequential identifiers first
		switch (mcomIdentifier)
		{
			case "MCOMA":
				zoneIndex = 0; mcomIndex = 0; return true;
			case "MCOMB":
				if (m_iMCOMsPerZone == 1) { zoneIndex = 1; mcomIndex = 0; }
				else { zoneIndex = 0; mcomIndex = 1; }
				return true;
			case "MCOMC":
				if (m_iMCOMsPerZone == 1) { zoneIndex = 2; mcomIndex = 0; }
				else { zoneIndex = 1; mcomIndex = 0; }
				return true;
			case "MCOMD":
				zoneIndex = 1; mcomIndex = 1; return true;
			case "MCOME":
				zoneIndex = 2; mcomIndex = 0; return true;
			case "MCOMF":
				zoneIndex = 2; mcomIndex = 1; return true;
		}
		
		// Legacy compatibility - parse zone number (1-3) to index (0-2)
		if (mcomIdentifier.Contains("Zone1"))
			zoneIndex = 0;
		else if (mcomIdentifier.Contains("Zone2"))
			zoneIndex = 1;
		else if (mcomIdentifier.Contains("Zone3"))
			zoneIndex = 2;
		else
			return false;
		
		// Legacy compatibility - parse MCOM letter to index
		if (mcomIdentifier.Contains("Alpha"))
			mcomIndex = 0;
		else if (mcomIdentifier.Contains("Beta"))
			mcomIndex = 1;
		else
			return false;
		
		return true;
	}
	
	/**
	 * Delete MCOM entity from the world after destruction
	 * @param mcomEntity The MCOM entity to delete
	 */
	protected void DeleteMCOMEntity(IEntity mcomEntity)
	{
		if (!mcomEntity)
			return;
			
		// Clear the reference from member variables
		if (mcomEntity == m_Zone1AlphaMCOM)
			m_Zone1AlphaMCOM = null;
		else if (mcomEntity == m_Zone1BetaMCOM)
			m_Zone1BetaMCOM = null;
		else if (mcomEntity == m_Zone2AlphaMCOM)
			m_Zone2AlphaMCOM = null;
		else if (mcomEntity == m_Zone2BetaMCOM)
			m_Zone2BetaMCOM = null;
		else if (mcomEntity == m_Zone3AlphaMCOM)
			m_Zone3AlphaMCOM = null;
		else if (mcomEntity == m_Zone3BetaMCOM)
			m_Zone3BetaMCOM = null;
			
		// Delete the entity from the world
		SCR_EntityHelper.DeleteEntityAndChildren(mcomEntity);
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
				m_iCurrentZone = zoneNumber + 1;
				m_sMessageContent = string.Format("Zone %1 Cleared! Zone %2 is now unlocked.|20|Attackers advance!", zoneNumber, zoneNumber + 1);
			}
			else
			{
				m_sMessageContent = "All zones cleared! Attackers have won!|60|Attacker Victory!";
				HandleGameEnd(true); // Attackers won
			}
		}
		
		if (zoneCleared && zoneNumber < m_iNumberOfZones)
		{
			// Update map markers for new active zone
			GetGame().GetCallqueue().CallLater(UpdateAllMCOMMarkers, 500, false);
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
	 * Handle game end conditions
	 * @param attackersWon True if attackers won, false if defenders won
	 */
	protected void HandleGameEnd(bool attackersWon)
	{
		// Advance gamemode to AAR state
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode)
			GetGame().GetCallqueue().CallLater(gamemode.AdvanceGamemodeState, 3000, false, true);
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
		
		// Remove map marker for destroyed MCOM
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerManager && !m_bHideMapMarkers)
		{
			// Refresh all MCOM markers to reflect the new state
			GetGame().GetCallqueue().CallLater(UpdateAllMCOMMarkers, 100, false);
		}
		
		// Spawn explosion effects
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = destroyedMCOMEntity.GetOrigin();
		
		// Delayed explosion effects for better synchronization
		GetGame().GetCallqueue().CallLater(CreateExplosionEffects, 385, false, spawnParams, destroyedMCOMEntity);
		
		// Initial explosion
		GetGame().SpawnEntityPrefab(Resource.Load("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et"), GetGame().GetWorld(), spawnParams);
	}
	
	/**
	 * Create delayed explosion effects
	 * @param spawnParams Spawn parameters for effects
	 * @param destroyedMCOM The destroyed MCOM entity
	 */
	protected void CreateExplosionEffects(EntitySpawnParams spawnParams, IEntity destroyedMCOM)
	{
		// Spawn rubble
		GetGame().SpawnEntityPrefab(Resource.Load("{5A81BD9171FC3B07}Prefabs/Structures/Ruins/HouseRuins/HouseRuin_01/HouseRuin_01_BrickPile_Big.et"), GetGame().GetWorld(), spawnParams);
		
		// Secondary explosion
		GetGame().SpawnEntityPrefab(Resource.Load("{BCE4E0823FCFBCB7}Prefabs/Weapons/Warheads/Explosions/Explosion_AmmoRack_Large.et"), GetGame().GetWorld(), spawnParams);
		
		// Fire effects
		GetGame().SpawnEntityPrefab(Resource.Load("{4BE47BA2B7E3877E}Prefabs/Systems/Fire/Wrapper_Fire_Large_Damage.et"), GetGame().GetWorld(), spawnParams);
		
		// Delete the original MCOM entity
		SCR_EntityHelper.DeleteEntityAndChildren(destroyedMCOM);
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
		array<string> zones = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
		array<string> letters = {"A", "B", "A", "B", "A", "B"};
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
		IEntity mcomEntity;
		
		// Try to get from dynamic arrays first
		int zoneIndex, mcomIndex;
		if (ParseMCOMIdentifier(mcomIdentifier, zoneIndex, mcomIndex))
		{
			if (m_aMCOMEntities && zoneIndex < m_aMCOMEntities.Count() && mcomIndex < m_aMCOMEntities[zoneIndex].Count())
			{
				mcomEntity = m_aMCOMEntities[zoneIndex][mcomIndex];
				if (mcomEntity)
					return mcomEntity;
			}
		}
		
		// Fallback to legacy member variables for backward compatibility
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
		
		// If still null, try to find by entity name
		if (!mcomEntity)
		{
			string entityName = GetMCOMEntityName(mcomIdentifier);
			if (!entityName.IsEmpty())
				mcomEntity = GetGame().GetWorld().FindEntityByName(entityName);
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
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "Zone1Alpha": // Legacy compatibility
				return m_Zone1AlphaID;
			case "MCOMB":
			case "Zone1Beta": // Legacy compatibility
				return m_Zone1BetaID;
			case "MCOMC":
			case "Zone2Alpha": // Legacy compatibility
				return m_Zone2AlphaID;
			case "MCOMD":
			case "Zone2Beta": // Legacy compatibility
				return m_Zone2BetaID;
			case "MCOME":
			case "Zone3Alpha": // Legacy compatibility
				return m_Zone3AlphaID;
			case "MCOMF":
			case "Zone3Beta": // Legacy compatibility
				return m_Zone3BetaID;
		}
		return EntityID.INVALID;
	}
	
	/**
	 * Get zone number from MCOM identifier
	 * @param mcomIdentifier The MCOM identifier
	 * @return Zone number (1-3)
	 */
	protected int GetZoneNumber(string mcomIdentifier)
	{
		// Handle new sequential naming
		switch (mcomIdentifier)
		{
			case "MCOMA":
			case "MCOMB":
				return 1;
			case "MCOMC":
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
	 * Get display name for site (Alpha/Beta)
	 * @param mcomIdentifier The MCOM identifier
	 * @return Site display name
	 */
	protected string GetSiteDisplayName(string mcomIdentifier)
	{
		if (mcomIdentifier.EndsWith("Alpha")) return "Alpha";
		if (mcomIdentifier.EndsWith("Beta")) return "Beta";
		return "Unknown";
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
		
		// Check all possible MCOMs and add those in the current active zone
		array<string> allMCOMs = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
		
		for (int i = 0; i < allMCOMs.Count(); i++)
		{
			if (IsMCOMInActiveZone(allMCOMs[i]))
			{
				activeMCOMs.Insert(allMCOMs[i]);
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
		
		EntityID entityID = mcomEntity.GetID();
		
		if (entityID == m_Zone1AlphaID) return "MCOMA";
		if (entityID == m_Zone1BetaID) return "MCOMB";
		if (entityID == m_Zone2AlphaID) return "MCOMC";
		if (entityID == m_Zone2BetaID) return "MCOMD";
		if (entityID == m_Zone3AlphaID) return "MCOME";
		if (entityID == m_Zone3BetaID) return "MCOMF";
		
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
		
		// Determine marker details
		string markerText;
		string iconPath;
		int markerColor;
		
		if (mcomIdentifier.EndsWith("Alpha"))
		{
			if (isPlanted)
				markerText = "MCOM Alpha (PLANTED!)";
			else
				markerText = "MCOM Alpha";
			iconPath = "{21A2A457BD0E42C1}UI/Objectives/A.edds";
		}
		else
		{
			if (isPlanted)
				markerText = "MCOM Beta (PLANTED!)";
			else
				markerText = "MCOM Beta";
			iconPath = "{7F4A8D140283CCCE}UI/Objectives/B.edds";
		}
		
		// Use bright red for planted, white for unplanted
		if (isPlanted)
			markerColor = ARGB(200, 255, 50, 50);
		else
			markerColor = ARGB(200, 0, 0, 0);
		
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
