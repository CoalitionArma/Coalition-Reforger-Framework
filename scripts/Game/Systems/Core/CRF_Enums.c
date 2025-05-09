/*
	HOW TO ADD A ROLE 101:
	- Create the specified role across all character faction prefabs and name it with the method: CRF_GS_(Faction Key)_(Role)_P, ie: 
		CRF_GS_BLUFOR_AAR_P

	- Create a "Pretty Name" in all caps with spaces having underscores in the bellow enum class CRF_EGearRole, ie:
		ASSISTANT_AUTOMATIC_RIFLEMAN
	this is to make it easier to search when adding the role to a global/local gearscript array

	- Then just add corresponding case into the CRF_RoleHelper roleFileStrings array bellow using the pretty name you made to match the (Role) value you added to the character prefab (make sure you trail it with a _ and end it with a _P) ie:
	what's on the Enum:
		AUTOMATIC_RIFLEMAN,
		ASSISTANT_AUTOMATIC_RIFLEMAN, <------
		RIFLEMAN,

	what's on the array
		"_AR_P",
		"_AAR_P", <------ NOTE THAT THE POSITION IS THE SAME IN THE ENUM AND IN THE ARRAY AND IS NOT TACKED ONTO THE END OF THE ARRAY!
		"_Rifleman_P",

	- Now you have to go to the corresponding global files:
		(Configs\Gearscripts\CRF_Global_Equipment_Config.conf)
		(Configs\Gearscripts\CRF_Global_Weapons_Config.conf)
	and just add the role you created bellow (make sure you validate and reload scripts) into the correcsponding array(s) of equipment you want it to receive, any custom equipment would have to go through a gear script.

	There, you have added a role, good for you, now stop bothering me about adding in roles manually -Njpatman
*/

enum CRF_EGearRole
{
	UNARMED = 0,
	//-------------------------------------------- LEADERSHIP --------------------------------------------
	COMPANY_COMMANDER,
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
	//-------------------------------------------- SQUAD LEVEL -------------------------------------------
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
	//------------------------------------------- SPECIALITIES -------------------------------------------
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
	//--------------------------------------- VEHICLE SPECIALITIES ---------------------------------------
	VEHICLE_DRIVER,
	VEHICLE_GUNNER,
	VEHICLE_LOADER,
	PILOT,
	CREW_CHIEF,
	LOGI_RUNNER,
	INDIRECT_GUNNER,
	INDIRECT_LOADER,
}

//------------------------------------------------------------------------------------
// Role helper class for the above Enums
//------------------------------------------------------------------------------------

class CRF_RoleHelper
{
	protected ref static array<string> roleFileStrings = {
		"_Unarmed_P",
		//-------------------------------------------- LEADERSHIP --------------------------------------------
		"_COY_P",
		"_1SG_P",
		"_PL_P",
		"_PSG_P",
		"_MO_P",
		"_FO_P",
		"_JTAC_P",
		"_SL_P",
		"_VehLead_P",
		"_IndirectLead_P",
		"_LogiLead_P",
		//-------------------------------------------- SQUAD LEVEL -------------------------------------------
		"_TL_P",
		"_Medic_P",
		"_RTO_P",
		"_Gren_P",
		"_AR_P",
		"_AAR_P",
		"_Rifleman_P",
		"_AT_P",
		"_AAT_P",
		"_Demo_P",
		//------------------------------------------- SPECIALITIES -------------------------------------------
		"_HAT_P",
		"_AHAT_P",
		"_MAT_P",
		"_AMAT_P",
		"_HMG_P",
		"_AHMG_P",
		"_MMG_P",
		"_AMMG_P",
		"_AA_P",
		"_AAA_P",
		"_Sniper_P",
		"_Spotter_P",
		"_DroneOp_P",
		"_ComEngi_P",
		//--------------------------------------- VEHICLE SPECIALITIES ---------------------------------------
		"_VehDriver_P",
		"_VehGunner_P",
		"_VehLoader_P",
		"_Pilot_P",
		"_CrewChief_P",
		"_LogiRunner_P",
		"_IndirectGunner_P",
		"_IndirectLoader_P"
	};
	
