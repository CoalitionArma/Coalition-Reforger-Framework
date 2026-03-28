 /**
 * Administrative menu for server management
 * Provides tools for player management including respawn, gear reset, teleport, etc.
 */
class CRF_AdminMenu : ChimeraMenuBase
{
	//-----------------------------------------------------------------------------
	// UI Components
	//-----------------------------------------------------------------------------
	
	// Core components
	protected CRF_PlayerControllerManager m_clientComponent;
	protected InputManager m_InputManager;
	protected SCR_ChatPanel m_ChatPanel;
	protected bool m_bFocused = true;
	
	// Main widgets
	protected Widget m_wRoot;
	protected Widget m_wMenuContent;
	protected Widget m_wConfirmationMenu;
	protected FrameWidget m_gearResetMenuRoot;
	
	// Game managers
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	
	// List containers
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	protected OverlayWidget m_list5Root;
	
	// List components
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	protected SCR_ListBoxComponent m_list5;
	
	// Text input widgets
	protected MultilineEditBoxWidget m_editBox1;
	protected EditBoxWidget m_editbox2;
	protected EditBoxWidget m_editbox3;
	protected WindowWidget m_windowBox1;
	
	// Menu navigation buttons
	protected SCR_ButtonTextComponent m_respawnMenuButton;
	protected SCR_ButtonTextComponent m_resetGearMenuButton;
	protected SCR_ButtonTextComponent m_teleportMenuButton;
	protected SCR_ButtonTextComponent m_hintMenuButton;
	protected SCR_ButtonTextComponent m_healMenuButton;
	protected SCR_ButtonTextComponent m_ticketMenuButton;
	protected SCR_ButtonTextComponent m_GamemodeMenuButton;
	
	// Action buttons
	protected SCR_ButtonTextComponent m_actionButton;
	protected SCR_ButtonTextComponent m_searchButton1;
	protected SCR_ButtonTextComponent m_searchButton2;
	protected SCR_ButtonTextComponent m_menuButton1;
	protected SCR_ButtonTextComponent m_menuButton2;
	protected SCR_ButtonTextComponent m_menuButton3;
	protected SCR_ButtonTextComponent m_menuButton4;
	
	// Ticket State
	protected int m_iSelectedTicket = -1;
	
	// Gear Script List
	protected ref CRF_GearScriptConfigStruct m_gearsetlist;
	
	protected bool m_bGameModeMenuOpen = false;

	// Modular display components
	protected CRF_AdminActionLogDisplay m_ActionLogDisplay;
	protected CRF_AdminGamemodePanel    m_AdminGamemodePanel;
	
	// New modular UI components
	protected ref CRF_PlayerListDisplay      m_PlayerListDisplay;
	protected ref CRF_SearchBoxComponent     m_SearchBoxComponent;
	protected ref CRF_RoleListDisplay        m_RoleListDisplay;
	protected ref CRF_DeadPlayerListDisplay  m_DeadPlayerListDisplay;
	protected ref CRF_GroupListDisplay       m_GroupListDisplay;
	protected ref CRF_SpawnpointListDisplay  m_SpawnpointListDisplay;
	protected ref CRF_TicketListDisplay      m_TicketListDisplay;
	protected ref CRF_FactionListDisplay     m_FactionListDisplay;

	//-----------------------------------------------------------------------------
	// General UI Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the menu when it opens
	 * Sets up all UI elements and displays initial respawn menu
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Get manager instances
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_clientComponent = CRF_PlayerControllerManager.GetInstance();
		m_AdminMenuManager= CRF_AdminMenuManager.GetInstance();

		// Setup menu roots
		m_wRoot = GetRootWidget();

		// --- Modular: admin action log sidebar ---
		Widget actionLogRoot = m_wRoot.FindAnyWidget("AdminActionLogDisplay");
		if (actionLogRoot)
			m_ActionLogDisplay = CRF_AdminActionLogDisplay.Cast(actionLogRoot.FindHandler(CRF_AdminActionLogDisplay));

		// Set up menu navigation buttons
		InitializeMenuButtons();

		// Initialize chat panel
		InitializeChat();
		
		// Populate Admin Logs
		if (m_ActionLogDisplay)
			m_ActionLogDisplay.Populate();
		else
			PopulateAdminActionsList();
		
