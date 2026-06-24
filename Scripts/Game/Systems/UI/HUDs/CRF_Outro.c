modded enum ChimeraMenuPreset
{
	CRF_Outro
}

class CRF_Outro: ChimeraMenuBase
{
	protected static const ResourceName STATS_LAYOUT = "{7CDA3F81B4920E56}UI/layouts/HUD/Intro/CRF_AARStats.layout";
	protected Widget              m_wAARStatsRoot;
	protected ref CRF_AARStatsHUD m_pAARStatsHUD;
	protected float               m_fStatsFadeElapsed;
	protected bool                m_bFadingStats;

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
		GetGame().GetCallqueue().CallLater(CreateStatsPanel, 5000, false);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		CRF_Gamemode.GetInstance().m_bIsInEndCredits = true;
	}
	
	override void OnMenuClose()
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetCallqueue().Remove(CreateStatsPanel);
		if (m_pAARStatsHUD)
		{
			m_pAARStatsHUD.Cleanup();
			m_pAARStatsHUD = null;
		}
		if (m_wAARStatsRoot)
		{
			m_wAARStatsRoot.RemoveFromHierarchy();
			m_wAARStatsRoot = null;
		}
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_Outro);
	}
	
	void SubTitle()
	{
		// Show outcome text if a winning faction was set
		CRF_RplBroadcastManager rplBroadcast = CRF_RplBroadcastManager.GetInstance();
		if (rplBroadcast && rplBroadcast.m_sOutroWinningFaction != "")
		{
			TextWidget outcomeTxt = TextWidget.Cast(GetRootWidget().FindAnyWidget("TitleText3"));
			if (outcomeTxt)
			{
				outcomeTxt.SetText(GetOutcomeText(rplBroadcast.m_sOutroWinningFaction));
				outcomeTxt.SetVisible(true);
			}
		}

		GetRootWidget().FindAnyWidget("TitleText1").SetVisible(true);
		GetRootWidget().FindAnyWidget("TitleText2").SetVisible(true);
	}

	protected string GetOutcomeText(string faction)
	{
		if (faction == "BLUFOR")  return "BLUFOR VICTORY";
		if (faction == "OPFOR")   return "OPFOR VICTORY";
		if (faction == "INDFOR")  return "INDFOR VICTORY";
		if (faction == "CIV")     return "CIVILIAN VICTORY";
		return "";
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		if (m_bFadingStats && m_wAARStatsRoot)
		{
			m_fStatsFadeElapsed += tDelta;
			float opacity = Math.Clamp(m_fStatsFadeElapsed / 2.0, 0.0, 1.0);
			m_wAARStatsRoot.SetOpacity(opacity);
			if (opacity >= 1.0)
				m_bFadingStats = false;
		}
	}
	
	protected void CreateStatsPanel()
	{
		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (!hudManager)
			return;

		m_wAARStatsRoot = hudManager.CreateLayout(STATS_LAYOUT, EHudLayers.ALWAYS_TOP, 0);
		if (!m_wAARStatsRoot)
			return;

		m_wAARStatsRoot.SetOpacity(0);
		m_pAARStatsHUD = new CRF_AARStatsHUD(m_wAARStatsRoot);
		m_fStatsFadeElapsed = 0;
		m_bFadingStats = true;
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