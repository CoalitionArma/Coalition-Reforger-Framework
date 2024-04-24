class CRF_SearchAndDestroyDisplay : SCR_InfoDisplay
{
	protected string storageString;
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;
	protected CRF_SearchAndDestroyGameModeComponent m_SDComponent = null;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		if (!m_SDComponent || !m_wTimer || !m_wBackground) {
			m_SDComponent = CRF_SearchAndDestroyGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGameModeComponent));
			m_wTimer      = TextWidget.Cast(m_wRoot.FindWidget("Timer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("Background"));
			return;
		};
		
		if(!m_SDComponent.m_sMessageContent.IsEmpty() && m_SDComponent.m_sMessageContent.IsDigitAt(0)) {
			array<string> messageSplitArray = {};
			m_SDComponent.m_sMessageContent.Split(":", messageSplitArray, false);
			string stringToDisplay = string.Format("%1:%2", messageSplitArray[1], messageSplitArray[2]);
			
			if (storageString != stringToDisplay) {
				m_wTimer.SetText(stringToDisplay);
				m_wTimer.SetOpacity(1);
				m_wBackground.SetOpacity(1);
				
				storageString = stringToDisplay;
				
				switch (true) {
					case (stringToDisplay == "01:00"): {
						if (m_SDComponent.aSitePlanted)
							SCR_PopUpNotification.GetInstance().PopupMsg("Site A will be destroyed in 1 minute!", 10);
						else
							SCR_PopUpNotification.GetInstance().PopupMsg("Site B will be destroyed in 1 minute!", 10);
						
						AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
						break;
					};
					
					case (stringToDisplay == "00:30" || stringToDisplay == "00:15" || stringToDisplay == "00:05" || stringToDisplay == "00:04" || stringToDisplay == "00:03" || stringToDisplay == "00:02" || stringToDisplay == "00:01" || stringToDisplay == "00:00"): {
						AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
						break;
					};
				}
			};
		} else {
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);		
		};
	}	
}