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
	
	// Data collections
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<vector> m_spawnPoints = {};
	protected ref array<Faction> m_factions = {};
	protected ref array<string> m_selectableFactions = {};
	
	// Ticket State
	protected int m_iSelectedTicket = -1;
	
	// Gear Script List
	protected ref CRF_GearScriptConfigStruct m_gearsetlist;
	
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

		// Set up menu navigation buttons
		InitializeMenuButtons();

		// Initialize chat panel
		InitializeChat();
		
		// Populate Admin Logs
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
		
		// Remove Gamemode updater
		GetGame().GetCallqueue().Remove(GamemodeMenuUpdate);
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

		// Clear data collections
		m_outGroups.Clear();
		m_spawnPoints.Clear();
		m_groupIDList.Clear();
		m_allPlayers.Clear();
		m_factions.Clear();
		m_selectableFactions.Clear();
		
		// Remove Gamemode updater
		GetGame().GetCallqueue().Remove(GamemodeMenuUpdate);
	}

	/**
	 * Populates a list box with available roles
	 * @param list The list box to populate with roles
	 */
	void AddRoles(SCR_ListBoxComponent list)
	{
		array<string> roleNames = {};
		SCR_Enum.GetEnumNames(CRF_EGearRole, roleNames);
		
		foreach (string role : roleNames)
		{
			list.AddItem(role);
		}
	}

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
	ResourceName GetPrefab(int groupID, int index)
	{
		SCR_AIGroup group = m_groupManagerComponent.FindGroup(groupID);
		if (!group)
			return ResourceName.Empty;
			
		string factionKey = group.GetFaction().GetFactionKey();
		ResourceName prefab = CRF_RoleHelper.RoleToResource(index, factionKey);
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
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		// Load Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("MenuButton3");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("MenuButton4");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4)
			return;
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(ResetGear);
		menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		menuButton2.m_OnClicked.Insert(AddGIRadio);
		menuButton3.m_OnClicked.Insert(AddBinos);
		menuButton4.m_OnClicked.Insert(AddMap);
		
		// Setup selection change handler
		playerList.m_OnChanged.Insert(UpdateDefaultGear);
		
		// Change menu title
		UpdateMenuTitle("Gear Reset");

		// Populate player list
		PopulatePlayerList(playerList);
		
		// Add available roles
		AddRoles(roleList);
	}
	
	/**
	 * Populates a list with active players
	 * @param list The list to populate with player names
	 */
	protected void PopulatePlayerList(SCR_ListBoxComponent list)
	{
		// Get all players
		m_playerManager.GetPlayers(m_allPlayers);
		TStringArray playerNames = {};

		// Get and sort player names
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		// Add players to list if they're in a group and not spectating
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			
			if (!m_groupManagerComponent.GetPlayerGroup(playerId))
				continue;
				
			if (CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
				continue;
			
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
			list.AddItemWithColor(string.Format("%1", name), playerFaction.GetFactionColor());
		}
	}

	/**
	 * Updates default gear selection based on player selection
	 */
	void UpdateDefaultGear()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		// If no player selected, return
		if (playerList.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Find the player's role in the list and select it
		for (int i = 0; i < roleList.GetItemCount(); i++)
		{
			if (i == CRF_RoleHelper.ResourceToRole(CRF_SlottingManager.GetInstance().GetPlayerSlotResource(playerId)))
			{
				roleList.SetItemSelected(i, true);
				return;
			}
		}
	}

	/**
	 * Adds leadership radio to selected player
	 */
	void AddLeaderRadio()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_GearscriptManager.GetInstance().GetGearScriptSettings(factionKey).m_rLongRangeRadioPrefab;
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds GI radio to selected player
	 */
	void AddGIRadio()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_GearscriptManager.GetInstance().GetGearScriptSettings(factionKey).m_rShortRangeRadioPrefab;
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds binoculars to selected player
	 */
	void AddBinos()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get binoculars prefab and add item
		string binosPrefab = GetBinos(playerId);
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, binosPrefab, true);
	}
	
	/**
	* Adds map to selected player
	*/
	void AddMap()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		//Add map
		const string mapPrefab = "{13772C903CB5E4F7}Prefabs/Items/Equipment/Maps/PaperMap_01_folded.et";
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, mapPrefab, true);
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
			BaseContainerTools.LoadContainer(CRF_GearscriptManager.GetInstance().GetGearScriptResource(factionKey)).GetResource().ToBaseContainer()
		));

		return gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab;
	}

	/**
	 * Reset a player's gear to their role default
	 */
	void ResetGear()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent roleList = GetListBox("RoleListBox0");
		if (!playerList || !roleList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		if (roleList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get player's group
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		int groupID = playerGroup.GetGroupID();
		
		// Get the prefab for the selected role
		ResourceName prefab = GetPrefab(groupID, roleList.GetSelectedItem());
		if (prefab.IsEmpty())
			return;

		// Reset player's gear
		CRF_RplToAuthorityManager.GetInstance().ResetGear(playerId, prefab, true);
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load List Boxes
		SCR_ButtonTextComponent replyButton = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent assignButton = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent closeButton = GetMenuButton("MenuButton2");
		SCR_ButtonTextComponent searchButton = GetMenuButton("SearchButton0");
		if (!replyButton || !assignButton || !closeButton || !searchButton)
			return;
		
		// Setup button event handlers
		replyButton.m_OnClicked.Insert(ReplyToTicket);
		assignButton.m_OnClicked.Insert(AssignAdminToTicket);
		closeButton.m_OnClicked.Insert(CloseAdminTicket);
		searchButton.m_OnClicked.Insert(SearchList0);
		
		// Setup selection change handlers
		playerList.m_OnChanged.Insert(PopulateTicketMessages);
		
		// Change title of the menu
		UpdateMenuTitle("Tickets");

		// Populate list of players that need help
		PopulateOpenTicketList();
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
		CRF_RplToAuthorityManager.GetInstance().AssignAdminTicket(m_iSelectedTicket, adminID, false);
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
		CRF_RplToAuthorityManager.GetInstance().CloseAdminTicket(m_iSelectedTicket, adminID, true);
		
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
		CRF_RplToAuthorityManager.GetInstance().ReplyAdminMessage(reply, m_iSelectedTicket, adminID, false);
		
		// Clear Text in reply box
		replyBox.SetText("");
	}
	
	/**
	* Populates the list of messages selected in list 1
	*/
	void PopulateTicketMessages()
	{		
		int playerID = -1;

		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent ticketMessagesList = GetListBox("TicketMessagesListBox0");
		if (!playerList || !ticketMessagesList)
			return;
		
		// Clear old Messages 
		ticketMessagesList.Clear();
		
		// Check if a ticket was selected either via the list or pre ui refresh
		if (playerList.GetSelectedItem() != -1)
		{
			// Get selected player ID
			string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
			playerID = GetplayerIdFromName(playerName);
			if (playerID == 0)
				return;
			
			// Store selected ticket for use after ui refresh
			m_iSelectedTicket = playerID;
		}
		else if (m_iSelectedTicket != -1)
			// Get the stored ticket selected before refresh
			playerID = m_iSelectedTicket;
		else
			return;

		// Get messages in the ticket
		array<ref CRF_TicketMessageData> messages = m_AdminMenuManager.GetTicketMessages(playerID);
		if (!messages)
			return;
		
		// Format and add the messages to the list
		foreach (int i, ref CRF_TicketMessageData message : messages)
		{
			ticketMessagesList.AddItem(string.Format("%1 - %2: %3", message.timestamp, message.sender, message.msg));
		}

	}
		
	/**
	 * Populates the list of players that need help
	 */
	void PopulateOpenTicketList()
	{
		TStringArray playerNames = {};

		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Grab player ids that have open tickets
		array<int> openTickets = m_AdminMenuManager.GetOpenTickets();
		
		// Clear old ticket list
		playerList.Clear();

		// Get and sort player names
		foreach (int playerId : openTickets)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		// Open tickets to list
		foreach (string name : playerNames)
		{
			playerList.AddItem(string.Format("%1", name));
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!playerList || !groupList)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("BLUFOR");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("OPFOR");
		SCR_ButtonTextComponent menuButton3 = GetMenuButton("INDFOR");
		SCR_ButtonTextComponent menuButton4 = GetMenuButton("CIV");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2 || !menuButton3 || !menuButton4)
			return;
			
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(RespawnPlayer);
		menuButton1.m_OnClicked.Insert(RespawnSide);
		menuButton2.m_OnClicked.Insert(RespawnSide);
		menuButton3.m_OnClicked.Insert(RespawnSide);
		menuButton4.m_OnClicked.Insert(RespawnSide);
		
		// Setup selection change handlers
		playerList.m_OnChanged.Insert(UpdateSpawnGroupRequest);
		groupList.m_OnChanged.Insert(UpdateSpawnpoint);
		
		// Change title of the menu
		UpdateMenuTitle("Respawn");

		// Get all players and groups
		m_playerManager.GetPlayers(m_allPlayers);
		m_outGroups = CRF_SlottingManager.GetInstance().GetAllGroups();

		// Populate Dead Players list
		PopulateDeadPlayersList();
		
		// Populate Groups list
		PopulateGroupsList();
	}
	
	/**
	 * Populates the list of dead/spectating players
	 */
	protected void PopulateDeadPlayersList()
	{
		TStringArray playerNames = {};
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		// Get and sort player names
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		// Add dead or spectating players to list
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);

			if (CRF_SlottingManager.GetInstance().IsPlayerConsideredDead(playerId) ||
				CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
			{
				playerList.AddItemWithColor(string.Format("%1", name), playerFaction.GetFactionColor());
			}
		}
	}
	
	/**
	 * Populates the list of available groups
	 */
	protected void PopulateGroupsList()
	{
		foreach (SCR_AIGroup group : m_outGroups)
		{
			// Load List Boxes
			SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
			if (!groupList)
				return;
			
			// Get faction info
			Faction groupFaction = group.GetFaction();
			if (!groupFaction)
				continue;
				
			string factionKey = groupFaction.GetFactionKey();
			if (factionKey.IsEmpty() || factionKey == "SPEC")
				continue;
				
			string factionTag = factionKey.Substring(0, 3);
			
			// Add group to list
			groupList.AddItem(string.Format("%1 | %2", factionTag, group.GetCustomNameWithOriginal()));
			m_groupIDList.Insert(group.GetGroupID());
		}
	}

	/**
	 * Requests server to provide group ID for selected player
	 */
	void UpdateSpawnGroupRequest()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!groupList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
		
		int selectedGroupID = CRF_SlottingManager.GetInstance().GetPlayerSlotGroup(playerId);
		
		foreach (int i, SCR_AIGroup group : m_outGroups)
		{
			if (selectedGroupID == group.GetGroupID())
			{
				// Adjust index for client mode
				int itemIndex = i;
				if (RplSession.Mode() == RplMode.Client)
					itemIndex = i - 1;

				groupList.SetItemSelected(itemIndex, true);
				return;
			}
		};
	}

	/**
	 * Updates spawnpoint list based on selected group
	 */
	void UpdateSpawnpoint()
	{
		// Load List Boxes
		SCR_ListBoxComponent respawnPoints = GetListBox("SpawnpointListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		if (!respawnPoints || !groupList)
			return;
		
		if (groupList.GetSelectedItem() < 0)
			return;
			
		// Clear previous data
		respawnPoints.Clear();
		m_spawnPoints.Clear();
		
		// Get selected group ID
		int groupID = m_groupIDList.Get(groupList.GetSelectedItem());

		// Get player names
		TStringArray playerNames = {};
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		playerNames.Sort(false);

		// Add players from the selected group as spawnpoints
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			SCR_AIGroup playerGroup = CRF_SlottingManager.GetInstance().GetPlayerSlotGroup(playerId);
			
			if (!playerGroup)
				continue;
				
			if (playerGroup.GetGroupID() == groupID)
			{
				IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				if (!playerEntity)
					continue;
					
				respawnPoints.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerId)));
				m_spawnPoints.Insert(playerEntity.GetOrigin());
			}
		}
	}

	/**
	 * Respawns the selected player at the selected spawnpoint and group
	 */
	void RespawnPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent respawnPoints = GetListBox("SpawnpointListBox0");
		SCR_ListBoxComponent groupList = GetListBox("GroupListBox0");
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!respawnPoints || !groupList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		if (groupList.GetSelectedItem() < 0)
			return;

		if (respawnPoints.GetSelectedItem() < 0)
			return;
		
		// Get selected player
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get selected group and spawnpoint
		int groupID = m_groupIDList.Get(groupList.GetSelectedItem());
		vector spawnpoint = m_spawnPoints.Get(respawnPoints.GetSelectedItem());
		
		// Spawn player on group
		CRF_RplToAuthorityManager.GetInstance().SpawnOnGroup(playerId, spawnpoint, groupID, true);

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
		
		CRF_RplToAuthorityManager.GetInstance().RespawnFaction(button.GetName(), true);
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent playerList1 = GetListBox("PlayerListBox1");
		if (!playerList0 || !playerList1)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent searchButton1 = GetMenuButton("SearchButton1");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!searchButton0 || !searchButton1 || !menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		searchButton1.m_OnClicked.Insert(SearchList1);
		menuButton0.m_OnClicked.Insert(TeleportLocalToSelected);
		menuButton1.m_OnClicked.Insert(TeleportPlayers);
		menuButton2.m_OnClicked.Insert(TeleportSelectedToLocal);
		
		// Change title of the menu
		UpdateMenuTitle("Teleport");

		// Populate player lists
		PopulatePlayerList(playerList0);
		PopulatePlayerList(playerList1);
	}

	/**
	 * Teleports local player to selected player
	 */
	void TeleportLocalToSelected()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		if (!playerList0)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId2 = GetplayerIdFromName(playerName);
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		m_clientComponent.TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerId2);
	}
	
	/**
	 * Teleports selected player to local player
	 */
	void TeleportSelectedToLocal()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		if (!playerList0)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId2 = GetplayerIdFromName(playerName);
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		CRF_RplToAuthorityManager.GetInstance().TeleportPlayers(playerId2, SCR_PlayerController.GetLocalPlayerId(), true);
	}

	/**
	 * Teleports player 1 to player 2's position
	 */
	void TeleportPlayers()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList0 = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent playerList1 = GetListBox("PlayerListBox1");
		if (!playerList0 || !playerList1)
			return;
		
		if (playerList0.GetSelectedItem() < 0)
			return;

		if (playerList1.GetSelectedItem() < 0)
			return;

		// Get selected player IDs
		string playerName1 = TextWidget.Cast(playerList0.GetElementComponent(playerList0.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		string playerName2 = TextWidget.Cast(playerList1.GetElementComponent(playerList1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId1 = GetplayerIdFromName(playerName1);
		int playerId2 = GetplayerIdFromName(playerName2);

		if (playerId1 == 0 || playerId2 == 0)
			return;

		// Teleport player 1 to player 2
		CRF_RplToAuthorityManager.GetInstance().TeleportPlayers(playerId1, playerId2, true);
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		SCR_ListBoxComponent factionList = GetListBox("FactionListBox0");
		if (!playerList || !factionList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		SCR_ButtonTextComponent menuButton2 = GetMenuButton("MenuButton2");
		if (!searchButton0 || !menuButton0 || !menuButton1 || !menuButton2)
			return;
		
		// Setup existing hint text if available
		hintBox.SetText(m_clientComponent.m_sHintText);
		
		// Setup button event handlers
		searchButton0.m_OnClicked.Insert(SearchList0);
		menuButton0.m_OnClicked.Insert(SendHintAll);
		menuButton1.m_OnClicked.Insert(SendHintFaction);
		menuButton2.m_OnClicked.Insert(SendHintPlayer);
		
		// Change title of the menu
		UpdateMenuTitle("Hint");

		// Populate player list
		PopulatePlayerList(playerList);
		
		// Populate faction list
		PopulateFactionList();
	}
	
	/**
	 * Populates the list of active factions
	 */
	protected void PopulateFactionList()
	{
		// Get all factions
		GetGame().GetFactionManager().GetFactionsList(m_factions);
		
		// Load List Boxes
		SCR_ListBoxComponent factionList = GetListBox("FactionListBox0");
		if (!factionList)
			return;
		
		// Add factions with active players
		foreach (Faction faction : m_factions)
		{
			if (SCR_FactionManager.SGetFactionPlayerCount(faction) > 0)
			{
				factionList.AddItem(faction.GetFactionName());
				m_selectableFactions.Insert(faction.GetFactionKey());
			}
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
		CRF_RplToAuthorityManager.GetInstance().SendHint(data);
	}

	/**
	 * Sends hint message to players in selected faction
	 */
	void SendHintFaction()
	{
		// Load List Boxes
		SCR_ListBoxComponent factionList = GetListBox("FactionListBox0");
		if (!factionList)
			return;
	
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		if (factionList.GetSelectedItem() == -1)
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		string factionKey = m_selectableFactions.Get(factionList.GetSelectedItem());
		CRF_RplToAuthorityManager.GetInstance().SendHint(data, -1, factionKey);
	}

	/**
	 * Sends hint message to selected player
	 */
	void SendHintPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Reply Box
		MultilineEditBoxWidget hintBox = GetMultilineEditBox("HintBox0");
		if (!hintBox)
			return;
		
		if (playerList.GetSelectedItem() == -1)
			return;

		string data = hintBox.GetText();
		m_clientComponent.m_sHintText = data;
		
		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		CRF_RplToAuthorityManager.GetInstance().SendHint(data, playerId);
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
		
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Menu Buttons
		SCR_ButtonTextComponent searchButton0 = GetMenuButton("SearchButton0");
		SCR_ButtonTextComponent menuButton0 = GetMenuButton("MenuButton0");
		SCR_ButtonTextComponent menuButton1 = GetMenuButton("MenuButton1");
		if (!searchButton0 || !menuButton0 || !menuButton1)
			return;
		
		// Setup button event handlers
		menuButton0.m_OnClicked.Insert(HealPlayer);
		menuButton1.m_OnClicked.Insert(HealPlayerVehicle);
		searchButton0.m_OnClicked.Insert(SearchList0);
		
		// Change title of the menu
		UpdateMenuTitle("Heal");
		

		// Populate player list
		PopulatePlayerList(playerList);
	}
	
	/**
	 * Heals the selected player
	 */
	void HealPlayer()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Heal player only (not vehicle)
		CRF_RplToAuthorityManager.GetInstance().Heal(playerId, true, false);
	}
	
	/**
	 * Repairs the vehicle of the selected player
	 */
	void HealPlayerVehicle()
	{
		// Load List Boxes
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		if (playerList.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(playerList.GetElementComponent(playerList.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Heal player and vehicle
		CRF_RplToAuthorityManager.GetInstance().Heal(playerId, true, true);
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

		foreach (string name, string path : m_gearsetlist.gearset)
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
		GamemodeMenuUpdate();

		GetGame().GetCallqueue().CallLater(GamemodeMenuUpdate, 1000, true);
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
		
		CRF_RplToAuthorityManager.GetInstance().UpdateTimer(delta);
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
		
		CRF_RplToAuthorityManager.GetInstance().UpdateTicket(action, faction, delta);
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
			CRF_RplToAuthorityManager.GetInstance().UpdateGearSet(faction, gearSetPath);
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

		CRF_RplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState(true);
		CloseConfirmAction();
	}
	
	void GamemodeMenuUpdate()
	{	
		// Get current mission time
		string m_sServerWorldTime = CRF_GamemodeManager.GetInstance().GetServerWorldTime();
		
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
				case "BLUFOR" : resourceName = gm.m_BLUFORGearScriptSettings.m_rGearScript; break;
				case "OPFOR" : resourceName = gm.m_OPFORGearScriptSettings.m_rGearScript; break;
				case "INDFOR" : resourceName = gm.m_INDFORGearScriptSettings.m_rGearScript; break;
				case "CIV" : resourceName = gm.m_CIVILIANGearScriptSettings.m_rGearScript; break;
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

	}
	
	//-----------------------------------------------------------------------------
	// Search Methods
	//-----------------------------------------------------------------------------
	
	/**
	 * Search the first player list
	 */
	void SearchList0()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;
		
		// Load Search Box
		EditBoxWidget searchBox = GetEditBox("SearchBox0");
		if (!searchBox)
			return;
		
		SearchPlayerList(playerList, searchBox.GetText());
	}
	
	/**
	 * Search the second player list
	 */
	void SearchList1()
	{
		// Load List Box
		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox1");
		if (!playerList)
			return;
		
		// Load Search Box
		EditBoxWidget searchBox = GetEditBox("SearchBox1");
		if (!searchBox)
			return;
		
		SearchPlayerList(playerList, searchBox.GetText());
	}
	
	/**
	 * Filters a player list based on search text
	 * @param list The list box to filter
	 * @param searchData The search text to filter by
	 */
	void SearchPlayerList(SCR_ListBoxComponent list, string searchData)
	{
		TStringArray playerNames = {};
		m_playerManager.GetPlayers(m_allPlayers);
		list.Clear();

		// If search is empty, show all players
		if (searchData == "")
		{
			foreach (int playerId : m_allPlayers)
				playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		} 
		else 
		{
			// Otherwise filter by search text
			string searchLower = searchData;
			searchLower.ToLower();
			
			foreach (int playerId : m_allPlayers)
			{
				string playerName = m_playerManager.GetPlayerName(playerId);
				string playerNameLower = playerName;
				playerNameLower.ToLower();

				if (playerNameLower.Contains(searchLower))
					playerNames.Insert(playerName);
			}
		}

		// Sort and add filtered players
		playerNames.Sort(false);
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
			
			if (!m_groupManagerComponent.GetPlayerGroup(playerId))
				continue;
				
			if (CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
				continue;
			
			list.AddItemWithColor(string.Format("%1", name), playerFaction.GetFactionColor());
		}
	}
	
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