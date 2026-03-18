modded enum ChimeraMenuPreset
{
	CRF_Outro
}

class CRF_Outro: ChimeraMenuBase
{
	string SanitizeMissionName(string fullName)
	{
	    array<string> parts = {};
		
	    fullName.Split(" ", parts, true);
	
	    // Remove the first two tokens like "CRF" and "CO50"/"COTVT55"
	    if (parts.Count() > 2)
	    {
	        string cleanName;
	        for (int i = 2; i < parts.Count(); i++)
	        {
	            if (i > 2)
	                cleanName += " ";
	            cleanName += parts[i];
	        }
			cleanName.ToUpper();
	        return cleanName;
	    }
	
		fullName.ToUpper();
	    return fullName; // fallback if unexpected format
	}
	
	override void OnMenuOpen()
	{
		TextWidget.Cast(GetRootWidget().FindAnyWidget("TitleText")).SetText(SanitizeMissionName(GetGame().GetMissionName()));
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		GetGame().GetCallqueue().CallLater(SubTitle, 3000, false);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		CRF_Gamemode.GetInstance().m_bIsInEndCredits = true;
	}
	
	override void OnMenuClose()
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_Outro);
	}
	
	void SubTitle()
	{
		GetRootWidget().FindAnyWidget("TitleText1").SetVisible(true);
		GetRootWidget().FindAnyWidget("TitleText2").SetVisible(true);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
	}
	
	void Action_Exit()
	{
		// Note: Opening pause menu instead of directly exiting the game
		// because players often accidentally exit the game
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}