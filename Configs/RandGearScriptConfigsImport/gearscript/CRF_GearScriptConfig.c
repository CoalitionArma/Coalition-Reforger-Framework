[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute(defvalue: "Blufor", uiwidget: UIWidgets.ComboBox, desc: "This is an Int value using a combobox", enums: { ParamEnum("BLUFOR", "0"), ParamEnum("OPFOR", "1"), ParamEnum("INDFOR", "2"),ParamEnum("CIV", "3") })]
	int m_Faction;
	[Attribute(category: "settings")]
	ref CRF_DefaultGearSettings m_DefaultGearSettings;
	[Attribute(category: "settings")]
	ref CRF_MedicalGear m_MedicalSettings;

	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_Rifleman;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanDemo;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanMedic;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAR;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAAR;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanMAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAMAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanHAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAHAT;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanMMG;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAMMG;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanHMG;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanAHMG;
	[Attribute(category: "Rifleman")]
	ref CRF_RoleGear m_RiflemanSniper;
}