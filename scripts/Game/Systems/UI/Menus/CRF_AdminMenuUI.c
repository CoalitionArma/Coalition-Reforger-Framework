modded enum ChimeraMenuPreset
{
	CoalAdminMenu
}

class CRF_AdminMenu : ChimeraMenuBase
{
	protected CRF_ClientComponent m_clientComponent;
	protected InputManager m_InputManager;
	protected SCR_ChatPanel m_ChatPanel;
	protected bool m_bFocused = true;
	protected Widget m_wRoot;
	protected FrameWidget m_adminMenuRoot;
	protected FrameWidget m_gearResetMenuRoot;
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	protected MultilineEditBoxWidget m_editBox1;
	protected EditBoxWidget m_editbox2;
	protected EditBoxWidget m_editbox3;
	protected WindowWidget m_windowBox1;
	protected SCR_ButtonTextComponent m_respawnMenuButton;
	protected SCR_ButtonTextComponent m_resetGearMenuButton;
	protected SCR_ButtonTextComponent m_teleportMenuButton;
	protected SCR_ButtonTextComponent m_hintMenuButton;
	protected SCR_ButtonTextComponent m_healMenuButton;
	protected SCR_ButtonTextComponent m_actionButton;
	protected SCR_ButtonTextComponent m_searchButton1;
	protected SCR_ButtonTextComponent m_searchButton2;
	protected SCR_ButtonTextComponent m_menuButton1;
	protected SCR_ButtonTextComponent m_menuButton2;
	protected SCR_ButtonTextComponent m_menuButton3;
	protected SCR_ButtonTextComponent m_menuButton4;
	protected TextWidget m_respawnMenuText;
	protected TextWidget m_resetGearMenuText;
	protected TextWidget m_teleportMenuText;
	protected TextWidget m_hintMenuText;
	protected TextWidget m_healMenuText;
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<vector> m_spawnPoints = {};
	protected ref array<Faction> m_factions = {};
	protected ref array<string> m_selectableFactions = {};

	//------------------------------------------------------------------------------------------------
	//-------------------------------General UI Members-----------------------------------------------
	//------------------------------------------------------------------------------------------------

	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_clientComponent = CRF_ClientComponent.GetInstance();

		//Menu Roots
		m_wRoot = GetRootWidget();
		m_adminMenuRoot = FrameWidget.Cast(m_wRoot.FindWidget("AdminMenuTools"));

		//Populate the List Boxes and Buttons
		m_list1Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List1Box"));
		m_list1 = SCR_ListBoxComponent.Cast(m_list1Root.FindHandler(SCR_ListBoxComponent));
		m_list2Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List2Box"));
		m_list2 = SCR_ListBoxComponent.Cast(m_list2Root.FindHandler(SCR_ListBoxComponent));
		m_list3Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List3Box"));
		m_list3 = SCR_ListBoxComponent.Cast(m_list3Root.FindHandler(SCR_ListBoxComponent));
		m_list4Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List4Box"));
		m_list4 = SCR_ListBoxComponent.Cast(m_list4Root.FindHandler(SCR_ListBoxComponent));
		m_actionButton = SCR_ButtonTextComponent.GetButtonText("ActionButton", m_adminMenuRoot);
		m_searchButton1 = SCR_ButtonTextComponent.GetButtonText("SearchButton1", m_adminMenuRoot);
		m_searchButton2 = SCR_ButtonTextComponent.GetButtonText("SearchButton2", m_adminMenuRoot);
		m_menuButton1 = SCR_ButtonTextComponent.GetButtonText("MenuButton1", m_adminMenuRoot);
		m_menuButton2 = SCR_ButtonTextComponent.GetButtonText("MenuButton2", m_adminMenuRoot);
		m_menuButton3 = SCR_ButtonTextComponent.GetButtonText("MenuButton3", m_adminMenuRoot);
		m_menuButton4 = SCR_ButtonTextComponent.GetButtonText("MenuButton4", m_adminMenuRoot);
		m_editBox1 = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox1"));
		m_editbox2 = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox2"));
		m_editbox3 = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox3"));
		m_windowBox1 = WindowWidget.Cast(m_wRoot.FindAnyWidget("Window0"));

