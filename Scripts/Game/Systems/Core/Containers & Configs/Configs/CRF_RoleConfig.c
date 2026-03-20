[BaseContainerProps(configRoot: true)]
class CRF_RolesConfig
{		
	[Attribute()]
	ref array<ref CRF_RoleConfig> m_RoleConfigs;
	
	protected ref map<CRF_EGearRole, CRF_RoleConfig> m_RoleConfigsMap = new map<CRF_EGearRole, CRF_RoleConfig>;
	
	map<CRF_EGearRole,CRF_RoleConfig> GetRoleConfigMap()
	{
		return m_RoleConfigsMap;
	}
	
	CRF_RoleConfig FindRoleConfig(CRF_EGearRole role)
	{
		return m_RoleConfigsMap.Get(role);
	}
	
	void CRF_RolesConfig()
	{
		foreach(CRF_RoleConfig roleConfig : m_RoleConfigs)
			m_RoleConfigsMap.Set(roleConfig.m_Role, roleConfig);	
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(CRF_EGearRole, "m_Role")]
class CRF_RoleConfig
{		
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	CRF_EGearRole m_Role;
	
	[Attribute()]
	string m_sRoleName;
	
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sRoleDescription;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds")]
	ResourceName m_RoleIcon;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_ESlotType))]
	CRF_ESlotType m_SlottingType;

	[Attribute(uiwidget: "resourcePickerSimple", params: "et")]
	ResourceName m_RoleResource;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptWeapons))]
	ref array<CRF_EGearscriptWeapons> m_aWeapons;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptMagazines))]
	ref array<CRF_EGearscriptMagazines> m_aMagazines;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptItems))]
	ref array<CRF_EGearscriptItems> m_aItems;
}