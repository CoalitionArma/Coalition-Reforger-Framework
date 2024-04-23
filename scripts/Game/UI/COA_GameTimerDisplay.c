class COA_GameTimerDisplay : SCR_InfoDisplay
{
	// Get premade game timer widget and background 
	
	protected SCR_MapEntity m_MapEntity;
	protected TextWidget m_wTimer = null;
	protected ImageWidget m_wBackground = null;
	protected CRF_SafestartGameModeComponent m_Safestart = null;
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		Print("UpdateValues");
		m_Safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (!m_Safestart || !m_Safestart.GetSafestartStatus())
			return;
		
		if (!m_Safestart || m_wTimer || m_wBackground)
		{
			m_Safestart = CRF_SafestartGameModeComponent.GetInstance();
			m_wTimer      = TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));
			return;
		}
		
		Print("All modules found");
		// if map is on screen
		if (m_MapEntity && m_MapEntity.IsOpen())
		{
			Print("map open");
			// Display it 
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		} else {
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
		}
	}	
	
	void TimerLoop(int timeLeft)
	{
		timeLeft--;
		// get time left in mission 
		string timeLeftString = SCR_FormatHelper.FormatTime(timeLeft); // must be in seconds
		m_wTimer.SetText(timeLeftString);
	}
}