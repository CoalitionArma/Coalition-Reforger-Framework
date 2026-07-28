modded class COA_Outro
{
    protected ref CRF_AARStatsHUD m_pAARStatsHUD;
    
	override void OnMenuOpen()
	{
        super.OnMenuOpen();
        GetGame().GetCallqueue().CallLater(CreateStatsPanel, 5000, false);
    }

	override void OnMenuClose()
	{
		super.OnMenuClose();

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
}