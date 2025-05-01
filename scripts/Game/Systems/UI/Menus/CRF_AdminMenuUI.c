modded enum ChimeraMenuPreset
{
	CoalAdminMenu
}

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
	protected CRF_PlayerControllerComponent m_clientComponent;
	protected InputManager m_InputManager;
	protected SCR_ChatPanel m_ChatPanel;
	protected bool m_bFocused = true;
	
	// Main widgets
	protected Widget m_wRoot;
	protected FrameWidget m_adminMenuRoot;
	protected FrameWidget m_gearResetMenuRoot;
	
	// Game managers
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	
	// List containers
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	
	// List components
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	
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
	
	// Action buttons
	protected SCR_ButtonTextComponent m_actionButton;
	protected SCR_ButtonTextComponent m_searchButton1;
	protected SCR_ButtonTextComponent m_searchButton2;
	protected SCR_ButtonTextComponent m_menuButton1;
	protected SCR_ButtonTextComponent m_menuButton2;
	protected SCR_ButtonTextComponent m_menuButton3;
	protected SCR_ButtonTextComponent m_menuButton4;
	
	// Menu text widgets
	protected TextWidget m_respawnMenuText;
	protected TextWidget m_resetGearMenuText;
	protected TextWidget m_teleportMenuText;
	protected TextWidget m_hintMenuText;
	protected TextWidget m_healMenuText;
	
	// Data collections
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<vector> m_spawnPoints = {};
	protected ref array<Faction> m_factions = {};
	protected ref array<string> m_selectableFactions = {};

	//-----------------------------------------------------------------------------
	// General UI Methods
	//-----------------------------------------------------------------------------

	/**
	 * Initialize the menu when it opens
	 * Sets up all UI elements and displays initial respawn menu
	 */
	override void OnMenuOpen()
	{
		// Get manager instances
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_clientComponent = CRF_PlayerControllerComponent.GetInstance();

		// Setup menu roots
		m_wRoot = GetRootWidget();
		m_adminMenuRoot = FrameWidget.Cast(m_wRoot.FindWidget("AdminMenuTools"));

		// Initialize list boxes
		InitializeListBoxes();
		
		// Initialize action buttons
		InitializeActionButtons();
		
		// Initialize edit boxes
		InitializeEditBoxes();

		// Set up the initial menu (Respawn)
		ClearMenu();
		InitializeRespawnMenu();

		// Set up menu navigation buttons
		InitializeMenuButtons();

		// Initialize chat panel
		InitializeChat();
	}
	
	/**
	 * Initialize all list box components
	 */
	protected void InitializeListBoxes()
	{
		// List box 1
		m_list1Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List1Box"));
		m_list1 = SCR_ListBoxComponent.Cast(m_list1Root.FindHandler(SCR_ListBoxComponent));
		
		// List box 2
		m_list2Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List2Box"));
		m_list2 = SCR_ListBoxComponent.Cast(m_list2Root.FindHandler(SCR_ListBoxComponent));
		
		// List box 3
		m_list3Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List3Box"));
		m_list3 = SCR_ListBoxComponent.Cast(m_list3Root.FindHandler(SCR_ListBoxComponent));
		
		// List box 4
		m_list4Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List4Box"));
		m_list4 = SCR_ListBoxComponent.Cast(m_list4Root.FindHandler(SCR_ListBoxComponent));
	}
	
	/**
	 * Initialize all action buttons
	 */
	protected void InitializeActionButtons()
	{
		m_actionButton = SCR_ButtonTextComponent.GetButtonText("ActionButton", m_adminMenuRoot);
		m_searchButton1 = SCR_ButtonTextComponent.GetButtonText("SearchButton1", m_adminMenuRoot);
		m_searchButton2 = SCR_ButtonTextComponent.GetButtonText("SearchButton2", m_adminMenuRoot);
		m_menuButton1 = SCR_ButtonTextComponent.GetButtonText("MenuButton1", m_adminMenuRoot);
		m_menuButton2 = SCR_ButtonTextComponent.GetButtonText("MenuButton2", m_adminMenuRoot);
		m_menuButton3 = SCR_ButtonTextComponent.GetButtonText("MenuButton3", m_adminMenuRoot);
		m_menuButton4 = SCR_ButtonTextComponent.GetButtonText("MenuButton4", m_adminMenuRoot);
	}
	
	/**
	 * Initialize all edit boxes
	 */
	protected void InitializeEditBoxes()
	{
		m_editBox1 = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox1"));
		m_editbox2 = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox2"));
		m_editbox3 = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox3"));
		m_windowBox1 = WindowWidget.Cast(m_wRoot.FindAnyWidget("Window0"));
	}
	
	/**
	 * Initialize menu navigation buttons
	 */
	protected void InitializeMenuButtons()
	{
		// Respawn menu button
		m_respawnMenuButton = SCR_ButtonTextComponent.GetButtonText("RespawnButton", m_wRoot);
		m_respawnMenuButton.m_OnClicked.Insert(RespawnButton);
		m_respawnMenuText = TextWidget.Cast(m_respawnMenuButton.GetRootWidget().FindWidget("RespawnText"));

		// Reset gear menu button
		m_resetGearMenuButton = SCR_ButtonTextComponent.GetButtonText("ResetGearButton", m_wRoot);
		m_resetGearMenuButton.m_OnClicked.Insert(ResetGearButton);
		m_resetGearMenuText = TextWidget.Cast(m_resetGearMenuButton.GetRootWidget().FindWidget("ResetGearText"));

		// Teleport menu button
		m_teleportMenuButton = SCR_ButtonTextComponent.GetButtonText("TeleportButton", m_wRoot);
		m_teleportMenuButton.m_OnClicked.Insert(TeleportButton);
		m_teleportMenuText = TextWidget.Cast(m_teleportMenuButton.GetRootWidget().FindWidget("TeleportText"));

		// Hint menu button
		m_hintMenuButton = SCR_ButtonTextComponent.GetButtonText("HintButton", m_wRoot);
		m_hintMenuButton.m_OnClicked.Insert(HintButton);
		m_hintMenuText = TextWidget.Cast(m_hintMenuButton.GetRootWidget().FindWidget("HintText"));

		// Heal menu button
		m_healMenuButton = SCR_ButtonTextComponent.GetButtonText("HealButton", m_wRoot);
		m_healMenuButton.m_OnClicked.Insert(HealButton);
		m_healMenuText = TextWidget.Cast(m_healMenuButton.GetRootWidget().FindWidget("HealText"));
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
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		if (m_ChatPanel)
			m_ChatPanel.SetAlwaysVisible(false);
	}

	/**
	 * Activates the Respawn menu
	 */
	void RespawnButton()
	{
		UpdateMenuButtonColors(m_respawnMenuText);
		ClearMenu();
		InitializeRespawnMenu();
	}

	/**
	 * Activates the Reset Gear menu
	 */
	void ResetGearButton()
	{
		UpdateMenuButtonColors(m_resetGearMenuText);
		ClearMenu();
		InitializeGearMenu();
	}

	/**
	 * Activates the Teleport menu
	 */
	void TeleportButton()
	{
		UpdateMenuButtonColors(m_teleportMenuText);
		ClearMenu();
		InitializeTeleportMenu();
	}

	/**
	 * Activates the Hint menu
	 */
	void HintButton()
	{
		UpdateMenuButtonColors(m_hintMenuText);
		ClearMenu();
		InitializeHintMenu();
	}

	/**
	 * Activates the Heal menu
	 */
	void HealButton()
	{
		UpdateMenuButtonColors(m_healMenuText);
		ClearMenu();
		InitializeHealMenu();
	}
	
	/**
	 * Updates menu button colors to highlight the active menu
	 * @param activeText The text widget of the active menu button
	 */
	protected void UpdateMenuButtonColors(TextWidget activeText)
	{
		// Default color for inactive buttons
		Color inactiveColor = Color.FromRGBA(115, 115, 115, 255);
		
		// Set all texts to inactive color
		m_respawnMenuText.SetColor(inactiveColor);
		m_resetGearMenuText.SetColor(inactiveColor);
		m_teleportMenuText.SetColor(inactiveColor);
		m_hintMenuText.SetColor(inactiveColor);
		m_healMenuText.SetColor(inactiveColor);
		
		// Set active button text to white
		activeText.SetColor(Color.FromInt(0xffffffff));
	}

	/**
	 * Clears all menu elements and data
	 * Resets visibility and clears event handlers
	 */
	void ClearMenu()
	{
		// Hide all UI elements
		m_list1Root.SetVisible(false);
		m_list2Root.SetVisible(false);
		m_list3Root.SetVisible(false);
		m_list4Root.SetVisible(false);
		m_editBox1.SetVisible(false);
		m_editbox2.SetVisible(false);
		m_editbox3.SetVisible(false);
		m_windowBox1.SetVisible(false);
		
		// Reset action buttons
		m_actionButton.SetVisible(false, false);
		m_actionButton.m_OnClicked.Clear();
		m_searchButton1.SetVisible(false, false);
		m_searchButton1.m_OnClicked.Clear();
		m_searchButton2.SetVisible(false, false);
		m_searchButton2.m_OnClicked.Clear();
		
		// Reset menu buttons
		m_menuButton1.SetVisible(false, false);
		m_menuButton2.SetVisible(false, false);
		m_menuButton3.SetVisible(false, false);
		m_menuButton4.SetVisible(false, false);
		m_menuButton1.m_OnClicked.Clear();
		m_menuButton2.m_OnClicked.Clear();
		m_menuButton3.m_OnClicked.Clear();
		m_menuButton4.m_OnClicked.Clear();
		
		// Clear list boxes
		m_list1.Clear();
		m_list2.Clear();
		m_list3.Clear();
		m_list4.Clear();
		m_list1.m_OnChanged.Clear();
		m_list2.m_OnChanged.Clear();
		m_list3.m_OnChanged.Clear();
		m_list4.m_OnChanged.Clear();
		
		// Clear text input
		m_editBox1.SetText("");

		// Clear data collections
		m_outGroups.Clear();
		m_spawnPoints.Clear();
		m_groupIDList.Clear();
		m_allPlayers.Clear();
		m_factions.Clear();
		m_selectableFactions.Clear();

		// Reset text labels
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List4Text")).SetText("");
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
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	/**
	 * Handle regaining menu focus
	 */
	override void OnMenuFocusGained()
	{
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
		// Setup UI elements
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_menuButton3.SetVisible(true, false);
		m_editbox2.SetVisible(true);
		
		// Setup button event handlers
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_actionButton.m_OnClicked.Insert(ResetGear);
		m_menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		m_menuButton2.m_OnClicked.Insert(AddGIRadio);
		m_menuButton3.m_OnClicked.Insert(AddBinos);
		
		// Set button and list text
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Reset Gear");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Leaders Radio");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add GI Radio");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Binos");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Roles");
		
		// Setup selection change handler
		m_list1.m_OnChanged.Insert(UpdateDefaultGear);

		// Populate player list
		PopulatePlayerList(m_list1);
		
		// Add available roles
		AddRoles(m_list2);
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
				
			list.AddItem(string.Format("%1", name));
		}
	}

	/**
	 * Updates default gear selection based on player selection
	 */
	void UpdateDefaultGear()
	{
		// If no player selected, return
		if (m_list1.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Find the player's role in the list and select it
		for (int i = 0; i < m_list2.GetItemCount(); i++)
		{
			if (CRF_SlottingManager.GetInstance().GetPlayerSlotResource(playerId).Contains(CRF_RoleHelper.RoleToString(i)))
			{
				m_list2.SetItemSelected(i, true);
				return;
			}
		}
	}

	/**
	 * Adds leadership radio to selected player
	 */
	void AddLeaderRadio()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_GearscriptManager.GetInstance().GetGearScriptSettings(factionKey).m_rLeadershipRadiosPrefab;
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds GI radio to selected player
	 */
	void AddGIRadio()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get the player's group and faction
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		// Get radio prefab and add item
		string factionKey = playerGroup.GetFaction().GetFactionKey();
		ResourceName radioPrefab = CRF_GearscriptManager.GetInstance().GetGearScriptSettings(factionKey).m_rGIRadiosPrefab;
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, radioPrefab, true);
	}

	/**
	 * Adds binoculars to selected player
	 */
	void AddBinos()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get binoculars prefab and add item
		string binosPrefab = GetBinos(playerId);
		CRF_RplToAuthorityManager.GetInstance().AddItem(playerId, binosPrefab, true);
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
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get player's group
		SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
		if (!playerGroup)
			return;
			
		int groupID = playerGroup.GetGroupID();
		
		// Get the prefab for the selected role
		ResourceName prefab = GetPrefab(groupID, m_list2.GetSelectedItem());
		if (prefab.IsEmpty())
			return;

		// Reset player's gear
		CRF_RplToAuthorityManager.GetInstance().ResetGear(playerId, prefab, true);
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
		// Setup UI elements
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_list3Root.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		
		// Setup button event handlers
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_actionButton.m_OnClicked.Insert(RespawnPlayer);
		
		// Setup selection change handlers
		m_list1.m_OnChanged.Insert(UpdateSpawnGroupRequest);
		m_list2.m_OnChanged.Insert(UpdateSpawnpoint);

		// Set button and list text
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Respawn Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Dead Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Groups");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("Spawnpoints");

		// Get all players and groups
		m_playerManager.GetPlayers(m_allPlayers);
		m_groupManagerComponent.GetAllPlayableGroups(m_outGroups);

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

		// Get and sort player names
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		// Add dead or spectating players to list
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);

			if (CRF_SlottingManager.GetInstance().IsPlayerConsideredDead(playerId) ||
				CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
			{
				m_list1.AddItem(string.Format("%1", name));
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
			// Get faction info
			Faction groupFaction = group.GetFaction();
			if (!groupFaction)
				continue;
				
			string factionKey = groupFaction.GetFactionKey();
			if (factionKey.IsEmpty() || factionKey == "SPEC")
				continue;
				
			string factionTag = factionKey.Substring(0, 3);
			
			// Add group to list
			m_list2.AddItem(string.Format("%1 | %2", factionTag, group.GetCustomNameWithOriginal()));
			m_groupIDList.Insert(group.GetGroupID());
		}
	}

	/**
	 * Requests server to provide group ID for selected player
	 */
	void UpdateSpawnGroupRequest()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;
			
		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Request player's group from server
		CRF_RplToAuthorityManager.GetInstance().RequestGroupIdFromServer(playerId, SCR_PlayerController.GetLocalPlayerId());
	}

	/**
	 * Updates the selected group based on group ID from server
	 * @param groupId The group ID to select
	 */
	void UpdateSpawnGroup(int groupId)
	{
		foreach (int i, SCR_AIGroup group : m_outGroups)
		{
			if (groupId == group.GetGroupID())
			{
				// Adjust index for client mode
				int itemIndex = i;
				if (RplSession.Mode() == RplMode.Client)
					itemIndex = i - 1;

				m_list2.SetItemSelected(itemIndex, true);
				return;
			}
		};
	}

	/**
	 * Updates spawnpoint list based on selected group
	 */
	void UpdateSpawnpoint()
	{
		if (m_list2.GetSelectedItem() < 0)
			return;
			
		// Clear previous data
		m_list3.Clear();
		m_spawnPoints.Clear();
		
		// Get selected group ID
		int groupID = m_groupIDList.Get(m_list2.GetSelectedItem());

		// Get player names
		TStringArray playerNames = {};
		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		playerNames.Sort(false);

		// Add players from the selected group as spawnpoints
		foreach (string name : playerNames)
		{
			int playerId = GetplayerIdFromName(name);
			SCR_AIGroup playerGroup = m_groupManagerComponent.GetPlayerGroup(playerId);
			
			if (!playerGroup)
				continue;
				
			if (playerGroup.GetGroupID() == groupID)
			{
				IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				if (!playerEntity)
					continue;
					
				m_list3.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerId)));
				m_spawnPoints.Insert(playerEntity.GetOrigin());
			}
		}
	}

	/**
	 * Respawns the selected player at the selected spawnpoint and group
	 */
	void RespawnPlayer()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		if (m_list3.GetSelectedItem() < 0)
			return;
		
		// Get selected player
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;
			
		// Get selected group and spawnpoint
		int groupID = m_groupIDList.Get(m_list2.GetSelectedItem());
		vector spawnpoint = m_spawnPoints.Get(m_list3.GetSelectedItem());
		
		// Spawn player on group
		CRF_RplToAuthorityManager.GetInstance().SpawnOnGroup(playerId, spawnpoint, groupID, true);

		// Refresh the menu after a short delay
		GetGame().GetCallqueue().CallLater(ClearMenu, 1250, false);
		GetGame().GetCallqueue().CallLater(InitializeRespawnMenu, 1825, false);
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
		// Setup UI elements
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_editbox3.SetVisible(true);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		m_searchButton2.SetVisible(true, false);
		
		// Setup button event handlers
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_searchButton2.m_OnClicked.Insert(SearchList2);
		m_menuButton1.m_OnClicked.Insert(TeleportLocal);
		m_menuButton2.m_OnClicked.Insert(TeleportPlayers);

		// Set button and list text
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport to Player 1");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport Player 1 to Player 2");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Player 1");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Player 2");

		// Populate player lists
		PopulatePlayerList(m_list1);
		PopulatePlayerList(m_list2);
	}

	/**
	 * Teleports local player to selected player
	 */
	void TeleportLocal()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId2 = GetplayerIdFromName(playerName);
		if (playerId2 == 0)
			return;

		// Teleport local player to target
		m_clientComponent.TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerId2);
	}

	/**
	 * Teleports player 1 to player 2's position
	 */
	void TeleportPlayers()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		// Get selected player IDs
		string playerName1 = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		string playerName2 = TextWidget.Cast(m_list2.GetElementComponent(m_list2.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
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
		// Setup UI elements
		m_editBox1.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_windowBox1.SetVisible(true);
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_menuButton1.SetVisible(true);
		m_menuButton2.SetVisible(true);
		m_menuButton3.SetVisible(true);
		m_searchButton1.SetVisible(true, false);

		// Setup existing hint text if available
		m_editBox1.SetText(m_clientComponent.m_sHintText);
		
		// Setup button event handlers
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_menuButton1.m_OnClicked.Insert(SendHintAll);
		m_menuButton2.m_OnClicked.Insert(SendHintFaction);
		m_menuButton3.m_OnClicked.Insert(SendHintPlayer);

		// Set button and list text
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Factions");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to All");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Faction");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Player");

		// Populate player list
		PopulatePlayerList(m_list1);
		
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
		
		// Add factions with active players
		foreach (Faction faction : m_factions)
		{
			if (SCR_FactionManager.SGetFactionPlayerCount(faction) > 0)
			{
				m_list2.AddItem(faction.GetFactionName());
				m_selectableFactions.Insert(faction.GetFactionKey());
			}
		}
	}

	/**
	 * Sends hint message to all players
	 */
	void SendHintAll()
	{
		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		CRF_RplToAuthorityManager.GetInstance().SendHint(data);
	}

	/**
	 * Sends hint message to players in selected faction
	 */
	void SendHintFaction()
	{
		if (m_list2.GetSelectedItem() == -1)
			return;

		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		string factionKey = m_selectableFactions.Get(m_list2.GetSelectedItem());
		CRF_RplToAuthorityManager.GetInstance().SendHint(data, -1, factionKey);
	}

	/**
	 * Sends hint message to selected player
	 */
	void SendHintPlayer()
	{
		if (m_list1.GetSelectedItem() == -1)
			return;

		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		
		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
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
		// Setup UI elements
		m_list1Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_menuButton1.SetVisible(true);
		m_searchButton1.SetVisible(true, false);
		m_editbox2.SetVisible(true);
		
		// Setup button event handlers
		m_actionButton.m_OnClicked.Insert(HealPlayer);
		m_menuButton1.m_OnClicked.Insert(HealPlayerVehicle);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		
		// Set button and list text
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Repair Vehicle");
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Heal Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");

		// Populate player list
		PopulatePlayerList(m_list1);
	}
	
	/**
	 * Heals the selected player
	 */
	void HealPlayer()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
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
		if (m_list1.GetSelectedItem() < 0)
			return;

		// Get selected player ID
		string playerName = TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText();
		int playerId = GetplayerIdFromName(playerName);
		if (playerId == 0)
			return;

		// Heal player and vehicle
		CRF_RplToAuthorityManager.GetInstance().Heal(playerId, true, true);
	}
	
	//-----------------------------------------------------------------------------
	// Search Methods
	//-----------------------------------------------------------------------------
	
	/**
	 * Search the first player list
	 */
	void SearchList1()
	{
		SearchPlayerList(m_list1, m_editbox2.GetText());
	}
	
	/**
	 * Search the second player list
	 */
	void SearchList2()
	{
		SearchPlayerList(m_list2, m_editbox3.GetText());
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
			
			if (!m_groupManagerComponent.GetPlayerGroup(playerId))
				continue;
				
			if (CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId)))
				continue;
				
			list.AddItem(string.Format("%1", name));
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

		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
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
}
