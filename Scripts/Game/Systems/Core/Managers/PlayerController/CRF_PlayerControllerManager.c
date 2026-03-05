class CRF_PlayerControllerManagerClass : ScriptComponentClass
{
}

class CRF_PlayerControllerManager : ScriptComponent
{
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// UI and Display
	string m_sHintText = "Type Here";      // Text displayed for hints to player
	bool m_bHUDVisible = true;             // Controls visibility of HUD elements
	Widget m_wSavedHintWidget;             // Reference to hint widget for reuse
	bool m_bIsListeningToSpec = false;
	vector m_vPlayersLastDeath[4];
	
	// Game Systems
	protected CRF_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected CRF_PlayerRplToAuthorityManager m_PlayerRplToAuthorityManager;  // Network authority manager
	protected CRF_PlayerCameraManager m_CameraManager;                  // Reference to the local camera manager

//=============================================================================================================================================================================================================================================================================================================================================================
//	 COMPONENT INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the player controller component
	//! Sets up input listeners and schedules initial setup calls
	void InitilizePlayerControllerComp()
	{
		// Skip initialization on dedicated servers or in editor
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get references to required systems
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_PlayerRplToAuthorityManager = CRF_PlayerRplToAuthorityManager.GetInstance();
		m_CameraManager = CRF_PlayerCameraManager.GetInstance();
		
		GetGame().GetCallqueue().Call(CRF_PlayerSettingsManager.GetInstance().InitFPSLock);
		GetGame().GetCallqueue().Call(CRF_PlayerSettingsManager.GetInstance().InitAudioLock);
		GetGame().GetCallqueue().Call(CRF_PlayerMenuManager.GetInstance().OpenCurrentStateMenu);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Initializes the player client
	//! Cleans up previous camera, closes menus, and sets up player-specific settings
	//! \param[in] playerCharacter - The spectator entity the server created and set to this player
	void InitilizePlayerClient(RplId playerCharID)
	{
		// Get player character
		IEntity playerCharacter = CRF_EntityHelper.GetCharacterFromRplId(playerCharID);
		
		// if we cant get the player character or it's null, wait another full initilization time before attempting again
		if (!playerCharacter || !SCR_ChimeraCharacter.Cast(playerCharacter))
		{
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(InitilizePlayerClient, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerCharID);
			return;
		};
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_PlayerRplToAuthorityManager = CRF_PlayerRplToAuthorityManager.GetInstance();
		
		// Close all menus
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			GetGame().GetMenuManager().CloseAllMenus();
			CRF_PlayerSettingsManager.GetInstance().ResetSettingsToStoredValues();
			if (!CVON_VONGameModeComponent.GetInstance())
				CRF_InitializationHelper.SetupRadioFrequency();
		}; 
		
		if (playerCharacter.GetPrefabData().GetPrefabName() == CRF_EntityHelper.GetSpectatorResource())
			InitilizeLocalSpectator(playerCharacter);
		else
			InitilizeLocalCharacter();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid spectator entity
	//! \param[in] playerCharacter - The spectator entity the server created and set to this player
	void InitilizeLocalSpectator(IEntity playerCharacter)
	{
		m_CameraManager.InitilizeSpecCamera();
		
		// Register for VON (voice chat)
		m_PlayerRplToAuthorityManager.CheckVONRegister(SCR_PlayerController.GetLocalPlayerId());
		
		// Open spectator menu if in game state
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
		
		// Turn on killfeed for specs
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeDeadLocal();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initilizes players if they have a valid slotted character
	void InitilizeLocalCharacter()
	{
		// Clean up previous camera if exists
		if (m_CameraManager.m_eCamera)
			delete m_CameraManager.m_eCamera;
		
		// Notify data collector about player spawn for movement tracking and stats
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		IEntity localPlayerEntity = SCR_PlayerController.GetLocalMainEntity();
		
		SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
		if (dataCollector && localPlayerEntity)
			dataCollector.NotifyPlayerSpawned(localPlayerId, localPlayerEntity);
		
		// Originally added for data collector
		m_Gamemode.GetOnPlayerSpawned().Invoke(localPlayerId, localPlayerEntity);
		
		// Reset Stored Pos
		GetGame().GetCallqueue().CallLater(m_CameraManager.UpdateStoredCameraPos, 200, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerControllerManager m_sInstance;
	void CRF_PlayerControllerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PlayerControllerManager GetInstance()
	{
		return m_sInstance;
	}
}
