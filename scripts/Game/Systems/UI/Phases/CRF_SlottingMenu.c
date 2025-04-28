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
	protected CRF_MenuManager m_MenuManager;
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_ButtonTextComponent m_wAdvanceButton;
	protected SCR_ButtonTextComponent m_wPreviewButton;
	
	protected int m_iBluforSlots = 0;
	protected int m_iOpforSlots = 0;
	protected int m_iIndforSlots = 0;
	protected int m_iCivSlots = 0;
	
	protected int m_iTakenBluforSlots = 0;
	protected int m_iTakenOpforSlots = 0;
	protected int m_iTakenIndforSlots = 0;
	protected int m_iTakenCivSlots = 0;

	
	protected Faction m_fSelectedFaction;
	protected int m_iSelectedplayerId = 0;
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
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
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
		
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			gameButton.SetEnabled(true);
		
		SCR_ButtonTextComponent.Cast(gameButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(InitilizePlayer);
		SCR_ButtonTextComponent.Cast(previewButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(OpenSlottingMenu);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlotPhaseButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceSlottingPhase);
		SCR_ButtonTextComponent.Cast(advanceButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceMenu);
		
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerList")).FindHandler(SCR_ListBoxComponent));
		m_cOrbatListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("OrbatList")).FindHandler(CRF_ListboxComponent));
		m_cUnslotPlayerListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerList")).FindHandler(CRF_ListboxComponent));

		m_cSlotListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleList")).FindHandler(CRF_ListboxComponent));
		InitSlots();
		
		CRF_GearscriptManager GearscriptManager = CRF_GearscriptManager.GetInstance();	
		ResourceName gearScriptResource;
		CRF_GearScriptConfig gearConfig;
	
		if(CRF_SlottingManager.GetInstance().IsFactionValid("BLUFOR"))
		{
			if(GearscriptManager)
			{	
				gearScriptResource = GearscriptManager.GetGearScriptResource("BLUFOR");
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
			
			m_wRoot.FindAnyWidget("BluforFrame").SetVisible(true);
				
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagBlufor")).LoadImageTexture(0, m_rBluforIcon);
			m_wRoot.FindAnyWidget("BluforBGSelect").SetColor(Color.FromRGBA(34, 196, 244, 33));
		};
		
		if(CRF_SlottingManager.GetInstance().IsFactionValid("OPFOR"))
		{
			if(GearscriptManager)
			{	
				gearScriptResource = GearscriptManager.GetGearScriptResource("OPFOR");
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
			
			m_wRoot.FindAnyWidget("OpforFrame").SetVisible(true);
			
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagOpfor")).LoadImageTexture(0, m_rOpforIcon);
			m_wRoot.FindAnyWidget("OpforBGSelect").SetColor(Color.FromRGBA(238, 49, 47, 33));
		};
		
		if(CRF_SlottingManager.GetInstance().IsFactionValid("INDFOR"))
		{
			if(GearscriptManager)
			{	
				gearScriptResource = GearscriptManager.GetGearScriptResource("INDFOR");
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
			
			m_wRoot.FindAnyWidget("IndforFrame").SetVisible(true);
				
			ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagIndfor")).LoadImageTexture(0, m_rIndforIcon);
			m_wRoot.FindAnyWidget("IndforBGSelect").SetColor(Color.FromRGBA(0, 177, 79, 33));
		};
		
		if(CRF_SlottingManager.GetInstance().IsFactionValid("CIV"))
		{
			if(GearscriptManager)
			{	
				gearScriptResource = GearscriptManager.GetGearScriptResource("CIV");
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
			
			m_wRoot.FindAnyWidget("CivFrame").SetVisible(true);
				
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
		
		if(CRF_SlottingManager.GetInstance().IsFactionValid("BLUFOR"))
			SelectFactionBlufor();
		else if(CRF_SlottingManager.GetInstance().IsFactionValid("OPFOR"))
			SelectFactionOpfor();
		else if(CRF_SlottingManager.GetInstance().IsFactionValid("INDFOR"))
			SelectFactionIndfor();
		else if(CRF_SlottingManager.GetInstance().IsFactionValid("CIV"))
			SelectFactionCiv();

		UpdateSlots();
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);			
	}
	
	override void OnMenuClose()
	{
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Remove(UpdateSlots);
		
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	void AdvanceSlottingPhase()
	{
		if(m_Gamemode.m_SlottingState == 2)
			return;
		
		CRF_RplToAuthorityManager.GetInstance().RequestAdvanceSlottingPhase();
	}
	
	void InitilizePlayer()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		CRF_RplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
		
		if(CRF_GamemodeManager.IsSpectator())
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
	}
	
	void SelectPlayerDelay()
	{
		GetGame().GetCallqueue().CallLater(SelectPlayer);
	}
	
	void SelectPlayer()
	{
		if(m_iSelectedplayerId == m_cUnslotPlayerListBoxComponent.GetElementComponent(m_cUnslotPlayerListBoxComponent.GetSelectedItem()).m_iplayerId)
			m_iSelectedplayerId = 0;
		else
			m_iSelectedplayerId = m_cUnslotPlayerListBoxComponent.GetElementComponent(m_cUnslotPlayerListBoxComponent.GetSelectedItem()).m_iplayerId;
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
		map<int, ref CRF_SlotData> tempMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		foreach (int slotId, ref CRF_SlotData slotData : tempMap)
		{			
			if(slotData.m_SlotUIData.m_bIsLockedSlot || slotData.m_SlotUIData.m_bIsDeadSlot)
				continue;
			
			switch(slotData.m_SlotFactionKey)
			{
				case "BLUFOR": 	{m_iBluforSlots++;	if(slotData.m_iSlotCurrentPlayerId > 0) m_iTakenBluforSlots++;		break;}
				case "OPFOR": 	{m_iOpforSlots++; 	if(slotData.m_iSlotCurrentPlayerId > 0) m_iTakenOpforSlots++;		break;}
				case "INDFOR": 	{m_iIndforSlots++;	if(slotData.m_iSlotCurrentPlayerId > 0) m_iTakenIndforSlots++;		break;}
				case "CIV":		{m_iCivSlots++;		if(slotData.m_iSlotCurrentPlayerId > 0) m_iTakenCivSlots++;		break;}
			}
		}
	}
	
	void UpdateSlots()
	{
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iTakenBluforSlots = 0;
		m_iTakenOpforSlots = 0;
		m_iTakenIndforSlots = 0;
		m_iTakenCivSlots = 0;
		m_cSlotListBoxComponent.Clear();
		m_cOrbatListBoxComponent.Clear();
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		InitSlots();
		
		if(!m_fSelectedFaction)
			return;
		
		PanelWidget.Cast(m_wRoot.FindAnyWidget("PlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		
		map<int, ref CRF_SlotData> tempMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		array<SCR_AIGroup> groups = {};
		SCR_GroupsManagerComponent.GetInstance().GetAllPlayableGroups(groups);
		
		foreach(SCR_AIGroup group : groups)
		{	
			int leadersInGroup = 0;
			int playersInGroup = 0;
			int deadPlayersInGroup = 0;
			
			if(group.GetFaction() != m_fSelectedFaction)
				continue;
			
			if(group.IsPrivate() && !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				continue;
			
			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			int orbatGroupIndex = m_cOrbatListBoxComponent.AddItemGroup(null, group, "{55D48B298362DA71}UI/Listbox/GroupListBoxOrbatElementNonAdmin.layout");
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupWidget().SetColor(group.GetFaction().GetFactionColor());
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			
			if(SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			{	
				m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetLockButton().m_OnClicked.Insert(LockGroupSlotsDelayed);
				if(group.IsPrivate())
					m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).SetLockImage("{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
			}
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().Update(SCR_GroupIdentityComponent.Cast(group.FindComponent(SCR_GroupIdentityComponent)).GetMilitarySymbol());
			
			foreach(int slotId, ref CRF_SlotData slotData : tempMap)
			{	
				if (slotData.m_iSlotCurrentGroup != RplComponent.Cast(group.FindComponent(RplComponent)).Id() 
					|| GetGame().GetFactionManager().GetFactionByKey(slotData.m_SlotFactionKey) != m_fSelectedFaction)
					continue;
				
				if(slotData.m_SlotUIData.m_bIsLockedSlot && (!SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId())))
					continue;
				
				if(slotData.m_SlotUIData.m_bIsDeadSlot)
				{
					deadPlayersInGroup++;
					continue;
				}
				
				if(slotData.m_iSlotCurrentPlayerId == 0 && slotData.m_SlotUIData.m_bIsDeadSlot)
					continue;
				
				int index = m_cSlotListBoxComponent.AddItemSlot(null , slotId);
				
				if(slotData.m_iSlotCurrentPlayerId >= 0)
					playersInGroup++;
				
				if(slotData.m_iSlotCurrentPlayerId > 0)
				{
					if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.m_iSlotCurrentPlayerId))
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
					else
					{
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDisconnectWidget().SetVisible(true);
					}
				}
				m_cSlotListBoxComponent.GetCRFElementComponent(index).GetSlotButton().m_OnClicked.Insert(SelectSlotDelay);				
				
				if(slotData.m_SlotUIData.m_iSlotType == CRF_ESlotType.LEADERORMEDIC && slotData.m_iSlotCurrentPlayerId > 0)
				{
					int orbatIndex = m_cOrbatListBoxComponent.AddItemSlot(null , slotId, "{BD36FFAE9AB69175}UI/Listbox/PlayerSlotListboxOrbatElementNonAdmin.layout");
					if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.m_iSlotCurrentPlayerId))
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
					else
					{
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetDisconnectWidget().SetVisible(true);
					}
					m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetSlotButton().SetVisible(false);
					
					if(leadersInGroup == 0)
					{
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetRoleImage(slotData.m_SlotUIData.m_rSlotIconResource, "groupRoleName");
						m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetGroupIconColor(GetGame().GetFactionManager().GetFactionByKey(slotData.m_SlotFactionKey).GetFactionColor());
					}
					leadersInGroup++;
				}
			
				if(SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				{	
					m_cSlotListBoxComponent.GetCRFElementComponent(index).GetLockButton().m_OnClicked.Insert(LockSlotDelay);
					m_cSlotListBoxComponent.GetCRFElementComponent(index).GetKickButton().m_OnClicked.Insert(KickSlotDelay);
					if(slotData.m_SlotUIData.m_bIsLockedSlot)
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetLockImage("{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
				}
			}
			if(leadersInGroup == 0)	
				m_cOrbatListBoxComponent.RemoveItem(orbatGroupIndex);
			if(playersInGroup == 0 && !SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
			if(deadPlayersInGroup > 0 && playersInGroup == 0 && SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
		}
		if(CRF_SlottingManager.GetInstance().IsPlayerInASlot(m_iSelectedplayerId))
			m_iSelectedplayerId = 0;
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		m_cUnslotPlayerListBoxComponent.Clear();
		foreach(int player : playerIds)
		{	
			if(player <= 0 || !SCR_FactionManager.SGetPlayerFaction(player))
				continue;
			if(SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey() != "SPEC")
				continue;
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			int index = m_cUnslotPlayerListBoxComponent.AddItemAndIconPlayer(GetGame().GetPlayerManager().GetPlayerName(player), "{D09E0DAC2494343C}UI/data/EMPTY.edds", "flag", null,  "{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout", player);
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(index);
			comp.GetSelectButton().m_OnClicked.Insert(SelectPlayerDelay);
			if(SCR_Global.IsAdmin(player))
				comp.SetColor(Color.Red);
			
			if(CRF_GamemodeManager.GetInstance().IsModerator(player))
				comp.SetColor(Color.Yellow);
			
			if(player == m_iSelectedplayerId)
				comp.SetColor(Color.DarkYellow);
		}
	}
	
	void KickSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(KickSlot, 10, false);
	}
	
	void KickSlot()
	{
		CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).m_iSlotId, 0);
	}
	
	void LockGroupSlotsDelayed()
	{
		GetGame().GetCallqueue().CallLater(LockGroupSlots, 10, false);
	}
	
	void LockGroupSlots()
	{
		SCR_AIGroup aiGroup = m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).group;
		int groupRplID = RplComponent.Cast(aiGroup.FindComponent(RplComponent)).Id();
		
		array<int> idsArray = CRF_SlottingManager.GetInstance().GetAllSlotIDsForGroup(groupRplID);
		
		if(!aiGroup.IsPrivate())
		{
			CRF_RplToAuthorityManager.GetInstance().UpdateGroupLockedState(groupRplID, true);
			
			foreach(int id : idsArray)
			{
				CRF_RplToAuthorityManager.GetInstance().UpdateSlotLockedState(id, true);	
			}
		}
		else
		{
			CRF_RplToAuthorityManager.GetInstance().UpdateGroupLockedState(groupRplID, false);
			
			foreach(int id : idsArray)
			{
				CRF_RplToAuthorityManager.GetInstance().UpdateSlotLockedState(id, false);	
			}
		}
	}
	
	//Buttons need a delay :)
	void LockSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(LockSlot, 10, false);
	}
	
	void LockSlot()
	{
		if(CRF_SlottingManager.GetInstance().GetSlotData(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).m_iSlotId).m_SlotUIData.m_bIsLockedSlot)
			CRF_RplToAuthorityManager.GetInstance().UpdateSlotLockedState(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).m_iSlotId, false);
		else
			CRF_RplToAuthorityManager.GetInstance().UpdateSlotLockedState(m_cSlotListBoxComponent.GetCRFElementComponent(m_cSlotListBoxComponent.GetSelectedItem()).m_iSlotId, true);
	}
	
	void OpenSlottingMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
	}
	
	void AdvanceMenu()
	{
		CRF_RplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState();
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
		ref array<int> playerIds = {};
		
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		m_cPlayerListBoxComponent.Clear();
		foreach(int player : playerIds)
		{	
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player) || !SCR_Global.IsAdmin(player))
				continue;
			int index;
			if(SCR_FactionManager.SGetPlayerFaction(player) && SCR_FactionManager.SGetPlayerFaction(player).GetFactionKey() != "SPEC")
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
				comp.SetColor(Color.Red);
			
			if(CRF_GamemodeManager.GetInstance().IsModerator(player))
				comp.SetColor(Color.Yellow);
			
			if(m_MenuManager.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 163, 0, 255));
		}
		foreach(int player : playerIds)
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
			
			if(m_MenuManager.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 163, 0, 255));
		}
		
		if (m_ChatPanel)
        	m_ChatPanel.OnUpdateChat(tDelta);
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText")).SetText("Players: " + GetGame().GetPlayerManager().GetPlayerCount());
		int leftRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).GetText().ToInt();
		int rightRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).GetText().ToInt();
		TextWidget.Cast(m_wRoot.FindAnyWidget("Final")).SetText(Math.Round(GetGame().GetPlayerManager().GetPlayerCount() / (leftRatio + rightRatio) * leftRatio).ToString() + " : " + Math.Round(GetGame().GetPlayerManager().GetPlayerCount() / (leftRatio + rightRatio) * rightRatio).ToString());
		
		if(CRF_SlottingManager.GetInstance().IsFactionValid("BLUFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsBlufor")).SetText(m_iTakenBluforSlots.ToString() + "/" + m_iBluforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).SetEnabled(true);
		}

		if(CRF_SlottingManager.GetInstance().IsFactionValid("OPFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsOpfor")).SetText(m_iTakenOpforSlots.ToString() + "/" + m_iOpforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).SetEnabled(true);
		}

		if(CRF_SlottingManager.GetInstance().IsFactionValid("INDFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsIndfor")).SetText(m_iTakenIndforSlots.ToString() + "/" + m_iIndforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).SetEnabled(true);
		}

		if(CRF_SlottingManager.GetInstance().IsFactionValid("CIV"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsCiv")).SetText(m_iTakenCivSlots.ToString() + "/" + m_iCivSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLockBG")).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLock")).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).SetEnabled(true);	
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
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(m_cSlotListBoxComponent.GetElementComponent(m_cSlotListBoxComponent.GetSelectedItem()));
		int slotId = comp.m_iSlotId;
		
		bool isAdmin = SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId());
		bool leaderAndMedicPhase = m_Gamemode.m_SlottingState == 0;
		bool slotNotLeaderOrMedic = slottingManager.GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.LEADERORMEDIC;
		bool specialtyPhase = m_Gamemode.m_SlottingState == 1;
		bool slotNotSpecialtyOrLM = slottingManager.GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.LEADERORMEDIC && slottingManager.GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.SPECIALTY;
		if (slotId == 0)
			return;
		
		// Return if leaders and medics phase but the slot is not leader or medic
		if (leaderAndMedicPhase && slotNotLeaderOrMedic && !isAdmin) 
			return;
			
		// Return if Specialties phase but the slot is not a specialty or leader/medic
		if (specialtyPhase && slotNotSpecialtyOrLM && !isAdmin)
			return;
		
		if (m_iSelectedplayerId > 0 && isAdmin)
		{
			if (slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId == m_iSelectedplayerId)
			{
				CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, 0);
				m_iSelectedplayerId = 0;
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
				return;
			} else if(slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId == 0) {
				if (slottingManager.IsPlayerInASlot(m_iSelectedplayerId))
					CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slottingManager.GetPlayerSlotID(m_iSelectedplayerId), 0);
				
				CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, m_iSelectedplayerId);
				m_iSelectedplayerId = 0;
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
				return;
			}
		}
		
		if (slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId != 0 && slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId != SCR_PlayerController.GetLocalPlayerId())
			return;
		
		if (slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId == SCR_PlayerController.GetLocalPlayerId())
		{
			CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, 0);
			return;
		} else if(slottingManager.GetSlotData(slotId).m_iSlotCurrentPlayerId == 0) {
			if (slottingManager.IsPlayerInASlot(SCR_PlayerController.GetLocalPlayerId()))
				CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slottingManager.GetPlayerSlotID(SCR_PlayerController.GetLocalPlayerId()), 0);
			
			CRF_RplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, GetGame().GetPlayerController().GetPlayerId());
			return;
		}
	}
	
	void Action_VONon()
	{
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