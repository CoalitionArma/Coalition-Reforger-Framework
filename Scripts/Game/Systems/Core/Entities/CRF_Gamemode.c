modded class SCR_BaseGameMode
{
	void SetGameState(SCR_EGameModeState state)
	{
		m_eGameState = state;
		Replication.BumpMe();
		// Explicitly raise the event on the authority, mirroring SCR_BaseGameMode.StartGameMode()
		// and EndGameMode(). The [RplProp onRplName] callback only fires automatically on proxy
		// (client) nodes when they receive the replicated value, never on the authority itself.
		// Without this, OnGameModeStart() and OnGameModeEnd() are never called on the server or
		// in Workbench (where there are no proxy nodes to receive the replication), which means
		// SCR_DataCollectorComponent.StartDataCollectorSession() never runs and the EntityEvent.FRAME
		// flag is never set — breaking stat tracking and the ENABLE_DIAG debug menu.
		OnGameStateChanged();
	}
}

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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Attributes Set By Plugins
	//------------------------------------------------------------------------------------
	[Attribute("0", UIWidgets.Hidden)]
	bool m_bRespawnEnabled;

	[Attribute("0", UIWidgets.Hidden)]
	bool m_bWaveRespawn;
	
	[Attribute("0", UIWidgets.Hidden)]
	bool m_bRallyPointsEnabled;

	[Attribute("60", UIWidgets.Hidden)]
	int m_iTimeToRespawn;
	
	[Attribute("0", UIWidgets.Hidden, desc: "Minutes before mission end when respawns disable (0 = never disable)", category: "CRF Gamemode Settings - Respawn")]
	int m_iRespawnCutoffMinutes;

	[Attribute("0", UIWidgets.Hidden, desc: "0 = Team-Based (faction ticket pool), 1 = Slot-Based (per-role/group respawn counts configured on each CRF_SlottingGroup)", category: "CRF Gamemode Settings - Respawn")]
	CRF_ERespawnMode m_eRespawnMode;
	
	[Attribute("45", UIWidgets.Hidden)]
	int m_iTimeLimitMinutes;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bLockUnusedSlots;
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bUseSafestartTimeLimit;
	
	[Attribute("0", UIWidgets.Hidden)]
	int m_iSafestartTimeLimit;

	[Attribute("true", UIWidgets.Hidden)]
	bool m_bUseCVON;
	
	[Attribute("", UIWidgets.Hidden)]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;

	[Attribute("", UIWidgets.Auto, desc: "Default descriptors pre-populated when running the Configure Descriptions plugin", category: "CRF Mission Settings - Descriptors")]
	ref array<ref CRF_MissionDescriptor> m_aDefaultMissionDescriptors;
	
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
	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Gamemode Settings - Advanced")]
	bool m_bMissionAllowsEspionage;
	
	[Attribute("false", "auto", "Separates spectator VON by faction - spectators can only hear their faction's voice chat", category: "CRF Gamemode Settings - Advanced")]
	bool m_bSeperateSpectatorsByFaction;
	
	[Attribute("false", "auto", "Hides other factions in spectator menu - spectators can only see and spectate players from their own faction", category: "CRF Gamemode Settings - Advanced")]
	bool m_bHideOtherSpectatorFactions;

	[Attribute("true", "auto", "If safestart turns on instantly after the lobby screen.", category: "CRF Gamemode Settings - Advanced")]
	bool m_bSafestartEnabledOnMissionStart;
	
	[Attribute("false", "auto", "Enable players being able to swap their clothes on the fly", category: "CRF Gamemode Settings - Advanced")]
	bool m_bEnableClothesSwapping;
	
	[Attribute("0", "auto", "Disables AI Crouching", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableAICrouching;
	
	[Attribute("true", "auto", "Disable chat messages except tickets & messages from admins/mods", category: "CRF Gamemode Settings - Advanced")]
	bool m_bDisableChat;

	// Spawn Point Settings
	//------------------------------------------------------------------------------------
	[Attribute("false", UIWidgets.CheckBox, desc: "If enabled, players spawn at a random point among BLUFOR's spawn points flagged 'Is Default Spawn' instead of always the same one", category: "CRF Gamemode Settings - Spawn")]
	bool m_bBLUFORRandomizeSpawnpoints;

	[Attribute("false", UIWidgets.CheckBox, desc: "If enabled, players spawn at a random point among OPFOR's spawn points flagged 'Is Default Spawn' instead of always the same one", category: "CRF Gamemode Settings - Spawn")]
	bool m_bOPFORRandomizeSpawnpoints;

	[Attribute("false", UIWidgets.CheckBox, desc: "If enabled, players spawn at a random point among INDFOR's spawn points flagged 'Is Default Spawn' instead of always the same one", category: "CRF Gamemode Settings - Spawn")]
	bool m_bINDFORRandomizeSpawnpoints;

	[Attribute("false", UIWidgets.CheckBox, desc: "If enabled, players spawn at a random point among CIV's spawn points flagged 'Is Default Spawn' instead of always the same one", category: "CRF Gamemode Settings - Spawn")]
	bool m_bCIVILIANRandomizeSpawnpoints;

	// Weather and Time Settings
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.Slider, "Time scale applied once the mission starts (after briefing/slotting). 1 = normal speed, 2 = twice as fast, etc.", "0.1 12 0.1", category: "CRF Mission Settings - Weather & Time")]
	float m_fMissionTimeScale;

	// Gearscript Settings
	//------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;
	[RplProp()] ResourceName m_rBLUFORCurrentGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;
	[RplProp()] ResourceName m_rOPFORCurrentGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;
	[RplProp()] ResourceName m_rINDFORCurrentGearScript;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gearscript Settings - Advanced")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;
	[RplProp()] ResourceName m_rCIVILIANCurrentGearScript;

	// Vehicle Gearscript Enable/Disable per Side
	//------------------------------------------------------------------------------------
	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for BLUFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bBLUFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for OPFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bOPFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for INDFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bINDFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for CIV vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bCIVILIANVehicleGearscriptEnabled;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Game State Properties
	//------------------------------------------------------------------------------------
	[RplProp(onRplName: "OnGamemodeStateChanged")]
	int m_GamemodeState = CRF_EGamemodeState.BRIEFING;

	[RplProp()]
	int m_SlottingState = CRF_ESlottingState.LEADERSANDMEDICS;
	
	// Manager References and System Components
	//------------------------------------------------------------------------------------
	protected ref ScriptInvoker m_OnStateChanged = new ScriptInvoker();
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_PermissionManager m_PermissionManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_LoggingManager m_LoggingManager;
	protected CRF_GarbageManager m_GarbageManager;
	
	[RplProp()]
	protected vector m_vGenericSpawn;
	
	bool m_bIsInEndCredits = false;
	
	// Staggered Player Initialization System
	//------------------------------------------------------------------------------------
	protected ref array<int> m_aPendingPlayerInitializations = {};
	protected ref map<int, int> m_mPlayerInitializationRetries = new map<int, int>();
	protected bool m_bProcessingInitializations = false;
	protected const int PLAYERS_PER_BATCH = 8;        // Players spawned per batch
	protected const int BATCH_INTERVAL_MS = 150;      // Milliseconds between batches
	protected const int MAX_PLAYER_INIT_RETRIES = 40; // ~6 seconds at 150ms interval
	protected float m_fBatchTimer = 0.0;              // Timer for batch processing

	// Reconnect Tracking — maps player GUID to slot ID for disconnected players.
	// Ensures a reconnecting player can reclaim their slot even if their numeric
	// player ID changes between disconnect and reconnect (possible on dedicated servers).
	//------------------------------------------------------------------------------------
	protected ref map<string, int> m_mReconnectSlotByGuid = new map<string, int>();

