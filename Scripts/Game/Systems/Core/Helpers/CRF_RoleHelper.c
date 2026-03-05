class CRF_RoleHelper
{	
	//------------------------------------------------------------------------------------------------
	static CRF_EGearRole ResourceToRole(ResourceName roleResource)
	{
		foreach(CRF_RoleConfig roleConfig : CRF_GamemodeManager.RolesConfig().m_RoleConfigs)
			if (roleConfig.m_RoleResource == roleResource)
				return roleConfig.m_Role;

		return CRF_EGearRole.RIFLEMAN;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName RoleToResource(CRF_EGearRole role)
	{
		CRF_RoleConfig roleConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);

		return roleConfig.m_RoleResource;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsValidGearscriptResource(ResourceName resource)
	{
		return resource.Contains("!GS_Characters");
	};

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsSquadLeaderRole(IEntity entity)
	{
		ref TIntArray roles = {CRF_EGearRole.COMPANY_COMMANDER, CRF_EGearRole.PLATOON_LEADER, CRF_EGearRole.MEDICAL_OFFICER, CRF_EGearRole.SQUAD_LEAD, CRF_EGearRole.VEHICLE_LEAD, CRF_EGearRole.INDIRECT_LEAD, CRF_EGearRole.LOGI_LEAD};
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		CRF_EGearRole role = ResourceToRole(prefab);

		return roles.Contains(role);
	}

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsTeamLeaderRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		CRF_EGearRole role = ResourceToRole(prefab);

		return (role == CRF_EGearRole.TEAM_LEAD);
	}
}