		//Initializes the Respawn Menu
		ClearMenu();
		InitializeRespawnMenu();

		m_respawnMenuButton = SCR_ButtonTextComponent.GetButtonText("RespawnButton", m_wRoot);
		m_respawnMenuButton.m_OnClicked.Insert(RespawnButton);
		m_respawnMenuText = TextWidget.Cast(m_respawnMenuButton.GetRootWidget().FindWidget("RespawnText"));

		m_resetGearMenuButton = SCR_ButtonTextComponent.GetButtonText("ResetGearButton", m_wRoot);
		m_resetGearMenuButton.m_OnClicked.Insert(ResetGearButton);
		m_resetGearMenuText = TextWidget.Cast(m_resetGearMenuButton.GetRootWidget().FindWidget("ResetGearText"));

		m_teleportMenuButton = SCR_ButtonTextComponent.GetButtonText("TeleportButton", m_wRoot);
		m_teleportMenuButton.m_OnClicked.Insert(TeleportButton);
		m_teleportMenuText = TextWidget.Cast(m_teleportMenuButton.GetRootWidget().FindWidget("TeleportText"));

		m_hintMenuButton = SCR_ButtonTextComponent.GetButtonText("HintButton", m_wRoot);
		m_hintMenuButton.m_OnClicked.Insert(HintButton);
		m_hintMenuText = TextWidget.Cast(m_hintMenuButton.GetRootWidget().FindWidget("HintText"));

		m_healMenuButton = SCR_ButtonTextComponent.GetButtonText("HealButton", m_wRoot);
		m_healMenuButton.m_OnClicked.Insert(HealButton);
		m_healMenuText = TextWidget.Cast(m_healMenuButton.GetRootWidget().FindWidget("HealText"));

