// Lower level Structure
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_CharacterSnapshot : Managed
{
    RplId i; // Replication ID
    string n // Player name or (AI)
    float x, y, z, yaw;
	int f, r; // Faction, Role (CRF_EGearRole)
   
    void CRF_VAAR_CharacterSnapshot(RplId id, string name, vector pos, vector aim, CRF_EGearRole role, int faction)
    {
		i = id;
        n = name;
        x = RoundCoord(pos[0]);
        y = RoundCoord(pos[1]);
        z = RoundCoord(pos[2]);
        yaw = RoundYaw(aim[0]);
		r = role;
		f = faction;
    }
}

// This represents single Vehicle
[BaseContainerProps()]
class CRF_VAAR_VehicleSnapshot : Managed
{
    RplId i; // Replicaiton ID
    string n; // Name
    float x, y, z, yaw;
	int f, t; // Faction, Type
	ref array<string> o = {}; // Occupants
	
    void CRF_VAAR_VehicleSnapshot(RplId id, string name, vector pos, vector aim, int type, int faction, array<string> occupants)
    {
		i = id;
        n = name;
        x = RoundCoord(pos[0]);
        y = RoundCoord(pos[1]);
        z = RoundCoord(pos[2]);
        yaw = RoundYaw(aim[0]);
		t = type;
		f = faction;
		
		foreach(string occupant : occupants)
		{
			o.Insert(occupant);
		}
    }
}

// This reperesents shot being fired by a player or AI
[BaseContainerProps()]
class CRF_VAAR_ShotEvent : Managed
{
    float x, z // Origin vector
	float ix, iz; // Impact vector
    
    void CRF_VAAR_ShotEvent(vector start, float impactX, float impactZ)
    {
        x = RoundCoord(start[0]); 
		z = RoundCoord(start[2]);
       	ix = RoundCoord(impactX); 
		iz = RoundCoord(impactZ);
    }
}

// This reperesents a kill
[BaseContainerProps()]
class CRF_VAAR_KillEvent : Managed
{
	string tn; // Target Name
	int tf; // Target Faction
	string kn; // Killer Name
	int kf; // Killer Faction
	
    void CRF_VAAR_KillEvent(string targetName, string killerName, int targetFaction, int killerFaction)
    {
		tn = targetName;
		tf = targetFaction;
		kn = killerName;
		kf = killerFaction;
    }
}

// Top level Structure
//------------------------------------------------------------------------------------

// This represents all Characters & Vehicles
[BaseContainerProps()]
class CRF_VAAR_EntitiesSnapshot : Managed
{
    ref array<ref CRF_VAAR_CharacterSnapshot> Chars = {};
	ref array<ref CRF_VAAR_VehicleSnapshot> Vehs = {};
}

// A single frame
[BaseContainerProps()]
class CRF_VAAR_Frame : Managed
{
    int Time;
    ref array<ref CRF_VAAR_EntitiesSnapshot> Ents = {};
	ref array<ref CRF_VAAR_ShotEvent> Shots = {};
	ref array<ref CRF_VAAR_KillEvent> Kills = {};
}

// Helpers
//------------------------------------------------------------------------------------
float RoundCoord(float value)
{
	return Math.Round(value * 10) /10;
}

float RoundYaw(float value)
{
	return Math.Round(value * 100) /100;
}