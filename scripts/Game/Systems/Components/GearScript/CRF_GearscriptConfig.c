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
	[Attribute("1", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(EGearRole), desc: "All roles that get rifes")]
	ref array<EGearRole> m_aRolesThatGetRifles;
	
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

/*
	HOW TO ADD A ROLE 101:
	- Create the specified role across all character faction prefabs and name it with the method: CRF_GS_(Faction Key)_(Role)_P, ie: 
		CRF_GS_BLUFOR_CombatEng_P

	- Create a "Pretty Name" in all caps with spaces having underscores in the bellow enum class EGearRole, ie:
		COMBAT_ENGINEER
	this is to make it easier to search when adding the role to a global/local gearscript array

	- Then just add corresponding case into the switch function bellow using pretty name to match the (Role) value you added to the character prefab (make sure you trail it with a _ and end it with a _P) ie:
		case EGearRole.COMBAT_ENGINEER : {m_sRole = "_CombatEng_P"; break;}

	- Now you have to go to the corresponding global files:
		(Configs\Gearscripts\CRF_Global_Equipment_Config.conf)
		(Configs\Gearscripts\CRF_Global_Weapons_Config.conf)
	and just add the role you created bellow (make sure you validate and reload scripts) into the correcsponding array(s) of equipment you want it to receive, any custom equipment would have to go through a gear script.

	There, you have added a role, good for you, now stop bothering me about adding in roles manually -Njpatman
*/

