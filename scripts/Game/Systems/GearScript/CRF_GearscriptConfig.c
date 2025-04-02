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
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetRifles;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetRifleUGLs;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetCarbines;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetPistols;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetARs;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetMMGs;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetHMGs;

	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetAT;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetMAT;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetHAT;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetAA;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetSnipers;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class CRF_GearScriptEquipmentConfig
{		
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetLeadershipRadios;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetRTORadios;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetLeadershipBinos;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetAssistantBinos;
	
	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetAssistantMags;

	[Attribute()]
	ref array<ref CRF_Role> m_aRolesThatGetMedicalItems;
}

//------------------------------------------------------------------------------------------------
// ROLES
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRole"}, "%1")]
class CRF_Role
{	
	/*
		HOW TO ADD A ROLE 101:
		- Create the specified role across all character faction prefabs and name it with the method: CRF_GS_(Faction Key)_(Role)_P, ie: 
			CRF_GS_BLUFOR_CombatEng_P
	
		- Create a "Pretty Name" in the bellow attribute ParamEnum array, ie:
			ParamEnum("Combat Engineer", ""),
	
		- Then just add corresponding case into the switch function bellow using pretty name to match the (Role) value you added to the character prefab (make sure you trail it with a _ and end it with a _P) ie:
			case "Combat Engineer" : {m_sRole = "_CombatEng_P"; break;}
	
		- Now you have to go to the corresponding global files:
			(Configs\Gearscripts\CRF_Global_Equipment_Config.conf)
			(Configs\Gearscripts\CRF_Global_Weapons_Config.conf)
		and just add the role you creaed bellow (make sure you validate and reload scripts) into the correcsponding array(s) of equipment you want it to receive, any custom equipment would have to go through a gear script.
	
		There, you have added a role, good for you, now stop bothering me about adding in roles manually -Njpatman
	*/
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {
		ParamEnum(																								"", ""), 
		ParamEnum("-------------------------------------------- Leadership --------------------------------------------",  ""),
		ParamEnum("Company Commander", 				""),
		ParamEnum("First Sergeant", 				""),
		ParamEnum("Platoon Leader", 				""),
		ParamEnum("Platoon Sergeant", 				""),
		ParamEnum("Medical Officer", 				""),
		ParamEnum("Forward Observer", 				""),
		ParamEnum("JTAC", 							""),
		ParamEnum("Squad Lead", 					""),
		ParamEnum("Vehicle Lead", 					""),
		ParamEnum("Indirect Lead", 					""),
		ParamEnum("Logi Lead", 						""),
		ParamEnum("-------------------------------------------- Squad Level -------------------------------------------",  ""),
		ParamEnum("Team Lead", 						""),
		ParamEnum("Medic", 							""),
		ParamEnum("Radio Telephone Operator", 		""),
		ParamEnum("Grenadier", 						""),
		ParamEnum("Automatic Rifleman", 			""),
		ParamEnum("Assistant Automatic Rifleman",	""),
		ParamEnum("Rifleman", 						""),
		ParamEnum("Rifleman AntiTank", 				""),
		ParamEnum("Assistant Rifleman AntiTank", 	""),
		ParamEnum("Rifleman Demo", 					""),
		ParamEnum("------------------------------------------- Specialities -------------------------------------------",  ""),
		ParamEnum("Heavy AntiTank", 				""),
		ParamEnum("Assistant Heavy AntiTank", 		""),
		ParamEnum("Medium AntiTank", 				""),
		ParamEnum("Assistant Medium AntiTank", 		""),
		ParamEnum("Heavy MachineGun", 				""),
		ParamEnum("Assistant Heavy MachineGun", 	""),
		ParamEnum("Medium MachineGun", 				""),
		ParamEnum("Assistant Medium MachineGun", 	""),
		ParamEnum("Anit-Air", 						""),
		ParamEnum("Assistant Anit-Air", 			""),
		ParamEnum("Sniper", 						""),
		ParamEnum("Spotter", 						""),
		ParamEnum("Drone Operator", 				""),
		ParamEnum("--------------------------------------- Vehicle Specialities ---------------------------------------",  ""),
		ParamEnum("Vehicle Driver", 				""),
		ParamEnum("Vehicle Gunner", 				""),
		ParamEnum("Vehicle Loader", 				""),
		ParamEnum("Pilot", 							""),
		ParamEnum("Crew Chief", 					""),
		ParamEnum("Logi Runner", 					""),
		ParamEnum("Indirect Gunner", 				""),
		ParamEnum("Indirect Loader", 				"")
	})]
	string m_sRole;
	
	//------------------------------------------------------------------------------------------------
	void CRF_Role()
	{
		switch(m_sRole)
		{
			//-------------------------------------------- Leadership --------------------------------------------
			case "Company Commander" : 				{m_sRole = "_COY_P"; 				break;}
			case "First Sergeant" : 				{m_sRole = "_1SG_P"; 				break;}
			case "Platoon Leader" : 				{m_sRole = "_PL_P"; 				break;}
			case "Platoon Sergeant" : 				{m_sRole = "_PSG_P"; 				break;}
			case "Medical Officer" : 				{m_sRole = "_MO_P"; 				break;}
			case "Forward Observer" : 				{m_sRole = "_FO_P"; 				break;}
			case "JTAC" : 							{m_sRole = "_JTAC_P"; 				break;}
			case "Squad Lead" : 					{m_sRole = "_SL_P"; 				break;}
			case "Vehicle Lead" : 					{m_sRole = "_VehLead_P"; 			break;}
			case "Indirect Lead" :					{m_sRole = "_IndirectLead_P"; 		break;}
			case "Logi Lead" :						{m_sRole = "_LogiLead_P"; 			break;}
			//-------------------------------------------- Squad Level -------------------------------------------
			case "Team Lead" : 						{m_sRole = "_TL_P"; 				break;}
			case "Medic" : 							{m_sRole = "_Medic_P"; 				break;}
			case "Radio Telephone Operator" : 		{m_sRole = "_RTO_P"; 				break;}
			case "Grenadier" : 						{m_sRole = "_Gren_P"; 				break;}
			case "Automatic Rifleman" : 			{m_sRole = "_AR_P"; 				break;}
			case "Assistant Automatic Rifleman" : 	{m_sRole = "_AAR_P"; 				break;}
			case "Rifleman" : 						{m_sRole = "_Rifleman_P"; 			break;}
			case "Rifleman AntiTank" : 				{m_sRole = "_AT_P"; 				break;}
			case "Assistant Rifleman AntiTank" : 	{m_sRole = "_AAT_P"; 				break;}
			case "Rifleman Demo" :					{m_sRole = "_Demo_P"; 				break;}
			//-------------------------------------------- Specialities -------------------------------------------
			case "Heavy AntiTank" : 				{m_sRole = "_HAT_P"; 				break;}
			case "Assistant Heavy AntiTank" : 		{m_sRole = "_AHAT_P"; 				break;}
			case "Medium AntiTank" : 				{m_sRole = "_MAT_P"; 				break;}
			case "Assistant Medium AntiTank" : 		{m_sRole = "_AMAT_P"; 				break;}
			case "Heavy MachineGun" : 				{m_sRole = "_HMG_P"; 				break;}
			case "Assistant Heavy MachineGun" : 	{m_sRole = "_AHMG_P"; 				break;}
			case "Medium MachineGun" : 				{m_sRole = "_MMG_P"; 				break;}
			case "Assistant Medium MachineGun" : 	{m_sRole = "_AMMG_P"; 				break;}
			case "Anit-Air" : 						{m_sRole = "_AA_P"; 				break;}
			case "Assistant Anit-Air" : 			{m_sRole = "_AAA_P"; 				break;}
			case "Sniper" : 						{m_sRole = "_Sniper_P"; 			break;}
			case "Spotter" : 						{m_sRole = "_Spotter_P"; 			break;}
			case "Drone Operator" : 				{m_sRole = "_DroneOp_P"; 			break;}
			//--------------------------------------- Vehicle Specialities ----------------------------------------
			case "Vehicle Driver" : 				{m_sRole = "_VehDriver_P"; 			break;}
			case "Vehicle Gunner" : 				{m_sRole = "_VehGunner_P"; 			break;}
			case "Vehicle Loader" : 				{m_sRole = "_VehLoader_P"; 			break;}
			case "Pilot" : 							{m_sRole = "_Pilot_P"; 				break;}
			case "Crew Chief" : 					{m_sRole = "_CrewChief_P"; 			break;}
			case "Logi Runner" : 					{m_sRole = "_LogiRunner_P"; 		break;}
			case "Indirect Gunner" : 				{m_sRole = "_IndirectGunner_P"; 	break;}
			case "Indirect Loader" : 				{m_sRole = "_IndirectLoader_P"; 	break;}
			//---------------------------------------------- Default ----------------------------------------------
			default:								{m_sRole = string.Empty; 			break;}
		}
	}
}	

