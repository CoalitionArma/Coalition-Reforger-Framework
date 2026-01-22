//------------------------------------------------------------------------------------------------
// CONTAINER
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_GearScriptContainer
{
	//------------------------------------------------------------------------------------------------
	// Vars set by plugin
	
	[Attribute("", UIWidgets.Hidden)]
	ResourceName m_rGearScript;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableShareableMarkers;
  
 	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableBFT;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableGIRadios;
	
	[Attribute("true", UIWidgets.Hidden)]
	bool m_bEnableRTORadios;
	
	//------------------------------------------------------------------------------------------------
	// Vars considered "advanced" and not set by plugin
	
	[Attribute("{E6555DA2F31B0EC0}Configs/Gearscripts/Additional Configs/CRF_Global_SightArsenal_Regular.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_SightArsenalConfig")]
	ResourceName m_rSightArsenal;
	
	[Attribute("{9D8E5FA08331042D}Configs/Gearscripts/Additional Configs/CRF_Global_SightArsenal_Magnified.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_SightArsenalConfig")]
	ResourceName m_rMagnifiedSightArsenal;
	
	[Attribute("{2E2626C733070162}Configs/Gearscripts/Additional Configs/CRF_Global_VehicleGearscriptValues.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all vehicles on this faction", "conf class=CRF_VehicleGearscriptConfig")]
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
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableIndividualBFT;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rShortRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rLongRangeRadioPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rRTORadiosPrefab;
}


// Simplified Container for Faction Plugin
[BaseContainerProps()]
class CRF_SimplifiedGearScriptContainer
{
	[Attribute("{6FFD426FE0C1079B}Configs/Gearscripts/Standard/80s/CRF_GS_CIV.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_GearScriptConfig")]
	ResourceName m_rGearScript;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableRTORadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableShareableMarkers;
  
 	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableBFT;
}

//------------------------------------------------------------------------------------------------
// MASTER
//------------------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute(category: "CRF Gearscript - Faction Settings")]
	string m_FactionName;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "edds", category: "CRF Gearscript - Faction Settings")]
	ResourceName m_FactionIcon;
	
	[Attribute("{11CAD6C8909CE567}Configs/_Identities/CRF_CharacterIdentity_European.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript Faction Identity", "conf class=CRF_CharacterIdentity", category: "CRF Gearscript - Faction Settings")]
	ResourceName m_FactionIdentity;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref CRF_Weapon_Class> m_Rifles;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref CRF_Weapon_Class> m_RifleUGLs;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref CRF_Weapon_Class> m_Carbines;
	
	[Attribute(category: "CRF Gearscript - Faction Weapons")]
	ref array<ref CRF_Weapon_Class> m_Pistols;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_AR;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_MMG;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_HMG;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_AT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_MAT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_HAT;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Spec_Weapon_Class m_AA;
	
	[Attribute(category: "CRF Gearscript - Specialty Faction Weapons")]
	ref CRF_Weapon_Class m_SNIPER;
	
	[Attribute(category: "CRF Gearscript - Faction Clothing")]
	ref array<ref CRF_Clothing> m_DefaultClothing;
	
	[Attribute("true", UIWidgets.CheckBox, category: "CRF Gearscript - Faction Gear")]
	bool m_bEnableLeadershipBinoculars;
	
	[Attribute("true", UIWidgets.CheckBox, category: "CRF Gearscript - Faction Gear")]
	bool m_bEnableAssistantBinoculars;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "CRF Gearscript - Faction Gear")]
	ResourceName m_sLeadershipBinocularsPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et", category: "CRF Gearscript - Faction Gear")]
	ResourceName m_sAssistantBinocularsPrefab;
	
	[Attribute(category: "CRF Gearscript - Faction Gear")]
	ref array<ref CRF_Inventory_Item> m_DefaultInventoryItems;
	
	[Attribute(category: "CRF Gearscript - Faction Medical Gear")]
	ref array<ref CRF_Inventory_Item>  m_InfantryMedicalItems;
	
	[Attribute(category: "CRF Gearscript - Faction Medical Gear")]
	ref array<ref CRF_Inventory_Item>  m_MedicMedicalItems;
	
	[Attribute(category: "CRF Gearscript - Custom Role Settings")]
	ref array<ref CRF_Role_Custom_Gear> m_RolesToSetCustomSettings;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_GearScriptRolesConfig
{		
	[Attribute()]
	ref array<ref CRF_RoleConfig> m_RoleConfigs;
	
	protected ref map<CRF_EGearRole, CRF_RoleConfig> m_RoleConfigsMap = new map<CRF_EGearRole, CRF_RoleConfig>;
	
	CRF_RoleConfig FindRoleConfig(CRF_EGearRole role)
	{
		return m_RoleConfigsMap.Get(role);
	}
	
	void CRF_GearScriptRolesConfig()
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
[BaseContainerProps(), CRF_BaseContainerCustomTitleResourceFields({"m_Weapon"}, "%1")]
class CRF_Weapon_Class : CRF_Base_Weapon_Class
{	
	[Attribute()]
	ref array<ref CRF_Magazine_Class> m_MagazineArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), CRF_BaseContainerCustomTitleResourceFields({"m_Weapon"}, "%1")]
class CRF_Spec_Weapon_Class : CRF_Base_Weapon_Class
{
	[Attribute()]
	ref array<ref CRF_Spec_Magazine_Class> m_MagazineArray;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), CRF_BaseContainerCustomTitleResourceFields({"m_MagazineCount", "m_Magazine"}, "[%1] : %2")]
class CRF_Magazine_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Magazine;
	
	[Attribute()]
	int m_MagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), CRF_BaseContainerCustomTitleResourceFields({"m_MagazineCount", "m_AssistantMagazineCount", "m_Magazine"}, "[%1 | %2] : %3")]
class CRF_Spec_Magazine_Class : CRF_Magazine_Class
{	
	[Attribute()]
	int m_AssistantMagazineCount;
}

//------------------------------------------------------------------------------------------------
// INVENTORY
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), CRF_BaseContainerCustomTitleResourceFields({ "m_iItemCount", "m_sItemPrefab" }, "[%1] : %2")]
class CRF_Inventory_Item
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sItemPrefab;
	
	[Attribute("")] 
	int m_iItemCount;
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
	ref array<ref CRF_Weapon_Class> m_Pistols;
	
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