// ── VAAR playback format v2 (header "v": 2) ──────────────────────────────────
// Single-letter keys keep the JSON small.
// Positions are int ×10 (10 cm precision), angles int ×1000 (degrees). The viewer divides them back.
// Y is omitted (the viewer is 2D) except an aircraft's height above the ground.
// IDs: player ID for players, RplId for AI and vehicles - the same ID everywhere it appears (snapshots,
// shots, kills, vehicle occupants).
// Names, roles and factions are written once per unit in the frame's roster (r / vr) when it is first
// seen or changes; snapshots only carry what changes every frame.

//! Character snapshot flags (CRF_VAAR_CharacterSnapshot.s)
enum CRF_EVAARCharacterState
{
	UNCONSCIOUS = 1,
	IN_VEHICLE = 2
}

//! Vehicle categories (CRF_VAAR_VehicleInfo.t). 1-5 keep their v1 values.
enum CRF_EVAARVehicleType
{
	UNKNOWN = 0,
	APC = 1,
	CAR = 2,
	TRUCK = 3,
	TANK = 4,
	MORTAR = 5,
	HELICOPTER = 6,
	PLANE = 7,
	BOAT = 8,
	SUPPORT = 9		// fuel, supply, repair, comms and arsenal trucks
}

//! Seat a vehicle occupant is in (CRF_VAAR_VehicleSnapshot.os)
enum CRF_EVAARSeat
{
	DRIVER = 0,		// driver or pilot
	GUNNER = 1,
	PASSENGER = 2
}

//! What hit (CRF_VAAR_ShotEvent.k)
enum CRF_EVAARShotKind
{
	BULLET = 0,
	GRENADE = 1,
	SHELL = 2,		// mortar / artillery
	ROCKET = 3
}

//! Thrown grenade type (CRF_VAAR_GrenadeSnapshot.t / CRF_VAAR_GrenadeThrowEvent.t)
enum CRF_EVAARGrenadeType
{
	EXPLOSIVE = 0,
	SMOKE = 1
}

// Roster entries
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_CharacterInfo : Managed
{
	int i;			// ID
	string n;		// name ("AI" for AI)
	int r;			// COA_EGearRole
	int f;			// faction (0 unknown, 1 BLUFOR, 2 OPFOR, 3 INDFOR, 4 CIV)
	int p;			// 1 = player

	void CRF_VAAR_CharacterInfo(int id, string name, int role, int faction, bool isPlayer)
	{
		i = id;
		n = name;
		r = role;
		f = faction;
		if (isPlayer)
			p = 1;
	}
}

[BaseContainerProps()]
class CRF_VAAR_VehicleInfo : Managed
{
	int i;			// ID
	string n;		// display name
	int t;			// CRF_EVAARVehicleType

	void CRF_VAAR_VehicleInfo(int id, string name, int type)
	{
		i = id;
		n = name;
		t = type;
	}
}

// Per-frame snapshots
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_CharacterSnapshot : Managed
{
	int i;			// ID
	int x;			// pos X ×10
	int z;			// pos Z ×10
	int a;			// aim yaw ×1000
	int s;			// CRF_EVAARCharacterState flags

	void CRF_VAAR_CharacterSnapshot(int id, vector pos, vector aim, int state)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		float yawDeg = 0;
		if (Math.AbsFloat(aim[0]) > 0.0001 || Math.AbsFloat(aim[2]) > 0.0001)
			yawDeg = Math.Atan2(aim[0], aim[2]) * Math.RAD2DEG;
		a = (int)Math.Round(yawDeg * 1000);
		s = state;
	}
}

[BaseContainerProps()]
class CRF_VAAR_VehicleSnapshot : Managed
{
	int i;			// ID
	int x;			// pos X ×10
	int z;			// pos Z ×10
	int a;			// yaw ×1000
	int f;			// faction (changes with the crew)
	int h;			// health 0-100, 0 = destroyed
	int y;			// aircraft height above the ground in metres, 0 for everything else
	ref array<int> oc = {};		// occupant IDs
	ref array<int> os = {};		// occupant seats (CRF_EVAARSeat), same order as oc

