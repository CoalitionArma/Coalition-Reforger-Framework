//------------------------------------------------------------------------------------
// CRF_GamemodeClass: Base class definition for the Coalition Reforger Framework Gamemode
//------------------------------------------------------------------------------------
class CRF_GamemodeClass : SCR_BaseGameModeClass {}

//------------------------------------------------------------------------------------
// Mission briefing descriptor for displaying mission information
//------------------------------------------------------------------------------------
[BaseContainerProps()]
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
	
	// General Gamemode Settings
	//------------------------------------------------------------------------------------
	[Attribute("45", "auto", "Mission Time (set to -1 to disable)", category: "CRF Gamemode General")]
	int m_iTimeLimitMinutes;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Gamemode General")]
	bool m_bAllowEspionage;

	[Attribute("false", "auto", "Enables AI autonomy while in GAME state", category: "CRF Gamemode General")]
	bool EnableAIInGameState;

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

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;

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
	
	// Manager References and System Components
	//------------------------------------------------------------------------------------
	protected ref ScriptInvoker m_OnStateChanged;
	protected static ref SCR_PlayerData m_PlayerData;
	
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;

	//===================================================================================
	// STATIC METHODS
	//===================================================================================
	
	/**
	 * Returns the singleton instance of the CRF_Gamemode
	 * @return CRF_Gamemode instance or null if not available
	 */
	static CRF_Gamemode GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
			
		return CRF_Gamemode.Cast(gameMode);
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
		m_SlottingManager.RequestSlottingUpdate();
		Replication.BumpMe();
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
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}

	/**
	 * Get the state change event invoker
	 * @return ScriptInvoker for state change events
	 */
	ScriptInvoker GetOnStateChanged()
	{
		if (!m_OnStateChanged)
			m_OnStateChanged = new ScriptInvoker();

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
			if (m_OnStateChanged)
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
		
		foreach (int player : players)
		{
			// Skip disconnected players
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			
			// Process player statistics data
			ProcessStats(dataCollector,player);

			// Skip players already in spectator
			if (CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(player)))
				continue;

			// Set player health to zero (kill them)
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(player);
			if (!playerEntity)
				continue;
				
			SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(
				playerEntity.FindComponent(SCR_CharacterDamageManagerComponent)
			);
			
			if (!damageManager)
				continue;
				
			HitZone defaultHitZone = damageManager.GetDefaultHitZone();
			if (defaultHitZone)
				defaultHitZone.SetHealth(0);
		}
		
		// Stores player profiles who havent disconnected
		dataCollector.OnGameEnd();
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
			
		// Initialize player if not in GAME state
		if (m_GamemodeState == CRF_EGamemodeState.BRIEFING || 
			m_GamemodeState == CRF_EGamemodeState.SLOTTING || 
			m_GamemodeState == CRF_EGamemodeState.AAR)
		{
			m_GamemodeManager.InitilizePlayer(iPlayerID);
		}
		
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
		if (m_GamemodeState != CRF_EGamemodeState.GAME)
			// Update generic spawnpoint for spectator cameras
			entity.GetWorldTransform(m_vGenericSpawn);
		
		// Apply gearscript if in play mode and not on client
		if (GetGame().InPlayMode() && RplSession.Mode() != RplMode.Client && entity && entity.GetPrefabData() && !m_GamemodeManager.IsSpectator(entity) && m_GamemodeState == CRF_EGamemodeState.GAME)
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
		
		// Data collector stuff for stats
		SCR_DataCollectorComponent dc = GetGame().GetDataCollector();
		SCR_InstigatorContextData inst = new SCR_InstigatorContextData(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity), entity, killerEntity, instigator);
		dc.OnPlayerKilled(inst);

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
		if (m_bRespawnEnabled && 
			!CRF_GamemodeManager.IsSpectator(entity) && 
			m_GamemodeState != CRF_EGamemodeState.AAR && 
			m_RespawnManager.TicketsRemaining(factionKey) &&
			!m_RespawnManager.GetFactionSpawnpoints(factionKey).IsEmpty() &&
			!factionKey.IsEmpty())
		{
			// Deduct ticket
			m_RespawnManager.SubtractTicket(factionKey);

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

		// Move player to spectator
		GetGame().GetCallqueue().CallLater(
			m_GamemodeManager.InitilizePlayer, 
			delay, 
			false, 
			playerId,
			vector.Zero
		);
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
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(CRF_SpectatorMenuUI));
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
