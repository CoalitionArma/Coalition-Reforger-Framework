class CRF_SafeStartDisplay : SCR_InfoDisplay
{
	protected ImageWidget m_wTimerImage;
	protected TextWidget m_wTimerDescription;
	protected TextWidget m_wTimerText;
	protected TextWidget m_wMissionStart;
	protected TextWidget m_wMissionStart2;
	
	protected ImageWidget m_wFactionsBackground;
	protected TextWidget m_wBlufor;
	protected TextWidget m_wOpfor;
	protected TextWidget m_wIndfor;
	protected TextWidget m_wBluforReady;
	protected TextWidget m_wOpforReady;
	protected TextWidget m_wIndforReady;
	
	protected CRF_SafestartGameModeComponent m_SafestartComponent = null;
	protected SCR_FactionManager m_FactionManager = null;
	
	protected float m_fCurrentOpacity = 0;
	protected bool  m_bAlreadyActivated = false;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		if (!m_SafestartComponent || !m_FactionManager) {
			m_SafestartComponent = CRF_SafestartGameModeComponent.GetInstance();
			m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			return;
		};
		
		if (!m_wTimerImage || !m_wTimerDescription || !m_wTimerText || !m_wMissionStart || !m_wMissionStart2 || !m_wFactionsBackground || !m_wBlufor || !m_wOpfor || !m_wIndfor || !m_wBluforReady || !m_wOpforReady || !m_wIndforReady) {
			m_wTimerImage         = ImageWidget.Cast(m_wRoot.FindWidget("TimerImage"));
			m_wTimerDescription   = TextWidget.Cast(m_wRoot.FindWidget("TimerDescription"));
			m_wTimerText          = TextWidget.Cast(m_wRoot.FindWidget("TimerText"));
			m_wMissionStart       = TextWidget.Cast(m_wRoot.FindWidget("MissionStart"));
			m_wMissionStart2      = TextWidget.Cast(m_wRoot.FindWidget("MissionStart2"));
			m_wFactionsBackground = ImageWidget.Cast(m_wRoot.FindWidget("FactionsBackground"));
			m_wBlufor             = TextWidget.Cast(m_wRoot.FindWidget("Blufor"));
			m_wOpfor              = TextWidget.Cast(m_wRoot.FindWidget("Opfor"));
			m_wIndfor             = TextWidget.Cast(m_wRoot.FindWidget("Indfor"));
			m_wBluforReady        = TextWidget.Cast(m_wRoot.FindWidget("BluforReady"));
			m_wOpforReady         = TextWidget.Cast(m_wRoot.FindWidget("OpforReady"));
			m_wIndforReady        = TextWidget.Cast(m_wRoot.FindWidget("IndforReady"));
		};
		
			
		if (m_fCurrentOpacity > 0)
		{
		 	m_fCurrentOpacity = m_fCurrentOpacity - 0.0025;
			
			m_wMissionStart.SetOpacity(m_fCurrentOpacity);
			m_wMissionStart2.SetOpacity(m_fCurrentOpacity);
		};
		
		if (m_SafestartComponent.GetSafestartStatus()) {
			if (!m_bAlreadyActivated) {
				StopMission();
				m_bAlreadyActivated = true;
			};
			
			UpdatePlayedFactions();
			UpdateTimer();
		};
		
		if (!m_SafestartComponent.GetSafestartStatus() && m_bAlreadyActivated) {
			StartMission();
			m_bAlreadyActivated = false;
		};
	}
	
	//------------------------------------------------------------------------------------------------

	// Additional functions

	//------------------------------------------------------------------------------------------------
	
	protected void UpdatePlayedFactions() 
	{
		array<string> outFactionsReady = m_SafestartComponent.GetWhosReady();
		
		if (!outFactionsReady || outFactionsReady.IsEmpty()) return;
		
		foreach (int i, string factionReady : outFactionsReady) {
			int colorToSet = 0;
			if (factionReady == "#Coal_SS_Faction_Ready")     {colorToSet = ARGB(185, 0, 190, 85);   };
			if (factionReady == "#Coal_SS_Faction_Not_Ready") {colorToSet = ARGB(185, 200, 65, 65);  };
			if (factionReady == "#Coal_SS_No_Faction")       {colorToSet = ARGB(185, 135, 135, 135);};
		
			switch (i) {
				case (0)  : {m_wBluforReady.SetText(factionReady); m_wBluforReady.SetColorInt(colorToSet); break;};
				case (1)  : {m_wOpforReady.SetText(factionReady);  m_wOpforReady.SetColorInt(colorToSet);  break;};
				case (2)  : {m_wIndforReady.SetText(factionReady); m_wIndforReady.SetColorInt(colorToSet); break;};
			};
		}
	}
	
	protected void UpdateTimer()
	{	
		
		m_wTimerText.SetText(m_SafestartComponent.GetServerWorldTime());
	}

	protected void StopMission()
	{
		m_wTimerDescription.SetOpacity(1);
		m_wTimerText.SetOpacity(1);
		m_wTimerImage.SetOpacity(1);

		m_wFactionsBackground.SetOpacity(1);
		m_wBlufor.SetOpacity(1);
		m_wOpfor.SetOpacity(1);
		m_wIndfor.SetOpacity(1);
		m_wBluforReady.SetOpacity(1);
		m_wOpforReady.SetOpacity(1);
		m_wIndforReady.SetOpacity(1);
		
		m_wMissionStart.SetOpacity(0);
		m_wMissionStart2.SetOpacity(0);
		
		m_fCurrentOpacity = 0;
	}
	
	protected void StartMission()
	{
		m_wTimerDescription.SetOpacity(0);
		m_wTimerText.SetOpacity(0);
		m_wTimerImage.SetOpacity(0);
		
		m_wFactionsBackground.SetOpacity(0);
		m_wBlufor.SetOpacity(0);
		m_wOpfor.SetOpacity(0);
		m_wIndfor.SetOpacity(0);
		m_wBluforReady.SetOpacity(0);
		m_wOpforReady.SetOpacity(0);
		m_wIndforReady.SetOpacity(0);
			
		m_wMissionStart.SetOpacity(1);
		m_wMissionStart2.SetOpacity(1);
		
		m_fCurrentOpacity = 1;
	}
}