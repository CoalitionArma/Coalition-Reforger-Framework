modded enum ChimeraMenuPreset 
{ 
  CoalAdminMenu
}

class CRF_AdminMenu: ChimeraMenuBase 
{
	protected CRF_ClientAdminMenuComponent m_clientAdminMenuComponent;
	protected InputManager m_InputManager;
	protected bool m_bFocused = true;
	protected Widget m_wRoot;
	protected FrameWidget m_adminMenuRoot;
	protected FrameWidget m_gearResetMenuRoot;
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	protected CRF_AdminMenuGameComponent m_adminMenuComponent;
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	protected MultilineEditBoxWidget m_editBox1;
	protected WindowWidget m_windowBox1;
	protected SCR_ButtonTextComponent m_respawnMenuButton;
	protected SCR_ButtonTextComponent m_resetGearMenuButton;
	protected SCR_ButtonTextComponent m_teleportMenuButton;
	protected SCR_ButtonTextComponent m_hintMenuButton;
	protected SCR_ButtonTextComponent m_actionButton;
	protected SCR_ButtonTextComponent m_menuButton1;
	protected SCR_ButtonTextComponent m_menuButton2;
	protected SCR_ButtonTextComponent m_menuButton3;
	protected SCR_ButtonTextComponent m_menuButton4;
	protected TextWidget m_respawnMenuText;
	protected TextWidget m_resetGearMenuText;
	protected TextWidget m_teleportMenuText;
	protected TextWidget m_hintMenuText;
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<vector> m_spawnPoints = {};
	protected ref array<Faction> m_factions = {};
	protected ref array<string> m_selectableFactions = {};
	
