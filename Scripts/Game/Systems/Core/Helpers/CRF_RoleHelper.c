class CRF_RoleHelper
{	
	//------------------------------------------------------------------------------------------------
	static CRF_EGearRole ResourceToRole(ResourceName roleResource)
	{
		foreach(CRF_RoleConfig roleConfig : CRF_GearscriptManager.GetRolesConfig().m_RoleConfigs)
			if (roleConfig.m_RoleResource == roleResource)
				return roleConfig.m_Role;

		return CRF_EGearRole.RIFLEMAN;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName RoleToResource(CRF_EGearRole role)
	{
		CRF_RoleConfig roleConfig = CRF_GearscriptManager.GetRolesConfig().FindRoleConfig(role);

		return roleConfig.m_RoleResource;
	}

	//------------------------------------------------------------------------------------------------
	//! Data-driven check for whether a resource is a registered gearscript role prefab.
	//! Checks against CRF_RolesConfig.m_RoleResource rather than guessing from the folder
	//! path, so it keeps working if role prefabs are ever renamed/relocated.
	static bool IsValidGearscriptResource(ResourceName resource)
	{
		CRF_RolesConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig();
		if (!rolesConfig)
			return false;

		foreach (CRF_RoleConfig roleConfig : rolesConfig.m_RoleConfigs)
			if (roleConfig.m_RoleResource == resource)
				return true;

		return false;
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

	//------------------------------------------------------------------------------------------------
	static bool IsMedicRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		CRF_EGearRole role = ResourceToRole(prefab);

		return (role == CRF_EGearRole.MEDIC || role == CRF_EGearRole.MEDICAL_OFFICER);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCombatEngineerRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		CRF_EGearRole role = ResourceToRole(prefab);

		return (role == CRF_EGearRole.COMBAT_ENGINEER);
	}
}