//------------------------------------------------------------------------------------------------
// WEAPONS
//------------------------------------------------------------------------------------------------

[BaseContainerProps()]
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

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sClothingType"}, "%1")]
class CRF_Clothing
{
	int m_iClothingType;
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {
		ParamEnum("", ""), 
		ParamEnum("HEADGEAR", 		"HEADGEAR"), 
		ParamEnum("SHIRT", 			"SHIRT"), 
		ParamEnum("ARMOREDVEST", 	"ARMOREDVEST"), 
		ParamEnum("PANTS", 			"PANTS"), 
		ParamEnum("BOOTS", 			"BOOTS"), 
		ParamEnum("BACKPACK", 		"BACKPACK"), 
		ParamEnum("VEST", 			"VEST"), 
		ParamEnum("HANDWEAR", 		"HANDWEAR"), 
		ParamEnum("HEAD", 			"HEAD"), 
		ParamEnum("EYES", 			"EYES"), 
		ParamEnum("EARS", 			"EARS"), 
		ParamEnum("FACE", 			"FACE"), 
		ParamEnum("NECK", 			"NECK"), 
		ParamEnum("EXTRA1", 		"EXTRA1"), 
		ParamEnum("EXTRA2", 		"EXTRA2"), 
		ParamEnum("WAIST", 			"WAIST"), 
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
			case "HEADGEAR" 	: {m_iClothingType = 0; 	break;}
			case "SHIRT" 		: {m_iClothingType = 1; 	break;}
			case "ARMOREDVEST" 	: {m_iClothingType = 2; 	break;}
			case "PANTS" 		: {m_iClothingType = 3; 	break;}
			case "BOOTS" 		: {m_iClothingType = 4; 	break;}
			case "BACKPACK" 	: {m_iClothingType = 5; 	break;}
			case "VEST" 		: {m_iClothingType = 6; 	break;}
			case "HANDWEAR" 	: {m_iClothingType = 7; 	break;}
			case "HEAD" 		: {m_iClothingType = 8; 	break;}
			case "EYES" 		: {m_iClothingType = 9; 	break;}
			case "EARS" 		: {m_iClothingType = 10; 	break;}
			case "FACE" 		: {m_iClothingType = 11; 	break;}
			case "NECK" 		: {m_iClothingType = 12; 	break;}
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
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sRole"}, "%1")]
class CRF_Role_Custom_Gear : CRF_Role
{	
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