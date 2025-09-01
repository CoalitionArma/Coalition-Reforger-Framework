//------------------------------------------------------------------------------------------------
// CONTAINER
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_GearScriptContainer
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_GearScriptConfig")]
	ResourceName m_rGearScript;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniArsenal;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rLeadershipRadiosPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rRTORadiosPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rGIRadiosPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableRTORadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
}

//------------------------------------------------------------------------------------------------
// MASTER
//------------------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute()]
	string m_FactionName;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds")]
	ResourceName m_FactionIcon;
	
	[Attribute()]
	ref CRF_Weapons m_FactionWeapons;
	
	[Attribute()]
	ref CRF_Default_Gear m_DefaultFactionGear;
	
	[Attribute()]
	ref CRF_Custom_Gear m_CustomFactionGear;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_GearScriptRolesConfig
{		
	[Attribute()]
	ref array<ref CRF_RoleConfig> m_RoleConfigs;
	
	CRF_RoleConfig FindRoleConfig(CRF_EGearRole role)
	{
		foreach(CRF_RoleConfig roleConfig : m_RoleConfigs)
			if (roleConfig.m_Role == role)
				return roleConfig;

		return new CRF_RoleConfig;
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
	ResourceName m_BluforVariant;
	
	[Attribute(uiwidget: "resourcePickerSimple", params: "et")]
	ResourceName m_OpforVariant;
	
	[Attribute(uiwidget: "resourcePickerSimple", params: "et")]
	ResourceName m_IndforVariant;
	
	[Attribute(uiwidget: "resourcePickerSimple", params: "et")]
	ResourceName m_CivVariant;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptWeapons))]
	ref array<CRF_EGearscriptWeapons> m_aWeapons;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptMagazines))]
	ref array<CRF_EGearscriptMagazines> m_aMagazines;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptItems))]
	ref array<CRF_EGearscriptItems> m_aItems;
}

//------------------------------------------------------------------------------------------------
// WEAPONS
//------------------------------------------------------------------------------------------------

class CRF_Base_Weapon_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Weapon;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_Attachments;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_Weapon"}, "%1")]
class CRF_Weapon_Class : CRF_Base_Weapon_Class
{	
	[Attribute()]
	ref array<ref CRF_Magazine_Class> m_MagazineArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_Weapon"}, "%1")]
class CRF_Spec_Weapon_Class : CRF_Base_Weapon_Class
{
	[Attribute()]
	ref array<ref CRF_Spec_Magazine_Class> m_MagazineArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_MagazineCount", "m_Magazine"}, "%1 : %2")]
class CRF_Magazine_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Magazine;
	
	[Attribute()]
	int m_MagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_MagazineCount", "m_AssistantMagazineCount", "m_Magazine"}, "%1 | %2 : %3")]
class CRF_Spec_Magazine_Class : CRF_Magazine_Class
{	
	[Attribute()]
	int m_AssistantMagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Weapons
{	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_Rifle;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_RifleUGL;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_Carbine;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_Pistol;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_AR;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_MMG;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_HMG;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_AT;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_MAT;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_HAT;
	
	[Attribute()]
	ref CRF_Spec_Weapon_Class m_AA;
	
	[Attribute()]
	ref CRF_Weapon_Class m_Sniper;
}

//------------------------------------------------------------------------------------------------
// INVENTORY
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({ "m_iItemCount", "m_sItemPrefab" }, "%1 : %2")]
class CRF_Inventory_Item
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sItemPrefab;
	
	[Attribute("")] 
	int m_iItemCount;
}

//------------------------------------------------------------------------------------------------
// GEAR
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_Default_Gear
{
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipBinoculars;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sLeadershipBinocularsPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableAssistantBinoculars;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sAssistantBinocularsPrefab;
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableMedicFrags;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_DefaultMedicMedicalItems;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_DefaultClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item> m_DefaultInventoryItems;
}

//------------------------------------------------------------------------------------------------
// CLOTHING
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(CRF_EGearscriptClothing, "m_iClothingType")]
class CRF_Clothing
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearscriptClothing))]
	CRF_EGearscriptClothing m_iClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
}

//------------------------------------------------------------------------------------------------
// ROLE CUSTOM GEAR
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_Custom_Gear
{	
	[Attribute()]
	ref array<ref CRF_Role_Custom_Gear> m_RolesToSetCustomGear;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(CRF_EGearRole, "m_Role")]
class CRF_Role_Custom_Gear
{	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	CRF_EGearRole m_Role;
	
	[Attribute()]
	string m_sRoleName;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_PrimaryWeapon;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_SecondaryWeapon;
	
	[Attribute()]
	ref array<ref CRF_Weapon_Class> m_Pistol;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_Clothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}