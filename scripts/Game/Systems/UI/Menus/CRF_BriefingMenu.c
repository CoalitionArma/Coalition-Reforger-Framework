modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_PreviewMenu
}

class CRF_PreviewMenuUI: ChimeraMenuBase
{
	//--- UI Widgets ---
	protected Widget m_wRoot;                                  // Root widget of the menu
	protected ImageWidget m_wPreview;                         // Preview phase indicator
	protected ImageWidget m_wSlotting;                        // Slotting phase indicator 
	protected ImageWidget m_wGame;                            // Game phase indicator
	protected ImageWidget m_wAAR;                             // After Action Report phase indicator
	protected ButtonWidget m_wBackButton;                     // Back button for description navigation
	
	//--- Components and Managers ---
	protected SCR_MapEntity m_MapEntity;                      // Map entity for displaying mission map
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent; // Component for player list
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent; // Component for mission descriptions
	protected CRF_Gamemode m_Gamemode;                        // Game mode instance
	protected CRF_MenuManager m_MenuManager;                  // Menu manager instance
	protected SCR_ChatPanel m_ChatPanel;                      // Chat panel component
	
	//--- Data Storage ---
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {}; // Active mission descriptors
	
	//--- MENU LIFECYCLE METHODS ---
	
	/**
	 * Initializes the menu when it's opened
	 */
	override void OnMenuOpen()
	{	
		// Don't open menu on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		// Initialize map if available
		if (m_MapEntity)
			GetGame().GetCallqueue().CallLater(OpenMap, 0);
		
		// Set up input actions
		RegisterInputActions();
		
		// Initialize chat panel
		InitializeChatPanel();
		
		// Initialize UI elements
		InitializeUIElements();
		
		// Initialize phase indicators
		SetupPhaseIndicators();
		
		// Setup player and description lists
		SetupListComponents();
		
		// Initialize description section
		DescriptionInit();
		
		// Configure navigation buttons based on game state
		ConfigureNavigationButtons();
	}
	
	/**
	 * Registers input action listeners
	 */
	protected void RegisterInputActions()
	{
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Initializes chat panel if available
	 */
	protected void InitializeChatPanel()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
	}
	
	/**
	 * Initializes basic UI elements and fetches widget references
	 */
	protected void InitializeUIElements()
	{
		m_wRoot = GetRootWidget();
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
		// Set mission text with author information
		UpdateMissionText();
		
		// Set weather text
		UpdateWeatherText();
		
		// Get phase indicator widgets
		m_wPreview = ImageWidget.Cast(m_wRoot.FindAnyWidget("PreviewBorder"));
		m_wSlotting = ImageWidget.Cast(m_wRoot.FindAnyWidget("SlottingBorder"));
		m_wGame = ImageWidget.Cast(m_wRoot.FindAnyWidget("GameBorder"));
		m_wAAR = ImageWidget.Cast(m_wRoot.FindAnyWidget("AARBorder"));
		
		// Initialize back button
		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_wBackButton.SetOpacity(0);
	}
	
	/**
	 * Updates mission text display with name and author
	 */
	protected void UpdateMissionText()
	{
		TextWidget missionText = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionText"));
		
		// Set mission name
		string missionName = "Unknown Mission";
		if (GetGame().GetMissionName())
			missionName = GetGame().GetMissionName();
		
		missionText.SetText(missionName);
		
		// Add author information if available
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		string author = "Unknown";
		
		if (header)
			author = header.m_sAuthor;
		
		missionText.SetText(missionName + " | By " + author);
	}
	
	/**
	 * Updates weather text display
	 */
	protected void UpdateWeatherText()
	{
		string currentStateName = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetCurrentWeatherState().GetStateName();
		TextWidget.Cast(m_wRoot.FindAnyWidget("WeatherText")).SetText("Weather: " + currentStateName);
	}
	
