[BaseContainerProps(configRoot: true), BaseContainerCustomTitleField("RoleSpecificGear")]
class CRF_RoleGear
{
	[Attribute(category: "settings")]
	bool m_isLeader;
	[Attribute(defvalue: "true", category: "settings")]
	bool m_useWeaponSettings;
	[Attribute(defvalue: "true", category: "settings")]
	bool m_useMedicalSeetings;
	[Attribute(defvalue: "true", category: "settingqs")]
	bool m_useLoadoutSettings;
	[Attribute(category: "settings")]
	bool m_hasBinos;
	[Attribute(category: "Weapon")]
	ResourceName m_Weapon;
	[Attribute(category: "Weapon")]
	ResourceName m_Primary2;
	[Attribute(category: "Weapon")]
	ResourceName m_Sidearm;
	[Attribute(category: "AMMO")]
	ref array<ref CRF_Clothing> m_ClothingOverride;
	[Attribute(category: "Extra")]
}
