//------------------------------------------------------------------------------------------------
//! In-game replays: players open recorded missions (CRF_VAAR_GamemodeComponent logs) from the main menu
//! and watch them locally, with the real soldiers, vehicles and effects on the real terrain.
//!
//!  CRF_ReplayBrowserMenu   main menu > REPLAYS: missions on this PC and on coalitiongroup.net
//!  CRF_ReplayDownloader    fetches a recording from the website, a minute of frames per request
//!  CRF_ReplayLauncher      loads the mission's terrain as a local game and starts the player in it
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

	protected static ref map<string, string> s_mTerrainWorlds;

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
	//! The terrain world a map key (the website's mission terrain, lower case) loads. Mission worlds are
	//! sub-scenes of these; the replay loads the bare terrain so none of the mission's own entities run.
	//! Read from the parents of the mission worlds in this mod - add a line here for a new map.
	static string GetTerrainWorld(string terrainKey)
	{
		if (!s_mTerrainWorlds)
		{
			s_mTerrainWorlds = new map<string, string>();
			s_mTerrainWorlds.Set("arland", "{A9806AF617972E97}worlds/Arland/Arland.ent");
			s_mTerrainWorlds.Set("eden", "{853E92315D1D9EFE}worlds/Eden/Eden.ent");
			s_mTerrainWorlds.Set("everon", "{853E92315D1D9EFE}worlds/Eden/Eden.ent");
			s_mTerrainWorlds.Set("kolguyev", "{1EA95DAE3230BEB0}worlds/Cain/Cain.ent");
			s_mTerrainWorlds.Set("chernarus", "{F513898A573B9C3F}Worlds/ChernarusMinusBeta.ent");
			s_mTerrainWorlds.Set("belleauwood", "{340ED3C1F743457E}Worlds/BelleauWood/BelleauWood.ent");
			s_mTerrainWorlds.Set("belize", "{D225252FA26D6D54}Worlds/UKSFO_S_Belize.ent");
			s_mTerrainWorlds.Set("falklands", "{B3AC4C35D795749C}Worlds/BritMapProject/BritMapProject.ent");
			s_mTerrainWorlds.Set("kunar", "{B70B908EF90E5F3A}Worlds/Kunar.ent");
			s_mTerrainWorlds.Set("kupyansk", "{9194858F3102F45C}worlds/KupyanskWorld.ent");
			s_mTerrainWorlds.Set("mogadishu", "{BCD52EA0F63F6EB3}Worlds/Mogadishu/mogadishu.ent");
			s_mTerrainWorlds.Set("normandy", "{55B6E6A52487C726}Worlds/TESTIsland.ent");
			s_mTerrainWorlds.Set("serhiivka", "{BC070CAEF616045B}Worlds/Serhiivka.ent");
			s_mTerrainWorlds.Set("vindrindop", "{1BE6FFB423CA6165}Worlds/VinDrinDop.ent");
			s_mTerrainWorlds.Set("wakeisland", "{2C7D70186FB9981F}Project Tropic.ent");
			s_mTerrainWorlds.Set("yeonpyeong", "{9D726E3A65816832}Yeonpyeong.ent");
		}

		string key = terrainKey;
		key.ToLower();
		key.Replace(" ", "");
		key.Replace("'", "");
		return s_mTerrainWorlds.Get(key);
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
// Recording frames, the shape CRF_VAAR_DataContainer writes (short names on purpose - they're the JSON keys)
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
}

class CRF_ReplayVehInfo
{
	int i;
	string n;
	int t;			// CRF_EVAARVehicleType
	string pf;		// prefab (v3)
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
}

class CRF_ReplayVeh
{
	int i;
	int x;
	int z;
	int a;			// yaw ×1000, clockwise from north
	int f;
	int h;			// health 0-100
	int y;			// aircraft altitude above ground [m]
	int wy;			// height ×10 (v3)
	int pi;			// pitch ×10 (v3)
	int ro;			// roll ×10 (v3)
	ref array<int> oc;
	ref array<int> os;
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
}

class CRF_ReplayKill
{
	int ki;
	int ti;
	string kn;
	string tn;
	int kf;
	int tf;
	string w;
	int d;
	int x;
	int z;
}

class CRF_ReplayGrenade
{
	int i;
	int x;
	int z;
	int t;			// CRF_EVAARGrenadeType
	int y;			// v3
}

class CRF_ReplayFrame
{
	int ts;
	ref array<ref CRF_ReplayCharInfo> r;
	ref array<ref CRF_ReplayVehInfo> vr;
	ref array<ref CRF_ReplayChar> c;
	ref array<ref CRF_ReplayVeh> v;
	ref array<ref CRF_ReplayShot> s;
	ref array<ref CRF_ReplayKill> k;
	ref array<ref CRF_ReplayGrenade> g;
}