enum EGearRole
{
	NONE							= 0,
	COMPANY_COMMANDER				= "_COY_P",
	FIRST_SERGEANT,
	PLATOON_LEADER,
	PLATOON_SERGEANT,
	MEDICAL_OFFICER,
	FORWARD_OBSERVER,
	JTAC,
	SQUAD_LEAD,
	VEHICLE_LEAD,
	INDIRECT_LEAD,
	LOGI_LEAD,
	TEAM_LEAD,
	MEDIC,
	RADIO_TELEPHONE_OPERATOR,
	GRENADIER,
	AUTOMATIC_RIFLEMAN,
	ASSISTANT_AUTOMATIC_RIFLEMAN,
	RIFLEMAN,
	RIFLEMAN_ANTITANK,
	ASSISTANT_RIFLEMAN_ANTITANK,
	RIFLEMAN_DEMO,
	HEAVY_ANTITANK,
	ASSISTANT_HEAVY_ANTITANK,
	MEDIUM_ANTITANK,
	ASSISTANT_MEDIUM_ANTITANK,
	HEAVY_MACHINEGUN,
	ASSISTANT_HEAVY_MACHINEGUN,
	MEDIUM_MACHINEGUN,
	ASSISTANT_MEDIUM_MACHINEGUN,
	ANTI_AIR,
	ASSISTANT_ANTI_AIR,
	SNIPER,
	SPOTTER,
	DRONE_OPERATOR,
	COMBAT_ENGINEER,
	VEHICLE_DRIVER,
	VEHICLE_GUNNER,
	VEHICLE_LOADER,
	PILOT,
	CREW_CHIEF,
	LOGI_RUNNER,
	INDIRECT_GUNNER,
	INDIRECT_LOADER,
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_iRole"}, "%1")]
class CRF_Role
{	
	string m_sRole;
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(EGearRole))]
	EGearRole m_iRole;
	
	//------------------------------------------------------------------------------------------------
	void CRF_Role()
	{
		m_sRole = ReturnRoleString(m_iRole);
	}
	
	static string ReturnRoleString(EGearRole roleInt)
	{
		string role = "";
		
		switch(roleInt)
		{
			//-------------------------------------------- LEADERSHIP --------------------------------------------
			case EGearRole.COMPANY_COMMANDER : 				{role = "_COY_P"; 				break;}
			case EGearRole.FIRST_SERGEANT : 				{role = "_1SG_P"; 				break;}
			case EGearRole.PLATOON_LEADER : 				{role = "_PL_P"; 				break;}
			case EGearRole.PLATOON_SERGEANT : 				{role = "_PSG_P"; 				break;}
			case EGearRole.MEDICAL_OFFICER : 				{role = "_MO_P"; 				break;}
			case EGearRole.FORWARD_OBSERVER : 				{role = "_FO_P"; 				break;}
			case EGearRole.JTAC : 							{role = "_JTAC_P"; 				break;}
			case EGearRole.SQUAD_LEAD : 					{role = "_SL_P"; 				break;}
			case EGearRole.VEHICLE_LEAD : 					{role = "_VehLead_P"; 			break;}
			case EGearRole.INDIRECT_LEAD :					{role = "_IndirectLead_P"; 		break;}
			case EGearRole.LOGI_LEAD :						{role = "_LogiLead_P"; 			break;}
			//-------------------------------------------- SQUAD LEVEL -------------------------------------------
			case EGearRole.TEAM_LEAD : 						{role = "_TL_P"; 				break;}
			case EGearRole.MEDIC : 							{role = "_Medic_P"; 			break;}
			case EGearRole.RADIO_TELEPHONE_OPERATOR : 		{role = "_RTO_P"; 				break;}
			case EGearRole.GRENADIER : 						{role = "_Gren_P"; 				break;}
			case EGearRole.AUTOMATIC_RIFLEMAN : 			{role = "_AR_P"; 				break;}
			case EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN : 	{role = "_AAR_P"; 				break;}
			case EGearRole.RIFLEMAN : 						{role = "_Rifleman_P"; 			break;}
			case EGearRole.RIFLEMAN_ANTITANK : 				{role = "_AT_P"; 				break;}
			case EGearRole.ASSISTANT_RIFLEMAN_ANTITANK : 	{role = "_AAT_P"; 				break;}
			case EGearRole.RIFLEMAN_DEMO :					{role = "_Demo_P"; 				break;}
			//------------------------------------------- SPECIALITIES -------------------------------------------
			case EGearRole.HEAVY_ANTITANK : 				{role = "_HAT_P"; 				break;}
			case EGearRole.ASSISTANT_HEAVY_ANTITANK : 		{role = "_AHAT_P"; 				break;}
			case EGearRole.MEDIUM_ANTITANK : 				{role = "_MAT_P"; 				break;}
			case EGearRole.ASSISTANT_MEDIUM_ANTITANK : 		{role = "_AMAT_P"; 				break;}
			case EGearRole.HEAVY_MACHINEGUN : 				{role = "_HMG_P"; 				break;}
			case EGearRole.ASSISTANT_HEAVY_MACHINEGUN : 	{role = "_AHMG_P"; 				break;}
			case EGearRole.MEDIUM_MACHINEGUN : 				{role = "_MMG_P"; 				break;}
			case EGearRole.ASSISTANT_MEDIUM_MACHINEGUN : 	{role = "_AMMG_P"; 				break;}
			case EGearRole.ANTI_AIR : 						{role = "_AA_P"; 				break;}
			case EGearRole.ASSISTANT_ANTI_AIR : 			{role = "_AAA_P"; 				break;}
			case EGearRole.SNIPER : 						{role = "_Sniper_P"; 			break;}
			case EGearRole.SPOTTER : 						{role = "_Spotter_P"; 			break;}
			case EGearRole.DRONE_OPERATOR : 				{role = "_DroneOp_P"; 			break;}
			case EGearRole.COMBAT_ENGINEER : 				{role = "_ComEngi_P"; 			break;}
			//--------------------------------------- VEHICLE SPECIALITIES ---------------------------------------
			case EGearRole.VEHICLE_DRIVER : 				{role = "_VehDriver_P"; 		break;}
			case EGearRole.VEHICLE_GUNNER : 				{role = "_VehGunner_P"; 		break;}
			case EGearRole.VEHICLE_LOADER : 				{role = "_VehLoader_P"; 		break;}
			case EGearRole.PILOT : 							{role = "_Pilot_P"; 			break;}
			case EGearRole.CREW_CHIEF : 					{role = "_CrewChief_P"; 		break;}
			case EGearRole.LOGI_RUNNER : 					{role = "_LogiRunner_P"; 		break;}
			case EGearRole.INDIRECT_GUNNER : 				{role = "_IndirectGunner_P"; 	break;}
			case EGearRole.INDIRECT_LOADER : 				{role = "_IndirectLoader_P"; 	break;}
		}
		
		return role;
	}
	
	static ResourceName ReturnRoleResource(int roleInt, FactionKey faction)
	{
		return SCR_Global.GetResourceName("Prefabs/Characters/Factions/" + faction + "/CRF_GS_" + faction + ReturnRoleString(roleInt) + ".et");
	}
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