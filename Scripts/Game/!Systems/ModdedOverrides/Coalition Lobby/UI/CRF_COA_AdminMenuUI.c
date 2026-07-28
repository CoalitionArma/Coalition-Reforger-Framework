modded class COA_AdminMenu
{
    //------------------------------------------------------------------------------------------------
    override void InitializeGamemodeMenu()
	{
		super.InitializeGamemodeMenu();
		
        //Toggle VAAR Recording
		SCR_ButtonTextComponent toggleVAARRecording = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("ToggleVAARButton").FindHandler(SCR_ButtonTextComponent));
		toggleVAARRecording.m_OnClicked.Insert(ToggleVAARRecording);
    }

    //------------------------------------------------------------------------------------------------
	void ToggleVAARRecording()
	{
		COA_PlayerRplToAuthorityManager.GetInstance().ToggleVAARRecording();
	}

    //------------------------------------------------------------------------------------------------
    override void GamemodeMenuUpdate()
	{
		super.GamemodeMenuUpdate();
		
		bool m_bVAARRecordingEnabled = CRF_VAAR_GamemodeComponent.GetInstance().m_bRecording;
		Widget VAARRecordingEnabledButton = m_wMenuContent.FindAnyWidget("ToggleVAARButton");
		TextWidget VAARRecordingEnabledText = TextWidget.Cast(VAARRecordingEnabledButton.FindWidget("ActionButtonText"));
		if (m_bVAARRecordingEnabled)
		{
			VAARRecordingEnabledText.SetText("Recording Enabled");
			VAARRecordingEnabledText.SetColorInt(Color.GREEN);
		}
		else
		{
			VAARRecordingEnabledText.SetText("Recording Disabled");
			VAARRecordingEnabledText.SetColorInt(Color.RED);
        };
    }
}