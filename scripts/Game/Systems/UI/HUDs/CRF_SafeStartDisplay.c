class CRF_SafeStartDisplay : SCR_InfoDisplay
{
	protected ImageWidget m_wTimerImage;
	protected TextWidget m_wTimerDescription;
	protected TextWidget m_wTimerText;
	protected TextWidget m_wMissionStart;
	protected TextWidget m_wMissionStart2;
	
	protected ImageWidget m_wFactionsBackground;
	protected OverlayWidget m_wBluforFrame;
	protected OverlayWidget m_wOpforFrame;
	protected OverlayWidget m_wIndforFrame;
	protected OverlayWidget m_wCivFrame;
	protected TextWidget m_wBluforReady;
	protected TextWidget m_wOpforReady;
	protected TextWidget m_wIndforReady;
	protected TextWidget m_wCivReady;
	protected PanelWidget m_wFactionsPanel;
	
	protected CRF_GamemodeComponent m_GamemodeComponent = null;
	protected SCR_FactionManager m_FactionManager = null;
	
	protected float m_fCurrentOpacity = 0;
	protected bool  m_bAlreadyActivated = false;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		if (!m_GamemodeComponent || !m_FactionManager || !m_wTimerImage || !m_wTimerDescription || !m_wTimerText || !m_wMissionStart || !m_wMissionStart2 || !m_wFactionsBackground || !m_wBluforFrame || !m_wOpforFrame || !m_wIndforFrame || !m_wBluforReady || !m_wOpforReady || !m_wIndforReady || !m_wCivFrame || !m_wCivReady || !m_wFactionsPanel) 
		{
			m_GamemodeComponent = CRF_GamemodeComponent.GetInstance();
			m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			m_wTimerImage         = ImageWidget.Cast(m_wRoot.FindAnyWidget("TimerImage"));
			m_wTimerDescription   = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerDescription"));
			m_wTimerText          = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerText"));
			m_wMissionStart       = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart"));
			m_wMissionStart2      = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart2"));
			m_wFactionsBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("FactionsBackground"));
			m_wBluforFrame        = OverlayWidget.Cast(m_wRoot.FindAnyWidget("BluforFrame"));
			m_wOpforFrame         = OverlayWidget.Cast(m_wRoot.FindAnyWidget("OpforFrame"));
			m_wIndforFrame        = OverlayWidget.Cast(m_wRoot.FindAnyWidget("IndforFrame"));
			m_wCivFrame           = OverlayWidget.Cast(m_wRoot.FindAnyWidget("CivFrame"));
			m_wBluforReady        = TextWidget.Cast(m_wRoot.FindAnyWidget("BluforReady"));
			m_wOpforReady         = TextWidget.Cast(m_wRoot.FindAnyWidget("OpforReady"));
			m_wIndforReady        = TextWidget.Cast(m_wRoot.FindAnyWidget("IndforReady"));
			m_wCivReady           = TextWidget.Cast(m_wRoot.FindAnyWidget("CivReady"));
			m_wFactionsPanel      = PanelWidget.Cast(m_wRoot.FindAnyWidget("FactionsPanel"));
			return;
		};
		
		if(!m_GamemodeComponent.m_bHUDVisible)
		{
			m_wTimerDescription.SetVisible(false);
			m_wTimerText.SetVisible(false);
			m_wTimerImage.SetVisible(false);
		
			m_wFactionsBackground.SetVisible(false);
			m_wFactionsPanel.SetVisible(false);
			m_wBluforFrame.SetVisible(false);
			m_wOpforFrame.SetVisible(false);
			m_wIndforFrame.SetVisible(false);
			m_wCivFrame.SetVisible(false);
			
			m_wMissionStart.SetVisible(false);
			m_wMissionStart2.SetVisible(false);
			return;
		} else {
			m_wTimerDescription.SetVisible(true);
			m_wTimerText.SetVisible(true);
			m_wTimerImage.SetVisible(true);
		
			m_wFactionsBackground.SetVisible(true);
			m_wFactionsPanel.SetVisible(true);
			
			m_wMissionStart.SetVisible(true);
			m_wMissionStart2.SetVisible(true);
		};
		
		if (m_fCurrentOpacity > 0)
		{
		 	m_fCurrentOpacity = m_fCurrentOpacity - 0.0025;
			
			m_wMissionStart.SetOpacity(m_fCurrentOpacity);
			m_wMissionStart2.SetOpacity(m_fCurrentOpacity);
		};
		
		if (m_GamemodeComponent.GetSafestartStatus()) {
			if (!m_bAlreadyActivated) {
				StopMission();
				m_bAlreadyActivated = true;
			};
			
			UpdatePlayedFactions();
			UpdateTimer();
		};
		
		if (!m_GamemodeComponent.GetSafestartStatus() && m_bAlreadyActivated) {
			StartMission();
			m_bAlreadyActivated = false;
		};
	}
	
	//------------------------------------------------------------------------------------------------

	// Additional functions

	//------------------------------------------------------------------------------------------------
	
	protected void UpdatePlayedFactions() 
	{
		array<string> outFactionsReady = m_GamemodeComponent.GetWhosReady();
		
		if (!outFactionsReady || outFactionsReady.IsEmpty()) return;
		
		foreach (int i, string factionReady : outFactionsReady) {
			int colorToSet = 0;
			if (factionReady == "#Coal_SS_Faction_Ready")     {colorToSet = ARGB(185, 0, 190, 85);   };
			if (factionReady == "#Coal_SS_Faction_Not_Ready") {colorToSet = ARGB(185, 200, 65, 65);  };
			if (factionReady == "#Coal_SS_No_Faction")       {colorToSet = ARGB(185, 135, 135, 135); };
		
			switch (i) {
				case (0)  : {
					m_wBluforReady.SetText(factionReady); 
					m_wBluforReady.SetColorInt(colorToSet); 
					
					if(factionReady != "#Coal_SS_No_Faction")
						m_wBluforFrame.SetVisible(true); 
					else
						m_wBluforFrame.SetVisible(false);
					 
					break;
				};
				case (1)  : {
					m_wOpforReady.SetText(factionReady);  
					m_wOpforReady.SetColorInt(colorToSet);  
					
					if(factionReady != "#Coal_SS_No_Faction")
						m_wOpforFrame.SetVisible(true); 
					else
						m_wOpforFrame.SetVisible(false); 
					
					break;
				};
				case (2)  : {
					m_wIndforReady.SetText(factionReady); 
					m_wIndforReady.SetColorInt(colorToSet);
					 
					if(factionReady != "#Coal_SS_No_Faction")
						m_wIndforFrame.SetVisible(true); 
					else
						m_wIndforFrame.SetVisible(false); 
					
					break;
				};
				case (3)  : {
					m_wCivReady.SetText(factionReady);    
					m_wCivReady.SetColorInt(colorToSet);    
					
					if(factionReady != "#Coal_SS_No_Faction")
						m_wCivFrame.SetVisible(true); 
					else
						m_wCivFrame.SetVisible(false); 
					
					break;
				};
			};
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateTimer()
	{	
		
		m_wTimerText.SetText(m_GamemodeComponent.GetServerWorldTime());
	}

	//------------------------------------------------------------------------------------------------
	protected void StopMission()
	{
		m_wTimerDescription.SetOpacity(1);
		m_wTimerText.SetOpacity(1);
		m_wTimerImage.SetOpacity(1);

		m_wFactionsBackground.SetOpacity(1);
		m_wFactionsPanel.SetOpacity(1);
		
		m_wMissionStart.SetOpacity(0);
		m_wMissionStart2.SetOpacity(0);
		
		m_fCurrentOpacity = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void StartMission()
	{
		m_wTimerDescription.SetOpacity(0);
		m_wTimerText.SetOpacity(0);
		m_wTimerImage.SetOpacity(0);
		
		m_wFactionsBackground.SetOpacity(0);
		m_wFactionsPanel.SetOpacity(0);
		m_wBluforFrame.SetVisible(false);
		m_wOpforFrame.SetVisible(false);
		m_wIndforFrame.SetVisible(false);
		m_wCivFrame.SetVisible(false);
			
		m_wMissionStart.SetOpacity(1);
		m_wMissionStart2.SetOpacity(1);
		
		m_fCurrentOpacity = 1;
	}
}