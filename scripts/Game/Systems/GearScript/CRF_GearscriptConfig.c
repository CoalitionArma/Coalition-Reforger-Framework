//------------------------------------------------------------------------------------------------
// MASTER
//------------------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute()]
	ref CRF_Weapons m_Weapons;
	
	[Attribute()]
	ref CRF_Default_Gear m_DefaultGear;
	
	[Attribute()]
	ref CRF_Custom_Gear m_CustomGear;
}

//------------------------------------------------------------------------------------------------
// WEAPONS
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_Magazine", "m_MagazineCount"}, "%2 %1")]
class CRF_Magazine_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Magazine;
	
	[Attribute()]
	int m_MagazineCount;
	
	[Attribute()]
	int m_AssistantMagazineCount;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_Weapon"}, "%1")]
class CRF_Weapon_Class
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_Weapon;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_Attachments;
	
	[Attribute()]
	ref array<ref CRF_Magazine_Class> m_MagazineArray;
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
	
	[Attribute()]
	ref CRF_Weapon_Class m_Sniper;
}

//------------------------------------------------------------------------------------------------
// INVENTORY
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({ "m_sItemPrefab", "m_iItemCount" }, "%2 %1")]
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
	bool m_bEnableLeadershipRadios;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sLeadershipRadiosPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipBinoculars;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sLeadershipBinocularsPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_sGIRadiosPrefab;
	
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
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("HEADGEAR", "HEADGEAR"), ParamEnum("SHIRT", "SHIRT"), ParamEnum("ARMOREDVEST", "ARMOREDVEST"), ParamEnum("PANTS", "PANTS"), ParamEnum("BOOTS", "BOOTS"), ParamEnum("BACKPACK", "BACKPACK"), ParamEnum("VEST", "VEST"), ParamEnum("HANDWEAR", "HANDWEAR"), ParamEnum("HEAD", "HEAD"), ParamEnum("EYES", "EYES"), ParamEnum("EARS", "EARS"), ParamEnum("FACE", "FACE"), ParamEnum("NECK", "NECK"), ParamEnum("EXTRA1", "EXTRA1"), ParamEnum("EXTRA2", "EXTRA2"), ParamEnum("WAIST", "WAIST"), ParamEnum("EXTRA3", "EXTRA3"), ParamEnum("EXTRA4", "EXTRA4")})]
	string m_sClothingType;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ref array<ResourceName> m_ClothingPrefabs;
}

//------------------------------------------------------------------------------------------------
// Role Custom Gear
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
class CRF_Custom_Gear
{	
	[Attribute()]
	ref array<ref CRF_Leadership_Custom_Gear> m_LeadershipCustomGear;
	
	[Attribute()]
	ref array<ref CRF_Squad_Level_Custom_Gear> m_SquadLevelCustomGear;
	
	[Attribute()]
	ref array<ref CRF_Infantry_Specialties_Custom_Gear> m_InfantrySpecialtiesCustomGear;
	
	[Attribute()]
	ref array<ref CRF_Vehicle_Specialties_Custom_Gear> m_VehicleSpecialtiesCustomGear;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRoleToOverride"}, "%1")]
class CRF_Leadership_Custom_Gear
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("Company Commander", "Company Commander"), ParamEnum("Platoon Leader", "Platoon Leader"),  ParamEnum("Medical Officer", "Medical Officer"), ParamEnum("Squad Lead", "Squad Lead"), ParamEnum("Forward Observer", "Forward Observer"), ParamEnum("JTAC", "JTAC"), ParamEnum("Vehicle Lead", "Vehicle Lead"), ParamEnum("Indirect Lead", "Indirect Lead"), ParamEnum("Logi Lead", "Logi Lead")})]
	string m_sRoleToOverride;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_CustomClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRoleToOverride"}, "%1")]
class CRF_Squad_Level_Custom_Gear
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("Team Lead", "Team Lead"), ParamEnum("Medic", "Medic"), ParamEnum("Grenadier", "Grenadier"), ParamEnum("Rifleman", "Rifleman"), ParamEnum("Rifleman Demo", "Rifleman Demo"), ParamEnum("Rifleman AntiTank", "Rifleman AntiTank"), ParamEnum("Assistant Rifleman AntiTank", "Assistant Rifleman AntiTank"), ParamEnum("Automatic Rifleman", "Automatic Rifleman"), ParamEnum("Assistant Automatic Rifleman", "Assistant Automatic Rifleman")})]
	string m_sRoleToOverride;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_CustomClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRoleToOverride"}, "%1")]
class CRF_Infantry_Specialties_Custom_Gear
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("Heavy AntiTank", "Heavy AntiTank"), ParamEnum("Assistant Heavy AntiTank", "Assistant Heavy AntiTank"), ParamEnum("Medium AntiTank", "Medium AntiTank"), ParamEnum("Assistant Medium AntiTank", "Assistant Medium AntiTank"), ParamEnum("Heavy MachineGun", "Heavy MachineGun"), ParamEnum("Assistant Heavy MachineGun", "Assistant Heavy MachineGun"), ParamEnum("Medium MachineGun", "Medium MachineGun"), ParamEnum("Assistant Medium MachineGun", "Assistant Medium MachineGun"), ParamEnum("Anit-Air", "Anit-Air"), ParamEnum("Assistant Anit-Air", "Assistant Anit-Air"), ParamEnum("Sniper", "Sniper"), ParamEnum("Spotter", "Spotter")})]
	string m_sRoleToOverride;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_CustomClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRoleToOverride"}, "%1")]
class CRF_Vehicle_Specialties_Custom_Gear
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("Vehicle Driver", "Vehicle Driver"), ParamEnum("Vehicle Gunner", "Vehicle Gunner"), ParamEnum("Vehicle Loader", "Vehicle Loader"), ParamEnum("Pilot", "Pilot"), ParamEnum("Crew Chief", "Crew Chief"), ParamEnum("Logi Runner", "Logi Runner"), ParamEnum("Indirect Gunner", "Indirect Gunner"), ParamEnum("Indirect Loader", "Indirect Loader")})]
	string m_sRoleToOverride;
	
	[Attribute()]
	ref array<ref CRF_Clothing> m_CustomClothing;
	
	[Attribute()]
	ref array<ref CRF_Inventory_Item>  m_AdditionalInventoryItems;
}