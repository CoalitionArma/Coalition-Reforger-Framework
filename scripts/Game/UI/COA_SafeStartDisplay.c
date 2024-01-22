class COA_SafeStartDisplay : SCR_InfoDisplay
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
	
	protected CRF_TNK_SafestartComponent m_SafestartComponent = null;
	protected SCR_FactionManager m_FactionManager = null;
	
	protected float m_fCurrentOpacity = 0;
	protected bool  m_bAlreadyActivated = false;
	protected int m_iTimeSafeStartBegan;

	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------

	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		GetGame().GetInputManager().ActivateContext("CoalitionReforgerFrameworkContext", 36000000);
		GetGame().GetInputManager().AddActionListener("ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
	}
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		if (!m_SafestartComponent || !m_FactionManager) {
			m_SafestartComponent = CRF_TNK_SafestartComponent.GetInstance();
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
				m_wOpforReady.SetText("N/A");
				m_wIndforReady.SetText("N/A");
				m_wBluforReady.SetText("N/A");
				m_wOpforReady.SetColorInt(ARGB(185, 135, 135, 135));
				m_wIndforReady.SetColorInt(ARGB(185, 135, 135, 135));
				m_wBluforReady.SetColorInt(ARGB(185, 135, 135, 135));
				m_bBluforReady = false;
				m_bOpforReady = false;
				m_bIndforReady = false;
				m_bAlreadyActivated = true;
			};
			
			if (m_iTimeSafeStartBegan == -1 && !m_bAlreadyActivated) {
		  		m_iTimeSafeStartBegan = m_SafestartComponent.GetTimeSafeStartBegan();
			};
			
			UpdatePlayedFactions();
			UpdateTimer();
		};
		
		if (!m_SafestartComponent.GetSafestartStatus() && m_bAlreadyActivated) {
			StartMission();
			m_bAlreadyActivated = false;
			m_iTimeSafeStartBegan = -1;
		};
	}
	
	//------------------------------------------------------------------------------------------------

	// Additional functions

	//------------------------------------------------------------------------------------------------
	
	protected void UpdatePlayedFactions() 
	{
		array<bool> outFactionsReady = m_SafestartComponent.GetWhosReady();
		
		m_bBluforReady = outFactionsReady[0];
		m_bOpforReady = outFactionsReady[1];
		m_bIndforReady = outFactionsReady[2];
		
		SCR_SortedArray<SCR_Faction> outFaction = new SCR_SortedArray<SCR_Faction>;
		m_FactionManager.GetSortedFactionsList(outFaction);
		
		if (!outFaction || outFaction.IsEmpty()) return;
		
		array<SCR_Faction> outArray = new array<SCR_Faction>;
		outFaction.ToArray(outArray);
		
		foreach(SCR_Faction faction : outArray) {
			if (faction.GetPlayerCount() == 0) continue;
			
			Color factionColor = faction.GetOutlineFactionColor();
			float rg = Math.Max(factionColor.R(), factionColor.G());
			float rgb = Math.Max(rg, factionColor.B());
			
			switch (true) {
				case(!m_bOpforReady && rgb == factionColor.R())  : {m_wOpforReady.SetText("Not Ready");  m_wOpforReady.SetColorInt(ARGB(185, 200, 65, 65));  break;};
				case(m_bOpforReady && rgb == factionColor.R())   : {m_wOpforReady.SetText("Ready");      m_wOpforReady.SetColorInt(ARGB(185, 0, 190, 85));   break;};
				case(!m_bIndforReady && rgb == factionColor.G()) : {m_wIndforReady.SetText("Not Ready"); m_wIndforReady.SetColorInt(ARGB(185, 200, 65, 65)); break;};
				case(m_bIndforReady && rgb == factionColor.G())  : {m_wIndforReady.SetText("Ready");     m_wIndforReady.SetColorInt(ARGB(185, 0, 190, 85));  break;};
				case(!m_bBluforReady && rgb == factionColor.B()) : {m_wBluforReady.SetText("Not Ready"); m_wBluforReady.SetColorInt(ARGB(185, 200, 65, 65)); break;};
				case(m_bBluforReady && rgb == factionColor.B())  : {m_wBluforReady.SetText("Ready");     m_wBluforReady.SetColorInt(ARGB(185, 0, 190, 85));  break;};
			};
		};
	}
	
	protected void UpdateTimer()
	{	
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
		
    int totalSeconds = (millis / 1000);
		
		m_wTimerText.SetText(SCR_FormatHelper.FormatTime(totalSeconds));
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
	
	void ToggleSideReady() 
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if(!groupManager) return;
		SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		if(!playersGroup) return;
		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId())) 
			COA_SafeStartPlayerComponent.GetInstance().Owner_ToggleSideReady();
	}
}