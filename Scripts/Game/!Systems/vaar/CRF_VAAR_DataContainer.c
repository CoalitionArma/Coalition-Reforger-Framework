// ── VAAR playback format v2 (header "v": 2) ──────────────────────────────────
// Single-letter keys keep the JSON small.
// Positions are int ×10 (10 cm precision), angles int ×1000 (degrees). The viewer divides them back.
// Y is omitted (the viewer is 2D) except an aircraft's height above the ground.
// IDs: player ID for players, RplId for AI and vehicles - the same ID everywhere it appears (snapshots,
// shots, kills, vehicle occupants).
// Names, roles and factions are written once per unit in the frame's roster (r / vr) when it is first
// seen or changes; snapshots only carry what changes every frame.
// Map markers (m, Oct 2026 on) are events: a marker is written once when it appears, then once when it is
// removed or (CRF shareable markers) first shared. The first frame lists every marker already placed.

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

//! Map marker event kind (CRF_VAAR_MarkerEvent.e)
enum CRF_EVAARMarkerEvent
{
	PLACED = 0,
	REMOVED = 1,
	SHARED = 2		// a CRF shareable marker reached its teammates
}

//! Map marker kind (CRF_VAAR_MarkerEvent.t)
enum CRF_EVAARMarkerKind
{
	CUSTOM = 0,		// icon, colour and text picked by the player
	MILITARY = 1,	// NATO symbol
	OTHER = 2		// any other synced marker type
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
	string pf;		// v3: character prefab (resource name) - the in-game replay spawns it
	string w;		// v3: prefab of the weapon in hand, "" for none (a roster entry is rewritten when it changes)

	void CRF_VAAR_CharacterInfo(int id, string name, int role, int faction, bool isPlayer, string prefab = "", string weapon = "")
	{
		i = id;
		n = name;
		r = role;
		f = faction;
		if (isPlayer)
			p = 1;
		pf = prefab;
		w = weapon;
	}
}

[BaseContainerProps()]
class CRF_VAAR_VehicleInfo : Managed
{
	int i;			// ID
	string n;		// display name
	int t;			// CRF_EVAARVehicleType
	string pf;		// v3: vehicle prefab (resource name) - the in-game replay spawns it

	void CRF_VAAR_VehicleInfo(int id, string name, int type, string prefab = "")
	{
		i = id;
		n = name;
		t = type;
		pf = prefab;
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
	int a;			// aim yaw ×1000 - counter-clockwise from north (the opposite way to a vehicle's)
	int s;			// CRF_EVAARCharacterState flags
	int y;			// v3: pos Y (height above sea level) ×10 - the vehicle's while riding in one
	int p;			// v3: aim pitch in degrees ×10, up positive
	int t;			// v3: stance (ECharacterStance: 0 standing, 1 crouched, 2 prone)

	void CRF_VAAR_CharacterSnapshot(int id, vector pos, vector aim, int state, int stance)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		y = (int)Math.Round(pos[1] * 10);
		float yawDeg = 0;
		float flat = Math.Sqrt(aim[0] * aim[0] + aim[2] * aim[2]);
		if (Math.AbsFloat(aim[0]) > 0.0001 || Math.AbsFloat(aim[2]) > 0.0001)
			yawDeg = Math.Atan2(aim[0], aim[2]) * Math.RAD2DEG;
		a = (int)Math.Round(yawDeg * 1000);
		p = (int)Math.Round(Math.Atan2(aim[1], flat) * Math.RAD2DEG * 10);
		s = state;
		t = stance;
	}

	//! 3D data off (game mode > AAR Recording): a v2 file, the 3D fields left at 0
	void Strip3D()
	{
		y = 0;
		p = 0;
		t = 0;
	}
}

[BaseContainerProps()]
class CRF_VAAR_VehicleSnapshot : Managed
{
	int i;			// ID
	int x;			// pos X ×10
	int z;			// pos Z ×10
	int a;			// yaw ×1000 - clockwise from north, the same way the map is drawn
	int f;			// faction (changes with the crew)
	int h;			// health 0-100, 0 = destroyed
	int y;			// aircraft height above the ground in metres, 0 for everything else
	int wy;			// v3: pos Y (height above sea level) ×10
	int pi;			// v3: pitch in degrees ×10, nose up positive
	int ro;			// v3: roll in degrees ×10
	ref array<int> oc = {};		// occupant IDs
	ref array<int> os = {};		// occupant seats (CRF_EVAARSeat), same order as oc

	void CRF_VAAR_VehicleSnapshot(int id, vector pos, vector angles, int faction, int health, int altitude)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		wy = (int)Math.Round(pos[1] * 10);
		a = (int)Math.Round(angles[0] * 1000);
		pi = (int)Math.Round(angles[1] * 10);
		ro = (int)Math.Round(angles[2] * 10);
		f = faction;
		h = health;
		y = altitude;
	}

	//! 3D data off (game mode > AAR Recording): a v2 file, the 3D fields left at 0
	void Strip3D()
	{
		wy = 0;
		pi = 0;
		ro = 0;
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
	int y;			// v3: pos Y ×10

	void CRF_VAAR_GrenadeSnapshot(int id, vector pos, int type)
	{
		i = id;
		x = (int)Math.Round(pos[0] * 10);
		z = (int)Math.Round(pos[2] * 10);
		y = (int)Math.Round(pos[1] * 10);
		t = type;
	}

	//! 3D data off (game mode > AAR Recording): a v2 file, the 3D fields left at 0
	void Strip3D()
	{
		y = 0;
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

//! A map marker placed, removed or shared since the last frame. Removed and shared events only carry e and i.
[BaseContainerProps()]
class CRF_VAAR_MarkerEvent : Managed
{
	int e;			// CRF_EVAARMarkerEvent
	int i;			// marker ID
	int x;			// pos X ×10
	int z;			// pos Z ×10
	int t;			// CRF_EVAARMarkerKind
	int f;			// faction that can see it (0 = everyone / unknown, 1 BLUFOR, 2 OPFOR, 3 INDFOR, 4 CIV)
	int o;			// placed by (player ID, -1 = the mission or server)
	string on;		// placed by, name
	int c;			// colour 0xRRGGBB (sRGB)
	string ic;		// custom marker icon (imageset quad name)
	string n;		// text
	int r;			// rotation in degrees
	int s;			// 1 = visible to its whole faction already (not an unshared CRF marker)
	int mi;			// military symbol identity (EMilitarySymbolIdentity)
	string ml;		// military symbol label, e.g. "Land - Infantry"

	void CRF_VAAR_MarkerEvent(int eventKind, int markerId)
	{
		e = eventKind;
		i = markerId;
		o = -1;
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
	int sy;			// v3: start Y ×10 (the shooter's eyes, or the vehicle's origin + 2 m)
	int hy;			// v3: hit Y ×10

	void CRF_VAAR_ShotEvent(int shooterId, vector start, vector hit, int kind)
	{
		si = shooterId;
		sx = (int)Math.Round(start[0] * 10);
		sz = (int)Math.Round(start[2] * 10);
		sy = (int)Math.Round(start[1] * 10);
		hx = (int)Math.Round(hit[0] * 10);
		hz = (int)Math.Round(hit[2] * 10);
		hy = (int)Math.Round(hit[1] * 10);
		k = kind;
	}

	//! 3D data off (game mode > AAR Recording): a v2 file, the 3D fields left at 0
	void Strip3D()
	{
		sy = 0;
		hy = 0;
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
	ref array<ref CRF_VAAR_MarkerEvent>       m = {};   // map markers placed, removed or shared since the last frame
}
