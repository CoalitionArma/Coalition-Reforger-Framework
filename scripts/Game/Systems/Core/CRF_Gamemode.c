//------------------------------------------------------------------------------------
// CRF_GamemodeClass: Base class definition for the Coalition Reforger Framework Gamemode
//------------------------------------------------------------------------------------
class CRF_GamemodeClass : SCR_BaseGameModeClass {}

//------------------------------------------------------------------------------------
// Mission briefing descriptor for displaying mission information
//------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sTitle"}, "%1")]
class CRF_MissionDescriptor
{
	[Attribute("")]
	string m_sTitle;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sTextData;

	[Attribute("")]
	ref array<string> m_aFactionKeys;

	[Attribute("")]
	bool m_bShowForAnyFaction;
}

//------------------------------------------------------------------------------------
// CRF_Gamemode: Main gamemode controller for Coalition Reforger Framework
// Handles mission flow, player management, respawn, and faction settings
//------------------------------------------------------------------------------------
class CRF_Gamemode : SCR_BaseGameMode
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	// Game State Properties
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "OnGamemodeStateChanged")]
	int m_GamemodeState = CRF_EGamemodeState.BRIEFING;

	[RplProp()]
	int m_SlottingState = CRF_ESlottingState.LEADERSANDMEDICS;
	
	//AI Settings
	[Attribute("1", "auto", "Disables AI Crouching", category: "CRF AI Settings")]
	bool m_bDisableAICrouching;
	
	// General Gamemode Settings
	//------------------------------------------------------------------------------------
	[Attribute("0", "auto", "Should this mission go to AAR after)", category: "CRF Gamemode General")]
	bool m_bUseAAR;
	
	[Attribute("45", "auto", "Mission Time (set to -1 to disable)", category: "CRF Gamemode General")]
	int m_iTimeLimitMinutes;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Gamemode General")]
	bool m_bAllowEspionage;

	[Attribute("false", "auto", "Enables AI autonomy while in GAME state", category: "CRF Gamemode General")]
	bool EnableAIInGameState;
	
	[RplProp()] bool m_bCurrentEnableAIInGameState = EnableAIInGameState;
	
	[Attribute("true", "auto", "Disable chat messages except tickets & messages from admins/mods", category: "CRF Gamemode General")]
	bool m_bDisableChat;

	[Attribute("true", "auto", "Should we lock all JIP slots after SafeStart turns off? COOP = FALSE", category: "CRF Gamemode General")]
	bool m_bLockSlotsAfterSafestart;

	[Attribute("true", "auto", "If safestart turns on instantly after the lobby screen.", category: "CRF Gamemode General")]
	bool m_bSafestartInstantlyEnabled;

	// Mission Descriptors (shown in briefing)
	[Attribute("", category: "CRF Gamemode General")]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;

	// Faction Settings
	//------------------------------------------------------------------------------------
	[Attribute("1", "auto", "", category: "CRF Gamemode Slotting")]
	int m_iFactionOneRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Gamemode Slotting")]
	string m_sFactionOneKey;

	[Attribute("1", "auto", "", category: "CRF Gamemode Slotting")]
	int m_iFactionTwoRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Gamemode Slotting")]
	string m_sFactionTwoKey;

	// Gearscript Settings
	//------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;
	[RplProp()] ResourceName m_rBLUFORCurrentGearScript = m_BLUFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;
	[RplProp()] ResourceName m_rOPFORCurrentGearScript = m_OPFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;
	[RplProp()] ResourceName m_rINDFORCurrentGearScript = m_INDFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;
	[RplProp()] ResourceName m_rCIVILIANCurrentGearScript = m_CIVILIANGearScriptSettings.m_rGearScript;

	// Respawn Settings
	//------------------------------------------------------------------------------------
	[Attribute("0", "auto", "", category: "CRF Gamemode Respawn")]
	bool m_bRespawnEnabled;

	[Attribute("0", "auto", "", category: "CRF Gamemode Respawn")]
	bool m_bWaveRespawn;

	[Attribute("60", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Gamemode Respawn")]
	int m_iTimeToRespawn;

	[Attribute("0", UIWidgets.EditBox, "Amount of BLUFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iBLUFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of OPFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iOPFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iINDFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iCIVTickets;

	// Generic spawn point for spectator camera (handles entity streaming)
	[RplProp()]
	vector m_vGenericSpawn[4];
	bool m_bGenericSpawnSet = false;
	
	// Manager References and System Components
	//------------------------------------------------------------------------------------
	protected ref ScriptInvoker m_OnStateChanged;
	protected static ref SCR_PlayerData m_PlayerData;
	
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_LoggingManager m_LoggingManager;
	
	protected static CRF_Gamemode m_sInstance;
	
	protected ref array<Vehicle> m_aSpawnedVehicles = {};
	
	bool m_bIsInEndCredits = false;
	
	// Staggered Player Initialization System
	//------------------------------------------------------------------------------------
	protected ref array<int> m_aPendingPlayerInitializations = {};
	protected bool m_bProcessingInitializations = false;
	protected const int PLAYERS_PER_BATCH = 8;        // Players spawned per batch
	protected const int BATCH_INTERVAL_MS = 150;      // Milliseconds between batches
	protected float m_fBatchTimer = 0.0;              // Timer for batch processing

	//===================================================================================
	// STATIC METHODS
	//===================================================================================
	
	/**
	 * Returns the singleton instance of the CRF_Gamemode
	 * @return CRF_Gamemode instance or null if not available
	 */
	
	void CRF_Gamemode(IEntitySource src, IEntity parent)
	{
		m_sInstance = this;
		// Initialize ScriptInvoker to avoid null checks - PERFORMANCE OPTIMIZATION
		m_OnStateChanged = new ScriptInvoker();
	}
	
	static CRF_Gamemode GetInstance()
	{
		return m_sInstance;
	}

	//===================================================================================
	// INITIALIZATION AND SETUP
	//===================================================================================
	
	/**
	 * Initialize the gamemode and all required manager instances
	 * @param owner The entity that owns this component
	 */
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Load configs on dedicated server
		if (RplSession.Mode() == RplMode.Dedicated) {
			CRF_ModeratorConfig.LoadConfig();	
			CRF_DonatorConfig.LoadConfig();
		}
			
	
		// Initialize all manager references
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_LoggingManager = CRF_LoggingManager.GetInstance();
		
		// Enable frame events for batch processing
		SetEventMask(EntityEvent.FRAME);
	}
	
	//===================================================================================
	// FRAME UPDATES
	//===================================================================================
	
	/**
	 * Frame update for batch processing player initializations
	 * More reliable than CallLater for time-critical operations
	 */
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Only process if we have pending initializations
		if (!m_bProcessingInitializations || m_aPendingPlayerInitializations.IsEmpty())
			return;
		
		// Accumulate time
		m_fBatchTimer += timeSlice * 1000; // Convert to milliseconds
		
		// Check if enough time has passed for next batch
		if (m_fBatchTimer >= BATCH_INTERVAL_MS)
		{
			ProcessPlayerBatch();
			m_fBatchTimer = 0.0; // Reset timer
		}
	}
	
	//===================================================================================
	// STATE MANAGEMENT
	//===================================================================================
	
	/**
	 * Progress to the next slotting state
	 * Updates all slotting UI and synchronizes across network
	 */
	void AdvanceSlottingState()
	{
		m_SlottingState += 1;
		Replication.BumpMe();  // m_SlottingState is [RplProp()] - auto-synced to clients
		
		// Notify all clients to refresh their slotting UI
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.NotifySlottingPhaseChanged();
	}

	/**
	 * Progress to the next gamemode state
	 * @param overriden Set to true to allow advancing from AAR or GAME states
	 */
	void AdvanceGamemodeState(bool overriden = false)
	{
		// Prevent advancing from AAR or GAME unless explicitly overridden
		if ((m_GamemodeState == CRF_EGamemodeState.AAR || m_GamemodeState == CRF_EGamemodeState.GAME) && !overriden)
			return;

		m_GamemodeState += 1;
		if (m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			foreach (Vehicle vehicle: m_aSpawnedVehicles)
			{
				if (!vehicle)
					continue;
				
				vehicle.SpawnVehiclePassengers();
			}
		}
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}

	/**
	 * Get the state change event invoker
	 * @return ScriptInvoker for state change events
	 */
	ScriptInvoker GetOnStateChanged()
	{
		return m_OnStateChanged;
	}
	
	/**
	 * Handle gamemode state changes
	 * Triggers UI updates and state-specific logic
	 */
	protected void OnGamemodeStateChanged()
	{
		// Server-side state change handling
		if (Replication.IsServer())
		{
			// Invoke state changed (invoker already initialized in constructor)
			m_OnStateChanged.Invoke();
			
			// Set basic game mode states for basegamemode
			// useful for default components that reference it like datacollector
			switch (m_GamemodeState) {
				case CRF_EGamemodeState.GAME: {
					SetGameState(SCR_EGameModeState.GAME);
					break;
				}
				
				case CRF_EGamemodeState.AAR: {
					//SetGameState(SCR_EGameModeState.POSTGAME);
					EnterAAR();
					break;
				}
				
			}	
		}
		
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerComp)
			playerControllerComp.OpenCurrentStateMenu();
	}
	
	/**
	 * Handle entering the After Action Report state
	 * Processes player data and prepares for mission end
	 */
	protected void EnterAAR()
	{
		// Server only just in case
		if (Replication.IsClient())
			return;
		
		//Print("[CRF] EnterAAR()");
		SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
		dataCollector.OnGameModeEnd(GetEndGameData());
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		CRF_MenuManager menuManager = CRF_MenuManager.GetInstance();
		
		foreach (int player : players)
		{
			// Skip disconnected players
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			
			// Process player statistics data
			ProcessStats(dataCollector,player);
			
			if (m_bUseAAR)
			{
				IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(player);
				if (!playerEntity)
					continue;
				
				// Check if player is already dead/spectating
				bool isPlayerAlreadyDead = CRF_GamemodeManager.IsSpectator(playerEntity);
				
				// Move all players to spectator mode for AAR interface and communication
				// This preserves their actual alive/dead status while allowing AAR participation
				if (!isPlayerAlreadyDead)
				{
					// Player is alive - force into spectator for AAR without marking as dead
					ForcePlayerToSpectatorForAAR(player, playerEntity);
				}
				// Players already in spectator mode don't need repositioning
				
				//Adds them to default channel
				menuManager.AddPlayerToChannel(player, 1, false);
			}
		}
		
		if (!m_bUseAAR)
			CRF_RplBroadcastManager.GetInstance().BroadcastOutro();
		
		// Stores player profiles who havent disconnected
		dataCollector.OnGameEnd();
		
		// Make sure we close logging memory leak
		m_LoggingManager.OnGameModeEnd(GetEndGameData());
	}
	
	void ProcessStats(SCR_DataCollectorComponent dataCollector, int player)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(player);
		//PrintFormat("[CRF] Logging Stats for player %1",name);
		// Process player statistics data
		if (!m_PlayerData)
		{
			if (!dataCollector)
			{
				Print("[CRF] CRF_Gamemode SCR_DataCollectorComponent: No data collector was found.", LogLevel.ERROR);
				return;
			}
	
			m_PlayerData = dataCollector.GetPlayerData(player, false);
	
			// If player data isn't available yet, register for notification when it arrives
			if (!m_PlayerData)
			{
				SCR_DataCollectorCommunicationComponent communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(
					GetGame().GetPlayerManager().GetPlayerController(player).FindComponent(SCR_DataCollectorCommunicationComponent)
				);
				
				if (communicationComponent)
					communicationComponent.GetOnDataReceived().Insert(OnDataReceived);
			} else {
				m_PlayerData.CalculateStatsChange();
			}
		}
	}
	
	//===================================================================================
	// PLAYER MANAGEMENT
	//===================================================================================
	
	/**
	 * Handle player data received from network
	 * @param playerData Player statistics and progress data
	 */
	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		m_PlayerData = playerData;
		m_PlayerData.CalculateStatsChange();
	}
	
	/**
	 * Process player connection after authentication
	 * @param iPlayerID ID of the connecting player
	 */
	protected override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);
		
		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		m_GamemodeManager.InitilizePlayer(iPlayerID, CRF_GamemodeManager.ZERO_SPAWN_VECTOR);

		// Check if player is a moderator/donator and set privileges
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(iPlayerID);
		if (!playerIdentity.IsEmpty()) {
			if (CRF_ModeratorConfig.IsModerator(playerIdentity))
				m_GamemodeManager.SetPlayerStatus(iPlayerID, "mod");
			
			if (CRF_DonatorConfig.IsDonator(playerIdentity))
				m_GamemodeManager.SetPlayerStatus(iPlayerID, "don");
		}
	}
	
	
	//------------------------------------------------------------------------------------------------
	/*!
		Called after a player is disconnected.
		\param playerId PlayerId of disconnected player.
	*/
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		
		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually. 
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
	}
	
	//===================================================================================
	// ENTITY MANAGEMENT
	//===================================================================================
	
	/**
	 * Process entity spawning for players
	 * @param entity The spawned entity
	 */
	protected override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		// Check if we are not in the "GAME" state
		if (m_GamemodeState != CRF_EGamemodeState.GAME && !m_bGenericSpawnSet && !CRF_GamemodeManager.IsSpectator(entity))
		{
			// Update generic spawnpoint for spectator cameras
			entity.GetWorldTransform(m_vGenericSpawn);
			m_bGenericSpawnSet = true;
		}
			
		// Handle initial entity race condition fix
		if (entity && entity.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
			if (playerId > 0 && m_GamemodeState == CRF_EGamemodeState.GAME)
			{
				// Check if player should have a proper character instead of initial entity
				if (m_SlottingManager.IsPlayerInASlot(playerId) && !m_SlottingManager.IsPlayerConsideredDead(playerId))
				{
					// Schedule re-initialization to fix race condition
					GetGame().GetCallqueue().CallLater(OnControllableInitilizePlayerDelayed, 500, false, playerId, CRF_GamemodeManager.ZERO_SPAWN_VECTOR[0], CRF_GamemodeManager.ZERO_SPAWN_VECTOR[1], CRF_GamemodeManager.ZERO_SPAWN_VECTOR[2], CRF_GamemodeManager.ZERO_SPAWN_VECTOR[3]);
				}
			}
		}
		
		// Apply gearscript/identity if in play mode and not on client
		if (GetGame().InPlayMode() && entity && entity.GetPrefabData() && !m_GamemodeManager.IsSpectator(entity) && m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			// Ensure gearscript manager is available
			if (!m_GearscriptManager)
				m_GearscriptManager = CRF_GearscriptManager.GetInstance();
			
			// Schedule gearscript identity setup with appropriate delay
			GetGame().GetCallqueue().Call(
				m_GearscriptManager.SetEntityIdentity, 
				entity
			);
		
			// Apply gearscript if not on client
			if (RplSession.Mode() != RplMode.Client)
			{
				// Ensure gearscript manager is available
				if (!m_GearscriptManager)
					m_GearscriptManager = CRF_GearscriptManager.GetInstance();
				
				// Schedule gear setup with appropriate delay
				GetGame().GetCallqueue().Call(
					m_GearscriptManager.SetEntityGear, 
					entity, 
					entity.GetPrefabData().GetPrefabName()
				);
			};
		}
	}

	/**
	 * Process entity death/destruction for players
	 * Handles respawn and spectator logic
	 * @param entity The destroyed entity
	 * @param killerEntity The entity that caused the destruction
	 * @param instigator The instigator context
	 */
	protected override void OnControllableDestroyed(IEntity entity, IEntity killerEntity, notnull Instigator instigator)
	{
		super.OnControllableDestroyed(entity, killerEntity, instigator);

		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Note: The base game's data collector is automatically triggered by super.OnControllableDestroyed()
		// Our modded CRF_SCR_DataCollectorComponent.OnPlayerKilled() hooks into this and calls the logging manager
		
		// Create instigator context for tracking kill details
		SCR_InstigatorContextData instigatorContextData = new SCR_InstigatorContextData(-1, entity, killerEntity, instigator);
		int playerId = instigatorContextData.GetVictimPlayerID();
		
		// Return if not a player character
		if (playerId <= 0 || instigatorContextData.GetVictimCharacterControlType() == SCR_ECharacterControlType.POSSESSED_AI)
			return;

		// Determine delay time for respawn/spectator
		int delay = 2000;
		if (CRF_GamemodeManager.IsSpectator(entity))
			delay = 0;
		
		// Get player faction
		Faction faction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
		FactionKey factionKey;
		
		if (faction)
			factionKey = faction.GetFactionKey();

		// Handle respawn if enabled and tickets available
		if (m_RespawnManager.m_bCurrentRespawnEnabled && 
			!CRF_GamemodeManager.IsSpectator(entity) && 
			m_GamemodeState != CRF_EGamemodeState.AAR && 
			m_RespawnManager.TicketsRemaining(factionKey) &&
			!m_RespawnManager.GetFactionSpawnpoints(factionKey).IsEmpty() &&
			!factionKey.IsEmpty())
		{
			// Deduct ticket
			m_RespawnManager.SubtractTicket(factionKey, 1);

			// Display respawn screen
			GetGame().GetCallqueue().CallLater(
				m_RplBroadcastManager.SendRespawnScreen, 
				(delay + 150), 
				false, 
				playerId
			);
		}

		// Update slot death state so player gets put into spec
		int slotID = m_SlottingManager.GetCharacterSlotID(entity);
		
		if(slotID != -1)
			m_SlottingManager.UpdateSlotDeathState(slotID, true);

		// Get death position for spectator camera initialization
		vector deathPosition[4];
		entity.GetWorldTransform(deathPosition);

		// Move player to spectator
		GetGame().GetCallqueue().CallLater(OnControllableInitilizePlayerDelayed, delay, false, playerId, deathPosition[0], deathPosition[1], deathPosition[2], deathPosition[3]);
	}
	
	/**
	* Can't use static vectors in callLater, so we just use this container method to act as a holder for the call later  
	* @param playerId ID of the player to initialize
	* @param locationZero Position 0 in the world vector to spawn the player
	* @param locationOne Position 1 in the world vector to spawn the player
	* @param locationTwo Position 2 in the world vector to spawn the player
	* @param locationThree Position 3 in the world vector to spawn the player
	*/
	void OnControllableInitilizePlayerDelayed(int playerId, vector locationZero, vector locationOne, vector locationTwo, vector locationThree)
	{
		vector location[4];
		
		location[0] = locationZero;
		location[1] = locationOne;
		location[2] = locationTwo;
		location[3] = locationThree;
		
		m_GamemodeManager.InitilizePlayer(playerId, location);
	}
	
	/**
	 * Forces a living player to die and enter spectator mode for AAR without permanently marking their slot as dead
	 * This allows proper death handling while preserving their alive status for AAR display
	 * @param playerId The player ID to kill and move to spectator
	 * @param playerEntity The player's current entity
	 */
	void ForcePlayerToSpectatorForAAR(int playerId, IEntity playerEntity)
	{
		if (!playerEntity || playerId <= 0)
			return;
		
		// Get the player's slot ID before killing them
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		if (slotId == -1)
			return;
		
		// Store original alive state (should be false since they're alive)
		bool originalDeadState = m_SlottingManager.IsPlayerConsideredDead(playerId);
		
		// Kill the player to trigger proper death handling and spectator transition
		// This will automatically handle the transition to spectator mode
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_CharacterDamageManagerComponent)
		);
		
		if (!damageManager)
			return;
			
		HitZone defaultHitZone = damageManager.GetDefaultHitZone();
		if (defaultHitZone)
			defaultHitZone.SetHealth(0);
		
		// Schedule restoration of original alive status after death processing completes
		// This ensures the player shows as alive in AAR even though they were killed for transition
		GetGame().GetCallqueue().CallLater(RestorePlayerAliveStatusForAAR, 3000, false, slotId, originalDeadState);
	}
	
	/**
	 * Restores a player's original alive/dead status after they've been moved to spectator for AAR
	 * @param slotId The slot ID of the player
	 * @param originalDeadState The player's original dead state before AAR
	 */
	void RestorePlayerAliveStatusForAAR(int slotId, bool originalDeadState)
	{
		// Restore the player's original alive/dead status
		// This ensures the AAR display shows their actual mission-end status
		m_SlottingManager.UpdateSlotDeathState(slotId, originalDeadState);
	}
	
	void UpdateGearscriptResource(string factionKey, string resource)
	{
		switch (factionKey)
		{
			case "BLUFOR" : m_rBLUFORCurrentGearScript = resource; break;
			case "OPFOR" : m_rOPFORCurrentGearScript = resource; break;
			case "INDFOR" : m_rINDFORCurrentGearScript = resource; break;
			case "CIV" : m_rCIVILIANCurrentGearScript = resource; break;
		}
		Replication.BumpMe();
	}
	
	void ToggleEnableAIInGameState()
	{
		m_bCurrentEnableAIInGameState = !m_bCurrentEnableAIInGameState;
		Replication.BumpMe();
	}
	
	void AddVehicleToArray(Vehicle vehicle)
	{
		if (m_aSpawnedVehicles.Contains(vehicle))
			return;
		
		m_aSpawnedVehicles.Insert(vehicle);
	}
	
	void RemoveVehicleFromArray(Vehicle vehicle)
	{
		if (!m_aSpawnedVehicles.Contains(vehicle))
			return;
		m_aSpawnedVehicles.RemoveItem(vehicle);
	}
	
	//===================================================================================
	// STAGGERED PLAYER INITIALIZATION SYSTEM
	//===================================================================================
	
	/**
	 * Queue a player for staggered initialization
	 * Prevents server overload by batching player spawns
	 * @param playerId ID of the player to initialize
	 */
	void QueuePlayerInitialization(int playerId)
	{
		// Don't queue if already pending
		if (m_aPendingPlayerInitializations.Contains(playerId))
			return;
		
		m_aPendingPlayerInitializations.Insert(playerId);
		
		// Start processing if not already running
		if (!m_bProcessingInitializations)
		{
			m_bProcessingInitializations = true;
			m_fBatchTimer = 0.0; // Reset timer
			
			// Notify slotting manager that mass initialization is starting
			if (m_SlottingManager)
				m_SlottingManager.SetMassInitializationInProgress(true);
			
			//Print(string.Format("[CRF] Starting batch initialization for %1 players", m_aPendingPlayerInitializations.Count()), LogLevel.NORMAL);
		}
	}
	
	/**
	 * Process a batch of pending player initializations
	 * Called by EOnFrame when timer interval is reached
	 * Spawns players in small groups to distribute server load
	 */
	protected void ProcessPlayerBatch()
	{
		if (m_aPendingPlayerInitializations.IsEmpty())
		{
			m_bProcessingInitializations = false;
			
			// Notify slotting manager that mass initialization is complete
			if (m_SlottingManager)
				m_SlottingManager.SetMassInitializationInProgress(false);
			
			Print("[CRF] Player initialization queue complete", LogLevel.NORMAL);
			return;
		}
		
		// Process a batch of players
		int playersToProcess = Math.Min(PLAYERS_PER_BATCH, m_aPendingPlayerInitializations.Count());
		
		Print(string.Format("[CRF] Processing batch: %1 players (%2 remaining)", 
			playersToProcess, m_aPendingPlayerInitializations.Count()), LogLevel.VERBOSE);
		
		for (int i = 0; i < playersToProcess; i++)
		{
			int playerId = m_aPendingPlayerInitializations[0];
			m_aPendingPlayerInitializations.Remove(0);
			
			// Initialize the player immediately
			if (m_GamemodeManager)
				m_GamemodeManager.InitilizePlayer(playerId, CRF_GamemodeManager.ZERO_SPAWN_VECTOR);
		}
	}
	
	/**
	 * Clear all pending player initializations
	 * Used when resetting game state
	 */
	void ClearPlayerInitializationQueue()
	{
		m_aPendingPlayerInitializations.Clear();
		m_bProcessingInitializations = false;
		
		if (m_SlottingManager)
			m_SlottingManager.SetMassInitializationInProgress(false);
	}
	
	/**
	 * Check if a player is waiting in the initialization queue
	 * @param playerId Player to check
	 * @return True if player is queued for initialization
	 */
	bool IsPlayerQueuedForInitialization(int playerId)
	{
		return m_aPendingPlayerInitializations.Contains(playerId);
	}
	
	vector ComputeAOCenter(vector pts[4])
	{
		vector sum = "0 0 0";
		int count = 0;
	
		for (int i = 0; i < 4; i++)
		{
			vector p = pts[i];
			if (p[0] == 0 && p[1] == 0 && p[2] == 0)   // ignore empty
				continue;
	
			sum += p;
			count++;
		}
	
		if (count == 0)
			return "0 0 0";   // no data
	
		return sum / count;
	}
	
	float ComputeAORadius(vector pts[4], vector center)
	{
		float maxDist = 0;
	
		for (int i = 0; i < 4; i++)
		{
			vector p = pts[i];
			if (p[0] == 0 && p[1] == 0 && p[2] == 0)
				continue;
	
			float d = vector.Distance(center, p);
			if (d > maxDist)
				maxDist = d;
		}
	
		return maxDist;
	}
	
	void GetAOCenterAndRadius(out vector center, out float radius)
	{
		CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
		//We are cooked
		if (!slottingMan)
			return;
		
	 	center = ComputeAOCenter(slottingMan.m_vLastSlotRegisteredPosition);
		radius = ComputeAORadius(slottingMan.m_vLastSlotRegisteredPosition, center);
	}
	
	bool DoesFactionShareMarker(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": 
				return m_BLUFORGearScriptSettings.m_bEnableShareableMarkers;
			case "OPFOR": 
				return m_OPFORGearScriptSettings.m_bEnableShareableMarkers;
			case "INDFOR": 
				return m_INDFORGearScriptSettings.m_bEnableShareableMarkers;
			case "CIV": 
				return m_CIVILIANGearScriptSettings.m_bEnableShareableMarkers;
    	 }
    	return true;
 	}
	
	bool IsSideBFTEnabled(string factionKey)
	{
		switch(factionKey)
		{
			case "BLUFOR":
				return m_BLUFORGearScriptSettings.m_bEnableBFT;
				break;
			case "OPFOR":
				return m_OPFORGearScriptSettings.m_bEnableBFT;
				break;
			case "INDFOR":
				return m_INDFORGearScriptSettings.m_bEnableBFT;
				break;
			case "CIV":
				return m_CIVILIANGearScriptSettings.m_bEnableBFT;
				break;
		}
   		return true;
	}
}
//------------------------------------------------------------------------------------
// Fix for manual camera to work with spectator menu
//------------------------------------------------------------------------------------
modded class SCR_ManualCamera
{
	/**
	 * Determine if camera control is disabled by menu
	 * Modified to allow camera control in spectator menu
	 * @return True if camera should be disabled, false otherwise
	 */
	override protected bool IsDisabledByMenu()
	{
		if (!m_MenuManager)
			return false;

		if (m_MenuManager.IsAnyDialogOpen())
			return true;

		MenuBase topMenu = m_MenuManager.GetTopMenu();
		
		// Allow camera control in editor and spectator menus
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(CRF_SpectatorMenu));
	}
}

modded class SCR_BaseGameMode
{
	void SetGameState(SCR_EGameModeState state)
	{
		m_eGameState = state;
		Replication.BumpMe();
	}
}
