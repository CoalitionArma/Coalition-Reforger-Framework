enum CRF_VAAR_EEventTypes
{
	KILL
}

// ── Key naming: single-letter short keys to minimise JSON file size ──────────
// Positions stored as int ×10 (10cm precision). Viewer divides by 10.
// Angle stored as int ×1000. Viewer divides by 1000.
// Y-coordinate omitted — the playback viewer is 2D only.

// Lower level Structure
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_CharacterSnapshot : Managed
{
	RplId i;       // characterID
	string n;      // characterName
	string r;      // characterrole
	int x;         // characterposX ×10
	int z;         // characterposZ ×10
	int a;         // characteryaw ×1000
	FactionKey f;  // characterFaction

	void CRF_VAAR_CharacterSnapshot(RplId id, string name, vector pos, vector aim, string role, FactionKey key)
	{
		i = id;
		n = name;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		a = (int)Math.Round(aim[0] * 1000);
		r = role;
		f = key;
	}
}

// This represents a single Vehicle
[BaseContainerProps()]
class CRF_VAAR_VehicleSnapshot : Managed
{
	RplId i;             // vehicleID
	string n;            // vehicleName
	string t;            // vehicleType
	int x;               // vehicleposX ×10
	int z;               // vehicleposZ ×10
	int a;               // vehicleyaw ×1000
	FactionKey f;        // vehicleFaction
	ref array<string> oc = {};  // vehicleOccupants

	void CRF_VAAR_VehicleSnapshot(RplId id, string name, vector pos, vector angles, string type, FactionKey key, array<string> occupants)
	{
		i = id;
		n = name;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		a = (int)Math.Round(angles[0] * 1000);
		t = type;
		f = key;

		foreach(string occupant : occupants)
		{
			oc.Insert(occupant);
		}
	}
}

// This represents a shot fired by a player or AI
[BaseContainerProps()]
class CRF_VAAR_ShotEvent : Managed
{
	RplId si;  // shooterID
	int sx;    // startX ×10
	int sz;    // startZ ×10
	int hx;    // shotHitX ×10
	int hz;    // shotHitZ ×10

	void CRF_VAAR_ShotEvent(RplId id, vector start, float hitX, float hitZ)
	{
		si = id;
		sx = (int)Math.Round(start[0] * 10);
		sz = (int)Math.Round(start[2] * 10);
		hx = (int)Math.Round(hitX * 10);
		hz = (int)Math.Round(hitZ * 10);
	}
}

// This represents a kill
[BaseContainerProps()]
class CRF_VAAR_KillEvent : Managed
{
	RplId ti;   // targetID
	RplId ki;   // killerID
	string tn;  // targetName
	string kn;  // killerName
	string tf;  // targetFaction
	string kf;  // killerFaction

	void CRF_VAAR_KillEvent(RplId target, RplId killer, string tName, string kName, FactionKey tFaction, FactionKey kFaction)
	{
		ti = target;
		tn = tName;
		tf = tFaction;
		ki = killer;
		kn = kName;
		kf = kFaction;
	}
}

// Top level Structure — Characters & Vehicles live directly on the frame
// (eliminates the always-single-element Entities wrapper array)
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_Frame : Managed
{
	float ts;  // Timestamp
	ref array<ref CRF_VAAR_CharacterSnapshot> c = {};  // Characters
	ref array<ref CRF_VAAR_VehicleSnapshot>   v = {};  // Vehicles
	ref array<ref CRF_VAAR_ShotEvent>         s = {};  // Shots
	ref array<ref CRF_VAAR_KillEvent>         k = {};  // Kills
}