		//Load chat
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));

		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

		m_ChatPanel.SetAlwaysVisible(true);
		m_ChatPanel.ExpandMessageLines(20); // Increase the amount of message liens
		m_ChatPanel.ForceShowFullHistory(); // Load full history
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		m_ChatPanel.SetAlwaysVisible(false)
	}

	//------------------------------------------------------------------------------------------------
	void RespawnButton()
	{
		m_respawnMenuText.SetColor(Color.FromInt(0xffffffff));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_healMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeRespawnMenu();
	}

	//------------------------------------------------------------------------------------------------
	void ResetGearButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromInt(0xffffffff));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_healMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeGearMenu();
	}

	//------------------------------------------------------------------------------------------------
	void TeleportButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromInt(0xffffffff));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_healMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeTeleportMenu();
	}

	//------------------------------------------------------------------------------------------------
	void HintButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromInt(0xffffffff));
		m_healMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeHintMenu();
	}

	//------------------------------------------------------------------------------------------------
	void HealButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_healMenuText.SetColor(Color.FromInt(0xffffffff));
		ClearMenu();
		InitializeHealMenu();
	}

	//------------------------------------------------------------------------------------------------
	void ClearMenu()
	{
		m_list1Root.SetVisible(false);
		m_list2Root.SetVisible(false);
		m_list3Root.SetVisible(false);
		m_list4Root.SetVisible(false);
		m_editBox1.SetVisible(false);
		m_editbox2.SetVisible(false);
		m_editbox3.SetVisible(false);
		m_windowBox1.SetVisible(false);
		m_actionButton.SetVisible(false, false);
		m_actionButton.m_OnClicked.Clear();
		m_searchButton1.SetVisible(false, false);
		m_searchButton1.m_OnClicked.Clear();
		m_searchButton2.SetVisible(false, false);
		m_searchButton2.m_OnClicked.Clear();
		m_menuButton1.SetVisible(false, false);
		m_menuButton2.SetVisible(false, false);
		m_menuButton3.SetVisible(false, false);
		m_menuButton4.SetVisible(false, false);
		m_menuButton1.m_OnClicked.Clear();
		m_menuButton2.m_OnClicked.Clear();
		m_menuButton3.m_OnClicked.Clear();
		m_menuButton4.m_OnClicked.Clear();
		m_list1.Clear();
		m_list2.Clear();
		m_list3.Clear();
		m_list4.Clear();
		m_list1.m_OnChanged.Clear();
		m_list2.m_OnChanged.Clear();
		m_list3.m_OnChanged.Clear();
		m_list4.m_OnChanged.Clear();
		m_editBox1.SetText("");

		m_outGroups.Clear();
		m_spawnPoints.Clear();
		m_groupIDList.Clear();
		m_allPlayers.Clear();
		m_factions.Clear();
		m_selectableFactions.Clear();

		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List4Text")).SetText("");
	}

	//------------------------------------------------------------------------------------------------
	void AddRoles(SCR_ListBoxComponent list)
	{
		array<string> roleNames = {};
		SCR_Enum.GetEnumNames(EGearRole, roleNames);
		foreach (string role : roleNames)
		{
			list.AddItem(role);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int GetPlayerIdFromName(string name)
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

	//------------------------------------------------------------------------------------------------
	ResourceName GetPrefab(int groupID, int index)
	{
		string factionKey = m_groupManagerComponent.FindGroup(groupID).GetFaction().GetFactionKey();

		ResourceName prefab = CRF_RoleHelper.RoleToResource(index, factionKey);

		return prefab;
	}

	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
	//-------------------------------Gear Menu UI Members---------------------------------------------
	//------------------------------------------------------------------------------------------------

	void InitializeGearMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_menuButton3.SetVisible(true, false);
		m_editbox2.SetVisible(true);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_actionButton.m_OnClicked.Insert(ResetGear);
		m_menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		m_menuButton2.m_OnClicked.Insert(AddGIRadio);
		m_menuButton3.m_OnClicked.Insert(AddBinos);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Reset Gear");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Leaders Radio");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add GI Radio");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Binos");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Roles");
		m_list1.m_OnChanged.Insert(UpdateDefaultGear);

		m_playerManager.GetPlayers(m_allPlayers);

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId) && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				m_list1.AddItem(string.Format("%1", name));
			}
		}

		AddRoles(m_list2);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateDefaultGear()
	{
		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());

		for (int i = 0; i < m_list2.GetItemCount(); i++)
		{
			if (CRF_GamemodeComponent.GetInstance().ReturnPlayerGearScriptsMapValue(playerId, "GSR").Contains(CRF_RoleHelper.RoleToString(i))) // GSR = Gear Script Resource
			{
				m_list2.SetItemSelected(i, true);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void AddLeaderRadio()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		m_clientComponent.AddItem(playerId, CRF_GamemodeComponent.GetInstance().GetGearScriptSettings(m_groupManagerComponent.GetPlayerGroup(playerId).GetFaction().GetFactionKey()).m_rLeadershipRadiosPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	void AddGIRadio()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		m_clientComponent.AddItem(playerId, CRF_GamemodeComponent.GetInstance().GetGearScriptSettings(m_groupManagerComponent.GetPlayerGroup(playerId).GetFaction().GetFactionKey()).m_rGIRadiosPrefab, true);
	}

	//------------------------------------------------------------------------------------------------
	void AddBinos()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		m_clientComponent.AddItem(playerId, GetBinos(playerId), true);
	}

	//------------------------------------------------------------------------------------------------
	string GetBinos(int playerId)
	{
		string factionKey = m_groupManagerComponent.GetPlayerGroup(playerId).GetFaction().GetFactionKey();

		CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(CRF_GamemodeComponent.GetInstance().GetGearScriptResource(factionKey)).GetResource().ToBaseContainer()));

		return gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab;
	}

	//------------------------------------------------------------------------------------------------
	void ResetGear()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		int groupID = m_groupManagerComponent.GetPlayerGroup(playerId).GetGroupID();
		ResourceName prefab = GetPrefab(groupID, m_list2.GetSelectedItem());

		if (prefab.IsEmpty())
			return;

		m_clientComponent.ResetGear(playerId, prefab, true);
	}

	//------------------------------------------------------------------------------------------------
	//-------------------------------Resspawn Menu UI Members-----------------------------------------
	//------------------------------------------------------------------------------------------------

	void InitializeRespawnMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_list3Root.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_actionButton.m_OnClicked.Insert(RespawnPlayer);
		m_list1.m_OnChanged.Insert(UpdateSpawnGroupRequest);
		m_list2.m_OnChanged.Insert(UpdateSpawnpoint);

		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Respawn Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Dead Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Groups");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("Spawnpoints");

		m_playerManager.GetPlayers(m_allPlayers);
		m_groupManagerComponent.GetAllPlayableGroups(m_outGroups);

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);

			if (SCR_FactionManager.SGetPlayerFaction(playerId).GetFactionKey() == "SPEC" || GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				m_list1.AddItem(string.Format("%1", name));
			}
		}

		foreach (SCR_AIGroup group : m_outGroups)
		{
			string factionTag = group.GetFaction().GetFactionKey().Substring(0, 3);

			if (factionTag.IsEmpty())
				continue;

			if (group.GetFaction().GetFactionKey() == "SPEC")
				continue;

			m_list2.AddItem(string.Format("%1 | %2", factionTag, group.GetCustomNameWithOriginal()));
			m_groupIDList.Insert(group.GetGroupID());
		}
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSpawnGroupRequest()
	{
		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());

		CRF_ClientComponent.GetInstance().RequestGroupIdFromServer(playerId, SCR_PlayerController.GetLocalPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSpawnGroup(int groupId)
	{
		foreach (int i, SCR_AIGroup group : m_outGroups)
		{
			if (groupId == group.GetGroupID())
			{
				if (RplSession.Mode() == RplMode.Client)
					i = i - 1;

				m_list2.SetItemSelected(i, true);
				return;
			}
		};
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSpawnpoint()
	{
		m_list3.Clear();
		m_spawnPoints.Clear();
		int groupID = m_groupIDList.Get(m_list2.GetSelectedItem());

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId))
			{
				if (m_groupManagerComponent.GetPlayerGroup(playerId).GetGroupID() == groupID)
				{
					m_list3.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerId)));
					m_spawnPoints.Insert(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetOrigin());
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		if (m_list3.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		int groupID = m_groupIDList.Get(m_list2.GetSelectedItem());
		vector spawnpoint = m_spawnPoints.Get(m_list3.GetSelectedItem());
		m_clientComponent.SpawnOnGroup(playerId, spawnpoint, groupID, true);

		GetGame().GetCallqueue().CallLater(ClearMenu, 1250, false);
		GetGame().GetCallqueue().CallLater(InitializeRespawnMenu, 1825, false);
	}

	//------------------------------------------------------------------------------------------------
	//-------------------------------Teleport Menu UI Members-----------------------------------------
	//------------------------------------------------------------------------------------------------

	void InitializeTeleportMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_editbox3.SetVisible(true);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_searchButton1.SetVisible(true, false);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_searchButton2.SetVisible(true, false);
		m_searchButton2.m_OnClicked.Insert(SearchList2);
		m_menuButton1.m_OnClicked.Insert(TeleportLocal);
		m_menuButton2.m_OnClicked.Insert(TeleportPlayers);

		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport to Player 1");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport Player 1 to Player 2");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Player 1");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Player 2");

		m_playerManager.GetPlayers(m_allPlayers);

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId) && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				m_list1.AddItem(string.Format("%1", name));
				m_list2.AddItem(string.Format("%1", name));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void TeleportLocal()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId2 = m_allPlayers.Get(m_list1.GetSelectedItem());

		if (!playerId2)
			return;

		m_clientComponent.TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerId2);
	}

	//------------------------------------------------------------------------------------------------
	void TeleportPlayers()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		if (m_list2.GetSelectedItem() < 0)
			return;

		int playerId1 = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		int playerId2 = GetPlayerIdFromName(TextWidget.Cast(m_list2.GetElementComponent(m_list2.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());

		if (playerId1 == 0)
			return;

		if (playerId2 == 0)
			return;

		m_clientComponent.TeleportPlayers(playerId1, playerId2, true);
	}

	//------------------------------------------------------------------------------------------------
	//-------------------------------Hint Menu UI Members---------------------------------------------
	//------------------------------------------------------------------------------------------------

	void InitializeHintMenu()
	{
		m_editBox1.SetVisible(true);
		m_editbox2.SetVisible(true);
		m_windowBox1.SetVisible(true);
		m_editBox1.SetText(m_clientComponent.m_sHintText);
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_menuButton1.SetVisible(true);
		m_menuButton2.SetVisible(true);
		m_menuButton3.SetVisible(true);
		m_searchButton1.SetVisible(true, false);

		m_searchButton1.m_OnClicked.Insert(SearchList1);
		m_menuButton1.m_OnClicked.Insert(SendHintAll);
		m_menuButton2.m_OnClicked.Insert(SendHintFaction);
		m_menuButton3.m_OnClicked.Insert(SendHintPlayer);

		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Factions");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to All");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Faction");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Player");

		m_playerManager.GetPlayers(m_allPlayers);

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
		{
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		}

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId) && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				m_list1.AddItem(string.Format("%1", name));
			}
		}

		GetGame().GetFactionManager().GetFactionsList(m_factions);
		foreach (Faction faction : m_factions)
		{
			if (SCR_FactionManager.SGetFactionPlayerCount(faction) > 0)
			{
				m_list2.AddItem(faction.GetFactionName());
				m_selectableFactions.Insert(faction.GetFactionKey());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void SendHintAll()
	{
		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		m_clientComponent.SendHintAll(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintFaction()
	{
		if (m_list2.GetSelectedItem() == -1)
			return;

		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		string factionKey = m_selectableFactions.Get(m_list2.GetSelectedItem());
		m_clientComponent.SendHintFaction(data, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintPlayer()
	{
		if (m_list1.GetSelectedItem() == -1)
			return;

		string data = m_editBox1.GetText();
		m_clientComponent.m_sHintText = data;
		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		m_clientComponent.SendHintPlayer(data, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//-------------------------------Heal Menu UI Members---------------------------------------------
	//------------------------------------------------------------------------------------------------
	
	void InitializeHealMenu()
	{
		m_list1Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_menuButton1.SetVisible(true);
		m_searchButton1.SetVisible(true, false);
		m_editbox2.SetVisible(true);
		m_actionButton.m_OnClicked.Insert(HealPlayer);
		m_menuButton1.m_OnClicked.Insert(HealPlayerVehicle);
		m_searchButton1.m_OnClicked.Insert(SearchList1);
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Repair Vehicle");
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Heal Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");

		m_playerManager.GetPlayers(m_allPlayers);

		TStringArray playerNames = {};

		foreach (int playerId : m_allPlayers)
			playerNames.Insert(m_playerManager.GetPlayerName(playerId));

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId) && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
				m_list1.AddItem(string.Format("%1", name));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void HealPlayer()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());

		m_clientComponent.HealPlayer(playerId, true);
	}
	
	//------------------------------------------------------------------------------------------------
	void HealPlayerVehicle()
	{
		if (m_list1.GetSelectedItem() < 0)
			return;

		int playerId = GetPlayerIdFromName(TextWidget.Cast(m_list1.GetElementComponent(m_list1.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());

		m_clientComponent.HealPlayerVehicle(playerId, true);
	}
	
	//------------------------------------------------------------------------------------------------
	void SearchList1()
	{
		SearchPlayerList(m_list1, m_editbox2.GetText())
	}
	
	//------------------------------------------------------------------------------------------------
	void SearchList2()
	{
		SearchPlayerList(m_list2, m_editbox3.GetText());
	}
	
	//------------------------------------------------------------------------------------------------
	void SearchPlayerList(SCR_ListBoxComponent list, string searchData)
	{
		TStringArray playerNames = {};
		m_playerManager.GetPlayers(m_allPlayers);
		list.Clear();

		if (searchData == "")
		{
			foreach (int playerId : m_allPlayers)
				playerNames.Insert(m_playerManager.GetPlayerName(playerId));
		} else {
			foreach (int playerId : m_allPlayers)
			{
				string playerName = m_playerManager.GetPlayerName(playerId);
				playerName.ToLower();
				searchData.ToLower();

				if (playerName.Contains(searchData))
					playerNames.Insert(m_playerManager.GetPlayerName(playerId));
			}
		}

		playerNames.Sort(false);

		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (m_groupManagerComponent.GetPlayerGroup(playerId) && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				list.AddItem(string.Format("%1", name));
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;

		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	//------------------------------------------------------------------------------------------------
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
}
