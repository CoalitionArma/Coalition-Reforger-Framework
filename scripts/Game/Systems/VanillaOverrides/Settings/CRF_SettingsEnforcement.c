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


modded class SCR_BaseGameMode : BaseGameMode
{
	protected override void OnGameModeStart()
	{
		super.OnGameModeStart();
		
		ShadowCheck();
		GrassCheck();
	}
}

modded class SCR_VideoSettingsSubMenu: SCR_SettingsSubMenuBase
{
	override void OnMenuItemChanged(SCR_SettingsBindingBase binding)
	{
		super.OnMenuItemChanged(binding);
		
		ShadowCheck();
		GrassCheck();
	}
};