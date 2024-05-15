class CRF_GameTimerDisplay : SCR_InfoDisplay
{
	// Get premade game timer widget and background 
	
	protected SCR_MapEntity m_MapEntity;
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;
	protected CRF_SafestartGameModeComponent m_Safestart;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	
	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		GetGame().GetCallqueue().CallLater(UpdateTimer, 250, true);
	}

	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		// Respawn support
		if (!m_Safestart || !m_wTimer || !m_wBackground || !m_MapEntity) 
		{
			m_Safestart = CRF_SafestartGameModeComponent.GetInstance();
			m_wTimer      = TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			return;
		};
	}
	
	void UpdateTimer()
	{
		if (!m_Safestart || !m_wTimer || !m_wBackground || !m_MapEntity) return;
		
		// get time left in mission
		m_sServerWorldTime = m_Safestart.GetServerWorldTime();
		
		if (m_sServerWorldTime == "N/A") {
			GetGame().GetCallqueue().Remove(UpdateTimer);
			return;
		};
		
		if (m_Safestart.GetSafestartStatus() || m_sServerWorldTime.IsEmpty() || m_sStoredServerWorldTime == m_sServerWorldTime) return;
		
		m_sStoredServerWorldTime = m_sServerWorldTime;
		
		// 15m / 5m warning / end warning
		if (m_sServerWorldTime == "00:15:00" || m_sServerWorldTime == "00:05:00" || m_sServerWorldTime == "Mission Time Expired!") {
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			switch (m_sServerWorldTime) {
			 case "00:15:00": {
					SCR_PopUpNotification.GetInstance().PopupMsg("Mission Ends In 15 Minutes!", 10);
					break;
				};
			 case "00:05:00": {
					SCR_PopUpNotification.GetInstance().PopupMsg("Mission Ends In 5 Minutes!", 10);
					break;
				};
			 case "Mission Time Expired!": {
					GetGame().GetCallqueue().Remove(UpdateTimer);
					SCR_PopUpNotification.GetInstance().PopupMsg(m_sServerWorldTime, 10);
					m_wTimer.SetText(m_sServerWorldTime);
					return;
				};
			};
		};
		
		array<string> messageSplitArray = {};
		m_sServerWorldTime.Split(":", messageSplitArray, false);
		
		// If the map isn't open and more than five minutes remaining or no time limit
		if (m_Safestart.GetSafestartStatus() || (messageSplitArray[1].ToInt() >= 5 && (!m_MapEntity || !m_MapEntity.IsOpen()))) {
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
			return;
		} else {
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		};
		
		if (messageSplitArray[0] == "00")
			m_sServerWorldTime = string.Format("%1:%2", messageSplitArray[1], messageSplitArray[2]);
		
		m_wTimer.SetText("Mission End: " + m_sServerWorldTime);
		
		switch (true) {
			case (messageSplitArray[1].ToInt() < 15 && messageSplitArray[1].ToInt() > 4): {m_wTimer.SetColorInt(ARGB(255, 230, 230, 0)); break;};
			case (messageSplitArray[1].ToInt() < 5):                                      {m_wTimer.SetColorInt(ARGB(255, 200, 65, 65)); break;};
		};
	}
}