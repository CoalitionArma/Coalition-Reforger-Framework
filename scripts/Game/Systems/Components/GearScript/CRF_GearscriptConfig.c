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
};

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
class CRF_GearScriptWeaponsConfig
{		
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetRifles;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetRifleUGLs;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetCarbines;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetPistols;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetARs;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetMMGs;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetHMGs;

	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetAT;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetMAT;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetHAT;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetAA;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetSnipers;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_GearScriptEquipmentConfig
{		
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetLeadershipRadios;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetRTORadios;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetLeadershipBinos;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetAssistantBinos;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetAssistantMags;

	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	ref array<EGearRole> m_aRolesThatGetMedicalItems;
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
};

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_Weapon"}, "%1")]
class CRF_Weapon_Class : CRF_Base_Weapon_Class
{	
	[Attribute()]
	ref array<ref CRF_Magazine_Class> m_MagazineArray;
};

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

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sClothingType"}, "%1")]
class CRF_Clothing
{
	int m_iClothingType;
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {
		ParamEnum("", ""), 
		ParamEnum("HEADGEAR", 	"HEADGEAR"), 
		ParamEnum("SHIRT", 		"SHIRT"), 
		ParamEnum("ARMOREDVEST", 	"ARMOREDVEST"), 
		ParamEnum("PANTS", 		"PANTS"), 
		ParamEnum("BOOTS", 		"BOOTS"), 
		ParamEnum("BACKPACK", 	"BACKPACK"), 
		ParamEnum("VEST", 		"VEST"), 
		ParamEnum("HANDWEAR", 	"HANDWEAR"), 
		ParamEnum("HEAD", 		"HEAD"), 
		ParamEnum("EYES", 		"EYES"), 
		ParamEnum("EARS", 		"EARS"), 
		ParamEnum("FACE", 		"FACE"), 
		ParamEnum("NECK", 		"NECK"), 
		ParamEnum("EXTRA1", 		"EXTRA1"), 
		ParamEnum("EXTRA2", 		"EXTRA2"), 
		ParamEnum("WAIST", 		"WAIST"), 
		ParamEnum("EXTRA3", 		"EXTRA3"), 
		ParamEnum("EXTRA4", 		"EXTRA4")
	})]
	string m_sClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
	
	//------------------------------------------------------------------------------------------------
	void CRF_Clothing()
	{
		switch (m_sClothingType)
		{
			case "HEADGEAR" 		: {m_iClothingType = 0; 	break;}
			case "SHIRT" 		: {m_iClothingType = 1; 	break;}
			case "ARMOREDVEST" 	: {m_iClothingType = 2; 	break;}
			case "PANTS" 		: {m_iClothingType = 3; 	break;}
			case "BOOTS" 		: {m_iClothingType = 4; 	break;}
			case "BACKPACK" 		: {m_iClothingType = 5; 	break;}
			case "VEST" 			: {m_iClothingType = 6; 	break;}
			case "HANDWEAR" 		: {m_iClothingType = 7; 	break;}
			case "HEAD" 			: {m_iClothingType = 8; 	break;}
			case "EYES" 			: {m_iClothingType = 9; 	break;}
			case "EARS" 			: {m_iClothingType = 10; 	break;}
			case "FACE" 			: {m_iClothingType = 11; 	break;}
			case "NECK" 			: {m_iClothingType = 12; 	break;}
			case "EXTRA1" 		: {m_iClothingType = 13; 	break;}
			case "EXTRA2" 		: {m_iClothingType = 14; 	break;}
			case "WAIST" 		: {m_iClothingType = 15; 	break;}
			case "EXTRA3" 		: {m_iClothingType = 16; 	break;}
			case "EXTRA4" 		: {m_iClothingType = 17; 	break;}
		};
	}
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
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EGearRole, "m_Role")]
class CRF_Role_Custom_Gear
{	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	EGearRole m_Role;
	
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