//------------------------------------------------------------------------------------------------
//! In-game replays: players open recorded missions (CRF_VAAR_GamemodeComponent logs) from the main menu
//! and watch them locally, with the real soldiers, vehicles and effects on the real terrain.
//!
//!  CRF_ReplayBrowserMenu   main menu > REPLAYS: missions on this PC and on coalitiongroup.net
//!  CRF_ReplayDownloader    fetches a recording from the website, a minute of frames per request
//!  CRF_ReplayLauncher      loads the replay world for the mission's terrain (CRF_ReplayWorlds) as a local game
//!  CRF_ReplayGameMode      the replay worlds' game mode: nobody spawns, it starts the player
//!  CRF_ReplayPlayer        reads the recording a frame at a time and puts the battle back together
//!  CRF_ReplayHUDMenu       timeline, speed, follow, kill feed
//!
//! Recordings live in $profile:vaar/replays as replay_<missionId>.json (the recorder's own format: a
//! header line, then one {"frame":{...}} per line) with a replay_<missionId>.meta.json next to them.
class CRF_Replay
{
	static const string API_BASE = "https://api.coalitiongroup.net/api";
	static const string DIR = "$profile:vaar/replays";

	// GameSessionStorage keys - survive the world change from the main menu to the terrain
	static const string STORAGE_FILE = "CRF_Replay_File";
	static const string STORAGE_WORLD = "CRF_Replay_World";
	static const string STORAGE_TITLE = "CRF_Replay_Title";

	//------------------------------------------------------------------------------------------------
	static string ReplayPath(int missionId)
	{
		return string.Format("%1/replay_%2.json", DIR, missionId);
	}

	//------------------------------------------------------------------------------------------------
	static string MetaPath(int missionId)
	{
		return string.Format("%1/replay_%2.meta.json", DIR, missionId);
	}

	//------------------------------------------------------------------------------------------------
	static string Clock(float seconds)
	{
		int s = Math.Max(0, Math.Floor(seconds));
		int h = s / 3600;
		int m = (s % 3600) / 60;
		int sec = s % 60;
		string mm = m.ToString(2);
		string ss = sec.ToString(2);
		if (h > 0)
			return string.Format("%1:%2:%3", h, mm, ss);

		return string.Format("%1:%2", mm, ss);
	}

	//------------------------------------------------------------------------------------------------
	static string SizeLabel(int bytes)
	{
		if (bytes >= 1048576)
			return string.Format("%1 MB", Math.Round(bytes / 104857.6) / 10);

		return string.Format("%1 KB", Math.Ceil(bytes / 1024));
	}
}

//------------------------------------------------------------------------------------------------
//! A downloaded replay (replay_<id>.meta.json)
class CRF_ReplayMeta
{
	int id;
	string name;
	string terrain;
	string terrainKey;
	string playedAt;
	int frames;
	int chunks;
	int chunksDone;
	bool complete;
}

//------------------------------------------------------------------------------------------------
//! A recording on the website (GET /aar/replays)
class CRF_ReplayListEntry
{
	int id;
	string name;
	string author;
	string terrain;
	string terrainKey;
	string result;
	string playedAt;
	int sizeBytes;
	int version;
}

//------------------------------------------------------------------------------------------------
//! GET /aar/<id>/replay
class CRF_ReplayInfo
{
	int id;
	string name;
	string terrainKey;
	string header;
	int frames;
	int chunkFrames;
	int chunks;
}

//------------------------------------------------------------------------------------------------
// Recording frames, the shape CRF_VAAR_DataContainer writes (short names on purpose - they're the JSON keys).
// Each reads its own fields with defaults: every format version leaves some out (v1 has no IDs on kills,
// no vehicle health, names inline on every character), and v1's occupant list holds strings.
//------------------------------------------------------------------------------------------------
class CRF_ReplayCharInfo
{
	int i;			// ID
	string n;		// name
	int r;			// COA_EGearRole
	int f;			// faction (1 BLUFOR, 2 OPFOR, 3 INDFOR, 4 CIV)
	int p;			// 1 = player
	string pf;		// prefab (v3)
	string w;		// weapon in hand (v3)

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(i, 0);
		context.ReadDefault(n, "");
		context.ReadDefault(r, 0);
		context.ReadDefault(f, 0);
		context.ReadDefault(p, 0);
		context.ReadDefault(pf, "");
		context.ReadDefault(w, "");
		return true;
	}
}

class CRF_ReplayVehInfo
{
	int i;
	string n;
	int t;			// CRF_EVAARVehicleType
	string pf;		// prefab (v3)

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(i, 0);
		context.ReadDefault(n, "");
		context.ReadDefault(t, 0);
		context.ReadDefault(pf, "");
		return true;
	}
}