	void CRF_VAAR_VehicleSnapshot(int id, vector pos, vector angles, int faction, int health, int altitude)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		a = (int)Math.Round(angles[0] * 1000);
		f = faction;
		h = health;
		y = altitude;
	}
}

//! A thrown grenade, every frame from the throw until it explodes or its smoke runs out
[BaseContainerProps()]
class CRF_VAAR_GrenadeSnapshot : Managed
{
	int i;			// grenade ID (numbered per recording, see CRF_VAAR_GrenadeThrowEvent)
	int x;			// pos X ×10
	int z;			// pos Z ×10
	int t;			// CRF_EVAARGrenadeType

	void CRF_VAAR_GrenadeSnapshot(int id, vector pos, int type)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		t = type;
	}
}

// Events
//------------------------------------------------------------------------------------
//! Written once, in the frame a grenade is thrown
[BaseContainerProps()]
class CRF_VAAR_GrenadeThrowEvent : Managed
{
	int i;			// grenade ID
	int ti;			// thrower ID
	int t;			// CRF_EVAARGrenadeType

	void CRF_VAAR_GrenadeThrowEvent(int grenadeId, int throwerId, int type)
	{
		i = grenadeId;
		ti = throwerId;
		t = type;
	}
}

[BaseContainerProps()]
class CRF_VAAR_ShotEvent : Managed
{
	int si;			// shooter ID
	int sx;			// start X ×10
	int sz;			// start Z ×10
	int hx;			// hit X ×10
	int hz;			// hit Z ×10
	int k;			// CRF_EVAARShotKind

	void CRF_VAAR_ShotEvent(int shooterId, vector start, float hitX, float hitZ, int kind)
	{
		si = shooterId;
		sx = (int)Math.Round(start[0] * 10);
		sz = (int)Math.Round(start[2] * 10);
		hx = (int)Math.Round(hitX * 10);
		hz = (int)Math.Round(hitZ * 10);
		k = kind;
	}
}

[BaseContainerProps()]
class CRF_VAAR_KillEvent : Managed
{
	int ki;			// killer ID (0 = none / environment)
	int ti;			// victim ID
	string kn;		// killer name (kept so name-only readers still work)
	string tn;		// victim name
	int kf;			// killer faction
	int tf;			// victim faction
	string w;		// weapon, or the vehicle the killer was in
	int d;			// distance in metres, -1 when unknown
	int x;			// victim pos X ×10
	int z;			// victim pos Z ×10

	void CRF_VAAR_KillEvent(int killerId, int targetId, string killerName, string targetName, int killerFaction, int targetFaction, string weapon, int distance, vector targetPos)
	{
		ki = killerId;
		ti = targetId;
		kn = killerName;
		tn = targetName;
		kf = killerFaction;
		tf = targetFaction;
		w = weapon;
		d = distance;
		x = (int)Math.Round(targetPos[0] * 10);
		z = (int)Math.Round(targetPos[2] * 10);
	}
}

// Top level structure
//------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_VAAR_Frame : Managed
{
	float ts;		// frame index (seconds = ts × header interval)
	ref array<ref CRF_VAAR_CharacterInfo>     r = {};   // character roster changes
	ref array<ref CRF_VAAR_VehicleInfo>       vr = {};  // vehicle roster changes
	ref array<ref CRF_VAAR_CharacterSnapshot> c = {};   // characters
	ref array<ref CRF_VAAR_VehicleSnapshot>   v = {};   // vehicles
	ref array<ref CRF_VAAR_ShotEvent>         s = {};   // shots
	ref array<ref CRF_VAAR_KillEvent>         k = {};   // kills
	ref array<ref CRF_VAAR_GrenadeSnapshot>   g = {};   // thrown grenades in the world
	ref array<ref CRF_VAAR_GrenadeThrowEvent> gt = {};  // grenades thrown since the last frame
}