//=============================================================================================================================================================================================================================================================================================================================================================
//	 INITIALIZATION AND SETUP
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the gamemode and all required manager instances
	//! \param[in] owner The entity that owns this component
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Populate RplProp gearscript resources from attribute containers now that
		// attributes are guaranteed to be loaded. The containers can be null if the
		// mission designer left a faction unassigned, so guard each one.
		if (m_BLUFORGearScriptSettings)
			m_rBLUFORCurrentGearScript = m_BLUFORGearScriptSettings.m_rGearScript;
		if (m_OPFORGearScriptSettings)
			m_rOPFORCurrentGearScript = m_OPFORGearScriptSettings.m_rGearScript;
		if (m_INDFORGearScriptSettings)
			m_rINDFORCurrentGearScript = m_INDFORGearScriptSettings.m_rGearScript;
		if (m_CIVILIANGearScriptSettings)
			m_rCIVILIANCurrentGearScript = m_CIVILIANGearScriptSettings.m_rGearScript;

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

		// Frame events enabled on-demand when batch processing starts
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		super.OnGameEnd();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FRAME UPDATES
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Frame update for batch processing player initializations
	//! More reliable than CallLater for time-critical operations
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATE MANAGEMENT
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Progress to the next slotting state
	//! Updates all slotting UI and synchronizes across network
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
	//! Progress to the next gamemode state
	//! \param[in] overriden Set to true to allow advancing from AAR or GAME states
	void AdvanceGamemodeState(bool overriden = false)
	{
		// Prevent advancing from AAR or GAME unless explicitly overridden
		if ((m_GamemodeState == CRF_EGamemodeState.AAR || m_GamemodeState == CRF_EGamemodeState.GAME) && !overriden)
			return;

		m_GamemodeState += 1;
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle gamemode state changes
	//! Triggers UI updates and state-specific logic
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
				case CRF_EGamemodeState.SLOTTING: {
					// Clear reconnect tracking — a new game cycle means all previous
					// disconnect records are no longer valid (slots may be reassigned).
					m_mReconnectSlotByGuid.Clear();
					break;
				}
				
				case CRF_EGamemodeState.GAME: {
					SetGameState(SCR_EGameModeState.GAME);
					ApplyMissionTimeScale();
					foreach (Vehicle vehicle : CRF_VehicleGearscriptManager.GetInstance().GetSpawnedVehicleArray())
					{
						if (!vehicle)
							continue;
						vehicle.SpawnVehiclePassengers();
					}
					break;
				}
				
				case CRF_EGamemodeState.AAR: {
					// Flush all player stats BEFORE SetGameState
					CRF_ServerStatsManager statsManager = CRF_ServerStatsManager.GetInstance();
					if (statsManager)
						statsManager.NotifyMissionEnded();

					SetGameState(SCR_EGameModeState.POSTGAME);

					// Open the outro screen on all clients, passing winning faction so clients can display it
					CRF_RplBroadcastManager rplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
					if (rplBroadcastManager)
					{
						string winningFaction = "";
						CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
						if (loggingManager)
							winningFaction = loggingManager.GetWinningFaction();
						rplBroadcastManager.BroadcastOutro(winningFaction);
					}
					break;
				}
			}	
		}
		
		CRF_PlayerMenuManager playerMenuManager = CRF_PlayerMenuManager.GetInstance();
		if (playerMenuManager)
			playerMenuManager.OpenCurrentStateMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Applies the mission header's time scale once gameplay begins. Does not affect Preview/Slotting/AAR,
	//! since those run before this is called (see OnGamemodeStateChanged, CRF_EGamemodeState.GAME).
	protected void ApplyMissionTimeScale()
	{
		if (m_fMissionTimeScale <= 0)
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return;

		manager.SetDayDuration(86400 / m_fMissionTimeScale);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER MANAGEMENT
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Process player connection after authentication
	//! \param[in] iPlayerID ID of the connecting player
	protected override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);
		
		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Reconnect restore: if this player has a pending GUID entry, force-update their slot's
		// player ID before InitilizePlayer runs so IsPlayerInASlot() finds the correct slot.
		// This handles dedicated-server scenarios where a reconnecting player may get a new
		// numeric player ID but the GUID (BI account identity) remains the same.
		if (IsMaster() && m_SlottingManager)
		{
			string reconnectGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(iPlayerID);
			int savedSlotId;
			if (!reconnectGuid.IsEmpty() && m_mReconnectSlotByGuid.Find(reconnectGuid, savedSlotId))
			{
				m_mReconnectSlotByGuid.Remove(reconnectGuid);
				m_SlottingManager.ForceUpdateSlotPlayerID(savedSlotId, iPlayerID);
			}
		}
		
		QueuePlayerInitialization(iPlayerID);

		// Get player's BI account GUID for privilege checks
		string playerGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(iPlayerID);
		
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
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Called after a player is disconnected.
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		
		// Save slot association by GUID before any cleanup so the player can reclaim their
		// slot on reconnect even if their numeric player ID changes (dedicated-server behaviour).
		if (IsMaster() && m_SlottingManager)
		{
			string disconnectGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			if (!disconnectGuid.IsEmpty())
			{
				int disconnectSlotId = m_SlottingManager.GetPlayerSlotID(playerId);
				if (disconnectSlotId >= 0)
					m_mReconnectSlotByGuid.Set(disconnectGuid, disconnectSlotId);
			}
		}
		
		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually. 
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		if (CRF_EntityHelper.IsSpectator(player)) // We need to delete all spectator characters on disconnect
			GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, player); // Need to call on next frame so we dont mess up the player controller.
		
		StopMovement(player);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets player movement to 0 on disconnect
	void StopMovement(IEntity player)
	{
		SCR_CharacterControllerComponent charCont = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		if (!charCont)
			return;

		charCont.SetMovement(0, "0 0 0");
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ENTITY MANAGEMENT
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Default player kill behaviour. Called when a player is killed (and HandlePlayerKilled returns true).
	protected override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Get player faction
		Faction faction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
		FactionKey factionKey;
		
		if (faction)
			factionKey = faction.GetFactionKey();

		// Handle respawn if enabled, tickets available, and within time window
		if (m_RespawnManager.CanPlayerRespawn(playerEntity, factionKey, playerId))
		{
			// Deduct a respawn (faction ticket or slot/group respawn, depending on m_eRespawnMode)
			m_RespawnManager.DeductPlayerRespawn(factionKey, playerId, 1);

			// Display respawn screen
			GetGame().GetCallqueue().CallLater(
				m_RplBroadcastManager.SendRespawnScreen,
				75,
				false,
				playerId
			);
		}
		
		// Update slot death state so player gets put into spec
		int slotID = m_SlottingManager.GetCharacterSlotID(playerEntity);
		if (slotID != -1)
		{
			m_SlottingManager.UpdateSlotDeathState(slotID, true);
			
			// Cache the killer's name at death time while GetInstigatorPlayerID() is reliable server-side.
			// Clients cannot perform this lookup accurately once the killer dies/respawns/disconnects.
			int killerPlayerId = killer.GetInstigatorPlayerID();
			string killerName = "";
			if (killerPlayerId > 0)
				killerName = GetGame().GetPlayerManager().GetPlayerName(killerPlayerId);
			m_SlottingManager.UpdateSlotKillerName(slotID, killerName);
		}
		
		// Move player to spectator
		QueuePlayerInitialization(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Process entity death/destruction for players
	//! Handles respawn and spectator logic
	//! \param[in] entity The destroyed entity
	//! \param[in] killerEntity The entity that caused the destruction
	//! \param[in] instigator The instigator context
	protected override void OnControllableDestroyed(IEntity entity, IEntity killerEntity, notnull Instigator instigator)
	{
		super.OnControllableDestroyed(entity, killerEntity, instigator);

		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		if (CRF_GearscriptCharacter.Cast(entity))
			m_GarbageManager.m_aDeadBodies.Insert(entity);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STAGGERED PLAYER INITIALIZATION SYSTEM
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Queue a player for staggered initialization
	//! Prevents server overload by batching player spawns
	//! \param[in] playerId ID of the player to initialize
	void QueuePlayerInitialization(int playerId)
	{
		if (playerId <= 0)
			return;

		// Don't queue if already pending
		if (m_aPendingPlayerInitializations.Contains(playerId))
			return;
		
		m_aPendingPlayerInitializations.Insert(playerId);
		m_mPlayerInitializationRetries.Set(playerId, 0);
		
		// Start processing if not already running
		if (!m_bProcessingInitializations)
		{
			m_bProcessingInitializations = true;
			m_fBatchTimer = 0.0;
			SetEventMask(EntityEvent.FRAME);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Process a batch of pending player initializations
	//! Called by EOnFrame when timer interval is reached
	//! Spawns players in small groups to distribute server load
	protected void ProcessPlayerBatch()
	{
		if (m_aPendingPlayerInitializations.IsEmpty())
		{
			m_bProcessingInitializations = false;
			ClearEventMask(EntityEvent.FRAME);
			return;
		}
		
		// Process a batch of players
		int playersToProcess = Math.Min(PLAYERS_PER_BATCH, m_aPendingPlayerInitializations.Count());
		
		Print(string.Format("[CRF] Processing batch: %1 players (%2 remaining)", 
			playersToProcess, m_aPendingPlayerInitializations.Count()), LogLevel.VERBOSE);

		if (!m_GamemodeManager)
			m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		if (!m_GamemodeManager)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();

		for (int i = playersToProcess - 1; i >= 0; i--)
		{
			int playerId = m_aPendingPlayerInitializations[i];

			if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			{
				m_mPlayerInitializationRetries.Remove(playerId);
				m_aPendingPlayerInitializations.Remove(i);
				continue;
			}

			bool initialized = m_GamemodeManager.InitilizePlayer(playerId);
			if (initialized)
			{
				m_mPlayerInitializationRetries.Remove(playerId);
				m_aPendingPlayerInitializations.Remove(i);
				continue;
			}

			int retryCount = 0;
			m_mPlayerInitializationRetries.Find(playerId, retryCount);
			retryCount++;
			m_mPlayerInitializationRetries.Set(playerId, retryCount);

			if (retryCount >= MAX_PLAYER_INIT_RETRIES)
			{
				Print(string.Format("[CRF] WARNING: Dropping player %1 from initialization queue after %2 failed attempts", playerId, retryCount), LogLevel.WARNING);
				m_mPlayerInitializationRetries.Remove(playerId);
				m_aPendingPlayerInitializations.Remove(i);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all pending player initializations
	//! Used when resetting game state
	void ClearPlayerInitializationQueue()
	{
		m_aPendingPlayerInitializations.Clear();
		m_mPlayerInitializationRetries.Clear();
		m_bProcessingInitializations = false;
		ClearEventMask(EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if a player is waiting in the initialization queue
	//! \param[in] playerId Player to check
	//! \return True if player is queued for initialization
	bool IsPlayerQueuedForInitialization(int playerId)
	{
		return m_aPendingPlayerInitializations.Contains(playerId);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/UPDATERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// Disable the vanilla 30-second auto-restart countdown on game mode end.
	// Returning -1 makes SCR_BaseGameMode.OnGameModeEnd skip RestartSession entirely
	// (it falls through to TryShutdownServer which is a no-op without -autoshutdown).
	override float GetAutoReloadDelay()
	{
		return -1;
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
	
	//------------------------------------------------------------------------------------------------
	void UpdateGenericSpawn()
	{
		m_vGenericSpawn = CRF_MissionHelper.GetAOCenter();
		Replication.BumpMe();
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
	
	//------------------------------------------------------------------------------------------------
	bool DoesFactionShareMarker(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": 	return m_BLUFORGearScriptSettings.m_bEnableShareableMarkers;
			case "OPFOR": 	return m_OPFORGearScriptSettings.m_bEnableShareableMarkers;
			case "INDFOR": 	return m_INDFORGearScriptSettings.m_bEnableShareableMarkers;
			case "CIV": 	return m_CIVILIANGearScriptSettings.m_bEnableShareableMarkers;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get gearscript resource for a faction
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	//! \return ResourceName for the gearscript or empty string if not found
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
	//! Get gearscript container for a faction
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	//! \return The gearscript container or null if not found
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
	
	//------------------------------------------------------------------------------------------------
	//! Whether initial/automatic spawns for this faction should be picked at random from among
	//! the spawn points flagged as a default spawn, rather than always using the same one.
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, INDFOR, CIV)
	bool GetRandomizeInitialSpawn(FactionKey factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return m_bBLUFORRandomizeSpawnpoints;
			case "OPFOR": return m_bOPFORRandomizeSpawnpoints;
			case "INDFOR": return m_bINDFORRandomizeSpawnpoints;
			case "CIV": return m_bCIVILIANRandomizeSpawnpoints;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns true when the vehicle gearscript system is enabled for the given faction.
	//! Returns true by default for unknown factions.
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, INDFOR, CIV)
	bool IsVehicleGearscriptEnabled(FactionKey factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return m_bBLUFORVehicleGearscriptEnabled;
			case "OPFOR":  return m_bOPFORVehicleGearscriptEnabled;
			case "INDFOR": return m_bINDFORVehicleGearscriptEnabled;
			case "CIV":    return m_bCIVILIANVehicleGearscriptEnabled;
		}
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Get Side BFT boolean value
	bool IsSideBFTEnabled(string factionKey)
	{
		switch(factionKey)
		{
			case "BLUFOR": 	return m_BLUFORGearScriptSettings.m_bEnableBFT;
			case "OPFOR": 	return m_OPFORGearScriptSettings.m_bEnableBFT;
			case "INDFOR": 	return m_INDFORGearScriptSettings.m_bEnableBFT;
			case "CIV":		return m_CIVILIANGearScriptSettings.m_bEnableBFT;
		}
		
   		return true;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_Gamemode m_sInstance;
	void CRF_Gamemode(IEntitySource src, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_Gamemode()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_Gamemode GetInstance()
	{
		return m_sInstance;
	}
}
