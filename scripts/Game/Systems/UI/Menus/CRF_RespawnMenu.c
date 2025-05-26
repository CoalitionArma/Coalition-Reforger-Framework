/**
 * Custom respawn menu implementation for the Coalition Reforger Framework
 * Provides timer display, map view, and communication capabilities during respawn
 */
class CRF_RespawnMenu: ChimeraMenuBase
{
	// UI components
	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_ChatPanel m_ChatPanel;
	
	/**
	 * Updates the respawn timer display on the UI
	 */
	void UpdateTimer()
	{	
		TextWidget timerWidget = TextWidget.Cast(GetRootWidget().FindAnyWidget("timerCountDown"));
		if (!timerWidget)
			return;
			
		timerWidget.SetText(SCR_FormatHelper.FormatTime(CRF_RespawnManager.GetInstance().m_iRespawnTimer));
	}
	
	/**
	 * Handles actions when menu is opened
	 * Sets up timer, map, voice chat and input listeners
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Start timer update loop
		GetGame().GetCallqueue().CallLater(UpdateTimer, 1000, true);

		// Initialize map if available
		if (m_MapEntity)
			InitializeMap();
		
		// Register input handlers
		RegisterInputHandlers();
		
		// Set up chat panel
		InitializeChatPanel();
	}
	
	/**
	 * Initializes the map when menu is opened
	 */
	protected void InitializeMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);
	}
	
	/**
	 * Registers all input action listeners for the menu
	 */
	protected void RegisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();
		
		// Voice communication handlers
		inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		
		// Menu navigation handlers
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Initializes the chat panel component
	 */
	protected void InitializeChatPanel()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
	}
	
	/**
	 * Initializes menu components when the menu is first created
	 */
	override void OnMenuInit()
	{	
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	/**
	 * Updates the menu state during each frame
	 * @param tDelta - Time elapsed since last frame
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
	}
	
	/**
	 * Cleans up resources when the menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		// Stop timer updates
		GetGame().GetCallqueue().Remove(UpdateTimer);
		
		// Remove input handlers
		UnregisterInputHandlers();
		
		// Close the map if open
		if (m_MapEntity)
			m_MapEntity.CloseMap();
	}
	
	/**
	 * Unregisters all input action listeners when closing the menu
	 */
	protected void UnregisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();
		
		inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Configures and opens the map with proper settings
	 */
	protected void OpenMapWithConfig()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		
		// Apply zoom out after short delay to ensure map is ready
		GetGame().GetCallqueue().CallLater(AdjustMapZoom, 100);
	}
	
	/**
	 * Adjusts map zoom level after map is opened
	 */
	protected void AdjustMapZoom()
	{
		if (m_MapEntity)
			m_MapEntity.ZoomOut();
	}
	
	/**
	 * Activates voice communication when key is pressed
	 */
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		if (!von)
			return;
			
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	/**
	 * Retrieves the player's radio transceiver for voice communication
	 * @return Radio transceiver entity
	 */
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		if (!entity)
			return null;
			
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			entity.FindComponent(SCR_InventoryStorageManagerComponent)
		);
		
		if (!inventoryManager)
			return null;
			
		ref array<IEntity> items = {};
		inventoryManager.GetItems(items);
		
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}
		
		if (!radioEntity)
			return null;
			
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		
		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);
			
		return transceiver;
	}
	
	/**
	 * Deactivates voice communication when key is released
	 */
	void Action_VONOff()
	{
		// Slight delay to avoid cutting off voice too abruptly
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	/**
	 * Delayed function to disable voice communication
	 */
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		if (!von)
			return;
			
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	/**
	 * Toggles the chat panel when chat key is pressed
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Short delay to prevent conflicts with other UI actions
		GetGame().GetCallqueue().CallLater(OpenChatPanel, 5);
	}
	
	/**
	 * Opens the chat panel if it's currently closed
	 */
	protected void OpenChatPanel()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
			if (chatManager)
				chatManager.OpenChatPanel(m_ChatPanel);
		}
	}
	
	/**
	 * Handles the exit/back action by opening the pause menu
	 */
	void Action_Exit()
	{
		// Open pause menu instead of exiting directly to prevent accidental exits
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}
	
	/**
	 * Opens the game's pause menu
	 */
	protected void OpenPauseMenu()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}