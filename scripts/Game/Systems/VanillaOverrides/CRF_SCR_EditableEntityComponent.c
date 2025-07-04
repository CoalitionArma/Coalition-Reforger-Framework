modded class SCR_EditableEntityComponent
{
	//------------------------------------------------------------------------------------------------
	//! Get information about the entity. When none exist, create a dummy one.
	//! \return Info class
	override SCR_UIInfo GetInfo(IEntity owner = null)
	{
		//--- From instance
		if (m_UIInfoInstance)
			return m_UIInfoInstance;
		
		ResourceName resourceName = m_Owner.GetPrefabData().GetPrefabName();
		
		//--- From Role Config
		if (CRF_RoleHelper.IsValidGearscriptResource(resourceName) && GetGame().GetMenuManager().GetTopMenu().IsInherited(CRF_SpectatorMenuUI))
		{
			CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();
			CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceName);
			
			if (role)
			{
				CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				
				if (roleConfig)
					return SCR_UIInfo.CreateInfo(roleConfig.m_sRoleName, roleConfig.m_sRoleDescription, roleConfig.m_RoleIcon);
			};
		};
		
		//--- From prefab
		SCR_EditableEntityComponentClass prefabData = GetEditableEntityData(owner);
		if (prefabData)
			return prefabData.GetInfo();
		else
			return null;
	}
}