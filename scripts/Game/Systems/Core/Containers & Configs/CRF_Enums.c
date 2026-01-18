/*
	HOW TO ADD A ROLE 101:
	Step 1 - Create the specified role across all character faction prefabs and name it with the method: CRF_GS_(Faction Key)_(Role)_P, ie: 
		CRF_GS_BLUFOR_AAR_P

	Step 2 -  Create a role in all caps with spaces having underscores in the bellow enum class CRF_EGearRole, ie:
		ASSISTANT_AUTOMATIC_RIFLEMAN

	Step 3 -  Now you have to go to the corresponding global file:
		Configs\Gearscripts\CRF_Global_Roles_Config.conf

		- Add a new item into the RoleConfigs array, Set the role with the role you created in step 2
		- Add the prerequisite prefabs you made in step 1
		- Add whatever weapons/gear you want for your new role
		
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
	//-------------------------------------------- OTHER -------------------------------------------------
	ZEUS,
	VIP
}

//------------------------------------------------------------------------------------
// Role helper class for the above Enums
//------------------------------------------------------------------------------------

class CRF_RoleHelper
{	
	//------------------------------------------------------------------------------------------------
	static CRF_EGearRole ResourceToRole(ResourceName roleResource)
	{
		foreach(CRF_RoleConfig roleConfig : CRF_GamemodeManager.RolesConfig().m_RoleConfigs)
			if (roleConfig.m_BluforVariant == roleResource || roleConfig.m_OpforVariant == roleResource || roleConfig.m_IndforVariant == roleResource || roleConfig.m_CivVariant == roleResource)
				return roleConfig.m_Role;
		
		return CRF_EGearRole.RIFLEMAN;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName RoleToResource(CRF_EGearRole roleInt, FactionKey factionKey)
	{
		ResourceName roleResource;
		
		CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();

		CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(roleInt);
		
		switch (factionKey)
		{
			case "BLUFOR":
				roleResource = roleConfig.m_BluforVariant;
				break;
			
			case "OPFOR":
				roleResource = roleConfig.m_OpforVariant;
				break;
			
			case "INDFOR":
				roleResource = roleConfig.m_IndforVariant;
				break;
			
			case "CIV":
				roleResource = roleConfig.m_CivVariant;
				break;
		}
		
		return roleResource;
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

		CRF_EGearRole role = ResourceToRole(prefab);

		return roles.Contains(role);
	}

	// Pulled from the respawn manager, need to find a better solution eventually^tm.
	//------------------------------------------------------------------------------------------------
	static bool IsTeamLeaderRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!IsValidGearscriptResource(prefab))
			return false;

		CRF_EGearRole role = ResourceToRole(prefab);

		return (role == CRF_EGearRole.TEAM_LEAD);
	}
}

//------------------------------------------------------------------------------------
// Enumeration for gearscript clothing
//------------------------------------------------------------------------------------

enum CRF_EGearscriptClothing
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

//------------------------------------------------------------------------------------
// Enumeration for gearscript weapons
//------------------------------------------------------------------------------------

enum CRF_EGearscriptWeapons
{
	RIFLE,
	RIFLEUGL,
	CARBINE,
	SNIPER,
	PISTOL,
	AR,
	AT,
	MMG,
	HMG,
	MAT,
	HAT,
	AA,
}


//------------------------------------------------------------------------------------
// Enumeration for gearscript magazines
//------------------------------------------------------------------------------------

enum CRF_EGearscriptMagazines
{
	RIFLE_MAG,
	RIFLEUGL_MAG,
	CARBINE_MAG,
	SNIPER_MAG,
	PISTOL_MAG,
	AR_MAG,
	AT_MAG,
	MMG_MAG,
	HMG_MAG,
	MAT_MAG,
	HAT_MAG,
	AA_MAG,
}


//------------------------------------------------------------------------------------
// Enumeration for additional gearscript items
//------------------------------------------------------------------------------------

enum CRF_EGearscriptItems
{
	SHORTRANGE_RADIO,
	LONGRANGE_RADIO,
	RTO_RADIO,
	ASSISTANT_BINO,
	LEADERSHIP_BINO,
	MEDIC_ITEMS,
}

//------------------------------------------------------------------------------------
// Enumerations for menus
//------------------------------------------------------------------------------------

modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_AARMenu,
	CRF_AdminMenu,
	CRF_PreviewMenu,
	CRF_RespawnMenu,
	CRF_SlottingMenu,
	CRF_SpectatorMenu,
	CRF_CharacterLoading,
	CRF_GungameStart,
	CRF_GunGameEnd
}

//------------------------------------------------------------------------------------
// Enumerations for our custom factions
//------------------------------------------------------------------------------------

modded enum EEditableEntityLabel
{
	FACTION_CRF_BLUFOR = 51870,
	FACTION_CRF_OPFOR = 51871,
	FACTION_CRF_INDFOR = 51872,
	FACTION_CRF_CIV = 51873,
};

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
	GENERAL_INFANTRY = 0,
	SQUAD_LEADER,
	TEAM_LEADER,
	MEDIC,
	ASSISTANT,
	SPECIALTY,
	SPECIALTY_ASSISTANT,
}

//------------------------------------------------------------------------------------
// Enumeration for polyzone effects
//------------------------------------------------------------------------------------

enum CRF_EPolyZoneEffectHUDType
{
	FreezeZone,
	FreezeZoneLeave,
	RestrictedZone,
	ScreenBlure,
	TriggerCapture,
}

//------------------------------------------------------------------------------------
// Enumeration for vehicle gear script
//------------------------------------------------------------------------------------
enum CRF_EVehicleGearScriptType
{
	Rifle,
	RifleUGL,
	Carbine,
	Pistol,
	AR,
	MMG,
	HMG,
	AT,
	MAT,
	HAT,
	AA,
	Sniper,
	HEGrenade,
	SmokeGrenade,
	HEGL,
	SmokeGl
}