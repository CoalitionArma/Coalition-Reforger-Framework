modded class COA_SafestartManager
{
    protected CRF_LoggingManager m_Logging;
    
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only initialize in actual gameplay
		if (!GetGame().InPlayMode())
			return;
		
		if (RplSession.Mode() != RplMode.Client) // Supports both workbench and dedi
			m_Logging = CRF_LoggingManager.Cast(m_Gamemode.FindComponent(CRF_LoggingManager));
	}

	//------------------------------------------------------------------------------------------------
	protected override void ToggleSafeStartServer(bool status)
	{
        super.ToggleSafeStartServer(status);

		if (!status)
		{  // Turn off safestart
			if (!m_bSafeStartEnabled)
				return;

			// Send notification message
			if (m_Logging)
				m_Logging.GameStarted();
		}
	};
}