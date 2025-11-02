//------------------------------------------------------------------------------------------------
// CONTAINER
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_GearScriptContainer
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_GearScriptConfig")]
	ResourceName m_rGearScript;
	
	[Attribute("{E6555DA2F31B0EC0}Configs/Gearscripts/CRF_Global_SightArsenal_Regular.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_SightArsenalConfig")]
	ResourceName m_rSightArsenal;
	
	[Attribute("{9D8E5FA08331042D}Configs/Gearscripts/CRF_Global_SightArsenal_Magnified.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_SightArsenalConfig")]
	ResourceName m_rMagnifiedSightArsenal;
	
	[Attribute("{2E2626C733070162}Configs/Gearscripts/CRF_Global_VehicleGearscriptValues.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all vehicles on this faction", "conf class=CRF_VehicleGearscriptConfig")]
	ResourceName m_rVehicleGearscriptValues;
	
	[Attribute("", desc: "Loadout values applied to all vehicles in this faction", "conf class=CRF_VehicleGearScriptLoadout")]
	ref CRF_VehicleGearScriptLoadout m_VehicleLoadout;
	
	[Attribute()] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute()]
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
	
	[Attribute()] 
	ref array<ResourceName> m_aSupplyTrucks;
	
	[Attribute()] 
	ref array<ResourceName> m_aAdditonalItemsForSupplyArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniWeaponArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableSightArsenal;
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableMagnifiedOptics;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rShortRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rLongRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rRTORadiosPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableRTORadios;
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
	
	[Attribute("{11CAD6C8909CE567}Configs/Identities/CRF_CharacterIdentity_European.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript Faction Identity", "conf class=CRF_CharacterIdentity")]
	ResourceName m_FactionIdentity;
	
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

//------------------------------------------------------------------------------------------------
// CHARACTER IDENTITY
//------------------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class CRF_CharacterIdentity
{	
	[Attribute()]
	ref array<ref CRF_Character_Visual_Identity> m_VisualIdentityArray;
	
	[Attribute()]
	ref array<ref CRF_Character_Sound_Identity> m_SoundIdentityArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Character_Visual_Identity
{	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Head;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Body;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Character_Sound_Identity
{	
	[Attribute()]
	int m_VoiceID;
	
	[Attribute()]
	float m_VoicePitch;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_SightArsenalConfig
{	
	[Attribute()]
	ref array<ResourceName> m_aSights;
}

[BaseContainerProps(configRoot:true)]
class CRF_VehicleGearscriptConfig
{
	[Attribute("1200")] int m_iAmountOfBulletsRifles;
	[Attribute("600")] int m_iAmountOfBulletsRifleUGLs;
	[Attribute("600")] int m_iAmountOfBulletsCarbines;
	[Attribute("200")] int m_iAmountOfBulletsPistols;
	[Attribute("2400")] int m_iAmountOfBulletsAR;
	[Attribute("3000")] int m_iAmountOfBulletsMMG;
	[Attribute("600")] int m_iAmountOfBulletsHMG;
	
	[Attribute("4")] int m_iAmountOfDisposables;
	[Attribute("8")] int m_iAmountOfRocketsAT;
	[Attribute("8")] int m_iAmountOfRocketsMAT;
	[Attribute("2")] int m_iAmountOfRocketsAA;
	
	[Attribute("120")] int m_iAmountOfBulletsSniper;
	
	[Attribute("8")] int m_iAmountOfGrenades;
	[Attribute("16")] int m_iAmountOfSmokeGrenades;
	
	[Attribute("20")] int m_iAmountOfHEGLs;
	[Attribute("40")] int m_iAmountOfSmokeGLs;
}

[BaseContainerProps()]
class CRF_VehicleGearscriptOverride
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EVehicleGearScriptType))]
	CRF_EVehicleGearScriptType m_VehicleAmmoType;
	
	[Attribute()] 
	int m_iAmountOfBullets;
}

[BaseContainerProps()]
class CRF_VehicleGearScriptAdditionalItem
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")] 
	ResourceName m_Prefab;
	
	[Attribute("1")] 
	int m_iAmountOfItemSupplyTruck;
	
	[Attribute("1")] 
	int m_iAmountOfItemRegularVehicle;
}

[BaseContainerProps()]
class CRF_VehicleGearScriptLoadout
{
	[Attribute("300")] int m_iAmountofAutoCannonAmmo;
	[Attribute("1200")] int m_iAmountofMachineGunAmmo;
}