	//------------------------------------------------------------------------------------------------
	static string RoleToString(CRF_EGearRole roleInt)
	{
		return roleFileStrings.Get(roleInt);
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_EGearRole StringToRole(string roleString)
	{
		return roleFileStrings.Find(roleString);
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName RoleToResource(CRF_EGearRole roleInt, FactionKey faction)
	{
		Resource resource = Resource.Load("Prefabs/Characters/Factions/" + faction + "/CRF_GS_" + faction + RoleToString(roleInt) + ".et");
		return resource.GetResource().GetResourceName();
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsValidGearscriptResource(ResourceName resource)
	{
		return resource.Contains("CRF_GS_");
	};
	
	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsSquadLeaderRole(IEntity entity)
	{
		ref TIntArray roles = {CRF_EGearRole.COMPANY_COMMANDER, CRF_EGearRole.PLATOON_LEADER, CRF_EGearRole.MEDICAL_OFFICER, CRF_EGearRole.SQUAD_LEAD, CRF_EGearRole.VEHICLE_LEAD, CRF_EGearRole.INDIRECT_LEAD, CRF_EGearRole.LOGI_LEAD};
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		int role = StringToRole(PrefabToRole(prefab));

		return roles.Contains(role);
	}

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsTeamLeaderRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		int role = StringToRole(PrefabToRole(prefab));

		return (role == CRF_EGearRole.TEAM_LEAD);
	}

	// Converts a full resource name to a role.
	//------------------------------------------------------------------------------------------------
	static string PrefabToRole(ResourceName prefab)
	{
		array<string> value = {};
		prefab.Split("_", value, true);

		string role = "_" + value[3] + "_" + value[4];

		role.Split(".", value, true);
		role = value[0];

		return role;
	}
}

//------------------------------------------------------------------------------------
// Enumerations for game state tracking
//------------------------------------------------------------------------------------

enum CRF_EGamemodeState
{
	BRIEFING,   // Initial mission briefing phase
	SLOTTING,   // Player role selection phase
	GAME,       // Active gameplay phase
	AAR         // After Action Report phase
}

enum CRF_ESlottingState
{
	LEADERSANDMEDICS,  // Only leaders and medics can select slots
	SPECIALTIES,       // Specialist roles become available
	EVERYONE           // All roles available to all players
}

//------------------------------------------------------------------------------------
// Enumeration for group flag types
//------------------------------------------------------------------------------------

enum CRF_EFlagType
{
	INFANTRY = 0,
	AMPHIBIOUS,
	ANTI_AIR,
	ANTI_AIR_ARTILLERY,
	ANTI_ARMOR_MOTORIZED,
	ANTI_ARMOR,
	ARMORED,
	ARTILLERY,
	COMBINED,
	HELICOPER,
	ATTACK_HELICOPTER,
	INFANTRY_AIR,
	MACHINEGUN,
	MAINTENANCE,
	MEDICAL,
	MORTAR,
	MOTORIZED,
	MOTORIZED_INFANTRY,
	RECON,
	RECON_MOTORIZED,
	SIGNAL,
	SNIPER,
	SUPPLY_MOTORIZED,
	UNDEFINED,
}

//------------------------------------------------------------------------------------
// Enumeration for slot types
//------------------------------------------------------------------------------------

enum CRF_ESlotType
{
	REGULAR = 0,
	LEADERORMEDIC,
	SPECIALTY,
}

//------------------------------------------------------------------------------------
// Enumeration for clothing types
//------------------------------------------------------------------------------------

enum CRF_EClothingType
{
	HEADGEAR = 0,
	SHIRT,
	ARMOREDVEST,
	PANTS,
	BOOTS,
	BACKPACK,
	VEST,
	HANDWEAR,
	HEAD,
	EYES,
	EARS,
	FACE,
	NECK,
	EXTRA1,
	EXTRA2,
	WAIST,
	EXTRA3,
	EXTRA4,
}