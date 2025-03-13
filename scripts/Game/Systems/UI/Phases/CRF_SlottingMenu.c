modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_SlottingMenu
}

class CRF_SlottingMenuUI: ChimeraMenuBase
{
	protected Widget m_wRoot;
	protected ImageWidget m_wPreview;
	protected ImageWidget m_wSlotting;
	protected ImageWidget m_wGame;
	protected ImageWidget m_wAAR;
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent;
	protected SCR_ListBoxComponent m_cUnslotPlayerListBoxComponent;
	protected CRF_ListboxComponent m_cSlotListBoxComponent;
	protected CRF_ListboxComponent m_cOrbatListBoxComponent;
	protected CRF_Gamemode m_Gamemode;
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_ButtonTextComponent m_wAdvanceButton;
	protected SCR_ButtonTextComponent m_wPreviewButton;
	protected int localSlotChanges = 0;
	protected int m_iBluforSlots = 0;
	protected int m_iOpforSlots = 0;
	protected int m_iIndforSlots = 0;
	protected int m_iCivSlots = 0;
	protected Faction m_fSelectedFaction;
	protected int m_iSelectedPlayerID = 0;
	protected int m_LocalSlottingState;
	
	ResourceName m_rBluforIcon;
	ResourceName m_rOpforIcon;
	ResourceName m_rIndforIcon;
	ResourceName m_rCivIcon;

	override void OnMenuOpen()
	{	
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);

		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		m_wRoot = GetRootWidget();
		m_Gamemode = CRF_Gamemode.Cast(GetGame().GetGameMode());
		
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		m_LocalSlottingState = m_Gamemode.m_SlottingState;
		
		TextWidget missionText = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionText"));
		if(GetGame().GetMissionName())
			missionText.SetText(GetGame().GetMissionName());
		else
			missionText.SetText("Unknown Mission");
		
		if(SCR_MissionHeader.Cast(GetGame().GetMissionHeader()))
			missionText.SetText(missionText.GetText() + " | By " + SCR_MissionHeader.Cast(GetGame().GetMissionHeader()).m_sAuthor);
		else
			missionText.SetText(missionText.GetText() + " | By " + "Unkown");
		
		string currentStateName = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetCurrentWeatherState().GetStateName();
		TextWidget.Cast(m_wRoot.FindAnyWidget("WeatherText")).SetText("Weather: " + currentStateName);
		
		m_wPreview = ImageWidget.Cast(m_wRoot.FindAnyWidget("PreviewBorder"));
		m_wSlotting = ImageWidget.Cast(m_wRoot.FindAnyWidget("SlottingBorder"));
		m_wGame = ImageWidget.Cast(m_wRoot.FindAnyWidget("GameBorder"));
		m_wAAR = ImageWidget.Cast(m_wRoot.FindAnyWidget("AARBorder"));
		
		int gameState = CRF_Gamemode.Cast(GetGame().GetGameMode()).m_GamemodeState; 
		switch(gameState)
		{
			case 0: {m_wPreview.SetColor(Color.FromRGBA(122, 0, 0, 255));	 break;}
			case 1: {m_wSlotting.SetColor(Color.FromRGBA(122, 0, 0, 255));	 break;}
			case 2: {m_wGame.SetColor(Color.FromRGBA(122, 0, 0, 255)); 	 	 break;}
			case 3: {m_wAAR.SetColor(Color.FromRGBA(122, 0, 0, 255)); 		 break;}
		}
		
		ButtonWidget previewButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewButton"));
		ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
		ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
		ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
		aarButton.SetEnabled(false);
		advanceButton.SetEnabled(false);
		gameButton.SetEnabled(false);
		m_wRoot.FindAnyWidget("UnslottedPlayers").SetOpacity(0);
		m_wRoot.FindAnyWidget("SlottingPhases").SetOpacity(0);
		FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(0);
		
		if(m_Gamemode.m_GamemodeState == CRF_GamemodeState.GAME)
			gameButton.SetEnabled(true);
		
