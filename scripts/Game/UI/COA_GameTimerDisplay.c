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
		
		GetGame().GetCallqueue().Remove(GameLive);
		
		// Main timer - runs after safestart
		GetGame().GetCallqueue().CallLater(StartTimer, 1000, true);
	}
	
	void StartTimer()
	{
		--m_iCountDown;
		// get time left in mission 
		m_sTimeLeft = SCR_FormatHelper.FormatTime(m_iCountDown);
		m_wTimer.SetText(m_sTimeLeft);
		
		// if map is on screen
		if (m_MapEntity && m_MapEntity.IsOpen())
		{
			// Display it 
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		} else {
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
		}
			
		// 15m / 5m warning	/ end warning													
		if (m_iCountDown == 900 || m_iCountDown == 300 || m_iCountDown == 0)
		{
			// Change timer text to red
			m_wTimer.SetColor(Color.Red);
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			switch (m_iCountDown) 
			{
				case 900:
					SCR_PopUpNotification.GetInstance().PopupMsg("15 minute warning!", 5);
					return;
				case 300:
					SCR_PopUpNotification.GetInstance().PopupMsg("5 minute warning!", 5);
					return;
				case 0:
					SCR_PopUpNotification.GetInstance().PopupMsg("Mission time expired!", 5);
					return;
				default: {}
			}
			
		}
		
		// Mission timer up
		if (m_iCountDown <= 0)
		{
			m_wTimer.SetText("Mission Time Expired");
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		}
	}
}