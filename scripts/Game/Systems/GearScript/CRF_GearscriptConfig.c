//------------------------------------------------------------------------------------------------
// MASTER
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute()]
	ref CRF_Default_Gear m_DefaultGear;
	
	[Attribute()]
	ref CRF_Weapons m_Weapons;
	
	[Attribute()]
	ref array<ref CRF_Per_Role_Gear> m_RoleCustomGear;
}

//------------------------------------------------------------------------------------------------
// WEAPONS
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Weapon_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Weapon;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_Attachments;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Magazine;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	int m_MagazineCount;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_UGLMagazine;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	int m_UGLMagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Weapons
{
	[Attribute()]
	ref CRF_Weapon_Class m_Rifle;
	
	[Attribute()]
	ref CRF_Weapon_Class m_RifleUGL;
	
	[Attribute()]
	ref CRF_Weapon_Class m_Carbine;
	
	[Attribute()]
	ref CRF_Weapon_Class m_Pistol;
	
	[Attribute()]
	ref CRF_Weapon_Class m_AR;
	
	[Attribute()]
	ref CRF_Weapon_Class m_MMG;
	
	[Attribute()]
	ref CRF_Weapon_Class m_HMG;
	
	[Attribute()]
	ref CRF_Weapon_Class m_AT;
	
	[Attribute()]
	ref CRF_Weapon_Class m_MAT;
	
	[Attribute()]
	ref CRF_Weapon_Class m_HAT;
	
	[Attribute()]
	ref CRF_Weapon_Class m_AA;
}

//------------------------------------------------------------------------------------------------
// INVENTORY
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({ "m_sItemPrefab", "m_iItemCount" }, "%2 %1")]
class CRF_Inventory_Item
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sItemPrefab;
	
	[Attribute()]
	int m_iItemCount;
}

//------------------------------------------------------------------------------------------------
// GEAR
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Default_Gear
{
	[Attribute()]
	ref array<ref CRF_Clothing> m_DefaultClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item> m_DefaultInventoryItems;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_Per_Role_Gear
{
	[Attribute()]
	ref array<ref CRF_Clothing> m_ClothingOverrides;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}

//------------------------------------------------------------------------------------------------
// CLOTHING
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sClothingType"}, "%1")]
class CRF_Clothing
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("HEADGEAR", "HEADGEAR"), ParamEnum("SHIRT", "SHIRT"), ParamEnum("ARMOREDVEST", "ARMOREDVEST"), ParamEnum("PANTS", "PANTS"), ParamEnum("BOOTS", "BOOTS"), ParamEnum("BACKPACK", "BACKPACK"), ParamEnum("VEST", "VEST"), ParamEnum("HANDWEAR", "HANDWEAR"), ParamEnum("HEAD", "HEAD"), ParamEnum("EYES", "EYES"), ParamEnum("EARS", "EARS"), ParamEnum("FACE", "FACE"), ParamEnum("NECK", "NECK"), ParamEnum("EXTRA1", "EXTRA1"), ParamEnum("EXTRA2", "EXTRA2"), ParamEnum("WAIST", "WAIST"), ParamEnum("EXTRA3", "EXTRA3"), ParamEnum("EXTRA4", "EXTRA4")})]
	string m_sClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
}

//------------------------------------------------------------------------------------------------
// Per Role Gear
//------------------------------------------------------------------------------------------------

// -- Leadership -- \\
[BaseContainerProps(category: "Leadership")]
class CRF_Company_Commander : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Platoon_Commander : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Squad_Lead : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Indirect_Lead: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Logi_Lead: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Medical_Officer: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Leadership")]
class CRF_Vehicle_Lead : CRF_Per_Role_Gear {}

// -- Squad Level -- \\
[BaseContainerProps(category: "Squad Level")]
class CRF_Team_Lead: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Rifleman : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Rifleman_Demo : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Rifleman_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Assistant_Rifleman_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Grenadier : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Automatic_Rifleman : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Assistant_Automatic_Rifleman : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Squad Level")]
class CRF_Medic: CRF_Per_Role_Gear {}

// -- Specialties -- \\
[BaseContainerProps(category: "Specialties")]
class CRF_Heavy_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Assistant_Heavy_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Medium_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Assistant_Medium_AntiTank : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Heavy_MachineGun : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Assistant_Heavy_MachineGun : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Pilot: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Sniper : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Spotter : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_Forward_Observer : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Specialties")]
class CRF_JTAC : CRF_Per_Role_Gear {}

// -- Crewmen/Misc -- \\
[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_CrewChief : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Indirect_Gunner: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Indirect_Loader: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Logi_Runner: CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Vehicle_Driver : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Vehicle_Gunner : CRF_Per_Role_Gear {}

[BaseContainerProps(category: "Crewmen/Misc")]
class CRF_Vehicle_Loader : CRF_Per_Role_Gear {}