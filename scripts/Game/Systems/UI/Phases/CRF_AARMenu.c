modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_AARMenu
}


class CRF_AARMenuUI: ChimeraMenuBase
{
	protected Widget m_wRoot;
	protected ImageWidget m_wPreview;
	protected ImageWidget m_wSlotting;
	protected ImageWidget m_wGame;
	protected ImageWidget m_wAAR;
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_MapEntity m_MapEntity;
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent;
	protected CRF_ListboxComponent m_cSlotListBoxComponent;
	protected int m_iBluforSlots = 0;
	protected int m_iOpforSlots = 0;
	protected int m_iIndforSlots = 0;
	protected int m_iCivSlots = 0;
	protected int m_iAliveBluforSlots = 0;
	protected int m_iAliveOpforSlots = 0;
	protected int m_iAliveIndforSlots = 0;
	protected int m_iAliveCivSlots = 0;
	protected Faction m_fSelectedFaction;
	protected ButtonWidget m_wBackButton;
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {};
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent;
	protected Widget m_wFactions;
	protected Widget m_wMissionDescription;
	protected Widget m_wRoleFrame;
	protected Widget m_wLeftFaction;
	
	override void OnMenuOpen()
	{	
		//Are we the server?????
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		//Loads the map
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		
		//INPUT MANAGERS
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		
		//Loads the chat
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		//MORE INPUT MANAGERS
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		m_wRoot = GetRootWidget();
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_wFactions = m_wRoot.FindAnyWidget("Factions");
		m_wMissionDescription = m_wRoot.FindAnyWidget("DescriptionList");
		m_wRoleFrame = m_wRoot.FindAnyWidget("RoleList");
		m_wLeftFaction = m_wRoot.FindAnyWidget("LeftFaction");
		
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
		
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerList")).FindHandler(SCR_ListBoxComponent));
		m_cSlotListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleList")).FindHandler(CRF_ListboxComponent));
		
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagBlufor")).LoadImageTexture(1, SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("BLUFOR")).GetFactionFlag());
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagBlufor")).SetImage(1);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagOpfor")).LoadImageTexture(1, SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("OPFOR")).GetFactionFlag());
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagOpfor")).SetImage(1);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagIndfor")).LoadImageTexture(1, SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("INDFOR")).GetFactionFlag());
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagIndfor")).SetImage(1);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagCiv")).LoadImageTexture(1, SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("CIV")).GetFactionFlag());
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FlagCiv")).SetImage(1);
		
		m_wRoot.FindAnyWidget("BluforBGSelect").SetColor(Color.FromRGBA(34, 196, 244, 33));
		m_wRoot.FindAnyWidget("OpforBGSelect").SetColor(Color.FromRGBA(238, 49, 47, 33));
		m_wRoot.FindAnyWidget("IndforBGSelect").SetColor(Color.FromRGBA(0, 177, 79, 33));
		m_wRoot.FindAnyWidget("CivBGSelect").SetColor(Color.FromRGBA(168, 110, 207, 33));
		InitSlots();
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
			SelectFactionOpfor();
		}
		
		UpdateSlots();
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		
		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_wBackButton.SetOpacity(0);
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("DescriptionList")).FindHandler(SCR_ListBoxComponent));
		
		DescriptionInit();
	}
	
	override void OnMenuClose()
	{
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Remove(UpdateSlots);
		
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
				m_cMissionDescriptionListBoxComponent.AddItem(description.m_sTitle, null, "{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout");
				m_aActiveDescriptors.Insert(description);
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
	
	void InitSlots()
	{
		map<int, ref CRF_SlotData> tempMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		foreach (int slotId, CRF_SlotData slotData : tempMap)
		{
			if(slotData.m_SlotUIData.m_bIsLockedSlot || slotData.m_iSlotCurrentPlayerId == 0)
				continue;
			
			switch(slotData.m_SlotFactionKey)
			{
				case "BLUFOR": 	{m_iBluforSlots++; if(!slotData.m_SlotUIData.m_bIsDeadSlot) m_iAliveBluforSlots++;		break;}
				case "OPFOR": 	{m_iOpforSlots++; if(!slotData.m_SlotUIData.m_bIsDeadSlot) m_iAliveOpforSlots++;		break;}
				case "INDFOR": 	{m_iIndforSlots++; if(!slotData.m_SlotUIData.m_bIsDeadSlot) m_iAliveIndforSlots++;		break;}
				case "CIV":		{m_iCivSlots++;	if(!slotData.m_SlotUIData.m_bIsDeadSlot) m_iAliveCivSlots++;			break;}
			}
		}
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
	
	void UpdateSlots()
	{
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;
		m_cSlotListBoxComponent.Clear();
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		InitSlots();
		PanelWidget.Cast(m_wRoot.FindAnyWidget("PlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		
		map<int, ref CRF_SlotData> tempMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		array<SCR_AIGroup> groups = {};
		SCR_GroupsManagerComponent.GetInstance().GetAllPlayableGroups(groups);
		
		foreach(SCR_AIGroup group : groups)
		{	
			int playersInGroup = 0;
			
			if(group.GetFaction() != m_fSelectedFaction)
				continue;
			
			if(group.IsPrivate())
				continue;
			
			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupWidget().SetColor(group.GetFaction().GetFactionColor());
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().Update(SCR_GroupIdentityComponent.Cast(group.FindComponent(SCR_GroupIdentityComponent)).GetMilitarySymbol());
			
			foreach(int slotId, ref CRF_SlotData slotData : tempMap)
			{	
				if (slotData.m_iSlotCurrentGroup != RplComponent.Cast(group.FindComponent(RplComponent)).Id() 
					|| slotData.m_SlotUIData.m_bIsLockedSlot 
					|| slotData.m_iSlotCurrentPlayerId == 0 
					|| GetGame().GetFactionManager().GetFactionByKey(slotData.m_SlotFactionKey) != m_fSelectedFaction)
					continue;
				
				int index = m_cSlotListBoxComponent.AddItemSlot(null , slotId);
				
				if(!slotData.m_SlotUIData.m_bIsDeadSlot)
				{
					if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.m_iSlotCurrentPlayerId))
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
					else
					{
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDisconnectWidget().SetVisible(true);
					}
				}		
				else
				{
					if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.m_iSlotCurrentPlayerId))
					{
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDeathWidget().SetVisible(true);
					}
					else
					{
						m_cSlotListBoxComponent.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.m_iSlotCurrentPlayerId));
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDisconnectWidget().SetVisible(true);
						m_cSlotListBoxComponent.GetCRFElementComponent(index).GetDeathWidget().SetVisible(true);
					}
				}
				m_cSlotListBoxComponent.GetCRFElementComponent(index).GetSlotButton().SetEnabled(false);
					
				playersInGroup++;
			}
			
			if(playersInGroup == 0)
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
		}
	}
	
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
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
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText")).SetText("Players: " + GetGame().GetPlayerManager().GetPlayerCount());
		
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
		
		if (m_ChatPanel)
        	m_ChatPanel.OnUpdateChat(tDelta);
		
		if(m_iBluforSlots > 0)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsBlufor")).SetText(m_iAliveBluforSlots.ToString() + "/" + m_iBluforSlots);
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
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsOpfor")).SetText(m_iAliveOpforSlots.ToString() + "/" + m_iOpforSlots);
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
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsIndfor")).SetText(m_iAliveIndforSlots.ToString() + "/" + m_iIndforSlots);
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
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsCiv")).SetText(m_iAliveCivSlots.ToString() + "/" + m_iCivSlots);
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
		
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		if(cursorWidget)
		{
			Widget parentWidget = cursorWidget.GetParent();
			float leftFactionX = FrameSlot.GetPosX(m_wLeftFaction);
			if(parentWidget == m_wFactions || parentWidget == m_wMissionDescription || parentWidget == m_wRoleFrame || parentWidget == m_wLeftFaction || cursorWidget.FindHandler(CRF_ListBoxElementComponent))
			{
				leftFactionX += tDelta * 2400.0;
				if (leftFactionX > -14)
					leftFactionX = -14;
			}
			FrameSlot.SetPosX(m_wLeftFaction, leftFactionX);
		}
		else
		{
			float leftFactionX = FrameSlot.GetPosX(m_wLeftFaction);
			leftFactionX -= tDelta * 2400.0;
			if (leftFactionX < -671)
				leftFactionX = -671;
			FrameSlot.SetPosX(m_wLeftFaction, leftFactionX);
		}
		
	}
	
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