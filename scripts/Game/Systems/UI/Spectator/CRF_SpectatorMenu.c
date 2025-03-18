modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_SpectatorMenu
}

class CRF_SpectatorMenuUI: ChimeraMenuBase
{
	protected ref array<RplId> m_aEntityIcons = {};
	protected ref array<Widget> m_aSpectatorWidgets = {};
	protected ref array<ref CRF_SpectatorLabelIconCharacter> m_aSpectatorIcons = {};
	protected CRF_Gamemode m_Gamemode;
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_MapEntity m_MapEntity;
	protected bool m_bIsMapOpened = false;
	Widget m_wRoot;
	protected FrameWidget m_wMapFrame;
	protected Widget m_wPlayerSlotWidget;
	protected CRF_ListboxComponent m_wPlayerSlots;
	protected CRF_ListboxComponent m_wVONChannels;
	protected int localSlotChanges = 0;
	protected int m_iBluforSlots = 0;
	protected int m_iOpforSlots = 0;
	protected int m_iIndforSlots = 0;
	protected int m_iCivSlots = 0;
	protected int m_iAliveBluforSlots = 0;
	protected int m_iAliveOpforSlots = 0;
	protected int m_iAliveIndforSlots = 0;
	protected int m_iAliveCivSlots = 0;
	protected Faction m_fSelectedFaction;
	protected Widget m_wBluforButton;
	protected Widget m_wOpforButton;
	protected Widget m_wIndforButton;
	protected Widget m_wCivButton;
	protected Widget m_wSlotSelector;
	protected FrameWidget m_wFrameSlots;
	protected FrameWidget m_wFrameChannels;
	protected SCR_PlayerController pc;
	protected IEntity m_eSpecEntity;
	protected bool m_bFPPEntityValidityCheck;
	protected Animation m_aAnimation;
	protected int m_iLocalChannelUpdates = 0;
	ref array<Widget> m_aRequest = {};
	protected bool m_bHideUi = false;
	
	// -- BEGIN Port from CRF_GameTimerDisplay.c --
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;
	
	protected ImageWidget m_wTicketOneImage 
	protected TextWidget m_wTicketOneText
	protected TextWidget m_wTicketOneNumber
	protected ImageWidget m_wTicketOneBackground
	
	protected ImageWidget m_wTicketTwoImage 
	protected TextWidget m_wTicketTwoText
	protected TextWidget m_wTicketTwoNumber
	protected ImageWidget m_wTicketTwoBackground
	
	protected ImageWidget m_wTicketThreeImage 
	protected TextWidget m_wTicketThreeText
	protected TextWidget m_wTicketThreeNumber
	protected ImageWidget m_wTicketThreeBackground
	
	protected ImageWidget m_wTicketFourImage 
	protected TextWidget m_wTicketFourText
	protected TextWidget m_wTicketFourNumber
	protected ImageWidget m_wTicketFourBackground

	protected CRF_GamemodeComponent m_GamemodeComponent;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	protected SCR_PopUpNotification m_PopUpNotification = null;
	// -- END Port --
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		m_wRoot = GetRootWidget();
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_wMapFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("MapFrame"));
		m_wPlayerSlotWidget = m_wRoot.FindAnyWidget("PlayerSlots");
		m_wPlayerSlots = CRF_ListboxComponent.Cast(m_wPlayerSlotWidget.FindHandler(CRF_ListboxComponent));
		m_wVONChannels = CRF_ListboxComponent.Cast(m_wRoot.FindAnyWidget("VONChannels").FindHandler(CRF_ListboxComponent));
		pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().AddActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
		GetGame().GetInputManager().AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
		GetGame().GetInputManager().AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);	
