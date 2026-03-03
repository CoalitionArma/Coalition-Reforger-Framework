class CRF_ResourceManagerClass : ScriptComponentClass {}

class CRF_ResourceManager : ScriptComponent
{
	static ref CRF_GearScriptRolesConfig m_RolesConfig;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		LoadConfigurations();
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GearScriptRolesConfig RolesConfig()
	{
		return m_RolesConfig;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load necessary configurations for gearscript
	 */
	protected void LoadConfigurations()
	{
		ResourceName rolesConfigPath;
		if (!CVON_VONGameModeComponent.GetInstance())
			  rolesConfigPath = "{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf";
		else
			rolesConfigPath = "{F04F02DBFC65553E}Configs/Gearscripts/Additional Configs/CRF_CVON_Global_Roles_Config.conf";
		
		m_RolesConfig = CRF_GearScriptRolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_ResourceManager m_sInstance;
	void CRF_ResourceManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_ResourceManager GetInstance()
	{
		return m_sInstance;
	}
}