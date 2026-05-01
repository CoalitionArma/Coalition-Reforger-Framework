enum CRF_VAAR_EEventTypes
{
	KILL
}

// Lower level Structure
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_CharacterSnapshot : Managed
{
    RplId characterID;
    string characterName, characterrole;
    float characterposX, characterposY, characterposZ, characteryaw;
	FactionKey characterFaction;
    
    void CRF_VAAR_CharacterSnapshot(RplId id, string name, vector pos, vector aim, string role, FactionKey key)
    {
		characterID = id;
        characterName = name;
        characterposX = pos[0];
        characterposY = pos[1];
        characterposZ = pos[2];
        characteryaw = aim[0];
		characterrole = role;
		characterFaction = key;
    }
}

// This represents single Vehicle
[BaseContainerProps()]
class CRF_VAAR_VehicleSnapshot : Managed
{
    RplId vehicleID;
    string vehicleName, vehicleType;
    float vehicleposX, vehicleposY, vehicleposZ, vehicleyaw;
	FactionKey vehicleFaction;
	ref array<string> vehicleOccupants = {};
	
    void CRF_VAAR_VehicleSnapshot(RplId id, string name, vector pos, vector aim, string type, FactionKey key, array<string> occupants)
    {
		vehicleID = id;
        vehicleName = name;
        vehicleposX = pos[0];
        vehicleposY = pos[1];
        vehicleposZ = pos[2];
        vehicleyaw = aim[0];
		vehicleType = type;
		vehicleFaction = key;
		
		foreach(string occupant : occupants)
		{
			vehicleOccupants.Insert(occupant);
		}
    }
}

// This reperesents shot being fired by a player or AI
[BaseContainerProps()]
class CRF_VAAR_ShotEvent : Managed
{
    RplId shooterID;
    float startX, startZ, shotHitX, shotHitZ;
    
    void CRF_VAAR_ShotEvent(RplId id, vector start, float hitX, float hitZ)
    {
        shooterID = id;
        startX = start[0]; 
		startZ = start[2];
        shotHitX = hitX; 
		shotHitZ = hitZ;
    }
}

// This reperesents a kill
[BaseContainerProps()]
class CRF_VAAR_KillEvent : Managed
{
    RplId targetID, KillerID;
	string targetName, targetFaction;
	string killerName, killerFaction; 
	
    void CRF_VAAR_KillEvent(RplId target, RplId killer, string tName, string kName, FactionKey tFaction, FactionKey kFaction)
    {
		targetID = target;
		targetName = tName;
		targetFaction = tFaction;
		KillerID = killer;
		killerName = kName;
		killerFaction = kFaction;
    }
}

// Top level Structure
//------------------------------------------------------------------------------------

// This represents all Characters & Vehicles
[BaseContainerProps()]
class CRF_VAAR_EntitiesSnapshot : Managed
{
    ref array<ref CRF_VAAR_CharacterSnapshot> Characters = {};
	ref array<ref CRF_VAAR_VehicleSnapshot> Vehicles = {};
}

// A single frame
[BaseContainerProps()]
class CRF_VAAR_Frame : Managed
{
    float Timestamp;
    ref array<ref CRF_VAAR_EntitiesSnapshot> Entities = {};
	ref array<ref CRF_VAAR_ShotEvent> Shots = {};
	ref array<ref CRF_VAAR_KillEvent> Kills = {};
}