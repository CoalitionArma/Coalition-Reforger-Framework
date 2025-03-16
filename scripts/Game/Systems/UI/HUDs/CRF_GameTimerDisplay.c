class CRF_GameTimerDisplay : SCR_InfoDisplay
{
	// Get premade game timer widget and background 
	
	protected SCR_MapEntity m_MapEntity;
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
	
	//------------------------------------------------------------------------------------------------
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		GetGame().GetCallqueue().CallLater(UpdateTimer, 1000, true);
	};

	//------------------------------------------------------------------------------------------------
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		// Respawn support
		if (!m_GamemodeComponent || !m_wTimer || !m_wBackground || !m_MapEntity) 
		{
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
			
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			return;
		};
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTimer()
	{	
		if (!m_GamemodeComponent || !m_wTimer || !m_wBackground || !m_MapEntity || SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et") return;
		
		if(!m_GamemodeComponent.m_bHUDVisible)
		{
			m_wTimer.SetVisible(false);
			m_wBackground.SetVisible(false);
			m_wTicketOneImage.SetVisible(false);
			m_wTicketOneText.SetVisible(false);
			m_wTicketOneNumber.SetVisible(false);
			m_wTicketOneBackground.SetVisible(false);
			return;
		} else {
			m_wTimer.SetVisible(true);
			m_wBackground.SetVisible(true);
			m_wTicketOneImage.SetVisible(true);
			m_wTicketOneText.SetVisible(true);
			m_wTicketOneNumber.SetVisible(true);
			m_wTicketOneBackground.SetVisible(true);
		}
		
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_GamemodeState.GAME && !CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
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
		
		if (m_sServerWorldTime == "N/A") {
			GetGame().GetCallqueue().Remove(UpdateTimer);
			return;
		};
		
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
					GetGame().GetCallqueue().Remove(UpdateTimer);
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
	}
}
