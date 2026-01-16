void ShadowCheck()
{
	int shadows;
	int dshadows;
	UserSettings m_Pipeline = GetGame().GetEngineUserSettings().GetModule("PipelineUserSettings");
	
	m_Pipeline.Get("ShadowQuality", shadows);
	if (shadows < 2) {
		m_Pipeline.Set("ShadowQuality", 2);
		GetGame().UserSettingsChanged();
	};
	
	GetGame().GetEngineUserSettings().GetModule("VideoUserSettings").Get("DistantShadowsQuality",dshadows);
	if (dshadows < 2) {
		GetGame().GetEngineUserSettings().GetModule("VideoUserSettings").Set("DistantShadowsQuality", 2);
		GetGame().UserSettingsChanged();
	};
}

void GrassCheck()
{
	int grassDistance;
	int grassQual;
	int geomDetail;
	
	GetGame().GetEngineUserSettings().GetModule("GrassMaterialSettings").Get("Lod",grassQual);
	GetGame().GetEngineUserSettings().GetModule("GrassMaterialSettings").Get("Distance",grassDistance);
	GetGame().GetEngineUserSettings().GetModule("ResourceManagerUserSettings").Get("GeometricDetail",geomDetail);
	
	if (grassQual != 3) {
		GetGame().GetEngineUserSettings().GetModule("GrassMaterialSettings").Set("Lod",3);
		GetGame().UserSettingsChanged();
	};
	
	if (grassDistance < 300)
	{
		GetGame().GetEngineUserSettings().GetModule("GrassMaterialSettings").Set("Distance",300);
		GetGame().UserSettingsChanged();
	}
	
	if (geomDetail < 2)
	{
		GetGame().GetEngineUserSettings().GetModule("ResourceManagerUserSettings").Set("GeometricDetail",2);
		GetGame().UserSettingsChanged();
	}
}

void GammaBrightnessCheck()
{
	// Default values for gamma, brightness, and contrast
	const float DEFAULT_GAMMA = 1.0;
	const float DEFAULT_BRIGHTNESS = 1.0;
	const float DEFAULT_CONTRAST = 1.0;
	
	// Tolerance for float comparison
	const float TOLERANCE = 0.01;
	
	// Get current values
	float currentGamma = System.GetFinalImageGamma();
	float currentBrightness = System.GetFinalImageBrightness();
	float currentContrast = System.GetFinalImageContrast();
	
	// Check if any value is different from default
	bool needsReset = false;
	string violationType = "";
	
	if (Math.AbsFloat(currentGamma - DEFAULT_GAMMA) > TOLERANCE)
	{
		needsReset = true;
		violationType = string.Format("Gamma: %.2f (Default: %.2f)", currentGamma, DEFAULT_GAMMA);
	}
	
	if (Math.AbsFloat(currentBrightness - DEFAULT_BRIGHTNESS) > TOLERANCE)
	{
		needsReset = true;
		if (!violationType.IsEmpty())
			violationType += ", ";
		violationType += string.Format("Brightness: %.2f (Default: %.2f)", currentBrightness, DEFAULT_BRIGHTNESS);
	}
	
	if (Math.AbsFloat(currentContrast - DEFAULT_CONTRAST) > TOLERANCE)
	{
		needsReset = true;
		if (!violationType.IsEmpty())
			violationType += ", ";
		violationType += string.Format("Contrast: %.2f (Default: %.2f)", currentContrast, DEFAULT_CONTRAST);
	}
	
	// Reset to default and notify admins if needed
	if (needsReset)
	{
		System.SetFinalImageAttributes(DEFAULT_GAMMA, DEFAULT_BRIGHTNESS, DEFAULT_CONTRAST);
		
		// Get player information
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
		{
			int playerId = playerController.GetPlayerId();
			
			// Send violation report to server via RPC
			CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
			if (rplManager)
				rplManager.ReportSettingsViolation(playerId, violationType);
		}
	}
}


modded class SCR_BaseGameMode : BaseGameMode
{
	protected override void OnGameModeStart()
	{
		super.OnGameModeStart();
		
		ShadowCheck();
		GrassCheck();
		//GammaBrightnessCheck();
	}
}

modded class SCR_VideoSettingsSubMenu: SCR_SettingsSubMenuBase
{
	override void OnMenuItemChanged(SCR_SettingsBindingBase binding)
	{
		super.OnMenuItemChanged(binding);
		
		ShadowCheck();
		GrassCheck();
		//GammaBrightnessCheck();
	}
};