	//-------------------------------------------------------------------------------------------
	//-------------------------------General UI Members------------------------------------------
	//-------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_adminMenuComponent = CRF_AdminMenuGameComponent.GetInstance();
		m_clientAdminMenuComponent = CRF_ClientAdminMenuComponent.GetInstance();
		
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
		m_menuButton1 = SCR_ButtonTextComponent.GetButtonText("MenuButton1", m_adminMenuRoot);
		m_menuButton2 = SCR_ButtonTextComponent.GetButtonText("MenuButton2", m_adminMenuRoot);
		m_menuButton3 = SCR_ButtonTextComponent.GetButtonText("MenuButton3", m_adminMenuRoot);
		m_menuButton4 = SCR_ButtonTextComponent.GetButtonText("MenuButton4", m_adminMenuRoot);
		m_editBox1 = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("EditBox1"));
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
	}
	
	override void OnMenuClose()
	{
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
	}
	
	void RespawnButton()
	{
		m_respawnMenuText.SetColor(Color.FromInt(0xffffffff));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeRespawnMenu();
	}
	
	void ResetGearButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromInt(0xffffffff));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeGearMenu();
	}
	
	void TeleportButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromInt(0xffffffff));
		m_hintMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeTeleportMenu();
	}
	
	void HintButton()
	{
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_teleportMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		m_hintMenuText.SetColor(Color.FromInt(0xffffffff));
		ClearMenu();
		InitializeHintMenu();
	}
	
	void ClearMenu()
	{
		m_list1Root.SetVisible(false);
		m_list2Root.SetVisible(false);
		m_list3Root.SetVisible(false);
		m_list4Root.SetVisible(false);
		m_editBox1.SetVisible(false);
		m_windowBox1.SetVisible(false);
		m_actionButton.SetVisible(false, false);
		m_actionButton.m_OnClicked.Clear();
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
		
		m_allPlayers.Clear();
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
	
	void AddRoles(SCR_ListBoxComponent list)
	{
		list.AddItem("COY");
		list.AddItem("MO");
		list.AddItem("FO");
		list.AddItem("JTAC");
		list.AddItem("PL");
		list.AddItem("Medic");
		list.AddItem("SL");
		list.AddItem("RTO");
		list.AddItem("TL");
		list.AddItem("AR");
		list.AddItem("AAR");
		list.AddItem("AT");
		list.AddItem("AAT");
		list.AddItem("Gren");
		list.AddItem("Demo");
		list.AddItem("Rifleman");
		list.AddItem("MMG");
		list.AddItem("AMMG");
		list.AddItem("HMG");
		list.AddItem("AHMG");
		list.AddItem("Sniper");
		list.AddItem("Spotter");
		list.AddItem("MAT");
		list.AddItem("AMAT");
		list.AddItem("HAT");
		list.AddItem("AHAT");
		list.AddItem("AA");
		list.AddItem("AAA");
		list.AddItem("VehLead");
		list.AddItem("VehDriver");
		list.AddItem("VehGunner");
		list.AddItem("VehLoader");
		list.AddItem("IndirectLead");	
		list.AddItem("IndirectGunner");
		list.AddItem("IndirectLoader");
		list.AddItem("Pilot");
		list.AddItem("CrewChief");
		list.AddItem("LogiLead");
		list.AddItem("LogiRunner");
	}
	
	string GetPrefab(int groupID, string role)
	{
		string factionKey = m_groupManagerComponent.FindGroup(groupID).GetFaction().GetFactionKey();
		
		if (factionKey != "BLUFOR" && factionKey != "OPFOR" && factionKey != "INDFOR" && factionKey != "CIV")
		{
			PrintFormat("CRF_ADMINMENU - ERROR | FACTION KEY: %1 DOES NOT MATCH ANY FACTION KEY IN GEARSCRIPT", factionKey);
			return "";
		}
		
		return "Prefabs/Characters/Factions/" + factionKey + "/CRF_GS_" + factionKey + "_" + role + "_P.et";
	}
	
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
	
	//-------------------------------------------------------------------------------------------
	//-------------------------------Gear Menu UI Members----------------------------------------
	//-------------------------------------------------------------------------------------------
	void InitializeGearMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_menuButton3.SetVisible(true, false);
		m_actionButton.m_OnClicked.Insert(ResetGear);
		m_menuButton1.m_OnClicked.Insert(AddLeaderRadio);
		m_menuButton2.m_OnClicked.Insert(AddGIRadio);
		m_menuButton3.m_OnClicked.Insert(AddBinos);
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Reset Gear");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Leaders Radio");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add GI Radio");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Add Binos");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Roles");
		
		
		m_playerManager.GetPlayers(m_allPlayers);
		foreach(int playerID : m_allPlayers)
		{ 
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list1.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				
			}
		}
		AddRoles(m_list2);
	}
	
	void AddLeaderRadio()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		int playerID = m_allPlayers.Get(m_list1.GetSelectedItem());
		m_clientAdminMenuComponent.AddItem(playerID, GetLeadersRadio(playerID));
	}
	
	void AddGIRadio()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		int playerID = m_allPlayers.Get(m_list1.GetSelectedItem());
		m_clientAdminMenuComponent.AddItem(playerID, GetGIRadio(playerID));
	}
	
	void AddBinos()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		int playerID = m_allPlayers.Get(m_list1.GetSelectedItem());
		m_clientAdminMenuComponent.AddItem(playerID, GetBinos(playerID));
	}
	
	string GetLeadersRadio(int playerID)
	{
		string factionKey = m_groupManagerComponent.GetPlayerGroup(playerID).GetFaction().GetFactionKey();
		
		return CRF_GearScriptGamemodeComponent.GetInstance().GetGearScriptSettings(factionKey).m_rLeadershipRadiosPrefab;
	}
	
	string GetGIRadio(int playerID)
	{
		string factionKey = m_groupManagerComponent.GetPlayerGroup(playerID).GetFaction().GetFactionKey();
		
		return CRF_GearScriptGamemodeComponent.GetInstance().GetGearScriptSettings(factionKey).m_rGIRadiosPrefab;
	}
	
	string GetBinos(int playerID)
	{
		string factionKey = m_groupManagerComponent.GetPlayerGroup(playerID).GetFaction().GetFactionKey();
		
		CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(CRF_GearScriptGamemodeComponent.GetInstance().GetGearScriptResource(factionKey)).GetResource().ToBaseContainer()));
		
		return gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab;
	}

	
	void ResetGear()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		if(m_list2.GetSelectedItem() < 0)
			return;
		
		int playerID = m_allPlayers.Get(m_list1.GetSelectedItem());
		int groupID = m_groupManagerComponent.GetPlayerGroup(playerID).GetGroupID();
		string prefab = GetPrefab(groupID, TextWidget.Cast(m_list2.GetElementComponent(m_list2.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		
		if(prefab.IsEmpty())
			return;
		
		m_clientAdminMenuComponent.ResetGear(playerID, prefab);
	}
	//-------------------------------------------------------------------------------------------
	//-------------------------------Resspawn Menu UI Members------------------------------------
	//-------------------------------------------------------------------------------------------
	void InitializeRespawnMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_list3Root.SetVisible(true);
		m_list4Root.SetVisible(true);
		m_actionButton.SetVisible(true, false);
		m_actionButton.m_OnClicked.Insert(RespawnPlayer);
		m_list1.m_OnChanged.Insert(UpdateSpawnpoint);
		
		TextWidget.Cast(m_actionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Respawn Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Groups");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Dead Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("Roles");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List4Text")).SetText("Spawnpoints");
		
		m_playerManager.GetPlayers(m_allPlayers);
		m_groupManagerComponent.GetAllPlayableGroups(m_outGroups);
		foreach(int playerID : m_allPlayers)
		{ 
			if(!m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list2.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				
			}
		}
		int rowNumber = 0;
		foreach(SCR_AIGroup group : m_outGroups)
		{
			string factionTag;
			if(group.GetFaction().GetFactionKey() == "BLUFOR")
				factionTag = "BLU";
			if(group.GetFaction().GetFactionKey() == "OPFOR")
				factionTag = "OPF";
			if(group.GetFaction().GetFactionKey() == "INDFOR")
				factionTag = "IND";
			if(group.GetFaction().GetFactionKey() == "CIV")
				factionTag = "CIV";
			
			m_list1.AddItem(string.Format("%1 | %2", factionTag , group.GetCustomName()));
			m_groupIDList.Insert(group.GetGroupID());
			
		}
		AddRoles(m_list3);
	}
	
	void UpdateSpawnpoint()
	{
		m_list4.Clear();
		m_spawnPoints.Clear();
		int groupID = m_groupIDList.Get(m_list1.GetSelectedItem());
		m_playerManager.GetPlayers(m_allPlayers);
		m_list2.Clear();
		m_allPlayers.Clear();
		m_playerManager.GetPlayers(m_allPlayers);
		foreach(int playerID : m_allPlayers)
		{
			if(!m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list2.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				
			}
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				if(m_groupManagerComponent.GetPlayerGroup(playerID).GetGroupID() == groupID)
				{
					m_list4.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
					m_spawnPoints.Insert(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID).GetOrigin());		
				}
			}
		}
	}
	
	void RespawnPlayer()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		if(m_list2.GetSelectedItem() < 0)
			return;
		
		if(m_list3.GetSelectedItem() < 0)
			return;
		
		if(m_list4.GetSelectedItem() < 0)
			return;
		
		int playerID = m_allPlayers.Get(m_list2.GetSelectedItem());
		int groupID = m_groupIDList.Get(m_list1.GetSelectedItem());
		string prefab = GetPrefab(groupID, TextWidget.Cast(m_list3.GetElementComponent(m_list3.GetSelectedItem()).GetRootWidget().FindAnyWidget("Text")).GetText());
		vector spawnpoint = m_spawnPoints.Get(m_list4.GetSelectedItem());
		m_clientAdminMenuComponent.SpawnGroup(playerID, prefab, spawnpoint , groupID);
		ClearMenu();
		InitializeRespawnMenu();
	}
	//-------------------------------------------------------------------------------------------
	//-------------------------------Teleport Menu UI Members------------------------------------
	//-------------------------------------------------------------------------------------------
	void InitializeTeleportMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_menuButton1.SetVisible(true, false);
		m_menuButton2.SetVisible(true, false);
		m_menuButton1.m_OnClicked.Insert(TeleportLocal);
		m_menuButton2.m_OnClicked.Insert(TeleportPlayers);
		
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport to Player 1");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Teleport Player 1 to Player 2");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Player 1");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Player 2");
		
		m_playerManager.GetPlayers(m_allPlayers);
		foreach(int playerID : m_allPlayers)
		{ 
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list1.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				m_list2.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				
			}
		}
	}
	
	void TeleportLocal()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		int playerID2 = m_allPlayers.Get(m_list1.GetSelectedItem());
		
		if(!playerID2)
			return;
		
		m_clientAdminMenuComponent.TeleportLocalPlayer(SCR_PlayerController.GetLocalPlayerId(), playerID2);
	}
	
	void TeleportPlayers()
	{
		if(m_list1.GetSelectedItem() < 0)
			return;
		
		if(m_list2.GetSelectedItem() < 0)
			return;
		
		int playerID1 = m_allPlayers.Get(m_list1.GetSelectedItem());
		int playerID2 = m_allPlayers.Get(m_list2.GetSelectedItem());
		
		if(!playerID1)
			return;
		
		if(!playerID2)
			return;
		
		m_clientAdminMenuComponent.TeleportPlayers(playerID1, playerID2);
	}
	//-------------------------------------------------------------------------------------------
	//-------------------------------Hint Menu UI Members----------------------------------------
	//-------------------------------------------------------------------------------------------
	
	void InitializeHintMenu()
	{
		m_editBox1.SetVisible(true);
		m_windowBox1.SetVisible(true);
		m_editBox1.SetText(m_clientAdminMenuComponent.m_sHintText);
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_menuButton1.SetVisible(true);
		m_menuButton2.SetVisible(true);
		m_menuButton3.SetVisible(true);
		
		m_menuButton1.m_OnClicked.Insert(SendHintAll);
		m_menuButton2.m_OnClicked.Insert(SendHintFaction);
		m_menuButton3.m_OnClicked.Insert(SendHintPlayer);
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Factions");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_menuButton1.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to All");
		TextWidget.Cast(m_menuButton2.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Faction");
		TextWidget.Cast(m_menuButton3.GetRootWidget().FindWidget("MenuButtonText")).SetText("Send to Player");
		
		m_playerManager.GetPlayers(m_allPlayers);
		foreach(int playerID : m_allPlayers)
		{ 
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list1.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
			}
		}
	
		GetGame().GetFactionManager().GetFactionsList(m_factions);
		foreach(Faction faction : m_factions)
		{
			if(SCR_FactionManager.SGetFactionPlayerCount(faction) > 0)
			{
				m_list2.AddItem(faction.GetFactionName());
				m_selectableFactions.Insert(faction.GetFactionKey());
			}
		}
	}
	
	void SendHintAll()
	{
		string data = m_editBox1.GetText();
		m_clientAdminMenuComponent.m_sHintText = data;
		m_clientAdminMenuComponent.SendHintAll(data);
	}
	
	void SendHintFaction()
	{
		if(m_list2.GetSelectedItem() == -1)
			return;
		
		string data = m_editBox1.GetText();
		m_clientAdminMenuComponent.m_sHintText = data;
		string factionKey = m_selectableFactions.Get(m_list2.GetSelectedItem());
		m_clientAdminMenuComponent.SendHintFaction(data, factionKey);
	}
	
	void SendHintPlayer()
	{
		if(m_list1.GetSelectedItem() == -1)
			return;
		
		string data = m_editBox1.GetText();
		m_clientAdminMenuComponent.m_sHintText = data;
		int playerID = m_allPlayers.Get(m_list1.GetSelectedItem());
		m_clientAdminMenuComponent.SendHintPlayer(data, playerID);
	}
}