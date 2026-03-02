//------------------------------------------------------------------------------------
// CRF_GamemodeClass: Base class definition for the Coalition Reforger Framework Gamemode
//------------------------------------------------------------------------------------
class CRF_GamemodeClass : SCR_BaseGameModeClass {}

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
	
	// Attributes Set By Plugins
	//------------------------------------------------------------------------------------
	[Attribute("0", UIWidgets.Hidden)]
	bool m_bRespawnEnabled;

	[Attribute("0", UIWidgets.Hidden)]
	bool m_bWaveRespawn;

	[Attribute("60", UIWidgets.Hidden)]
	int m_iTimeToRespawn;
	
	[Attribute("0", UIWidgets.Hidden, desc: "Minutes before mission end when respawns disable (0 = never disable)", category: "CRF Gamemode Settings - Respawn")]
	int m_iRespawnCutoffMinutes;
	
	[Attribute("45", UIWidgets.Hidden)]
	int m_iTimeLimitMinutes;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bAllowEspionage;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bLockUnusedSlots;

	[Attribute("true", UIWidgets.Hidden)]
	bool m_bSafestartInstantlyEnabled;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bUseSafestartTimeLimit;
	
	[Attribute("0", UIWidgets.Hidden)]
	int m_iSafestartTimeLimit;
	
	[Attribute("", UIWidgets.Hidden)]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;
	
	[Attribute("", UIWidgets.Hidden)]
	int m_iFactionOneRatio;

	[Attribute("", UIWidgets.Hidden)]
	int m_iFactionTwoRatio;
	
	[Attribute("", UIWidgets.Hidden)]
	string m_sFactionOneKey;

	[Attribute("", UIWidgets.Hidden)]
	string m_sFactionTwoKey;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_BluforSlots;

	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_OpforSlots;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_IndforSlots;
	
	[Attribute("", UIWidgets.Hidden)]
	ref array <ref CRF_SlottingGroup> m_CivSlots;
	
	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iBLUFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iOPFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iINDFORTickets;

	[Attribute("0", UIWidgets.Hidden), RplProp()]
	int m_iCIVTickets;
	
	// Advanced Gamemode Settings
	//------------------------------------------------------------------------------------
	[Attribute("0", "auto", "Disables AI Crouching", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableAICrouching;
	
	[Attribute("true", "auto", "Disable chat messages except tickets & messages from admins/mods", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableChat;

	// Gearscript Settings
	//------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;
	[RplProp()] ResourceName m_rBLUFORCurrentGearScript = m_BLUFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;
	[RplProp()] ResourceName m_rOPFORCurrentGearScript = m_OPFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;
	[RplProp()] ResourceName m_rINDFORCurrentGearScript = m_INDFORGearScriptSettings.m_rGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;
	[RplProp()] ResourceName m_rCIVILIANCurrentGearScript = m_CIVILIANGearScriptSettings.m_rGearScript;
	
	// Manager References and System Components
	//------------------------------------------------------------------------------------
	protected ref ScriptInvoker m_OnStateChanged;
	protected static ref SCR_PlayerData m_PlayerData;
	
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_PermissionManager m_PermissionManager
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_LoggingManager m_LoggingManager;
	protected CRF_GarbageManager m_GarbageManager
	
	protected static CRF_Gamemode m_sInstance;
	
	[RplProp()]
	protected vector m_vGenericSpawn;
	
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	static CRF_Gamemode GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetGenericSpawn()
	{
		return m_vGenericSpawn;
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnStateChanged()
	{
		return m_OnStateChanged;
	}

	//===================================================================================
	// INITIALIZATION AND SETUP
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
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
			CRF_BugReportConfig.LoadConfig();
			
			// Initialize sight arsenal registry for optimized RPC
			CRF_SightArsenalRegistry.InitializeRegistry();
		}
	
		// Initialize all manager references
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_PermissionManager = CRF_PermissionManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_LoggingManager = CRF_LoggingManager.GetInstance();
		m_GarbageManager = CRF_GarbageManager.GetInstance();
		
		// Enable frame events for batch processing
		SetEventMask(EntityEvent.FRAME);
	}

	//===================================================================================
	// FRAME UPDATES
	//===================================================================================

	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
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
			foreach (Vehicle vehicle: CRF_VehicleGearscriptManager.GetInstance().GetSpawnedVehicleArray())
			{
				if (!vehicle)
					continue;
				
				vehicle.SpawnVehiclePassengers();
			}
		}
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}
	
	//------------------------------------------------------------------------------------------------
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
					SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
					dataCollector.OnGameModeEnd(GetEndGameData());

					array<int> players = {};
					GetGame().GetPlayerManager().GetAllPlayers(players);

					foreach (int player : players)
					{
						// Skip disconnected players
						if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
							continue;

						// Process player statistics data
						ProcessStats(dataCollector, player);
					}

					CRF_RplBroadcastManager.GetInstance().BroadcastOutro();

					// Stores player profiles who havent disconnected
					dataCollector.OnGameEnd();

					// Make sure we close logging memory leak
					m_LoggingManager.OnGameModeEnd(GetEndGameData());
					break;
				}
				
			}	
		}
		
		CRF_PlayerMenuManager playerMenuManager = CRF_PlayerMenuManager.GetInstance();
		if (playerMenuManager)
			playerMenuManager.OpenCurrentStateMenu();
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Handle player data received from network
	 * @param playerData Player statistics and progress data
	 */
	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		m_PlayerData = playerData;
		m_PlayerData.CalculateStatsChange();
	}
	
	//------------------------------------------------------------------------------------------------
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
			
		m_GamemodeManager.InitilizePlayer(iPlayerID, CRF_EntityHelper.ZERO_SPAWN_VECTOR);

		// Get player's BI account GUID for privilege checks
		string playerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(iPlayerID);
		
		// Check if player is the mission designer and grant admin chat
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		
		if (missionHeader && missionHeader.m_sAuthorGUID && !missionHeader.m_sAuthorGUID.IsEmpty() && !playerGUID.IsEmpty())
		{
			// Compare player's BI account GUID with mission author's GUID
			if (playerGUID == missionHeader.m_sAuthorGUID)
			{
				// Grant session admin (admin chat) to mission designer
				GetGame().GetPlayerManager().GivePlayerRole(iPlayerID, EPlayerRole.SESSION_ADMINISTRATOR);
			}
		}

		// Check if player is a moderator/donator and set privileges
		if (!playerGUID.IsEmpty()) {
			if (CRF_ModeratorConfig.IsModerator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "mod");
			
			if (CRF_DonatorConfig.IsDonator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "don");
		}
		if (!playerGUID.IsEmpty()) {
			if (CRF_ModeratorConfig.IsModerator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "mod");
			
			if (CRF_DonatorConfig.IsDonator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "don");
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
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Process entity spawning for players
	 * @param entity The spawned entity
	 */
	protected override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		
		if (!character)
			return;
			
		// Handle initial entity race condition fix
		if (character.GetPrefabData().GetPrefabName() == CRF_EntityHelper.GetSpectatorResource())
		{
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
			if (playerId > 0 && m_GamemodeState == CRF_EGamemodeState.GAME)
			{
				// Check if player should have a proper character instead of initial entity
				if (m_SlottingManager.IsPlayerInASlot(playerId) && !m_SlottingManager.IsPlayerConsideredDead(playerId))
				{
					// Schedule re-initialization to fix race condition
					GetGame().GetCallqueue().CallLater(OnControllableInitilizePlayerDelayed, 500, false, playerId, CRF_EntityHelper.ZERO_SPAWN_VECTOR[0], CRF_EntityHelper.ZERO_SPAWN_VECTOR[1], CRF_EntityHelper.ZERO_SPAWN_VECTOR[2], CRF_EntityHelper.ZERO_SPAWN_VECTOR[3]);
				}
			}
		}
		
		// Apply gearscript/identity if in play mode and are initilizing a gearscript character
		if (GetGame().InPlayMode() && character.GetPrefabData() && CRF_RoleHelper.IsValidGearscriptResource(character.GetPrefabData().GetPrefabName()))
		{	
			// Ensure gearscript manager is available
			if (!m_GearscriptManager)
				m_GearscriptManager = CRF_GearscriptManager.GetInstance();
			
			// Schedule gearscript identity setup with appropriate delay
			GetGame().GetCallqueue().Call(
				m_GearscriptManager.SetEntityIdentity, 
				character
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
					character, 
					character.GetPrefabData().GetPrefabName()
				);
			};
		}
	}

	//------------------------------------------------------------------------------------------------
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
		
		m_GarbageManager.m_aDeadBodies.Insert(entity);
		
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
		if (CRF_EntityHelper.IsSpectator(entity))
			delay = 0;
		
		// Get player faction
		Faction faction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
		FactionKey factionKey;
		
		if (faction)
			factionKey = faction.GetFactionKey();

		// Handle respawn if enabled, tickets available, and within time window
		if (m_RespawnManager.m_bCurrentRespawnEnabled && 
			!CRF_EntityHelper.IsSpectator(entity) && 
			m_GamemodeState != CRF_EGamemodeState.AAR && 
			m_RespawnManager.TicketsRemaining(factionKey) &&
			m_RespawnManager.IsRespawnTimeAllowed() &&
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
		GetGame().GetCallqueue().CallLater(OnControllableInitilizePlayerDelayed, delay, false, playerId, deathPosition[0], deathPosition[1], deathPosition[2], deathPosition[3], true);
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//===================================================================================
	// STAGGERED PLAYER INITIALIZATION SYSTEM
	//===================================================================================
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
				m_GamemodeManager.InitilizePlayer(playerId, CRF_EntityHelper.ZERO_SPAWN_VECTOR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if a player is waiting in the initialization queue
	 * @param playerId Player to check
	 * @return True if player is queued for initialization
	 */
	bool IsPlayerQueuedForInitialization(int playerId)
	{
		return m_aPendingPlayerInitializations.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateGenericSpawn()
	{
		m_vGenericSpawn = CRF_MissionHelper.GetAOCenter();
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get gearscript resource for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return ResourceName for the gearscript or empty string if not found
	 */
	ResourceName GetGearScriptResource(FactionKey factionKey)
	{
		CRF_GearScriptContainer container = GetGearScriptSettings(factionKey);
		if (!container)
		{
			PrintFormat("NO GEARSCRIPT ASSIGNED TO: %1", factionKey, LogLevel.WARNING);
			return "";
		}
		
		switch (factionKey)
		{
			case "BLUFOR": return m_rBLUFORCurrentGearScript;
			case "OPFOR": return m_rOPFORCurrentGearScript;
			case "INDFOR": return m_rINDFORCurrentGearScript;
			case "CIV": return m_rCIVILIANCurrentGearScript;
		}

		return m_rCIVILIANCurrentGearScript;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get gearscript container for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return The gearscript container or null if not found
	 */
	CRF_GearScriptContainer GetGearScriptSettings(FactionKey factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return m_BLUFORGearScriptSettings;
			case "OPFOR": return m_OPFORGearScriptSettings;
			case "INDFOR": return m_INDFORGearScriptSettings;
			case "CIV": return m_CIVILIANGearScriptSettings;
		}
		
		return m_CIVILIANGearScriptSettings;
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