	/**
	 * Updates phase indicators based on current gamemode state
	 */
	protected void SetupPhaseIndicators()
	{
		// Get phase indicator widgets
		m_wPreview = ImageWidget.Cast(m_wRoot.FindAnyWidget("PreviewBorder"));
		m_wSlotting = ImageWidget.Cast(m_wRoot.FindAnyWidget("SlottingBorder"));
		m_wGame = ImageWidget.Cast(m_wRoot.FindAnyWidget("GameBorder"));
		m_wAAR = ImageWidget.Cast(m_wRoot.FindAnyWidget("AARBorder"));
		
		// Highlight the current phase
		int gameState = CRF_Gamemode.Cast(GetGame().GetGameMode()).m_GamemodeState; 
		switch(gameState)
		{
			case 0: {m_wPreview.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 1: {m_wSlotting.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 2: {m_wGame.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 3: {m_wAAR.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
		}
	}
	
	/**
	 * Sets up list box components
	 */
	protected void SetupListComponents()
	{
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(
			OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayersList")).FindHandler(SCR_ListBoxComponent)
		);
		
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(
			OverlayWidget.Cast(m_wRoot.FindAnyWidget("DescriptionList")).FindHandler(SCR_ListBoxComponent)
		);
	}
	
	/**
	 * Configures navigation buttons based on current game state
	 */
	protected void ConfigureNavigationButtons()
	{
		ButtonWidget slottingButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlottingButton"));
		ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
		ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
		ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
		
		// Disable all buttons by default
		slottingButton.SetEnabled(false);
		gameButton.SetEnabled(false);
		aarButton.SetEnabled(false);
		advanceButton.SetEnabled(false);
		FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(0);
		
		// Enable buttons based on current game state
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.SLOTTING)
			slottingButton.SetEnabled(true);
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			slottingButton.SetEnabled(true);
			gameButton.SetEnabled(true);
		}
		
		// Register button handlers
		SCR_ButtonTextComponent.Cast(slottingButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(OpenSlottingMenu);
		SCR_ButtonTextComponent.Cast(gameButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(InitializePlayer);
		SCR_ButtonTextComponent.Cast(advanceButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceMenu);
	}
	
	/**
	 * Initializes player and advances to game
	 */
	void InitializePlayer()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
		CRF_RplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
	}
	
	/**
	 * Opens the slotting menu
	 */
	void OpenSlottingMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
	}
	
	/**
	 * Requests advancing to the next gamemode state
	 */
	void AdvanceMenu()
	{
		CRF_RplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState(false);
	}
	
	/**
	 * Called when menu updates
	 * @param tDelta Time since last update
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		// Activate map context if map is available
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		// Update time display
		UpdateTimeDisplay();
		
		// Update player list
		UpdatePlayerList();
		
		// Check for admin privileges
		CheckAdminPrivileges();
		
		// Update chat panel
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	/**
	 * Updates the time display with current game time
	 */
	protected void UpdateTimeDisplay()
	{
		TimeContainer timeContainer = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetTime();
		int hours = timeContainer.m_iHours;
		int minutes = timeContainer.m_iMinutes;
		
		string minuteString;
		string hourString;
		
		// Format minutes with leading zero if needed
		if (minutes < 10)
			minuteString = "0" + minutes.ToString();
		else
			minuteString = minutes.ToString();
		
		// Format hours with leading zero if needed
		if (hours < 10)
			hourString = "0" + hours.ToString();
		else
			hourString = hours.ToString();
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText")).SetText("Time: " + hourString + ":" + minuteString);
	}
	
	/**
	 * Updates the player list with connected players
	 */
	protected void UpdatePlayerList()
	{
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		m_cPlayerListBoxComponent.Clear();
		
		foreach (int player : playerIds)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
				
			int index = m_cPlayerListBoxComponent.AddItem(
				GetGame().GetPlayerManager().GetPlayerName(player), 
				null, 
				"{51F58D728FBCAD99}UI/Listbox/PlayerListboxElementNoIcon.layout"
			);
			
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);
			
			// Color code players by role
			if (SCR_Global.IsAdmin(player))
				comp.SetColor(Color.Red);
			else if (CRF_GamemodeManager.GetInstance().IsModerator(player))
				comp.SetColor(Color.Yellow);
			
			// Highlight talking players
			if (m_MenuManager.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 183, 0, 255));
		}
	}
	
	/**
	 * Checks if local player has admin privileges and updates UI accordingly
	 */
	protected void CheckAdminPrivileges()
	{
		if (SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			ButtonWidget slottingButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlottingButton"));
			ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
			ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
			ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
			// Enable admin controls
			slottingButton.SetEnabled(true);
			gameButton.SetEnabled(true);
			advanceButton.SetEnabled(true);
			FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(1);
		}
	}
	
	/**
	 * Called when menu is closed
	 */
	override void OnMenuClose()
	{
		// Close map if open
		if (m_MapEntity)
			m_MapEntity.CloseMap();

		// Remove input action listeners
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Initializes the mission description section
	 */
	void DescriptionInit()
	{
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(false);
		
		// Reset back button
		m_wBackButton.SetOpacity(0);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Clear();
		
		// Clear description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText("");
		
		// Clear list components
		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();
		
		// Get player faction
		string playerFaction = SCR_PlayerFactionAffiliationComponent.Cast(
			GetGame().GetPlayerController().FindComponent(SCR_PlayerFactionAffiliationComponent)
		).GetAffiliatedFactionKey();
		
		// Add relevant descriptions to list
		foreach (ref CRF_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			// Add description visible to all factions
			if (description.m_bShowForAnyFaction)
			{
				m_cMissionDescriptionListBoxComponent.AddItem(
					description.m_sTitle, 
					null, 
					"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
				);
				m_aActiveDescriptors.Insert(description);
				continue;
			}
			
			// Add description specific to player's faction
			foreach (string factionKey : description.m_aFactionKeys)
			{
				if (playerFaction == factionKey)
				{
					m_cMissionDescriptionListBoxComponent.AddItem(
						description.m_sTitle, 
						null, 
						"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
					);
					m_aActiveDescriptors.Insert(description);
					break;
				}
			}
		}
		
		// Register selection handler
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}
	
	/**
	 * Handles selection of a description item
	 */
	void DescriptionSelected()
	{
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(true);
		
		// Get selected description
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		if (index < 0 || index >= m_aActiveDescriptors.Count())
			return;
			
		string description = m_aActiveDescriptors.Get(index).m_sTextData;
		
		// Show back button
		m_wBackButton.SetOpacity(1);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Insert(DescriptionInit);
		
		// Clear description list
		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();
		
		// Set description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText(description);
	}
	
	//--- MAP METHODS ---
	
	/**
	 * Opens the map (requires multiple frames to complete)
	 */
	void OpenMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrap, 0); // Need two frames
	}
	
	/**
	 * Second step in opening map
	 */
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(
			EMapEntityMode.FULLSCREEN, 
			"{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", 
			GetRootWidget()
		);
		
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChange, 0);
	}
	
	/**
	 * Third step in opening map (handles zoom)
	 */
	void OpenMapWrapZoomChange()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChangeWrap, 0);
	}
	
	/**
	 * Final step in opening map (applies zoom)
	 */
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();
	}
	
	/**
	 * Called when menu is initialized
	 */
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	//--- VOICE COMMUNICATION METHODS ---
	
	/**
	 * Activates voice communication
	 */
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		
		SCR_VoNComponent von = SCR_VoNComponent.Cast(
			GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent)
		);
		
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	/**
	 * Gets the radio transceiver for voice communication
	 * @return Radio transceiver
	 */
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		ref array<IEntity> items = {};
		
		SCR_InventoryStorageManagerComponent.Cast(
			entity.FindComponent(SCR_InventoryStorageManagerComponent)
		).GetItems(items);
		
		// Find radio in inventory
		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}
		
		if (!radioEntity)
			return null;
			
		// Configure radio
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		
		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);
		
		return transceiver;
	}
	
	/**
	 * Deactivates voice communication with delay
	 */
	void Action_VONOff()
	{
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	/**
	 * Delayed voice communication deactivation
	 */
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(
			GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent)
		);
		
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	//--- CHAT METHODS ---
	
	/**
	 * Toggles chat panel
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay for better UI response
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	/**
	 * Opens chat panel if not already open
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	//--- MENU EXIT METHODS ---
	
	/**
	 * Handles menu exit action
	 */
	void Action_Exit()
	{
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0);
	}
	
	/**
	 * Opens pause menu
	 */
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}