class CRF_ReplayChar
{
	int i;
	int x;			// ×10
	int z;
	int a;			// aim yaw ×1000, counter-clockwise from north
	int s;			// CRF_EVAARCharacterState flags
	int y;			// height ×10 (v3)
	int p;			// aim pitch ×10 (v3)
	int t;			// stance (v3)
	string n;		// v1: name, role and faction on every character (later versions use the roster)
	int r;
	int f;

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(i, 0);
		context.ReadDefault(x, 0);
		context.ReadDefault(z, 0);
		context.ReadDefault(a, 0);
		context.ReadDefault(s, 0);
		context.ReadDefault(y, 0);
		context.ReadDefault(p, 0);
		context.ReadDefault(t, 0);
		context.ReadDefault(n, "");
		context.ReadDefault(r, 0);
		context.ReadDefault(f, 0);
		return true;
	}
}

class CRF_ReplayVeh
{
	int i;
	int x;
	int z;
	int a;			// yaw ×1000, clockwise from north
	int f;
	int h;			// health 0-100 (v1 has none: 100)
	int y;			// aircraft altitude above ground [m]
	int wy;			// height ×10 (v3)
	int pi;			// pitch ×10 (v3)
	int ro;			// roll ×10 (v3)
	string n;		// v1: name and type on every vehicle
	int t;

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(i, 0);
		context.ReadDefault(x, 0);
		context.ReadDefault(z, 0);
		context.ReadDefault(a, 0);
		context.ReadDefault(f, 0);
		context.ReadDefault(h, 100);
		context.ReadDefault(y, 0);
		context.ReadDefault(wy, 0);
		context.ReadDefault(pi, 0);
		context.ReadDefault(ro, 0);
		context.ReadDefault(n, "");
		context.ReadDefault(t, 0);
		return true;
	}
}

class CRF_ReplayShot
{
	int si;
	int sx;
	int sz;
	int hx;
	int hz;
	int k;			// CRF_EVAARShotKind
	int sy;			// v3
	int hy;			// v3

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(si, 0);
		context.ReadDefault(sx, 0);
		context.ReadDefault(sz, 0);
		context.ReadDefault(hx, 0);
		context.ReadDefault(hz, 0);
		context.ReadDefault(k, 0);
		context.ReadDefault(sy, 0);
		context.ReadDefault(hy, 0);
		return true;
	}
}

class CRF_ReplayKill
{
	int ki;			// IDs (none in v1 - match by name)
	int ti;
	string kn;
	string tn;
	int kf;
	int tf;
	string w;
	int d;
	int x;
	int z;

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(ki, 0);
		context.ReadDefault(ti, 0);
		context.ReadDefault(kn, "");
		context.ReadDefault(tn, "");
		context.ReadDefault(kf, 0);
		context.ReadDefault(tf, 0);
		context.ReadDefault(w, "");
		context.ReadDefault(d, 0);
		context.ReadDefault(x, 0);
		context.ReadDefault(z, 0);
		return true;
	}
}

class CRF_ReplayGrenade
{
	int i;
	int x;
	int z;
	int t;			// CRF_EVAARGrenadeType
	int y;			// v3

	//------------------------------------------------------------------------------------------------
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(i, 0);
		context.ReadDefault(x, 0);
		context.ReadDefault(z, 0);
		context.ReadDefault(t, 0);
		context.ReadDefault(y, 0);
		return true;
	}
}

class CRF_ReplayFrame
{
	int ts;
	ref array<ref CRF_ReplayCharInfo> r = {};
	ref array<ref CRF_ReplayVehInfo> vr = {};
	ref array<ref CRF_ReplayChar> c = {};
	ref array<ref CRF_ReplayVeh> v = {};
	ref array<ref CRF_ReplayShot> s = {};
	ref array<ref CRF_ReplayKill> k = {};
	ref array<ref CRF_ReplayGrenade> g = {};

	//------------------------------------------------------------------------------------------------
	//! Every list is optional - older formats and quiet frames leave them out
	bool SerializationLoad(LoadContext context)
	{
		context.ReadDefault(ts, 0);
		if (context.DoesKeyExist("r"))
			context.Read(r);

		if (context.DoesKeyExist("vr"))
			context.Read(vr);

		if (context.DoesKeyExist("c"))
			context.Read(c);

		if (context.DoesKeyExist("v"))
			context.Read(v);

		if (context.DoesKeyExist("s"))
			context.Read(s);

		if (context.DoesKeyExist("k"))
			context.Read(k);

		if (context.DoesKeyExist("g"))
			context.Read(g);

		return true;
	}
}
