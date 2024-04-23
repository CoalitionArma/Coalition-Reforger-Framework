class COA_GameTimerDisplay : SCR_InfoDisplay
{
	// Get premade game timer widget and background 
	
	protected SCR_MapEntity m_MapEntity;
	protected TextWidget m_wTimer = null;
	protected ImageWidget m_wBackground = null;
	protected CRF_SafestartGameModeComponent m_Safestart = null;
	protected SCR_BaseGameMode m_GameMode;
	protected string m_sTimeLeft;
	protected int m_iCountDown;
	
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		// Main timer, which we wait until safestart is over to begin
		m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		GetGame().GetCallqueue().CallLater(GameLive, 1000, true);
	}
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		// Respawn support
		if (!m_Safestart || !m_wTimer || !m_wBackground)
		{
			m_Safestart = CRF_SafestartGameModeComponent.GetInstance();
			m_wTimer      = TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));
			m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			m_iCountDown = m_Safestart.timeLimitMinutes * 60; // must be in seconds
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			return;
		}
	}	
	
	void GameLive()
	{
		if(!m_GameMode.IsRunning() || m_Safestart.GetSafestartStatus())
			return;
		
		Print("[COA] Safestart: " + m_Safestart.GetSafestartStatus());
		GetGame().GetCallqueue().Remove(GameLive);
		
		// Main timer - runs after safestart
		GetGame().GetCallqueue().CallLater(StartTimer, 1000, true);
	}
	
	void StartTimer()
	{
		Print("[COA] StartTimer");
		--m_iCountDown;
		// get time left in mission 
		m_sTimeLeft = SCR_FormatHelper.FormatTime(m_iCountDown);
		m_wTimer.SetText(m_sTimeLeft);
		PrintFormat("[COA] Timer: %1",m_sTimeLeft);
		
		// if map is on screen
		if (m_MapEntity && m_MapEntity.IsOpen())
		{
			Print("[COA] map open");
			// Display it 
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		} else {
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
		}
	}
}