		SCR_ButtonTextComponent.Cast(gameButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(EnterGame);
		SCR_ButtonTextComponent.Cast(previewButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(OpenSlottingMenu);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlotPhaseButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceSlottingPhase);
		SCR_ButtonTextComponent.Cast(advanceButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceMenu);
		
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerList")).FindHandler(SCR_ListBoxComponent));
		m_cOrbatListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("OrbatList")).FindHandler(CRF_ListboxComponent));
		m_cUnslotPlayerListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerList")).FindHandler(CRF_ListboxComponent));

		m_cSlotListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleList")).FindHandler(CRF_ListboxComponent));
		InitSlots();
		
		CRF_GamemodeComponent gamemodeComponent = CRF_GamemodeComponent.GetInstance();	
		ResourceName gearScriptResource;
		CRF_GearScriptConfig gearConfig;
	
		if(m_iBluforSlots > 0)
		{
			if(gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("BLUFOR");
				if(!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if(gearConfig)
					{
						if(!gearConfig.m_FactionIcon.IsEmpty())
							m_rBluforIcon = gearConfig.m_FactionIcon;
					};
				};
			};
			
			if(m_rBluforIcon.IsEmpty())
				m_rBluforIcon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("BLUFOR")).GetFactionFlag();
			
			m_wRoot.FindAnyWidget("BluforButton").SetVisible(true);
			m_wRoot.FindAnyWidget("BluforBGSelect").SetVisible(true);
				
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagBlufor")).LoadImageTexture(0, m_rBluforIcon);
			m_wRoot.FindAnyWidget("BluforBGSelect").SetColor(Color.FromRGBA(34, 196, 244, 33));
		};
		
		if(m_iOpforSlots > 0)
		{
			if(gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("OPFOR");
				if(!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if(gearConfig)
					{
						if(!gearConfig.m_FactionIcon.IsEmpty())
							m_rOpforIcon = gearConfig.m_FactionIcon;
					};
				};
			};
			
			if(m_rOpforIcon.IsEmpty())
				m_rOpforIcon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("OPFOR")).GetFactionFlag();
			
			m_wRoot.FindAnyWidget("OpforButton").SetVisible(true);
			m_wRoot.FindAnyWidget("OpforBGSelect").SetVisible(true);	
			
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagOpfor")).LoadImageTexture(0, m_rOpforIcon);
			m_wRoot.FindAnyWidget("OpforBGSelect").SetColor(Color.FromRGBA(238, 49, 47, 33));
		};
		
		if(m_iIndforSlots > 0)
		{
			if(gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("INDFOR");
				if(!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if(gearConfig)
					{
						if(!gearConfig.m_FactionIcon.IsEmpty())
							m_rIndforIcon = gearConfig.m_FactionIcon;
					};
				};
			};
			
			if(m_rIndforIcon.IsEmpty())
				m_rIndforIcon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("INDFOR")).GetFactionFlag();
			
			m_wRoot.FindAnyWidget("IndforButton").SetVisible(true);
			m_wRoot.FindAnyWidget("IndforBGSelect").SetVisible(true);	
				
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagIndfor")).LoadImageTexture(0, m_rIndforIcon);
			m_wRoot.FindAnyWidget("IndforBGSelect").SetColor(Color.FromRGBA(0, 177, 79, 33));
		};
		
		if(m_iCivSlots > 0)
		{
			if(gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("CIV");
				if(!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if(gearConfig)
					{
						if(!gearConfig.m_FactionIcon.IsEmpty())
							m_rCivIcon = gearConfig.m_FactionIcon;
					};
				};
			};
			
			if(m_rCivIcon.IsEmpty())
				m_rCivIcon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("CIV")).GetFactionFlag();
			
			m_wRoot.FindAnyWidget("CivButton").SetVisible(true);
			m_wRoot.FindAnyWidget("CivBGSelect").SetVisible(true);	
				
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagCiv")).LoadImageTexture(0, m_rCivIcon);
			m_wRoot.FindAnyWidget("CivBGSelect").SetColor(Color.FromRGBA(168, 110, 207, 33));
		};	
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		if (gamemode.m_iFactionOneRatio > 0 && !gamemode.m_sFactionOneKey.IsEmpty())
		{
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).SetText(gamemode.m_iFactionOneRatio.ToString());
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Text")).SetText(gamemode.m_sFactionOneKey);
		
			Color colorOne;
			
			switch(gamemode.m_sFactionOneKey)
			{
				case "BLU" : colorOne = Color.FromRGBA(0, 20, 255, 255); break;
				case "OPF" : colorOne = Color.FromRGBA(188, 0, 0, 255); break;
				case "IND" : colorOne = Color.FromRGBA(0, 145, 43, 255); break;
				case "CIV" : colorOne = Color.FromRGBA(137, 0, 188, 255); break;
			}
			
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Image")).SetColor(colorOne);
		}
		
		if (gamemode.m_iFactionTwoRatio > 0 && !gamemode.m_sFactionTwoKey.IsEmpty())
		{
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).SetText(gamemode.m_iFactionTwoRatio.ToString());
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Text")).SetText(gamemode.m_sFactionTwoKey);
		
			Color colorTwo;
			
			switch(gamemode.m_sFactionTwoKey)
			{
				case "BLU" : colorTwo = Color.FromRGBA(0, 20, 255, 255); break;
				case "OPF" : colorTwo = Color.FromRGBA(188, 0, 0, 255); break;
				case "IND" : colorTwo = Color.FromRGBA(0, 145, 43, 255); break;
				case "CIV" : colorTwo = Color.FromRGBA(137, 0, 188, 255); break;
			}
			
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Image")).SetColor(colorTwo);
		}

		
		if (gamemode.m_iFactionTwoRatio < 0 || gamemode.m_sFactionTwoKey.IsEmpty() || gamemode.m_iFactionOneRatio < 0 || gamemode.m_sFactionOneKey.IsEmpty())
		{
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).SetVisible(false);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Image")).SetVisible(false);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1IntImage")).SetVisible(false);
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Text")).SetVisible(false);
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).SetVisible(false);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Image")).SetVisible(false);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2IntImage")).SetVisible(false);
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Text")).SetVisible(false);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FinalImage")).SetVisible(false);
			TextWidget.Cast(m_wRoot.FindAnyWidget("Final")).SetVisible(false);
		}
		
		if(m_iBluforSlots > 0)
		{
			m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
			SelectFactionBlufor();
		}
		else if(m_iOpforSlots > 0)
		{
			m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
			SelectFactionOpfor();
		}
		else if(m_iIndforSlots > 0)
		{
			m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
			SelectFactionIndfor();
		}
		else if(m_iCivSlots > 0)
		{
			m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
			SelectFactionCiv();
		}
		localSlotChanges = m_Gamemode.m_iSlotChanges;
		UpdateSlots();
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);			
	}
	
	override void OnMenuClose()
	{
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	void AdvanceSlottingPhase()
	{
		if(m_Gamemode.m_SlottingState == 2)
			return;
		
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).AdvanceSlottingPhase();
	}
	
	void EnterGame()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).EnterGame(GetGame().GetPlayerController().GetPlayerId());
	}
	
	void SelectPlayerDelay()
	{
		GetGame().GetCallqueue().CallLater(SelectPlayer);
	}
	
	void SelectPlayer()
	{
		if(m_iSelectedPlayerID == m_cUnslotPlayerListBoxComponent.GetElementComponent(m_cUnslotPlayerListBoxComponent.GetSelectedItem()).m_iPlayerID)
			m_iSelectedPlayerID = 0;
		else
			m_iSelectedPlayerID = m_cUnslotPlayerListBoxComponent.GetElementComponent(m_cUnslotPlayerListBoxComponent.GetSelectedItem()).m_iPlayerID;
		UpdateSlots();
	}
	
	void SelectFactionBlufor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(34, 196, 244, 33));
		UpdateSlots();
	}
	
	void SelectFactionOpfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(238, 49, 47, 33));
		UpdateSlots();
	}
	
	void SelectFactionIndfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(0, 177, 79, 33));
		UpdateSlots();
	}
	
	void SelectFactionCiv()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(168, 110, 207, 33));
		UpdateSlots();
	}
	
	void InitSlots()
	{
		for(int i = 0; i < m_Gamemode.m_aEntitySlots.Count(); i++)
		{
			if(m_Gamemode.m_aSlots.Get(i) == -1 || m_Gamemode.m_aSlots.Get(i) == -2)
				continue;
			
			if(!Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(i)))
				continue;

			switch(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(i))).GetEntity()).GetFaction().GetFactionKey())
			{
				case "BLUFOR": 	{m_iBluforSlots++; 	break;}
				case "OPFOR": 	{m_iOpforSlots++; 	break;}
				case "INDFOR": 	{m_iIndforSlots++; 	break;}
				case "CIV":		{m_iCivSlots++;		break;}
			}
		}
	}
	
	void UpdateSlots()
	{
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_cSlotListBoxComponent.Clear();
		m_cOrbatListBoxComponent.Clear();
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		InitSlots();
		PanelWidget.Cast(m_wRoot.FindAnyWidget("PlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		for(int i = 0; i < m_Gamemode.m_aGroupRplIDs.Count(); i++)
		{
			int leadersInGroup = 0;
			int playersInGroup = 0;
			int deadPlayersInGroup = 0;
			
			if(!Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i)))
				continue;
			
			SCR_AIGroup group = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i))).GetEntity());
			if(group.GetFaction() != m_fSelectedFaction)
				continue;
			if(m_Gamemode.m_aGroupLockedStatus.Get(i) && (!SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId())))
				continue;
			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			int orbatGroupIndex = m_cOrbatListBoxComponent.AddItemGroup(null, group, "{55D48B298362DA71}UI/Listbox/GroupListBoxOrbatElementNonAdmin.layout");
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupWidget().SetColor(group.GetFaction().GetFactionColor());
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			
			if(SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			{	
				m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetLockButton().m_OnClicked.Insert(LockGroupSlotsDelayed);
				if(m_Gamemode.m_aGroupLockedStatus.Get(i))
					m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).SetLockImage("{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
			}
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().Update(SCR_GroupIdentityComponent.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i))).GetEntity().FindComponent(SCR_GroupIdentityComponent)).GetMilitarySymbol());
			for(int g = 0; g < m_Gamemode.m_aEntitySlots.Count(); g++)
			{
				RplId currentGroupId = m_Gamemode.m_aPlayerGroupIDs.Get(g);
				
				if(currentGroupId != m_Gamemode.m_aGroupRplIDs.Get(i))
					continue;
				
				if(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(g))).GetEntity()).GetFaction() != m_fSelectedFaction)
					continue;
				
				if(m_Gamemode.m_aSlots.Get(g) == -1 && (!SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId())))
					continue;
				
				if(m_Gamemode.m_aSlots.Get(g) == -2)
				{
					deadPlayersInGroup++;
					continue;
				}
				
				if(m_Gamemode.m_aSlots.Get(g) == 0 && m_Gamemode.m_aEntityDeathStatus.Get(g) == true)
					continue;
				
				int index = m_cSlotListBoxComponent.AddItemSlot(null , m_Gamemode.m_aEntitySlots.Get(g));
				
				if(m_Gamemode.m_aSlots.Get(g) >= 0)
					playersInGroup++;
				
				if(m_Gamemode.m_aSlots.Get(g) > 0)
				{
					if(GetGame().GetPlayerManager().IsPlayerConnected(m_Gamemode.m_aSlots.Get(g)))
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(m_Gamemode.m_aSlots.Get(g)));
					else
					{
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(m_Gamemode.m_aSlotPlayerNames.Get(g));
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDisconnectWidget().SetVisible(true);
					}
				}
				m_cSlotListBoxComponent.GetCRFElementComponent(index).GetSlotButton().m_OnClicked.Insert(SelectSlotDelay);				
				
				if(m_Gamemode.m_aEntitySlotTypes.Get(g) == 0 && m_Gamemode.m_aSlots.Get(g) > 0)
				{
					int orbatIndex = m_cOrbatListBoxComponent.AddItemSlot(null , m_Gamemode.m_aEntitySlots.Get(g), "{BD36FFAE9AB69175}UI/Listbox/PlayerSlotListboxOrbatElementNonAdmin.layout");
					if(GetGame().GetPlayerManager().IsPlayerConnected(m_Gamemode.m_aSlots.Get(g)))
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(m_Gamemode.m_aSlots.Get(g)));
					else
					{
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(m_Gamemode.m_aSlotPlayerNames.Get(g));
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetDisconnectWidget().SetVisible(true);
					}
					m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetSlotButton().SetVisible(false);
					
					if(leadersInGroup == 0)
					{
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetRoleImage(m_Gamemode.m_aSlotIcons.Get(g), "groupRoleName");
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetGroupIconColor(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(g))).GetEntity()).GetFaction().GetFactionColor());
					}
					leadersInGroup++;
				}
			
				if(SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				{	
					m_cSlotListBoxComponent.GetCRFElementComponent(index).GetLockButton().m_OnClicked.Insert(LockSlotDelay);
					m_cSlotListBoxComponent.GetCRFElementComponent(index).GetKickButton().m_OnClicked.Insert(KickSlotDelay);
					if(m_Gamemode.m_aSlots.Get(g) == -1)
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetLockImage("{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
				}
			}
			if(leadersInGroup == 0)	
				m_cOrbatListBoxComponent.RemoveItem(orbatGroupIndex);
			if(playersInGroup == 0)
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
			if(deadPlayersInGroup > 0 && playersInGroup == 0 && SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
		}
		if(m_Gamemode.m_aSlots.Find(m_iSelectedPlayerID) != -1)
			m_iSelectedPlayerID = 0;
		ref array<int> playerIDs = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIDs);
		m_cUnslotPlayerListBoxComponent.Clear();
		foreach(int player : playerIDs)
		{	
			if(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey() != "SPEC")
				continue;
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			int index = m_cUnslotPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), "{D09E0DAC2494343C}UI/data/EMPTY.edds", "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout", player);
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(index);
			comp.GetSelectButton().m_OnClicked.Insert(SelectPlayerDelay);
			if(SCR_Global.IsAdmin(player))
				comp.SetColor(Color.FromRGBA(255, 0, 0, 255));
			
			
			if(player == m_iSelectedPlayerID)
				comp.SetColor(Color.FromRGBA(255, 163, 0, 255));
		}
		
		localSlotChanges = m_Gamemode.m_iSlotChanges;
	}
	
	void KickSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(KickSlot, 10, false);
	}
	
	void KickSlot()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(m_Gamemode.m_aEntitySlots.Find(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).entityID), 0);
	}
	
	void LockGroupSlotsDelayed()
	{
		GetGame().GetCallqueue().CallLater(LockGroupSlots, 10, false);
	}
	
	void LockGroupSlots()
	{
		int groupRplID = RplComponent.Cast(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group.FindComponent(RplComponent)).Id();
		if(!m_Gamemode.m_aGroupLockedStatus.Get(m_Gamemode.m_aGroupRplIDs.Find(groupRplID)))
		{
			for(int i = 0; i < m_Gamemode.m_aEntitySlots.Count(); i++)
			{
				if(m_Gamemode.m_aPlayerGroupIDs.Get(i) == RplComponent.Cast(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group.FindComponent(RplComponent)).Id())
				{
					SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(i, -1);	
				}
			}
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetGroupLocked(m_Gamemode.m_aGroupRplIDs.Find(RplComponent.Cast(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group.FindComponent(RplComponent)).Id()), true);
		}
		else
		{
			for(int i = 0; i < m_Gamemode.m_aEntitySlots.Count(); i++)
			{
				if(m_Gamemode.m_aPlayerGroupIDs.Get(i) == RplComponent.Cast(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group.FindComponent(RplComponent)).Id())
				{
					SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(i, 0);	
				}
			}
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetGroupLocked(m_Gamemode.m_aGroupRplIDs.Find(RplComponent.Cast(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group.FindComponent(RplComponent)).Id()), false);
		}
	}
	
	//Buttons need a delay :)
	void LockSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(LockSlot, 10, false);
	}
	
	void LockSlot()
	{
		if(m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).entityID)) == -1)
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(m_Gamemode.m_aEntitySlots.Find(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).entityID), 0);
		else
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(m_Gamemode.m_aEntitySlots.Find(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).entityID), -1);
	}
	
	void OpenSlottingMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
	}
	
	void AdvanceMenu()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).AdvanceGamemodeState();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		TimeContainer timeContainer = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetTime();
		int hours = timeContainer.m_iHours;
		int minutes = timeContainer.m_iMinutes;
		
		string minuteString;
		string hourString;
		if(minutes < 10)
			minuteString = "0" + minutes.ToString();
		else
			minuteString = minutes.ToString();
		
		if(hours < 10)
			hourString = "0" + hours.ToString();
		else
			hourString = hours.ToString();
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText")).SetText("Time: " + hourString + ":" +  minuteString);
		ref array<int> playerIDs = {};
		
		GetGame().GetPlayerManager().GetAllPlayers(playerIDs);
		m_cPlayerListBoxComponent.Clear();
		foreach(int player : playerIDs)
		{	
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player) || !SCR_Global.IsAdmin(player))
				continue;
			int index;
			if(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey() != "SPEC")
			{
				switch(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey())
				{
					case "BLUFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rBluforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "OPFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rOpforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "INDFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rIndforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "CIV" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rCivIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
				}
			}
			else
			{
				index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), "{D09E0DAC2494343C}UI/data/EMPTY.edds", "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout");
			}
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);
			if(SCR_Global.IsAdmin(player))
				comp.SetColor(Color.FromRGBA(255, 0, 0, 255));
			
			
			if(m_Gamemode.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 163, 0, 255));
		}
		foreach(int player : playerIDs)
		{	
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player) || SCR_Global.IsAdmin(player))
				continue;
			int index;
			if(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey() != "SPEC")
			{
				switch(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey())
				{
					case "BLUFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rBluforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "OPFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rOpforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "INDFOR" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rIndforIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
					case "CIV" : {index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), m_rCivIcon, "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout"); break;}
				}
			}
			else
			{
				index = m_cPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), "{D09E0DAC2494343C}UI/data/EMPTY.edds", "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout");
			}
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);	
			
			if(m_Gamemode.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 163, 0, 255));
		}
		
		if (m_ChatPanel)
        	m_ChatPanel.OnUpdateChat(tDelta);
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText")).SetText("Players: " + GetGame().GetPlayerManager().GetPlayerCount());
		int leftRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).GetText().ToInt();
		int rightRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).GetText().ToInt();
		TextWidget.Cast(m_wRoot.FindAnyWidget("Final")).SetText(Math.Round(GetGame().GetPlayerManager().GetPlayerCount() / (leftRatio + rightRatio) * leftRatio).ToString() + " : " + Math.Round(GetGame().GetPlayerManager().GetPlayerCount() / (leftRatio + rightRatio) * rightRatio).ToString());
		if(localSlotChanges != m_Gamemode.m_iSlotChanges)
			UpdateSlots();
		
		if(m_iBluforSlots > 0)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsBlufor")).SetText(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("BLUFOR")).ToString() + "/" + m_iBluforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).SetEnabled(true);
		}
		else
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsBlufor")).SetText("0/0");
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,167));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLock")).SetColor(Color.FromRGBA(255,255,255,255));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).SetEnabled(false);
		}
		if(m_iOpforSlots > 0)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsOpfor")).SetText(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("OPFOR")).ToString() + "/" + m_iOpforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).SetEnabled(true);
		}
		else
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsOpfor")).SetText("0/0");
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,167));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLock")).SetColor(Color.FromRGBA(255,255,255,255));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).SetEnabled(false);
		}
		if(m_iIndforSlots > 0)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsIndfor")).SetText(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("INDFOR")).ToString() + "/" + m_iIndforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).SetEnabled(true);
		}
		else
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsIndfor")).SetText("0/0");
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,167));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLock")).SetColor(Color.FromRGBA(255,255,255,255));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).SetEnabled(false);
		}
		if(m_iCivSlots > 0)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsCiv")).SetText(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("CIV")).ToString() + "/" + m_iCivSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).SetEnabled(true);	
		}
		else
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsCiv")).SetText("0/0");
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,167));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLock")).SetColor(Color.FromRGBA(255,255,255,255));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).SetEnabled(false);
		}	
		
		if(m_LocalSlottingState != m_Gamemode.m_SlottingState)
		{
			m_LocalSlottingState = m_Gamemode.m_SlottingState;
			AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");
		}
		
		if(m_Gamemode.m_SlottingState == 0)
			TextWidget.Cast(m_wRoot.FindAnyWidget("CurrentSlotPhase")).SetText("Leaders and Medics");
		else if(m_Gamemode.m_SlottingState == 1)
			TextWidget.Cast(m_wRoot.FindAnyWidget("CurrentSlotPhase")).SetText("Specialties");
		else
			TextWidget.Cast(m_wRoot.FindAnyWidget("CurrentSlotPhase")).SetText("Everyone");
		
		if(SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			ButtonWidget previewButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewButton"));
			ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
			ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
			ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
			gameButton.SetEnabled(true);
			advanceButton.SetEnabled(true);
			m_wRoot.FindAnyWidget("SlottingPhases").SetOpacity(1);
			FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(1);
			m_wRoot.FindAnyWidget("UnslottedPlayers").SetOpacity(1);
		}
	}
	
	void SelectSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(SelectSlot, 10, false);
	}
	
	void SelectSlot()
	{
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(m_cSlotListBoxComponent.GetElementComponent(m_cSlotListBoxComponent.GetSelectedItem()));
		bool isAdmin = SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId());
		bool leaderAndMedicPhase = m_Gamemode.m_SlottingState == 0;
		bool slotNotLeaderOrMedic = m_Gamemode.m_aEntitySlotTypes.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) != 0;
		bool specialtyPhase = m_Gamemode.m_SlottingState == 1;
		bool slotNotSpecialtyOrLM = m_Gamemode.m_aEntitySlotTypes.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) != 0 && m_Gamemode.m_aEntitySlotTypes.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) != 1;
		int index = m_Gamemode.m_aEntitySlots.Find(comp.entityID);
		if (index == -1)
			return;
		
		// Return if leaders and medics phase but the slot is not leader or medic
		if (leaderAndMedicPhase && slotNotLeaderOrMedic && !isAdmin) 
			return;
			
		// Return if Specialties phase but the slot is not a specialty or leader/medic
		if (specialtyPhase && slotNotSpecialtyOrLM && !isAdmin)
			return;
		
		if (m_iSelectedPlayerID > 0 && isAdmin)
		{
			if (m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) == m_iSelectedPlayerID)
			{
				SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(index, 0);
				m_iSelectedPlayerID = 0;
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
				return;
			} else if(m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) == 0) {
				if (m_Gamemode.m_aSlots.Find(m_iSelectedPlayerID) != -1)
					SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(m_Gamemode.m_aSlots.Find(m_iSelectedPlayerID), 0);
				
				SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(index, m_iSelectedPlayerID);
				m_iSelectedPlayerID = 0;
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
				return;
			}
		}
		
		if (m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) != 0 && m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) != GetGame().GetPlayerController().GetPlayerId())
			return;
		
		if (m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) == GetGame().GetPlayerController().GetPlayerId())
		{
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(index, 0);
			return;
		} else if(m_Gamemode.m_aSlots.Get(m_Gamemode.m_aEntitySlots.Find(comp.entityID)) == 0) {
			if (m_Gamemode.m_aSlots.Find(GetGame().GetPlayerController().GetPlayerId()) != -1)
				SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(m_Gamemode.m_aSlots.Find(GetGame().GetPlayerController().GetPlayerId()), 0);
			
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetSlot(index, GetGame().GetPlayerController().GetPlayerId());
			return;
		}
	}
	
	void Action_VONon()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(true, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	//From RL
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		ref array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		transiver.SetFrequency(1);
		return transiver;
	}
	
	
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	//From reforger lobby <3
	void Action_VONOff()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(false, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	//From reforger lobby <3
	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}