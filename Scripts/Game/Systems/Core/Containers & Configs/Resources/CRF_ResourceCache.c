class CRF_ResourceCache
{
	// Resource caching for optimized spawning
	protected ref map<ResourceName, Resource> m_mCachedResources = new map<ResourceName, Resource>();
	
	//------------------------------------------------------------------------------------------------
	void PreLoadSlottingResources()
	{
		CRF_GamemodeManager gamemodeManager = CRF_GamemodeManager.GetInstance();
		if (!gamemodeManager || !GetGame().InPlayMode())
			return;
		
		CRF_RolesConfig rolesConfig = gamemodeManager.RolesConfig();
		
		foreach(CRF_EGearRole role, CRF_RoleConfig roleConfig : rolesConfig.GetRoleConfigMap())
			LoadResource(roleConfig.m_RoleResource);
	}

	//------------------------------------------------------------------------------------------------
	//! Get a cached resource or load and cache it if not already cached
	//! Reduces repeated Resource.Load() calls during mass spawning
	//! \param[in] resourceName The resource path to load
	//! \return The loaded resource or null if invalid
	Resource GetCachedResource(ResourceName resourceName)
	{
		if (resourceName.IsEmpty())
			return null;
		
		Resource res = m_mCachedResources.Get(resourceName);
		if (!res)
			res = LoadResource(resourceName);
		
		return res;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Call this when unloading mission or changing scenarios
	void ClearResourceCache()
	{
		m_mCachedResources.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	protected Resource LoadResource(ResourceName resourceName)
	{
		Resource res = Resource.Load(resourceName);
		if (res)
			m_mCachedResources.Set(resourceName, res);
		
		return res;
	}
}