		// Delay opening of initial menu
		GetGame().GetCallqueue().Call(DelayedMenuInitialization);
	}
	
	// Set up the initial menu (Tickets)
	protected void DelayedMenuInitialization()
	{
		InitializeTicketMenu();
		UpdateMenuButtonColors(m_ticketMenuButton);
	}
	
	/**
	 * Get a list box from the current loaded menu
	 * @param name of the root widget of the list box
	 */
	protected SCR_ListBoxComponent GetListBox(string listbox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		Widget listRoot = OverlayWidget.Cast(widget.FindAnyWidget(listbox));
		return SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}
	
	/**
	 * Get a button from the current loaded menu
	 * @param name of the root widget of the button
	 */
	protected SCR_ButtonTextComponent GetMenuButton(string button, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return SCR_ButtonTextComponent.GetButtonText(button, widget);
	}
	
	/**
	 * Get a multiline edit box from the current loaded menu
	 * @param name of the root widget of the edit box
	 */
	protected MultilineEditBoxWidget GetMultilineEditBox(string multiEditBox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return MultilineEditBoxWidget.Cast(widget.FindAnyWidget(multiEditBox));
	}
	
	/**
	 * Get a edit box from the current loaded menu
	 * @param name of the root widget of the edit box
	 */
	protected EditBoxWidget GetEditBox(string EditBox, Widget widget = null)
	{
		if (!widget)
			widget = m_wMenuContent;
		
		return EditBoxWidget.Cast(widget.FindAnyWidget(EditBox));
	}
	
	/**
	 * Initialize menu navigation buttons
	 */
	protected void InitializeMenuButtons()
	{
		// Respawn menu button
		m_ticketMenuButton = SCR_ButtonTextComponent.GetButtonText("TicketButton", m_wRoot);
		m_ticketMenuButton.m_OnClicked.Insert(TicketButton);
		
		// Respawn menu button
		m_respawnMenuButton = SCR_ButtonTextComponent.GetButtonText("RespawnButton", m_wRoot);
		m_respawnMenuButton.m_OnClicked.Insert(RespawnButton);

		// Reset gear menu button
		m_resetGearMenuButton = SCR_ButtonTextComponent.GetButtonText("ResetGearButton", m_wRoot);
		m_resetGearMenuButton.m_OnClicked.Insert(ResetGearButton);

		// Teleport menu button
		m_teleportMenuButton = SCR_ButtonTextComponent.GetButtonText("TeleportButton", m_wRoot);
		m_teleportMenuButton.m_OnClicked.Insert(TeleportButton);

		// Hint menu button
		m_hintMenuButton = SCR_ButtonTextComponent.GetButtonText("HintButton", m_wRoot);
		m_hintMenuButton.m_OnClicked.Insert(HintButton);

		// Heal menu button
		m_healMenuButton = SCR_ButtonTextComponent.GetButtonText("HealButton", m_wRoot);
		m_healMenuButton.m_OnClicked.Insert(HealButton);
		
		// Heal menu button
		m_GamemodeMenuButton = SCR_ButtonTextComponent.GetButtonText("GamemodeButton", m_wRoot);
		m_GamemodeMenuButton.m_OnClicked.Insert(GamemodeButton);
	}
	
	/**
	 * Initialize the chat panel
	 */
	protected void InitializeChat()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));

		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

		m_ChatPanel.SetAlwaysVisible(true);
		m_ChatPanel.ExpandMessageLines(20); // Increase the amount of message lines
		m_ChatPanel.ForceShowFullHistory(); // Load full history
	}

	/**
	 * Clean up when menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();

		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		if (m_wMenuContent)
			delete m_wMenuContent;
		
		CloseConfirmAction();
		
		if (m_ChatPanel)
			m_ChatPanel.SetAlwaysVisible(false);
	}

	/**
	 * Activates the Respawn menu
	 */
	void TicketButton()
	{
		UpdateMenuButtonColors(m_ticketMenuButton);
		ClearMenu();
		InitializeTicketMenu();
	}
	
	/**
	 * Activates the Respawn menu
	 */
	void RespawnButton()
	{
		UpdateMenuButtonColors(m_respawnMenuButton);
		ClearMenu();
		InitializeRespawnMenu();
	}

	/**
	 * Activates the Reset Gear menu
	 */
	void ResetGearButton()
	{
		UpdateMenuButtonColors(m_resetGearMenuButton);
		ClearMenu();
		InitializeGearMenu();
	}

	/**
	 * Activates the Teleport menu
	 */
	void TeleportButton()
	{
		UpdateMenuButtonColors(m_teleportMenuButton);
		ClearMenu();
		InitializeTeleportMenu();
	}

	/**
	 * Activates the Hint menu
	 */
	void HintButton()
	{
		UpdateMenuButtonColors(m_hintMenuButton);
		ClearMenu();
		InitializeHintMenu();
	}

	/**
	 * Activates the Heal menu
	 */
	void HealButton()
	{
		UpdateMenuButtonColors(m_healMenuButton);
		ClearMenu();
		InitializeHealMenu();
	}	
	
	/**
	 * Activates the Gamemode Settings menu
	 */
	void GamemodeButton()
	{
		UpdateMenuButtonColors(m_GamemodeMenuButton);
		ClearMenu();
		InitializeGamemodeMenu();
	}
	
	/**
	 * Updates menu button colors to highlight the active menu
	 * @param activeButton The button widget of the active menu button
	 */
	protected void UpdateMenuButtonColors(SCR_ButtonTextComponent activeButton)
	{
		// Default color for inactive buttons
		Color inactiveColor = Color.FromSRGBA(23, 26, 28, 255);
		
		// Set all texts to inactive color
		m_ticketMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_respawnMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_resetGearMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_teleportMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_hintMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_healMenuButton.GetRootWidget().SetColor(inactiveColor);
		m_GamemodeMenuButton.GetRootWidget().SetColor(inactiveColor);
		
		// Set active button text to white
		activeButton.GetRootWidget().SetColor(Color.FromSRGBA(18, 20, 22, 255));
	}
	
	protected void UpdateMenuTitle(string title)
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("MenuSubTitle")).SetText(title);
	}

	/**
	 * Clears all menu elements and data
	 * Resets visibility and clears event handlers
	 */
	void ClearMenu()
	{
		// Remove menu widget
		if (m_wMenuContent)
			delete m_wMenuContent;

		// Clear modular panel component — its widget tree was just deleted
		m_AdminGamemodePanel = null;
		m_bGameModeMenuOpen = false;
	}

	/**
	/**
	 * Gets player ID from player name
	 * @param name The player name to search for
	 * @return The matching player ID or 0 if not found
	 */
	protected int GetplayerIdFromName(string name)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (int pid : playerIds)
		{
			if (GetGame().GetPlayerManager().GetPlayerName(pid) == name)
				return pid;
		}

		return 0;
	}

	/**
	 * Gets the resource prefab for a given group and role
	 * @param groupID The group ID
	 * @param index The role index
	 * @return The resource name for the corresponding prefab
	 */
	ResourceName GetPrefab(int index)
	{
		ResourceName prefab = CRF_RoleHelper.RoleToResource(index);
		return prefab;
	}

	/**
	 * Handle loss of menu focus
	 */
	override void OnMenuFocusLost()
	{
		super.OnMenuFocusLost();

		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}
	
	/**
	 * Update chat while menu is active
	 */
	float m_fUpdateBuffer = 0;
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bGameModeMenuOpen)
			{
				if (m_AdminGamemodePanel)
					m_AdminGamemodePanel.Update();
				else
					GamemodeMenuUpdate();
			}
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += tDelta;
	}
	
	/**
	 * Handle regaining menu focus
	 */
	override void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//-----------------------------------------------------------------------------
	// Gear Menu Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the Gear Reset menu
	 * Allows admins to reset player gear to role defaults
	 */
	void InitializeGearMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{5C7EC9AAE498F6B6}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/GearMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Get widget references
		Widget playerListWidget = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		Widget roleListWidget = m_wMenuContent.FindAnyWidget("RoleListBox0");
		Widget searchBoxWidget = m_wMenuContent.FindAnyWidget("SearchBox0");
		
		if (!playerListWidget || !roleListWidget)
			return;
		
		// Initialize player list component manually
		m_PlayerListDisplay = new CRF_PlayerListDisplay();
		m_PlayerListDisplay.HandlerAttached(playerListWidget);
		
		// Initialize role list component manually
		m_RoleListDisplay = new CRF_RoleListDisplay();
		m_RoleListDisplay.HandlerAttached(roleListWidget);
		
		// Initialize search box component manually
		if (searchBoxWidget)
		{
			m_SearchBoxComponent = new CRF_SearchBoxComponent();
			m_SearchBoxComponent.HandlerAttached(searchBoxWidget);
			if (m_SearchBoxComponent && m_PlayerListDisplay)
				m_SearchBoxComponent.SetTargetPlayerList(m_PlayerListDisplay);
		}
		
		// Load Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("MenuButton3");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("MenuButton4");
		if (!menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4)
			return;
		
		// Setup button event handlers
		if (searchButton0 && m_SearchBoxComponent)
			m_SearchBoxComponent.SetSearchButton(searchButton0);
		
		menuButton0.m_OnClicked.Insert(ResetGear);
		menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		menuButton2.m_OnClicked.Insert(AddGIRadio);
		menuButton3.m_OnClicked.Insert(AddBinos);
		menuButton4.m_OnClicked.Insert(AddMap);
		
		// Setup selection change handler
		if (m_PlayerListDisplay && m_PlayerListDisplay.GetListBoxComponent())
			m_PlayerListDisplay.GetListBoxComponent().m_OnChanged.Insert(UpdateDefaultGear);
		
		// Change menu title
		UpdateMenuTitle("Gear Reset");

		// Populate lists using new components
		if (m_PlayerListDisplay)
			m_PlayerListDisplay.PopulateList();
		
		if (m_RoleListDisplay)
			m_RoleListDisplay.PopulateRoles();
	}
	
	/**
	 * Updates default gear selection based on player selection
	 */
	void UpdateDefaultGear()
	{
		if (!m_PlayerListDisplay || !m_RoleListDisplay)
			return;
		
		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		// Set the role list to match player's current role
		m_RoleListDisplay.SetSelectedRoleForPlayer(playerId);
	}

	/**
	 * Adds leadership radio to selected player
	 */
	void AddLeaderRadio()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_Gamemode.GetInstance().GetGearScriptSettings(factionKey).m_rLongRangeRadioPrefab;
		CRF_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds GI radio to selected player
	 */
	void AddGIRadio()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_Gamemode.GetInstance().GetGearScriptSettings(factionKey).m_rShortRangeRadioPrefab;
		CRF_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds binoculars to selected player
	 */
	void AddBinos()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;
			
		// Get binoculars prefab and add item
		string binosPrefab = GetBinos(playerId);
		CRF_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, binosPrefab, true);
	}
	
	/**
	* Adds map to selected player
	*/
	void AddMap()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;
			
		//Add map
		const string mapPrefab = "{13772C903CB5E4F7}Prefabs/Items/Equipment/Maps/PaperMap_01_folded.et";
		CRF_PlayerRplToAuthorityManager.GetInstance().AddItem(playerId, mapPrefab, true);
	}

	/**
	 * Gets binoculars prefab for player's faction
	 * @param playerId The player ID to get binoculars for
	 * @return The binoculars prefab resource name
	 */
	string GetBinos(int playerId)
	{
		// Get player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return "";
			
		string factionKey = playerGroup.GetFaction().GetFactionKey();

		// Load the gear config for the faction
		CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(CRF_Gamemode.GetInstance().GetGearScriptResource(factionKey)).GetResource().ToBaseContainer()
		));

		return gearConfig.m_sLeadershipBinocularsPrefab;
	}

	/**
	 * Reset a player's gear to their role default
	 */
	void ResetGear()
	{
		if (!m_PlayerListDisplay || !m_RoleListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		// Get selected role index from component
		int roleIndex = m_RoleListDisplay.GetSelectedRoleIndex();
		if (roleIndex < 0)
			return;
		
		// Get the prefab for the selected role
		ResourceName prefab = GetPrefab(roleIndex);
		if (prefab.IsEmpty())
			return;

		// Reset player's gear
		CRF_PlayerRplToAuthorityManager.GetInstance().ResetGear(playerId, prefab, true);
	}
	
	//-----------------------------------------------------------------------------
	// Ticket Menu Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the ticket menu
	 * Allows admins to see admin messages
	 */
	void InitializeTicketMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{FD7582ED92D34192}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/TicketMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Initialize ticket list display component
		Widget ticketListWidget = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		if (ticketListWidget)
		{
			m_TicketListDisplay = new CRF_TicketListDisplay();
			m_TicketListDisplay.HandlerAttached(ticketListWidget);
		}
		
		// Load buttons
		SCR_ButtonTextComponent replyButton = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent assignButton = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent closeButton = GetMenuButton("MenuButton2");
		if (!replyButton || !assignButton || !closeButton)
			return;
		
		// Setup button event handlers
		replyButton.m_OnClicked.Insert(ReplyToTicket);
		assignButton.m_OnClicked.Insert(AssignAdminToTicket);
		closeButton.m_OnClicked.Insert(CloseAdminTicket);
		
		// Setup selection change handler
		if (m_TicketListDisplay)
		{
			SCR_ListBoxComponent listBox = m_TicketListDisplay.GetTicketListBox();
			if (listBox)
				listBox.m_OnChanged.Insert(GetTicketMessages);
		}
		
		// Change title of the menu
		UpdateMenuTitle("Tickets");

		// Populate list of tickets
		GetOpenTickets();
	}
	
	/**
	* Assign a admin to a ticket
	*/
	void AssignAdminToTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Check if a ticket is selected
		if (playerList.GetSelectedItem() == -1 && m_iSelectedTicket == -1)
			return;
		
		// Get ID of the admin
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Add the reply to ticket
		CRF_PlayerRplToAuthorityManager.GetInstance().AssignAdminTicket(m_iSelectedTicket, adminID, false);
	}
	
	/**
	* Assign a admin to a ticket
	*/
	void CloseAdminTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Get ID of the admin
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Reinitilaize the ticket menu
		TicketButton();
		
		// Check if a ticket is selected
		if (playerList.GetSelectedItem() == -1 && m_iSelectedTicket == -1)
			return;
		
		// Broadcast the removal of ticket
		CRF_PlayerRplToAuthorityManager.GetInstance().CloseAdminTicket(m_iSelectedTicket, adminID, true);
		
		// Deselect ticket
		m_iSelectedTicket = -1;
	}
	
	/**
	* Reply to a ticket selected in list1
	*/
	void ReplyToTicket()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget replyBox = GetMultilineEditBox("ReplyBox0");
		if (!replyBox)
			return;
		
		// If the reply box is empty
		if (replyBox.GetText() == "")
			return;
		
		// If no player is selected or if one was selected before a refresh
		if (playerList.GetSelectedItem() < 0 && m_iSelectedTicket < 1)
			return;
		
		// Get player ID of the admin replying to the message
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Get the text for the reply box
		string reply = replyBox.GetText();
		
		// Add reply to tickets array
		CRF_PlayerRplToAuthorityManager.GetInstance().ReplyAdminMessage(reply, m_iSelectedTicket, adminID, false);
		
		// Clear Text in reply box
		replyBox.SetText("");
	}
	
	/**
	* Request the list of messages from the server
	*/
	void GetTicketMessages()
	{
		int ticketID = -1;
		int adminID = SCR_PlayerController.GetLocalPlayerId();
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Check if a ticket was selected either via the list or pre ui refresh
		if (playerList.GetSelectedItem() != -1)
		{
			// Get selected player ID
			string ticketName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
			
			// Extract ID from ticketName in list
			array<string> data = {};
		
			ticketName.Split(":", data, true);
			ticketID = data[0].ToInt();
			if (ticketID == 0)
				return;

			// Store selected ticket for use after ui refresh
			m_iSelectedTicket = ticketID;
		}
		else if (m_iSelectedTicket != -1)
			// Get the stored ticket selected before refresh
			ticketID = m_iSelectedTicket;
		else
			return;
		
		CRF_PlayerRplToAuthorityManager.GetInstance().GetTicketMessages(adminID, ticketID);
	}
	
	/**
	* Populates the list of messages selected in list 1
	*/
	void PopulateTicketMessages(array<ref CRF_TicketMessageData> messages)
	{
		if (!messages)
			return;
		
		SCR_ListBoxComponent ticketMessagesList = GetListBox("TicketMessagesListBox0");
		if (!ticketMessagesList)
			return;
		
		// Clear old Messages 
		ticketMessagesList.Clear();
		
		// Format and add the messages to the list
		foreach (int i, ref CRF_TicketMessageData message : messages)
		{
			ticketMessagesList.AddItem(string.Format("%1 - %2: %3", message.timestamp, message.sender, message.msg));
		}

	}
	
	/**
	* Request the list of open tickets from the server
	*/
	void GetOpenTickets()
	{	
		CRF_PlayerRplToAuthorityManager.GetInstance().GetOpenTickets(SCR_PlayerController.GetLocalPlayerId());
	}
	
	/**
	 * Populates the list of players that need help
	 */
	void PopulateOpenTicketList(array<int> tickets)
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Clear old ticket list
		playerList.Clear();

		// Add the list of tickets with their IDs
		foreach (int playerId : tickets)
		{
			string name = m_playerManager.GetPlayerName(playerId);
			playerList.AddItem(string.Format("%1:%2", playerId ,name));
		}
	}
	
	/**
	* Populates the list of messages selected in list 1
	*/
	void PopulateAdminActionsList()
	{
		array<ref CRF_AdminActionLog> reversed = {};
		
		// Setup selection change handlers
		OverlayWidget list5root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List5Box"));
		SCR_ListBoxComponent list5 = SCR_ListBoxComponent.Cast(list5root.FindHandler(SCR_ListBoxComponent));
		
		// Get list of logs of admin aciton in the current mission
		array<ref CRF_AdminActionLog> actions = m_AdminMenuManager.GetAdminActionLogs();
		if (!actions)
			return;
		
		// Reverse the order of logs so latest is at the top
		for (int i = actions.Count() - 1; i >= 0; i--)
		{
			reversed.Insert(actions[i]);
		}
		
		// Clear old logs 
		list5.Clear();
		
		// Format and add the messages to the list
		foreach (int i, ref CRF_AdminActionLog action : reversed)
		{
			list5.AddItem(string.Format("%1 - %2", action.timestamp, action.action));
		}
	}

	//-----------------------------------------------------------------------------
	// Respawn Menu Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the Respawn menu
	 * Allows admins to respawn dead players
	 */
	void InitializeRespawnMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{0F4AF70DE5AA8A96}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/RespawnMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Initialize dead player list display component
		Widget deadPlayerListWidget = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		if (deadPlayerListWidget)
		{
			m_DeadPlayerListDisplay = new CRF_DeadPlayerListDisplay();
			m_DeadPlayerListDisplay.HandlerAttached(deadPlayerListWidget);
		}
		
		// Initialize group list display component
		Widget groupListWidget = m_wMenuContent.FindAnyWidget("GroupListBox0");
		if (groupListWidget)
		{
			m_GroupListDisplay = new CRF_GroupListDisplay();
			m_GroupListDisplay.HandlerAttached(groupListWidget);
		}
		
		// Initialize spawnpoint list display component
		Widget spawnListWidget = m_wMenuContent.FindAnyWidget("SpawnListBox0");
		if (spawnListWidget)
		{
			m_SpawnpointListDisplay = new CRF_SpawnpointListDisplay();
			m_SpawnpointListDisplay.HandlerAttached(spawnListWidget);
		}
		
		// Initialize search box component
		Widget searchEditWidget = m_wMenuContent.FindAnyWidget("SearchBox0");
		SCR_ButtonTextComponent searchButton = GetMenuButton("SearchButton0");
		if (searchEditWidget && searchButton && m_DeadPlayerListDisplay)
		{
			m_SearchBoxComponent = new CRF_SearchBoxComponent();
			m_SearchBoxComponent.HandlerAttached(searchEditWidget);
			if (m_SearchBoxComponent)
			{
				// Note: SearchBoxComponent expects CRF_PlayerListDisplay, but we have CRF_DeadPlayerListDisplay
				// Since both have similar interfaces, we may need to adjust SearchBoxComponent to be more generic
				// or create a separate search implementation for dead players
			}
		}
		
		// Load Menu Buttons
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("BLUFOR");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("OPFOR");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("INDFOR");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("CIV");
		if (!menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4)
			return;
			
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(RespawnPlayer);
		menuButton1.m_OnClicked.Insert(RespawnSide);
		menuButton2.m_OnClicked.Insert(RespawnSide);
		menuButton3.m_OnClicked.Insert(RespawnSide);
		menuButton4.m_OnClicked.Insert(RespawnSide);
		
		// Setup selection change handlers
		if (m_DeadPlayerListDisplay)
			m_DeadPlayerListDisplay.GetListBoxComponent().m_OnChanged.Insert(UpdateSpawnGroupRequest);
		
		if (m_GroupListDisplay)
			m_GroupListDisplay.GetListBoxComponent().m_OnChanged.Insert(UpdateSpawnpoint);
		
		// Change title of the menu
		UpdateMenuTitle("Respawn");

		// Populate lists
		if (m_DeadPlayerListDisplay)
			m_DeadPlayerListDisplay.PopulateList();
		
		if (m_GroupListDisplay)
			m_GroupListDisplay.PopulateList();
	}
	
	/**
	 * Requests server to provide group ID for selected player
	 */
	void UpdateSpawnGroupRequest()
	{
		if (!m_DeadPlayerListDisplay || !m_GroupListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_DeadPlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;
		
		// Auto-select the group for this player
		m_GroupListDisplay.SetSelectedGroupForPlayer(playerId);
	}

	/**
	 * Updates spawnpoint list based on selected group
	 */
	void UpdateSpawnpoint()
	{
		if (!m_GroupListDisplay || !m_SpawnpointListDisplay)
			return;
		
		// Get selected group ID from component
		int groupID = m_GroupListDisplay.GetSelectedGroupId();
		if (groupID < 0)
			return;
		
		// Populate spawnpoints for the selected group
		m_SpawnpointListDisplay.PopulateForGroup(groupID);
	}

	/**
	 * Respawns the selected player at the selected spawnpoint and group
	 */
	void RespawnPlayer()
	{
		if (!m_DeadPlayerListDisplay || !m_GroupListDisplay || !m_SpawnpointListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_DeadPlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		// Get selected group ID from component
		int groupID = m_GroupListDisplay.GetSelectedGroupId();
		if (groupID < 0)
			return;

		// Get selected spawnpoint from component
		vector spawnpoint = m_SpawnpointListDisplay.GetSelectedSpawnpoint();
		if (spawnpoint == vector.Zero)
			return;
		
		// Spawn player on group
		//CRF_PlayerRplToAuthorityManager.GetInstance().SpawnOnGroup(playerId, spawnpoint, groupID, true);

		// Refresh the menu after a short delay
		GetGame().GetCallqueue().CallLater(ClearMenu, 1250, false);
		GetGame().GetCallqueue().CallLater(InitializeRespawnMenu, 1825, false);
	}
	
	/**
	 * Respawns blufor
	 */
	void RespawnSide()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		CRF_PlayerRplToAuthorityManager.GetInstance().RespawnFaction(button.GetName(), true);
	}

	//-----------------------------------------------------------------------------
	// Teleport Menu Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the Teleport menu
	 * Allows admins to teleport players
	 */
	void InitializeTeleportMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{681BEBC7B2B45D4E}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/TeleportMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Initialize first player list display component
		Widget playerListWidget0 = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		if (playerListWidget0)
		{
			m_PlayerListDisplay = new CRF_PlayerListDisplay();
			m_PlayerListDisplay.HandlerAttached(playerListWidget0);
		}
		
		// Initialize second player list display component (for teleport destination)
		Widget playerListWidget1 = m_wMenuContent.FindAnyWidget("PlayerListBox1");
		CRF_PlayerListDisplay playerListDisplay2;
		if (playerListWidget1)
		{
			playerListDisplay2 = new CRF_PlayerListDisplay();
			playerListDisplay2.HandlerAttached(playerListWidget1);
		}
		
		// Initialize search box components
		Widget searchEditWidget0 = m_wMenuContent.FindAnyWidget("SearchBox0");
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		if (searchEditWidget0 && searchButton0 && m_PlayerListDisplay)
		{
			m_SearchBoxComponent = new CRF_SearchBoxComponent();
			m_SearchBoxComponent.HandlerAttached(searchEditWidget0);
			if (m_SearchBoxComponent)
			{
				m_SearchBoxComponent.SetTargetPlayerList(m_PlayerListDisplay);
				m_SearchBoxComponent.SetSearchButton(searchButton0);
			}
		}
		
		Widget searchEditWidget1 = m_wMenuContent.FindAnyWidget("SearchBox1");
		SCR_ButtonTextComponent searchButton1 = GetMenuButton("SearchButton1");
		CRF_SearchBoxComponent searchBoxComponent2;
		if (searchEditWidget1 && searchButton1 && playerListDisplay2)
		{
			searchBoxComponent2 = new CRF_SearchBoxComponent();
			searchBoxComponent2.HandlerAttached(searchEditWidget1);
			if (searchBoxComponent2)
			{
				searchBoxComponent2.SetTargetPlayerList(playerListDisplay2);
				searchBoxComponent2.SetSearchButton(searchButton1);
			}
		}
		
		// Load Menu Buttons
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(TeleportLocalToSelected);
		menuButton1.m_OnClicked.Insert(TeleportPlayers);
		menuButton2.m_OnClicked.Insert(TeleportSelectedToLocal);
		
		// Change title of the menu
		UpdateMenuTitle("Teleport");

		// Populate player lists
		if (m_PlayerListDisplay)
			m_PlayerListDisplay.PopulateList();
		
		if (playerListDisplay2)
			playerListDisplay2.PopulateList();
	}

	/**
	 * Teleports local player to selected player
	 */
	void TeleportLocalToSelected()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId2 = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerId2);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Teleports a player to another player's location
	 * @param playerId1 - Player to teleport
	 * @param playerId2 - Destination player
	 */
	void TeleportLocalPlayer(int playerId1, int playerId2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerId1, teleportLocation);
	}
	
	/**
	 * Teleports selected player to local player
	 */
	void TeleportSelectedToLocal()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId2 = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId2 == 0)
			return;

		// Teleport target player to local player
		CRF_PlayerRplToAuthorityManager.GetInstance().TeleportPlayers(playerId2, SCR_PlayerController.GetLocalPlayerId(), true);
	}

	/**
	 * Teleports player 1 to player 2's position
	 */
	void TeleportPlayers()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get first player ID from first list (m_PlayerListDisplay)
		int playerId1 = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId1 == 0)
			return;

		// Get second player list component
		Widget playerListWidget1 = m_wMenuContent.FindAnyWidget("PlayerListBox1");
		if (!playerListWidget1)
			return;
		
		CRF_PlayerListDisplay playerListDisplay2 = new CRF_PlayerListDisplay();
		playerListDisplay2.HandlerAttached(playerListWidget1);
		if (!playerListDisplay2)
			return;

		// Get second player ID from second list
		int playerId2 = playerListDisplay2.GetSelectedPlayerId();
		if (playerId2 == 0)
			return;

		// Teleport player 1 to player 2
		CRF_PlayerRplToAuthorityManager.GetInstance().TeleportPlayers(playerId1, playerId2, true);
	}

	//-----------------------------------------------------------------------------
	// Hint Menu Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the Hint menu
	 * Allows admins to send hint messages to players
	 */
	void InitializeHintMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{10F6DA929AEE2069}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/HintMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Initialize player list display component
		Widget playerListWidget = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		if (playerListWidget)
		{
			m_PlayerListDisplay = new CRF_PlayerListDisplay();
			m_PlayerListDisplay.HandlerAttached(playerListWidget);
		}
		
		// Initialize faction list display component
		Widget factionListWidget = m_wMenuContent.FindAnyWidget("FactionListBox0");
		if (factionListWidget)
		{
			m_FactionListDisplay = new CRF_FactionListDisplay();
			m_FactionListDisplay.HandlerAttached(factionListWidget);
		}
		
		// Initialize search box component
		Widget searchEditWidget = m_wMenuContent.FindAnyWidget("SearchBox0");
		SCR_ButtonTextComponent searchButton = GetMenuButton("SearchButton0");
		if (searchEditWidget && searchButton && m_PlayerListDisplay)
		{
			m_SearchBoxComponent = new CRF_SearchBoxComponent();
			m_SearchBoxComponent.HandlerAttached(searchEditWidget);
			if (m_SearchBoxComponent)
			{
				m_SearchBoxComponent.SetTargetPlayerList(m_PlayerListDisplay);
				m_SearchBoxComponent.SetSearchButton(searchButton);
			}
		}
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup existing hint text if available
		hintBox.SetText(m_clientComponent.m_sHintText);
		
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(SendHintAll);
		menuButton1.m_OnClicked.Insert(SendHintFaction);
		menuButton2.m_OnClicked.Insert(SendHintPlayer);
		
		// Change title of the menu
		UpdateMenuTitle("Hint");

		// Populate lists
		if (m_PlayerListDisplay)
			m_PlayerListDisplay.PopulateList();
		
		if (m_FactionListDisplay)
		{
			m_FactionListDisplay.SetOnlyShowActive(true);
			m_FactionListDisplay.PopulateActiveFactions();
		}
	}
	
	/**
	 * Sends hint message to all players
	 */
	void SendHintAll()
	{
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		CRF_PlayerRplToAuthorityManager.GetInstance().SendHint(data);
	}

	/**
	 * Sends hint message to players in selected faction
	 */
	void SendHintFaction()
	{
		if (!m_FactionListDisplay)
			return;
	
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;

		// Get selected faction from component
		string factionKey = m_FactionListDisplay.GetSelectedFactionKey();
		if (factionKey == "")
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		CRF_PlayerRplToAuthorityManager.GetInstance().SendHint(data, -1, factionKey);
	}

	/**
	 * Sends hint message to selected player
	 */
	void SendHintPlayer()
	{
		if (!m_PlayerListDisplay)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		CRF_PlayerRplToAuthorityManager.GetInstance().SendHint(data, playerId);
	}
	
	//-----------------------------------------------------------------------------
	// Heal Menu Methods
	//-----------------------------------------------------------------------------
	
	/**
	 * Initialize the Heal menu
	 * Allows admins to heal players and repair vehicles
	 */
	void InitializeHealMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{CCFF9CCE4508B294}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/HealMenu.layout");
		if (!m_wMenuContent)
			return;
		
		// Initialize player list display component
		Widget playerListWidget = m_wMenuContent.FindAnyWidget("PlayerListBox0");
		if (playerListWidget)
		{
			m_PlayerListDisplay = new CRF_PlayerListDisplay();
			m_PlayerListDisplay.HandlerAttached(playerListWidget);
		}
		
		// Initialize search box component
		Widget searchEditWidget = m_wMenuContent.FindAnyWidget("SearchBox0");
		SCR_ButtonTextComponent searchButton = GetMenuButton("SearchButton0");
		if (searchEditWidget && searchButton && m_PlayerListDisplay)
		{
			m_SearchBoxComponent = new CRF_SearchBoxComponent();
			m_SearchBoxComponent.HandlerAttached(searchEditWidget);
			if (m_SearchBoxComponent)
			{
				m_SearchBoxComponent.SetTargetPlayerList(m_PlayerListDisplay);
				m_SearchBoxComponent.SetSearchButton(searchButton);
			}
		}
		
		// Load Menu Buttons
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		if (!menuButton0 || !menuButton1)
			return;
		
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(HealPlayer);
		menuButton1.m_OnClicked.Insert(HealPlayerVehicle);
		
		// Change title of the menu
		UpdateMenuTitle("Heal");

		// Populate player list
		if (m_PlayerListDisplay)
			m_PlayerListDisplay.PopulateList();
	}
	
	/**
	 * Heals the selected player
	 */
	void HealPlayer()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		// Heal player only (not vehicle)
		CRF_PlayerRplToAuthorityManager.GetInstance().Heal(playerId, true, false);
	}
	
	/**
	 * Repairs the vehicle of the selected player
	 */
	void HealPlayerVehicle()
	{
		if (!m_PlayerListDisplay)
			return;

		// Get selected player ID from component
		int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
		if (playerId == 0)
			return;

		// Heal player and vehicle
		CRF_PlayerRplToAuthorityManager.GetInstance().Heal(playerId, true, true);
	}
	
	//-----------------------------------------------------------------------------
	// Gamemode Settings Menu
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the Respawn menu
	 * Allows admins to respawn dead players
	 */
	void InitializeGamemodeMenu()
	{
		// Load menu content widget
		m_wMenuContent = GetGame().GetWorkspace().CreateWidgets("{36D941F5D1C10513}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/GameModeMenu.layout");
		if (!m_wMenuContent)
			return;
		
		m_bGameModeMenuOpen = true;

		// --- Modular: gamemode panel live-update component ---
		Widget gamemodeComponentRoot = m_wMenuContent.FindAnyWidget("AdminGamemodePanel");
		if (gamemodeComponentRoot)
		{
			m_AdminGamemodePanel = CRF_AdminGamemodePanel.Cast(gamemodeComponentRoot.FindHandler(CRF_AdminGamemodePanel));
			if (m_AdminGamemodePanel)
				m_AdminGamemodePanel.Init(m_wMenuContent);
		}

		// Load Menu Sections
		Widget gamerTimer = m_wMenuContent.FindAnyWidget("GameTimer");
		Widget ticketCounters = m_wMenuContent.FindAnyWidget("Tickets");
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		
		// Load Buttons
		SCR_ButtonTextComponent resetGearButton = GetMenuButton("ApplyGearSets", gearSets);
		SCR_ButtonTextComponent aarGearButton = GetMenuButton("EnterAAR");
		if (!resetGearButton || !aarGearButton)
			return;
		
		// Setup invokers
		resetGearButton.m_OnClicked.Insert(ConfirmAction);
		aarGearButton.m_OnClicked.Insert(EnterAAR);
		
		/*
		*	!!!!! Changing the time delta is done below and in the menu layout !!!!!
		*/
		// Load Menu Buttons for Game Timer
		
		// Time Values
		ref array<int> timeValues = {10, 5, -5, -10};	
		
		foreach (int time : timeValues)
		{
			string buttonName = string.Format("%1", time);
			SCR_ButtonTextComponent button = GetMenuButton(buttonName, gamerTimer);
			if (!button)
				return;
				
			button.m_OnClicked.Insert(UpdateTime);
		}
		
		/*
		*	!!!!! Changing the ticket delta is done below and in the menu layout !!!!!
		*/
		// Load Menu Buttons for Tickets
		
		// Faction names
		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		// Actions
		ref array<string> actions = {"Add", "Subtract"};
		
		// Ticket values
		ref array<int> values = {10, 5, 1};
		
		foreach (string faction : factions)
		{
			foreach (string action : actions)
			{
				foreach (int value : values)
				{
					string buttonName = string.Format("%1_%2_%3", faction, action, value);
					SCR_ButtonTextComponent button = GetMenuButton(buttonName, ticketCounters);
					if (!button)
						return;
						
					button.m_OnClicked.Insert(UpdateTicket);
				}
			}
		}
		
		// Load config files into listboxs and array
		LoadGearConfigList();

		array<string> gearArray = {};
		foreach (string name, string path : m_gearsetlist.gearset)
			gearArray.Insert(name);
		
		gearArray.Sort();
		
		foreach (string name : gearArray)
		{
			foreach (string faction : factions)
			{
				SCR_ListBoxComponent listBox = GetListBox(string.Format("%1ListBox", faction) ,gearSets);
				listBox.AddItem(name);
			}
		}
		
		
		// Change title of the menu
		UpdateMenuTitle("Gamemode Settings");
		
		// Update menu data
		if (m_AdminGamemodePanel)
			m_AdminGamemodePanel.Update();
		else
			GamemodeMenuUpdate();
		
		//Toggle Respawn Wave Button
		SCR_ButtonTextComponent toggleWaveRespawn = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("RespawnWaveButton").FindHandler(SCR_ButtonTextComponent));
		toggleWaveRespawn.m_OnClicked.Insert(ToggleWaveRespawn);
		
		//Toggle Respawn Enabled Button
		SCR_ButtonTextComponent toggleRespawn = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("EnableRespawnButton").FindHandler(SCR_ButtonTextComponent));
		toggleRespawn.m_OnClicked.Insert(ToggleRespawn);
		
		//Setting Respawn Time Button
		EditBoxWidget.Cast(m_wMenuContent.FindAnyWidget("TicketsInput")).SetText(CRF_RespawnManager.GetInstance().m_iCurrentTimeToRespawn.ToString());
		SCR_ButtonTextComponent setRespawnTime = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("SetRespawnTimeButton").FindHandler(SCR_ButtonTextComponent));
		setRespawnTime.m_OnClicked.Insert(SetRespawnTime);
	}
	
	void ToggleWaveRespawn()
	{
		CRF_PlayerRplToAuthorityManager.GetInstance().ToggleWaveRespawn();
	}
	
	void ToggleRespawn()
	{
		CRF_PlayerRplToAuthorityManager.GetInstance().ToggleRespawn();
	}
	
	void SetRespawnTime()
	{
		int respawnTime = EditBoxWidget.Cast(m_wMenuContent.FindAnyWidget("TicketsInput")).GetText().ToInt();
		CRF_PlayerRplToAuthorityManager.GetInstance().SetRespawnTime(respawnTime);
	}
	
	void LoadGearConfigList()
	{
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		m_gearsetlist = new CRF_GearScriptConfigStruct();
	
		if (!ctx.LoadFromFile("configs/GearScripts/GearScriptsConfigList.json"))
			return;
		
		ctx.ReadValue("", m_gearsetlist);
	}
	
	/**
	 * Add time delta based on the button name that was clicked
	 * !!!!! Changing the time delta is done above and in the menu layout !!!!!
	 */
	void UpdateTime()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		// Get the delta from the button name
		int delta = button.GetName().ToInt() * 60000;
		
		CRF_PlayerRplToAuthorityManager.GetInstance().UpdateTimer(delta);
	}
	
	void UpdateTicket()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		
		array<string> requestParts = {};
		button.GetName().Split("_", requestParts, true);
		
		string action = requestParts[1];
		int delta = requestParts[2].ToInt();
		FactionKey faction = requestParts[0];
		
		CRF_PlayerRplToAuthorityManager.GetInstance().UpdateTicket(action, faction, delta);
	}
	
	void UpdateGearSets()
	{
		// Load gearsets section
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		
		// Check if faction gearset needs updating
		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string faction : factions)
		{
			SCR_ListBoxComponent listBox = GetListBox(string.Format("%1ListBox", faction) ,gearSets);
			int selectedItem = listBox.GetSelectedItem();
			if (selectedItem < 0)
				continue;
			
			// Get the gearset key from selected item in the list box
			string key = TextWidget.Cast(listBox.GetElementComponent(listBox.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
			string gearSetPath;
			m_gearsetlist.gearset.Find(key, gearSetPath);
			
			// Ask the server to update factions gear
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateGearSet(faction, gearSetPath);
		}

		CloseConfirmAction();
	}
	
	void ConfirmAction()
	{
		// Find the button currently focused
		Widget button = GetGame().GetWorkspace().GetFocusedWidget();
		if (!button)
			return;	
		
		// Load menu content widget
		m_wConfirmationMenu = GetGame().GetWorkspace().CreateWidgets("{905BF1B70A9A44AC}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/ConfirmationMenu.layout");
		if (!m_wMenuContent)
			return;

		// Get menu buttons
		SCR_ButtonTextComponent runButton = GetMenuButton("ExcuteButton", m_wConfirmationMenu);
		SCR_ButtonTextComponent cancelButton = GetMenuButton("CancelButton", m_wConfirmationMenu);
		
		// Get the function that needs confirming from the button name in the layout
		string confirmActionFunc = button.GetName();

		// Setup script invokers
		cancelButton.m_OnClicked.Insert(CloseConfirmAction);
		switch (confirmActionFunc)
		{
			case "EnterAAR" : runButton.m_OnClicked.Insert(EnterAAR); break;
			case "ApplyGearSets" : runButton.m_OnClicked.Insert(UpdateGearSets); break;
		}
	}
	
	/**
	 * Close confirmation popup
	 */
	void CloseConfirmAction()
	{
		if (m_wConfirmationMenu)
		 delete m_wConfirmationMenu;
	}
	
	/**
	 * Advanced the game state to AAR
	 */
	void EnterAAR()
	{
		if (!CRF_EGamemodeState.AAR)
			return;

		CRF_PlayerRplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState(true);
		CloseConfirmAction();
	}
	
	void GamemodeMenuUpdate()
	{	
		// Get current mission time
		string m_sServerWorldTime = CRF_GameTimerManager.GetInstance().GetServerWorldTime();
		
		// Grab timer
		Widget gamerTimer = m_wMenuContent.FindAnyWidget("GameTimer");
		TextWidget gameTimerText = TextWidget.Cast(gamerTimer.FindWidget("CurrentGameTime0"));
		
		// Update Timer
		gameTimerText.SetText(m_sServerWorldTime);
		
		CRF_Gamemode gm = CRF_Gamemode.GetInstance();

		ref array<string> factions = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		// Update Gearset titles to current gearsets
		Widget gearSets = m_wMenuContent.FindAnyWidget("GearSets");
		foreach (string faction : factions)
		{
			string resourceName;
			switch (faction)
			{
				case "BLUFOR" : resourceName = gm.m_rBLUFORCurrentGearScript; break;
				case "OPFOR" : resourceName = gm.m_rOPFORCurrentGearScript; break;
				case "INDFOR" : resourceName = gm.m_rINDFORCurrentGearScript; break;
				case "CIV" : resourceName = gm.m_rCIVILIANCurrentGearScript; break;
			}
			
			string gearSetName =  resourceName.Substring(resourceName.LastIndexOf("/") + 1, resourceName.LastIndexOf(".") - resourceName.LastIndexOf("/") - 1);
			gearSetName.Replace("CRF_GS_", "");
			TextWidget.Cast(gearSets.FindAnyWidget(string.Format("%1ListTitle", faction))).SetText(gearSetName);
		}

		// Grab Ticket Counters
		Widget ticketCounters = m_wMenuContent.FindAnyWidget("Tickets");
		TextWidget bluforTicketText = TextWidget.Cast(ticketCounters.FindWidget("BluforTicketCount"));
		TextWidget opforTicketText = TextWidget.Cast(ticketCounters.FindWidget("OpforTicketCount"));
		TextWidget indforTicketText = TextWidget.Cast(ticketCounters.FindWidget("IndforTicketCount"));
		TextWidget civTicketText = TextWidget.Cast(ticketCounters.FindWidget("CivTicketCount"));
		
		// Update Ticket Counters
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (respawnManager)
		{
			bluforTicketText.SetText(respawnManager.GetFactionTickets("BLUFOR").ToString());
			opforTicketText.SetText(respawnManager.GetFactionTickets("OPFOR").ToString());
			indforTicketText.SetText(respawnManager.GetFactionTickets("INDFOR").ToString());
			civTicketText.SetText(respawnManager.GetFactionTickets("CIV").ToString());
		}
		
		Widget respawnWaveButton = m_wMenuContent.FindAnyWidget("RespawnWaveButton");
		TextWidget respawnWaveButtonText = TextWidget.Cast(respawnWaveButton.FindWidget("ActionButtonText"));
		bool respawnWave = CRF_RespawnManager.GetInstance().m_bCurrentWaveRespawn;
		if (respawnWave)
		{
			respawnWaveButtonText.SetText("Respawn Wave Enabled");
			respawnWaveButtonText.SetColorInt(Color.GREEN);
		}
		else
		{
			respawnWaveButtonText.SetText("Respawn Wave Disabled");
			respawnWaveButtonText.SetColorInt(Color.RED);
		}
		
		bool m_bRespawnEnabled = CRF_RespawnManager.GetInstance().m_bCurrentRespawnEnabled;
		Widget respawnEnabledButton = m_wMenuContent.FindAnyWidget("EnableRespawnButton");
		TextWidget respawnEnabledText = TextWidget.Cast(respawnEnabledButton.FindWidget("ActionButtonText"));
		if (m_bRespawnEnabled)
		{
			respawnEnabledText.SetText("Respawns Enabled");
			respawnEnabledText.SetColorInt(Color.GREEN);
		}
		else
		{
			respawnEnabledText.SetText("Respawns Disabled");
			respawnEnabledText.SetColorInt(Color.RED);
		}
	}
	
	//-----------------------------------------------------------------------------
	// Search Methods
	//-----------------------------------------------------------------------------
	
	//-----------------------------------------------------------------------------
	// Chat Methods
	//-----------------------------------------------------------------------------
	
	/**
	 * Handles chat toggle action
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;

		GetGame().GetCallqueue().Call(OpenChatWrap);
	}
	
	/**
	 * Opens the chat panel if not already open
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// GETTERS
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Gets the title of the current open page
	 */
	string GetCurrentOpenTab()
	{
		// Get Sub menu title text
		TextWidget menuSubTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("MenuSubTitle"));
		if (!menuSubTitle)
			return "";
		
		return menuSubTitle.GetText();
	}
}
class CRF_GearScriptConfigStruct
{
	ref map<string, string> gearset;
}