//		 FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI FUCK BI
		GetGame().GetInputManager().AddActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
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
		m_wBluforButton = m_wRoot.FindAnyWidget("BLUSelectButton");
		m_wOpforButton = m_wRoot.FindAnyWidget("OPFSelectButton");
		m_wIndforButton = m_wRoot.FindAnyWidget("INDSelectButton");
		m_wCivButton = m_wRoot.FindAnyWidget("CIVSelectButton");
		m_wFrameSlots = FrameWidget.Cast(m_wRoot.FindAnyWidget("FrameSlots"));
		m_wSlotSelector = m_wRoot.FindAnyWidget("SlotSelector");
		m_wFrameChannels = FrameWidget.Cast(m_wRoot.FindAnyWidget("VONSlots"));
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wBluforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wOpforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wIndforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wCivButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);	
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("CreateChannel")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(CreateChannel);
		
		GetGame().GetCallqueue().CallLater(Action_VONon, 500, false);
		GetGame().GetCallqueue().CallLater(Action_VONOff, 550, false);
		
		// -- BEGIN Port from CRF_GameTimerDisplay.c --
		m_GamemodeComponent 		= CRF_GamemodeComponent.GetInstance();
		m_wTimer            		= TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
		m_wBackground       		= ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));
		
		m_wTicketOneImage			= ImageWidget.Cast(m_wRoot.FindWidget("TicketOneImage"));
		m_wTicketOneText			= TextWidget.Cast(m_wRoot.FindWidget("TicketOneText"));
		m_wTicketOneNumber			= TextWidget.Cast(m_wRoot.FindWidget("TicketOneNumber"));
		m_wTicketOneBackground     	= ImageWidget.Cast(m_wRoot.FindWidget("TicketOneBackground"));
		
		m_wTicketTwoImage			= ImageWidget.Cast(m_wRoot.FindWidget("TicketTwoImage"));
		m_wTicketTwoText			= TextWidget.Cast(m_wRoot.FindWidget("TicketTwoText"));
		m_wTicketTwoNumber			= TextWidget.Cast(m_wRoot.FindWidget("TicketTwoNumber"));
		m_wTicketTwoBackground     	= ImageWidget.Cast(m_wRoot.FindWidget("TicketTwoBackground"));
		
		m_wTicketThreeImage			= ImageWidget.Cast(m_wRoot.FindWidget("TicketThreeImage"));
		m_wTicketThreeText			= TextWidget.Cast(m_wRoot.FindWidget("TicketThreeText"));
		m_wTicketThreeNumber		= TextWidget.Cast(m_wRoot.FindWidget("TicketThreeNumber"));
		m_wTicketThreeBackground	= ImageWidget.Cast(m_wRoot.FindWidget("TicketThreeBackground"));
		
		m_wTicketFourImage			= ImageWidget.Cast(m_wRoot.FindWidget("TicketFourImage"));
		m_wTicketFourText			= TextWidget.Cast(m_wRoot.FindWidget("TicketFourText"));
		m_wTicketFourNumber			= TextWidget.Cast(m_wRoot.FindWidget("TicketFourNumber"));
		m_wTicketFourBackground		= ImageWidget.Cast(m_wRoot.FindWidget("TicketFourBackground"));		
		// -- END Port --
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateCompass()
	{
		float yaw = -pc.m_eCamera.GetYawPitchRoll()[0];
		float yawFloat = -yaw;
		
		if (yawFloat < 0) 
			yawFloat = 360 - Math.AbsFloat(yawFloat);
		
		FrameSlot.SetOffsets(FrameWidget.Cast(m_wRoot.FindAnyWidget("CompassFrameMoveable")), -1090 - 1880 * (yawFloat / 360), -63, -2750 + 1880 * (yawFloat / 360), -995);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		UpdateCompass();
		
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		if(m_iLocalChannelUpdates != m_Gamemode.m_iChannelChanges)
			UpdateChannel();
		
		foreach(Widget request: m_aRequest)
		{
			CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(request.FindHandler(CRF_ListBoxElementComponent));
			if (m_Gamemode.IsPlayerInChannel(comp.m_iPlayerId, comp.m_iChannelId))
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(m_aRequest.Find(request));
				continue;			
			}
			
			if (comp.m_bDeleteRequest)
			{
				if (FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) > 500)
				{
					request.RemoveFromHierarchy();
					m_aRequest.RemoveOrdered(m_aRequest.Find(request));
					continue;
				}
				FrameSlot.SetPosX(request.FindAnyWidget("ButtonAnim"), FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) + tDelta * 2000);
				continue;
			}
			else if (FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) > 0)
			{
				if (FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) - tDelta * 2000 > 0)
					FrameSlot.SetPosX(request.FindAnyWidget("ButtonAnim"), FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) - tDelta * 2000);
				else 
					FrameSlot.SetPosX(request.FindAnyWidget("ButtonAnim"), 0);
			}
			
			comp.GetProgress().SetCurrent(comp.GetProgress().GetCurrent() - tDelta);
			if (comp.GetProgress().GetCurrent() <= 0)
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(m_aRequest.Find(request));
				continue;
			}
		}
		
		if (m_eSpecEntity)
		{
			InputManager im = GetGame().GetInputManager();
			if (im.GetActionValue("ManualCameraMoveLateral") != 0 || im.GetActionValue("ManualCameraMoveVertical") || im.GetActionValue("ManualCameraMoveLongitudinal") != 0 || im.GetActionValue("ManualCameraRotate") != 0 || m_eSpecEntity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			{
				m_eSpecEntity = null;
				m_aAnimation = null;
				m_bFPPEntityValidityCheck = false;
				
				// Reset camera angle after leaving FPP
				vector mat = pc.m_eCamera.GetAngles();
				pc.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
			}
			else
			{
				m_bFPPEntityValidityCheck = true;
				SlotManagerComponent slotComp = SlotManagerComponent.Cast(m_eSpecEntity.FindComponent(SlotManagerComponent));
				EntitySlotInfo camera = slotComp.GetSlotByName("CRF_FPP");
				vector transform[4];
				camera.GetTransform(transform);
				pc.m_eCamera.SetTransform(transform);
			}
		} else if(!m_eSpecEntity && m_bFPPEntityValidityCheck)
		{
			vector mat = pc.m_eCamera.GetAngles();
			pc.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
		}
		
		foreach(RplId entityID: m_Gamemode.m_aCharacters)
		{
			if (!Replication.FindItem(entityID))
			{
				int index = m_aEntityIcons.Find(entityID);
				if (index == -1)
					continue;
				
				m_aEntityIcons.RemoveOrdered(index);
				delete m_aSpectatorWidgets.Get(index);
				m_aSpectatorWidgets.RemoveOrdered(index);
				m_aSpectatorIcons.RemoveOrdered(index);
				continue;
			}
			
			IEntity entity = RplComponent.Cast(Replication.FindItem(entityID)).GetEntity();
			if (SCR_PlayerController.GetLocalControlledEntity() == entity)
				continue;
			if (m_aEntityIcons.Find(entityID) != -1)
				continue;
			
			
			Widget spectatorIconWidget = GetGame().GetWorkspace().CreateWidgets("{68625BAD23CEE68F}UI/Spectator/SpectatorLabelIconCharacter.layout", FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame")));
			CRF_SpectatorLabelIconCharacter spectatorIcon = CRF_SpectatorLabelIconCharacter.Cast(spectatorIconWidget.FindHandler(CRF_SpectatorLabelIconCharacter));
			spectatorIcon.GetButton().m_OnClicked.Insert(SelectSpecCursor);
			spectatorIcon.SetEntity(entity, "Spine3");
			m_aEntityIcons.Insert(entityID);
			m_aSpectatorIcons.Insert(spectatorIcon);
			m_aSpectatorWidgets.Insert(spectatorIconWidget);
		}
		
		int x;
		int y;
		WidgetManager.GetMousePos(x, y);
		float sX;
		float sY;
		y = GetGame().GetWorkspace().DPIUnscale(y);
		m_wRoot.GetScreenSize(sX, sY);
		float leftSlotX = FrameSlot.GetPosX(m_wFrameSlots);
		float leftSlotY = FrameSlot.GetPosY(m_wFrameSlots);
		float leftVONX = FrameSlot.GetPosX(m_wFrameChannels);
		float leftVONY = FrameSlot.GetPosY(m_wFrameChannels);
		if (x <= leftSlotX + 220 && y >= leftSlotY && y <= leftSlotY + 450)
		{
			leftSlotX += tDelta * 2400.0;
			if (leftSlotX > 0)
				leftSlotX = 0;
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(false);
		}
		else
		{
			leftSlotX -= tDelta * 2400.0;
			if (leftSlotX < -200)
				leftSlotX = -200;
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(true);
		}
		if (x >= leftVONX -20 + sX && y >= leftVONY && y <= leftVONY + 450)
		{
			leftVONX -= tDelta * 2400.0;
			if (leftVONX < -220)
				leftVONX = -220;
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(false);
		}
		else
		{
			leftVONX += tDelta * 2400.0;
			if (leftVONX > -20)
				leftVONX = -20;
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(true);
		}
		
		UpdateIcons();
		if (localSlotChanges != m_Gamemode.m_iSlotChanges)
			UpdateSlots();
		
		if (m_ChatPanel)
        	m_ChatPanel.OnUpdateChat(tDelta);
		
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		sender.SetKillFeedTypeDeadLocal();
		
		// -- BEGIN Port from CRF_GameTimerDisplay.c --	
		if (!CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				if (!factionManager)
					return;
						
			if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
			{
				string faction = SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey();
				
				// This will cause the ui to not update tickets when in spectator!
				if (SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey() != "SPEC") 
				{
					m_wTicketOneText.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
					m_wTicketOneNumber.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
					m_wTicketOneImage.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
					
					switch(faction)
					{			
						case "BLUFOR"	:	{
							if (CRF_Gamemode.GetInstance().m_iBLUFORTickets == -1)
								m_wTicketOneNumber.SetText("INF");
							else
								m_wTicketOneNumber.SetText(CRF_Gamemode.GetInstance().m_iBLUFORTickets.ToString());
							break;
						}
						case "OPFOR"	:	{
							if (CRF_Gamemode.GetInstance().m_iOPFORTickets == -1)
								m_wTicketOneNumber.SetText("INF");
							else		
								m_wTicketOneNumber.SetText(CRF_Gamemode.GetInstance().m_iOPFORTickets.ToString());
							break;
						}
						case "INDFOR"	:	{
							if (CRF_Gamemode.GetInstance().m_iINDFORTickets == -1)
								m_wTicketOneNumber.SetText("INF");
							else
								m_wTicketOneNumber.SetText(CRF_Gamemode.GetInstance().m_iINDFORTickets.ToString());
	
							break;
						}
						case "CIV"	:	{
							if (CRF_Gamemode.GetInstance().m_iCIVTickets == -1)
								m_wTicketOneNumber.SetText("INF");
							else
								m_wTicketOneNumber.SetText(CRF_Gamemode.GetInstance().m_iCIVTickets.ToString());
							break;
						}
					}
				}
			} else {
				m_wTicketOneText.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
				m_wTicketOneNumber.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
				m_wTicketOneImage.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
				if (CRF_Gamemode.GetInstance().m_iBLUFORTickets == -1)
					m_wTicketOneNumber.SetText("INF");
				else
					m_wTicketOneNumber.SetText(CRF_Gamemode.GetInstance().m_iBLUFORTickets.ToString());
				
				m_wTicketTwoText.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
				m_wTicketTwoNumber.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
				m_wTicketTwoImage.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
				if (CRF_Gamemode.GetInstance().m_iOPFORTickets == -1)
					m_wTicketTwoNumber.SetText("INF");
				else
					m_wTicketTwoNumber.SetText(CRF_Gamemode.GetInstance().m_iOPFORTickets.ToString());
				
				m_wTicketThreeText.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
				m_wTicketThreeNumber.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
				m_wTicketThreeImage.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
				if (CRF_Gamemode.GetInstance().m_iINDFORTickets == -1)
					m_wTicketThreeNumber.SetText("INF");
				else
					m_wTicketThreeNumber.SetText(CRF_Gamemode.GetInstance().m_iINDFORTickets.ToString());
				
				m_wTicketFourText.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
				m_wTicketFourNumber.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
				m_wTicketFourImage.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
				if (CRF_Gamemode.GetInstance().m_iCIVTickets == -1)
					m_wTicketFourNumber.SetText("INF");
				else
					m_wTicketFourNumber.SetText(CRF_Gamemode.GetInstance().m_iCIVTickets.ToString());
			}
		}
		
		// get time left in mission
		m_sServerWorldTime = m_GamemodeComponent.GetServerWorldTime();
		
		if (m_GamemodeComponent.GetSafestartStatus() || m_sServerWorldTime.IsEmpty() || m_sStoredServerWorldTime == m_sServerWorldTime) return;
		
		m_sStoredServerWorldTime = m_sServerWorldTime;
		
		// 15m / 5m warning / end warning
		if (m_sServerWorldTime == "00:15:00" || m_sServerWorldTime == "00:05:00" || m_sServerWorldTime == "Mission Time Expired!") {
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			switch (m_sServerWorldTime) {
			 case "00:15:00": {
					m_PopUpNotification.PopupMsg("Mission Ends In 15 Minutes!", 10);
					break;
				};
			 case "00:05:00": {
					m_PopUpNotification.PopupMsg("Mission Ends In 5 Minutes!", 10);
					break;
				};
			 case "Mission Time Expired!": {
					m_PopUpNotification.PopupMsg(m_sServerWorldTime, 10);
					m_wTimer.SetText(m_sServerWorldTime);
					return;
				};
			};
		};
		
		array<string> messageSplitArray = {};
		m_sServerWorldTime.Split(":", messageSplitArray, false);
		
		// If the map isn't open and more than five minutes remaining or no time limit
		if (m_GamemodeComponent.GetSafestartStatus() || ((messageSplitArray[0] != "00" || messageSplitArray[1].ToInt() >= 5) && (!m_MapEntity || !m_MapEntity.IsOpen()))) {
			
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
			
			m_wTicketOneImage.SetOpacity(0);
			m_wTicketOneText.SetOpacity(0);
			m_wTicketOneNumber.SetOpacity(0);
			m_wTicketOneBackground.SetOpacity(0);
			
			m_wTicketOneImage.SetOpacity(0);
			m_wTicketOneText.SetOpacity(0);
			m_wTicketOneNumber.SetOpacity(0);
			m_wTicketOneBackground.SetOpacity(0);
			
			m_wTicketTwoImage.SetOpacity(0);
			m_wTicketTwoText.SetOpacity(0);
			m_wTicketTwoNumber.SetOpacity(0);
			m_wTicketTwoBackground.SetOpacity(0);
			
			m_wTicketThreeImage.SetOpacity(0);
			m_wTicketThreeText.SetOpacity(0);
			m_wTicketThreeNumber.SetOpacity(0);
			m_wTicketThreeBackground.SetOpacity(0);				
			
			m_wTicketFourImage.SetOpacity(0);
			m_wTicketFourText.SetOpacity(0);
			m_wTicketFourNumber.SetOpacity(0);
			m_wTicketFourBackground.SetOpacity(0);
			
			return;
		} else {
			
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
			
			// Check if any of the teams has tickets no point showing if no one has tickets
			if (CRF_Gamemode.GetInstance().m_iBLUFORTickets > -1 || CRF_Gamemode.GetInstance().m_iOPFORTickets > -1 || CRF_Gamemode.GetInstance().m_iINDFORTickets > -1 || CRF_Gamemode.GetInstance().m_iCIVTickets> -1)
			{
				m_wTicketOneImage.SetOpacity(1);
				m_wTicketOneText.SetOpacity(1);
				m_wTicketOneNumber.SetOpacity(1);
				m_wTicketOneBackground.SetOpacity(1);
				if (SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
				{
					m_wTicketOneImage.SetOpacity(1);
					m_wTicketOneText.SetOpacity(1);
					m_wTicketOneNumber.SetOpacity(1);
					m_wTicketOneBackground.SetOpacity(1);
					
					m_wTicketTwoImage.SetOpacity(1);
					m_wTicketTwoText.SetOpacity(1);
					m_wTicketTwoNumber.SetOpacity(1);
					m_wTicketTwoBackground.SetOpacity(1);
					
					m_wTicketThreeImage.SetOpacity(1);
					m_wTicketThreeText.SetOpacity(1);
					m_wTicketThreeNumber.SetOpacity(1);
					m_wTicketThreeBackground.SetOpacity(1);				
					
					m_wTicketFourImage.SetOpacity(1);
					m_wTicketFourText.SetOpacity(1);
					m_wTicketFourNumber.SetOpacity(1);
					m_wTicketFourBackground.SetOpacity(1);
				};
			};
		};
		
		if (messageSplitArray[0] == "00")
			m_sServerWorldTime = string.Format("%1:%2", messageSplitArray[1], messageSplitArray[2]);
		
		m_wTimer.SetText("Mission End: " + m_sServerWorldTime);
		
		switch (true) {
			case (messageSplitArray[0] == "00" && messageSplitArray[1].ToInt() < 15 && messageSplitArray[1].ToInt() > 4): {m_wTimer.SetColorInt(ARGB(255, 230, 230, 0)); break;};
			case (messageSplitArray[0] == "00" && messageSplitArray[1].ToInt() < 5):                                      {m_wTimer.SetColorInt(ARGB(255, 200, 65, 65)); break;};
			default : m_wTimer.SetColorInt(ARGB(255, 215, 215, 215));
		};
		// -- END Port --
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void CreateChannel()
	{
		foreach(string channel: m_Gamemode.m_aVONChannels)
		{
			//"Deafen|1,5,6"
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			string channelName = channelSplit.Get(0);
			if (channelName.Contains(GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId())))
				return;
		}
		pc.CreateChannel();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void HideUI()
	{
		if (m_wRoot.IsVisible())
		{
			m_wRoot.SetVisible(false);
			SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.Cast(pc.GetHUDManagerComponent());
			hudManager.GetHUDRootWidget().SetVisible(false);
		}
		else
		{
			m_wRoot.SetVisible(true);
			SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.Cast(pc.GetHUDManagerComponent());
			hudManager.GetHUDRootWidget().SetVisible(true);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateChannel()
	{
		m_wVONChannels.Clear();
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach(string channel: m_Gamemode.m_aVONChannels)
		{
			//"Deafen|1,5,6"
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			string channelName = channelSplit.Get(0);
			int index = m_wVONChannels.AddItemChannel(null, channelName);
			m_wVONChannels.GetCRFElementComponent(index).m_iChannelId = m_Gamemode.m_aVONChannels.Find(channel);
			m_wVONChannels.GetCRFElementComponent(index).GetChannelButton().m_OnClicked.Insert(JoinChannelDelay);
			ref array<string> playerIds = {};
			if (channelSplit.Count() == 1)
				continue;
			channelSplit.Get(1).Split(",", playerIds, true);
			foreach(string playerId: playerIds)
			{
				if(playerId == "")
					continue;
				if (!pm.IsPlayerConnected(playerId.ToInt()))
					continue;
				if(playerId.ToInt() != SCR_PlayerController.GetLocalPlayerId() && channelName == "Deafen")
					continue;
				int playerIndex = m_wVONChannels.AddItem(pm.GetPlayerName(playerId.ToInt()), null, "{68D74FF57296AFFB}UI/Listbox/PlayerListboxElementVON.layout");
				m_wVONChannels.GetCRFElementComponent(playerIndex).m_iPlayerId = SCR_PlayerController.GetLocalPlayerId();
				m_wVONChannels.GetCRFElementComponent(playerIndex).m_bIsPlayer = true;
			}
		}
		m_iLocalChannelUpdates = m_Gamemode.m_iChannelChanges;
		if (m_Gamemode.GetChannel(SCR_PlayerController.GetLocalPlayerId()) == 0)
			SetRadioPower(false);
		else
		{
			SetRadioPower(true);
		}
		
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void JoinChannelDelay()
	{
		GetGame().GetCallqueue().CallLater(JoinChannel, 10, false);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void JoinChannel()
	{
		CRF_ListBoxElementComponent comp = m_wVONChannels.GetCRFElementComponent(m_wVONChannels.GetSelectedItem());
		if (!comp)
			return;
		
		if (comp.m_iChannelId > 1)
			pc.RequestToJoinChannel(comp.m_iChannelId, SCR_PlayerController.GetLocalPlayerId());
		else
			pc.JoinChannel(SCR_PlayerController.GetLocalPlayerId(), comp.m_iChannelId);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectSpecCursor()
	{
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		if (!cursorWidget)
			return;
		Widget parent = cursorWidget.GetParent();
		if (!parent)
			return;
		CRF_SpectatorLabelIconCharacter icon = CRF_SpectatorLabelIconCharacter.Cast(parent.FindHandler(CRF_SpectatorLabelIconCharacter));
		if (!icon)
			return;
		
		m_eSpecEntity = icon.m_eEntity;
		m_aAnimation = icon.m_eEntity.GetAnimation();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectFactionBlufor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
		UpdateSlots();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectFactionOpfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
		UpdateSlots();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectFactionIndfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
		UpdateSlots();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectFactionCiv()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
		UpdateSlots();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void InitSlots()
	{
		for(int i = 0; i < m_Gamemode.m_aEntitySlots.Count(); i++)
		{
			if(m_Gamemode.m_aSlots.Get(i) == -1 || m_Gamemode.m_aSlots.Get(i) == 0)
				continue;
			
			switch(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(i))).GetEntity()).GetFaction().GetFactionKey())
			{
				case "BLUFOR": 	{m_iBluforSlots++; if(!m_Gamemode.m_aEntityDeathStatus.Get(i)) m_iAliveBluforSlots++; 	break;}
				case "OPFOR": 	{m_iOpforSlots++; if(!m_Gamemode.m_aEntityDeathStatus.Get(i)) m_iAliveOpforSlots++;	break;}
				case "INDFOR": 	{m_iIndforSlots++; if(!m_Gamemode.m_aEntityDeathStatus.Get(i)) m_iAliveIndforSlots++; 	break;}
				case "CIV":		{m_iCivSlots++;	if(!m_Gamemode.m_aEntityDeathStatus.Get(i)) m_iAliveCivSlots++;		break;}
			}
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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
		m_wPlayerSlots.Clear();
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		InitSlots();
		
		ResourceName icon;
		CRF_GamemodeComponent gamemodeComponent = CRF_GamemodeComponent.GetInstance();	
		ResourceName gearScriptResource;
		CRF_GearScriptConfig gearConfig;
		
		if (m_iBluforSlots > 0)
		{
			m_wRoot.FindAnyWidget("BLUButton").SetVisible(true);
			if (gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("BLUFOR");
				if (!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if (gearConfig)
					{
						if (!gearConfig.m_FactionIcon.IsEmpty())
							icon = gearConfig.m_FactionIcon;
					};
				};
			};
			if (icon.IsEmpty())
				icon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("BLUFOR")).GetFactionFlag();
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BLUFlag")).LoadImageTexture(0, icon);
			TextWidget.Cast(m_wRoot.FindAnyWidget("BLURatio")).SetText(m_iAliveBluforSlots.ToString() + "/" + m_iBluforSlots.ToString());
		}
		if (m_iOpforSlots > 0)
		{
			m_wRoot.FindAnyWidget("OPFButton").SetVisible(true);
			if (gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("OPFOR");
				if (!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if (gearConfig)
					{
						if (!gearConfig.m_FactionIcon.IsEmpty())
							icon = gearConfig.m_FactionIcon;
					};
				};
			};
			if (icon.IsEmpty())
				icon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("OPFOR")).GetFactionFlag();
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OPFFlag")).LoadImageTexture(0, icon);
			TextWidget.Cast(m_wRoot.FindAnyWidget("OPFRatio")).SetText(m_iAliveOpforSlots.ToString() + "/" + m_iOpforSlots.ToString());
		}
		if (m_iIndforSlots > 0)
		{
			m_wRoot.FindAnyWidget("INDButton").SetVisible(true);
			if (gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("INDFOR");
				if (!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if (gearConfig)
					{
						if (!gearConfig.m_FactionIcon.IsEmpty())
							icon = gearConfig.m_FactionIcon;
					};
				};
			};
			if (icon.IsEmpty())
				icon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("INDFOR")).GetFactionFlag();
			ImageWidget.Cast(m_wRoot.FindAnyWidget("INDFlag")).LoadImageTexture(0, icon);
			TextWidget.Cast(m_wRoot.FindAnyWidget("INDRatio")).SetText(m_iAliveIndforSlots.ToString() + "/" + m_iIndforSlots.ToString());
		}
		if (m_iCivSlots > 0)
		{
			m_wRoot.FindAnyWidget("CIVButton").SetVisible(true);
			if (gamemodeComponent)
			{	
				gearScriptResource = gamemodeComponent.GetGearScriptResource("CIV");
				if (!gearScriptResource.IsEmpty())
				{
					gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
					if (gearConfig)
					{
						if (!gearConfig.m_FactionIcon.IsEmpty())
							icon = gearConfig.m_FactionIcon;
					};
				};
			};
			if (icon.IsEmpty())
				icon = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("CIV")).GetFactionFlag();
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CIVFlag")).LoadImageTexture(0, icon);
			TextWidget.Cast(m_wRoot.FindAnyWidget("CIVRatio")).SetText(m_iAliveCivSlots.ToString() + "/" + m_iCivSlots.ToString());
		}
		for(int i = 0; i < m_Gamemode.m_aGroupRplIDs.Count(); i++)
		{
			int playersInGroup = 0;
			int deadPlayersInGroup = 0;
			
			if (!Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i)))
				continue;
			
			SCR_AIGroup group = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i))).GetEntity());
			if (group.GetFaction() != m_fSelectedFaction)
				continue;
			if (m_Gamemode.m_aGroupLockedStatus.Get(i))
				continue;
			int groupIndex = m_wPlayerSlots.AddItemSpecGroup(null, group);
			m_wPlayerSlots.GetCRFElementComponent(groupIndex).GetGroupWidget().SetColor(group.GetFaction().GetFactionColor());
			m_wPlayerSlots.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
			m_wPlayerSlots.GetCRFElementComponent(groupIndex).GetGroupIcon().Update(SCR_GroupIdentityComponent.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(i))).GetEntity().FindComponent(SCR_GroupIdentityComponent)).GetMilitarySymbol());
			for(int g = 0; g < m_Gamemode.m_aEntitySlots.Count(); g++)
			{
				RplId currentGroupId = m_Gamemode.m_aPlayerGroupIDs.Get(g);
				
				if (currentGroupId != m_Gamemode.m_aGroupRplIDs.Get(i))
					continue;
				
				if (SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aPlayerGroupIDs.Get(g))).GetEntity()).GetFaction() != m_fSelectedFaction)
					continue;
				
				if (m_Gamemode.m_aSlots.Get(g) == -1)
					continue;
				
				if (m_Gamemode.m_aSlots.Get(g) == -2)
				{
					deadPlayersInGroup++;
					continue;
				}
				
				if (m_Gamemode.m_aSlots.Get(g) == 0 || m_Gamemode.m_aEntityDeathStatus.Get(g) == true)
					continue;
				
				int index = m_wPlayerSlots.AddItemSpecSlot(null , m_Gamemode.m_aEntitySlots.Get(g));
				
				if (m_Gamemode.m_aSlots.Get(g) >= 0)
					playersInGroup++;
				
				m_wPlayerSlots.GetCRFElementComponent(index).GetSlotButton().m_OnClicked.Insert(SelectSpecDelay);	
				
				if (m_Gamemode.m_aSlots.Get(g) > 0)
				{
					if (GetGame().GetPlayerManager().IsPlayerConnected(m_Gamemode.m_aSlots.Get(g)))
						m_wPlayerSlots.GetCRFElementComponent(index).SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(m_Gamemode.m_aSlots.Get(g)));
					else
					{
						m_wPlayerSlots.GetCRFElementComponent(index).SetPlayerText(m_Gamemode.m_aSlotPlayerNames.Get(g));
						m_wPlayerSlots.GetCRFElementComponent(index).GetDisconnectWidget().SetVisible(true);
					}
				}		
			}
			if (playersInGroup == 0)
				m_wPlayerSlots.RemoveItem(groupIndex);
		}
		localSlotChanges = m_Gamemode.m_iSlotChanges;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectSpecDelay()
	{
		GetGame().GetCallqueue().CallLater(SelectSpec, 10, false);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SelectSpec()
	{
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(m_wPlayerSlots.GetElementComponent(m_wPlayerSlots.GetSelectedItem()));
		if (!Replication.FindItem(comp.entityID))
			return;
		
		m_eSpecEntity = RplComponent.Cast(Replication.FindItem(comp.entityID)).GetEntity();
		m_aAnimation = ChimeraCharacter.Cast(m_eSpecEntity).GetAnimation();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{		
		m_wRoot = GetRootWidget();
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		
		m_wRoot.FindAnyWidget("MapFrame").SetVisible(false);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
		GetGame().GetInputManager().RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
		GetGame().GetInputManager().RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
		GetGame().GetInputManager().RemoveActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
		if (GetGame().GetWorkspace().GetOpacity() == 0)
		{
			GetGame().GetWorkspace().SetOpacity(1);
		}
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		sender.SetKillFeedTypeNoneLocal();
	}
	
	// Add player list to spectator
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	
	// Teleporting players camera
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_ManualCameraTeleport()
	{
		MoveCamera(GetCursorWorldPos());
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void MoveCamera(vector worldPosition)
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
			if (teleportComponent)
			{
				teleportComponent.TeleportCamera(worldPosition, true, false);
			}
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	vector GetCursorWorldPos()
	{
		ArmaReforgerScripted game = GetGame();
		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		
		vector worldPos = "0 0 0";
		
		// If map is open return map cursor world position
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			float worldX, worldY;
			mapEntity.GetMapCursorWorldPosition(worldX, worldY);
			worldPos[0] = worldX;
			worldPos[2] = worldY;
			worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
			
			return worldPos;
		}
		
		
		vector cursorPos = GetCursorPos();
		vector outDir;
		vector startPos = workspace.ProjScreenToWorld(cursorPos[0], cursorPos[1], outDir, world, -1);
		outDir *= 1000.0;
	
		autoptr TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = startPos + outDir;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN; 
		
		float traceDis = world.TraceMove(trace, null);
		worldPos = startPos + outDir * traceDis;
		
		return worldPos;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	vector GetCursorPos()
	{
		int cursorX, cursorY;
		if (GetGame().GetInputManager() && GetGame().GetInputManager().IsUsingMouseAndKeyboard())
		{
			WidgetManager.GetMousePos(cursorX, cursorY);
			return Vector(GetGame().GetWorkspace().DPIUnscale(cursorX), GetGame().GetWorkspace().DPIUnscale(cursorY), 0);
		} else {
			return Vector(GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetWidth()/2.0), GetGame().GetWorkspace().DPIUnscale(GetGame().GetWorkspace().GetHeight()/2.0), 0);
		}
	}

	//From RL
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		ref array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if (item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		float multiplier = m_Gamemode.GetChannel(SCR_PlayerController.GetLocalPlayerId());
		RadioHandlerComponent rhc = RadioHandlerComponent.Cast(GetGame().GetPlayerController().FindComponent(RadioHandlerComponent));
		if (rhc)
			rhc.SetFrequency(transiver, 1000*multiplier); // Set new frequency
		return transiver;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetRadioPower(bool input)
	{
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		ref array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if (item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(input);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_VONon()
	{
		if (m_Gamemode.GetChannel(SCR_PlayerController.GetLocalPlayerId()) == 0)
			return;
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(true, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		SCR_VoNComponent von = SCR_VoNComponent.Cast(SCR_PlayerController.GetLocalMainEntity().FindComponent(SCR_VoNComponent));
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	//From reforger lobby <3
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_VONOff()
	{
		if (m_Gamemode.GetChannel(SCR_PlayerController.GetLocalPlayerId()) == 0)
			return;
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(false, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(SCR_PlayerController.GetLocalMainEntity().FindComponent(SCR_VoNComponent));
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateIcons()
	{
		foreach(CRF_SpectatorLabelIconCharacter spectatorIcon: m_aSpectatorIcons)
		{
			spectatorIcon.Update();
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
	
	//More RL code
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Action_ToggleMap()
	{
		if (!m_MapEntity.IsOpen()) 
			OpenMap();
		else 
			CloseMap();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void OpenMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(false);
		
		m_wMapFrame.SetVisible(true);
		
		// WUT?
		SCR_WidgetHelper.RemoveAllChildren(GetRootWidget().FindAnyWidget("ToolMenuHoriz"));
		
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configComp.GetEditorMapConfig(), m_wMapFrame);
		m_MapEntity.OpenMap(mapConfigFullscreen);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void CloseMap()
	{
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(true);
		
		m_wMapFrame.SetVisible(false);
		
		m_MapEntity.CloseMap();
	}
} 