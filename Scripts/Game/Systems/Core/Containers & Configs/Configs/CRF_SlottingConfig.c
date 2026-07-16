//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup
{
	[Attribute()]
	LocalizedString m_sCallsign;

	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType))]
	CRF_EFlagType m_FlagType;

	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;

	[Attribute("0", UIWidgets.SearchComboBox, desc: "Only used when the mission's Respawn Mode (Configure Settings plugin) is Slot-Based. Per-Slot: each role below tracks its own respawn count. Per-Group: the whole squad shares one pool.", enums: ParamEnumArray.FromEnum(CRF_ERespawnPoolType))]
	CRF_ERespawnPoolType m_eRespawnPoolType;

	[Attribute("-1", desc: "Shared respawn count for the whole group when Respawn Pool Type is Per-Group. -1 = unlimited.")]
	int m_iGroupRespawns;

	[Attribute(desc: "Per-role respawn counts used when Respawn Pool Type is Per-Slot. A role not listed here has unlimited respawns.")]
	ref array<ref CRF_RoleRespawnEntry> m_aRoleRespawns;

	//------------------------------------------------------------------------------------------------
	//! Looks up the configured respawn count for a role when Respawn Pool Type is Per-Slot.
	//! \param[in] role Role to look up
	//! \return Configured respawn count, or -1 (unlimited) if the role isn't listed
	int GetRoleRespawnCount(CRF_EGearRole role)
	{
		if (!m_aRoleRespawns)
			return -1;

		foreach (CRF_RoleRespawnEntry entry : m_aRoleRespawns)
		{
			if (entry.m_Role == role)
				return entry.m_iRespawns;
		}

		return -1;
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(CRF_EGearRole, "m_Role")]
class CRF_RoleRespawnEntry
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	CRF_EGearRole m_Role;

	[Attribute("-1", desc: "-1 = unlimited")]
	int m_iRespawns;
}