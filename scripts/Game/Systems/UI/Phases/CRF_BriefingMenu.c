modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_PreviewMenu
}

class CRF_PreviewMenuUI: ChimeraMenuBase
{
	protected SCR_MapEntity m_MapEntity;
	protected Widget m_wRoot;
	protected ImageWidget m_wPreview;
	protected ImageWidget m_wSlotting;
	protected ImageWidget m_wGame;
	protected ImageWidget m_wAAR;
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent;
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent;
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	protected ButtonWidget m_wBackButton;
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {};
	protected SCR_ChatPanel m_ChatPanel;
	
	override void OnMenuOpen()
	{	
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		m_wRoot = GetRootWidget();
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
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
		
		int gameState = CRF_Gamemode.GetInstance().m_GamemodeState; 
		switch(gameState)
		{
			case 0: {m_wPreview.SetColor(Color.FromRGBA(122, 0, 0, 255));	 break;}
			case 1: {m_wSlotting.SetColor(Color.FromRGBA(122, 0, 0, 255));	 break;}
			case 2: {m_wGame.SetColor(Color.FromRGBA(122, 0, 0, 255)); 	 	 break;}
			case 3: {m_wAAR.SetColor(Color.FromRGBA(122, 0, 0, 255)); 		 break;}
		}
		
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayersList")).FindHandler(SCR_ListBoxComponent));
		
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("DescriptionList")).FindHandler(SCR_ListBoxComponent));
		
		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_wBackButton.SetOpacity(0);
		
		DescriptionInit();
		
		ButtonWidget slottingButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlottingButton"));
		ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
		ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
		ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
		slottingButton.SetEnabled(false);
		gameButton.SetEnabled(false);
		aarButton.SetEnabled(false);
		advanceButton.SetEnabled(false);
		FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(0);
		
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.SLOTTING)
			slottingButton.SetEnabled(true);
		
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			slottingButton.SetEnabled(true);
			gameButton.SetEnabled(true);
		}
		
		SCR_ButtonTextComponent.Cast(slottingButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(OpenSlottingMenu);
		SCR_ButtonTextComponent.Cast(gameButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(InitilizePlayer);
		SCR_ButtonTextComponent.Cast(advanceButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceMenu);
	}
	
	void InitilizePlayer()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
		CRF_PlayerControllerComponent.GetInstance().InitilizePlayer();
	}
	
	void OpenSlottingMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
	}
	
	void AdvanceMenu()
	{
		CRF_RplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
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
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			int index = m_cPlayerListBoxComponent.AddItem(GetGame().GetPlayerManager().GetPlayerName(player), null, "{51F58D728FBCAD99}UI/Listbox/PlayerListboxElementNoIcon.layout");
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);
			if(GetGame().GetPlayerManager().HasPlayerRole(player, EPlayerRole.ADMINISTRATOR) || GetGame().GetPlayerManager().HasPlayerRole(player, EPlayerRole.SESSION_ADMINISTRATOR))
				comp.SetColor(Color.FromRGBA(255, 0, 0, 255));
			
			
			if(m_MenuManager.m_aPlayersTalking.Contains(player))
				comp.SetColor(Color.FromRGBA(255, 183, 0, 255));
		}
		if(SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			ButtonWidget slottingButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlottingButton"));
			ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
			ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
			ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
			slottingButton.SetEnabled(true);
			gameButton.SetEnabled(true);
			advanceButton.SetEnabled(true);
			FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(1);
		
		}
		
		if (m_ChatPanel)
        	m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	override void OnMenuClose()
	{
		if (m_MapEntity)
			m_MapEntity.CloseMap();

		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	void DescriptionInit()
	{
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(false);
		
		m_wBackButton.SetOpacity(0);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Clear();
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText("");
		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();
		foreach(ref CRF_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			if(description.m_bShowForAnyFaction)
			{
				m_cMissionDescriptionListBoxComponent.AddItem(description.m_sTitle, null, "{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout");
				m_aActiveDescriptors.Insert(description);
				continue;
			}
			foreach(string factionKey: description.m_aFactionKeys)
			{
				if(SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_PlayerFactionAffiliationComponent)).GetAffiliatedFactionKey() == factionKey)
				{
					m_cMissionDescriptionListBoxComponent.AddItem(description.m_sTitle, null, "{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout");
					m_aActiveDescriptors.Insert(description);
					continue;
				}
			}
		}
		
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}
	
	void DescriptionSelected()
	{
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(true);
		
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		string description = m_aActiveDescriptors.Get(index).m_sTextData;
		
		m_wBackButton.SetOpacity(1);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Insert(DescriptionInit);
		
		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();
		
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText(description);
	}
	
	//Thank your reforger lobby devs, all map shit accredited to them.
	void OpenMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrap, 0); // Need two frames
	}
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChange, 0);
	}
	void OpenMapWrapZoomChange()
	{
		// What i do with my life...
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChangeWrap, 0); // Another two frames
	}
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();
	}
	
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
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
	
	//From reforger lobby <3
	void Action_VONOff()
	{
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
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