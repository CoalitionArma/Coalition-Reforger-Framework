//------------------------------------------------------------------------------------------------
class CRF_ReplayCharProxy
{
	int m_iId;
	IEntity m_Entity;
	bool m_bKilled;			// a corpse now
	bool m_bHidden;			// riding in a vehicle
	int m_iKilledAt = -1;	// the death frame this body is from
	int m_iStance = -1;
	bool m_bUnconscious;	// as recorded (the posed body is never really unconscious)
	float m_fMoveSpeed = -1;	// movement last given to the controller (Walk / StopWalking)

	// Firing the recorded shots (AimAndFire)
	int m_iPendingShots;
	vector m_vShotTarget;
	float m_fRaisedUntil;	// m_fClock
	bool m_bTriggerDown;
	bool m_bWeaponRaised;
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayVehicleProxy
{
	int m_iId;
	IEntity m_Entity;
	bool m_bWrecked;
	int m_iHealth = 100;
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayTracer
{
	ref Shape m_Shape;
	float m_fExpireAt;
}

//------------------------------------------------------------------------------------------------
//! An explosion or smoke cloud, removed again when the playback goes back before it
class CRF_ReplayEffect
{
	IEntity m_Entity;
	int m_iFrame;
	float m_fExpireAt;
}

//------------------------------------------------------------------------------------------------
//! Following a soldier: the spectator's three cameras
enum CRF_EReplayCameraMode
{
	HELMET,		// first person, just off the head
	EYE,		// first person, exactly what they saw
	ORBIT		// third person, RMB to look around
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayKillEntry
{
	int m_iIndex;		// in the recording's kill list
	int m_iFrame;
	ref CRF_ReplayKill m_Kill;
}

//------------------------------------------------------------------------------------------------
//! Plays a recording back in the loaded terrain: reads it a frame at a time (an index of line offsets
//! lets it jump anywhere), spawns the recorded soldiers and vehicles from their prefabs with no AI or
//! physics simulation and poses them every frame, kills them at the recorded moment, and draws tracers,
//! explosions and smoke. Going back in time undoes deaths, wrecks and effects. Runs only in the local
//! replay session CRF_ReplayLauncher starts.
class CRF_ReplayPlayer
{
	protected static ref CRF_ReplayPlayer s_Instance;

	protected static const int INDEX_LINES_PER_TICK = 250;
	protected static const int FRAME_CACHE_SIZE = 80;
	protected static const float TRACER_SECONDS = 1.2;
	protected static const int MAX_TRACERS = 900;
	protected static const int EFFECT_JUMP_FRAMES = 4;		// a bigger jump than this is a seek - no effects replayed
	protected static const float SMOKE_SECONDS = 45;
	protected static const float BLAST_SECONDS = 8;
	protected static const int LINGER_FRAMES = 2;			// a dead character can stay listed this long
	protected static const int BODY_FRAMES = 600;			// bodies stay this long after the death
	protected static const int MAX_PENDING_SHOTS = 8;		// a soldier's real shots queued at most - more in a frame just show as tracers
	protected static const float RAISED_SECONDS = 2.5;		// the weapon stays up this long after the last shot
	protected static const float MAX_FIRING_SPEED = 2;		// faster playback: tracers only
	protected static const float MAX_WALK_PLAYBACK = 2;		// faster playback: soldiers are placed, not walked
	protected static const float WALK_SNAP_DISTANCE = 3;	// metres off the recorded path before a walking soldier is put back
	protected static const float WALK_SNAP_HEIGHT = 2.5;
	protected static const float WALK_CATCH_UP = 1.5;		// per second: how hard a walking soldier is steered back onto the path
	protected static const ref array<float> SPEEDS = {0.25, 0.5, 1, 2, 5, 10, 20};

	protected static const ResourceName CAMERA_MANAGER = "{33F9FD881E3700CC}Prefabs/World/Game/SCR_CameraManager.et";
	protected static const ResourceName FACTION_MANAGER = "{4A188E44289B9A50}Prefabs/MP/Managers/Factions/FactionManager_Editor.et";
	protected static const ResourceName MAP_ENTITY = "{731564B66F91B107}Prefabs/World/Game/MapEntity.et";
	protected static const ResourceName EXPLOSION_SHELL = "{318510C6FB1633D9}Particles/Weapon/Explosion_Mortar_Base.ptc";
	protected static const ResourceName EXPLOSION_GRENADE = "{5592BC9B67C60D16}Particles/Weapon/Explosion_RGD5.ptc";
	protected static const ResourceName SMOKE_GRENADE = "{F9D8383CC3FD7ECA}Particles/Weapon/Smoke_grenade_RDG-2.ptc";

	// Stand-ins for recordings without prefabs (before format v3) or prefabs from a mod that isn't loaded
	protected static const ResourceName FALLBACK_BLUFOR = "{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	protected static const ResourceName FALLBACK_OPFOR = "{DCB41B3746FDD1BE}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Rifleman.et";
	protected static const ResourceName FALLBACK_INDFOR = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	protected static const ResourceName FALLBACK_CIV = "{99F600AD4C6BA6A4}Prefabs/Characters/Factions/CIV/Character_CIV_base.et";
	protected static const ResourceName FALLBACK_CAR = "{259EE7B78C51B624}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469.et";
	protected static const ResourceName FALLBACK_TRUCK = "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et";
	protected static const ResourceName FALLBACK_APC = "{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et";
	protected static const ResourceName FALLBACK_HELICOPTER = "{70BAEEFC2D3FEE64}Prefabs/Vehicles/Helicopters/UH1H/UH1H.et";

	// Recording
	protected string m_sFile;
	protected string m_sTitle;
	protected ref FileHandle m_File;		// ref: a FileHandle is managed - without one it's freed straight away
	protected int m_iVersion = 2;
	protected float m_fInterval = 0.5;
	protected ref array<int> m_aOffsets = {};
	protected bool m_bIndexed;
	protected string m_sError;

	// Collected while indexing
	protected ref map<int, ref CRF_ReplayCharInfo> m_mCharInfo = new map<int, ref CRF_ReplayCharInfo>();
	protected ref map<int, ref CRF_ReplayVehInfo> m_mVehicleInfo = new map<int, ref CRF_ReplayVehInfo>();
	protected ref map<int, ref array<int>> m_mDeathFrames = new map<int, ref array<int>>();	// a player's ID comes back after a respawn
	protected ref map<string, ref array<int>> m_mDeathFramesByName = new map<string, ref array<int>>();	// v1 kills carry no IDs
	protected ref array<ref CRF_ReplayKillEntry> m_aKills = {};

	protected ref map<int, ref CRF_ReplayFrame> m_mFrameCache = new map<int, ref CRF_ReplayFrame>();

	// Playback
	protected float m_fTime;			// in frames
	protected int m_iSpeedIndex = 2;
	protected bool m_bPlaying;
	protected int m_iLastTick;
	protected int m_iLastRendered = -1;
	protected float m_fClock;			// seconds since the replay started (for effect lifetimes)

	protected ref map<int, ref CRF_ReplayCharProxy> m_mChars = new map<int, ref CRF_ReplayCharProxy>();
	protected ref map<int, ref CRF_ReplayVehicleProxy> m_mVehicles = new map<int, ref CRF_ReplayVehicleProxy>();
	protected ref array<ref CRF_ReplayTracer> m_aTracers = {};
	protected ref array<ref CRF_ReplayEffect> m_aEffects = {};
	protected ref map<int, vector> m_mSmokeLastPos = new map<int, vector>();
	protected ref set<int> m_SmokeSpawned = new set<int>();

	protected IEntity m_Camera;

	// Start-up diagnostics (first Workbench tests)
	protected int m_iLinesRead;
	protected bool m_bTicked;
	protected bool m_bReportedParseFailure;
	protected int m_iFollowedId;
	protected CRF_EReplayCameraMode m_eCameraMode = CRF_EReplayCameraMode.ORBIT;

	//------------------------------------------------------------------------------------------------
	static CRF_ReplayPlayer GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static void Start(string file, string title)
	{
		Stop();
		s_Instance = new CRF_ReplayPlayer();
		s_Instance.Init(file, title);
	}

	//------------------------------------------------------------------------------------------------
	static void Stop()
	{
		if (!s_Instance)
			return;

		s_Instance.Shutdown();
		s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void Init(string file, string title)
	{
		m_sFile = file;
		m_sTitle = title;
		EnsureManagers();

		m_File = FileIO.OpenFile(file, FileMode.READ);
		if (!m_File)
		{
			m_sError = "Couldn't open " + file;
			Print("[CRF_Replay] " + m_sError, LogLevel.ERROR);
		}
		else
		{
			string header;
			m_File.ReadLine(header);
			ReadHeader(header);
		}

		// A camera straight away - the game shows nothing (the loading screen stays up) until it has one
		vector focus = PeekFocus();
		SetupCamera(focus);
		Print(string.Format("[CRF_Replay] Started %1 (v%2, %3 s a frame) - camera over %4", file, m_iVersion, m_fInterval, focus), LogLevel.NORMAL);

		m_iLastTick = System.GetTickCount();
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
		MenuBase hud = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_ReplayHUD);
		if (!hud)
			Print("[CRF_Replay] Couldn't open the replay HUD (CRF_ReplayHUD in chimeraMenus.conf - restart Workbench after config changes)", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	//! Where the people are in the first frames, read ahead without moving the file on
	protected vector PeekFocus()
	{
		if (!m_File)
			return vector.Zero;

		int start = m_File.GetPos();
		vector focus = vector.Zero;
		string line;
		int n = 0;
		for (; n < 60 && m_File.ReadLine(line) >= 0; n++)
		{
			line = line.Trim();
			if (!line.StartsWith("{") && !line.StartsWith(","))
				continue;

			CRF_ReplayFrame frame = ParseLine(line);
			if (!frame || !frame.c || frame.c.IsEmpty())
				continue;

			float sx, sz;
			foreach (CRF_ReplayChar c : frame.c)
			{
				sx += c.x / 10.0;
				sz += c.z / 10.0;
			}

			focus = Vector(sx / frame.c.Count(), 0, sz / frame.c.Count());
			break;
		}

		m_File.Seek(start);
		if (focus == vector.Zero)
			Print(string.Format("[CRF_Replay] No soldiers in the first frames - read %1 lines from position %2", n, start), LogLevel.WARNING);

		return focus;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadHeader(string header)
	{
		// The header line opens the frames array - close it to read the rest
		string json = header.Trim();
		if (json.EndsWith("["))
			json += "]}";

		JsonLoadContext context = new JsonLoadContext(false);
		if (!context.LoadFromString(json))
			return;

		context.ReadValue("v", m_iVersion);
		float interval;
		if (context.ReadValue("interval", interval) && interval > 0)
			m_fInterval = interval;
	}

	//------------------------------------------------------------------------------------------------
	protected void Shutdown()
	{
		GetGame().GetCallqueue().Remove(Tick);
		if (m_File)
			m_File.Close();

		foreach (int charId, CRF_ReplayCharProxy charProxy : m_mChars)
		{
			DeleteEntity(charProxy.m_Entity);
		}

		foreach (int vehicleId, CRF_ReplayVehicleProxy vehicleProxy : m_mVehicles)
		{
			DeleteEntity(vehicleProxy.m_Entity);
		}

		foreach (CRF_ReplayEffect effect : m_aEffects)
		{
			DeleteEntity(effect.m_Entity);
		}

		m_mChars.Clear();
		m_mVehicles.Clear();
		m_aEffects.Clear();
		m_aTracers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! A bare terrain world has no game mode: bring the managers the camera and the soldiers need
	protected void EnsureManagers()
	{
		BaseWorld world = GetGame().GetWorld();
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		if (!GetGame().GetCameraManager())
			GetGame().SpawnEntityPrefab(Resource.Load(CAMERA_MANAGER), world, params);

		if (!GetGame().GetFactionManager())
			GetGame().SpawnEntityPrefab(Resource.Load(FACTION_MANAGER), world, params);

		// The map (HUD map key): terrain worlds leave it to their missions
		if (!SCR_MapEntity.GetMapInstance())
			GetGame().SpawnEntityPrefab(Resource.Load(MAP_ENTITY), world, params);
	}

	// ── Loop ─────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void Tick()
	{
		int now = System.GetTickCount();
		float dt = Math.Clamp((now - m_iLastTick) / 1000.0, 0, 0.25);
		m_iLastTick = now;
		m_fClock += dt;

		if (!m_bTicked)
		{
			m_bTicked = true;
			Print(string.Format("[CRF_Replay] Playback loop running (file open: %1)", m_File != null), LogLevel.NORMAL);
		}

		if (!m_File)
			return;

		if (!m_bIndexed)
		{
			IndexStep();
			if (m_bIndexed)
				OnIndexed();

			return;
		}

		int last = GetLastFrame();
		if (m_bPlaying)
		{
			m_fTime += dt * GetSpeed() / m_fInterval;
			if (m_fTime >= last)
			{
				m_fTime = last;
				m_bPlaying = false;
			}
		}

		Render();
	}

	// ── Index ────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! Line offsets of every frame, a few hundred lines per tick so loading doesn't freeze the game;
	//! rosters (prefabs) and kills are read on the way as playback needs them from the start
	protected void IndexStep()
	{
		string line;
		for (int n = 0; n < INDEX_LINES_PER_TICK; n++)
		{
			int position = m_File.GetPos();
			int length = m_File.ReadLine(line);
			if (length < 0)
			{
				m_bIndexed = true;
				return;
			}

			m_iLinesRead++;
			line = line.Trim();
			if (!line.StartsWith("{") && !line.StartsWith(","))
				continue;

			int frameIndex = m_aOffsets.Count();
			m_aOffsets.Insert(position);

			if (line.Contains("\"r\":[{") || line.Contains("\"vr\":[{") || line.Contains("\"k\":[{"))
				CollectEvents(ParseLine(line), frameIndex);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectEvents(CRF_ReplayFrame frame, int frameIndex)
	{
		if (!frame)
			return;

		if (frame.r)
		{
			foreach (CRF_ReplayCharInfo info : frame.r)
			{
				CRF_ReplayCharInfo known = m_mCharInfo.Get(info.i);
				if (known && !known.pf.IsEmpty() && info.pf.IsEmpty())
					info.pf = known.pf;

				m_mCharInfo.Set(info.i, info);
			}
		}

		if (frame.vr)
		{
			foreach (CRF_ReplayVehInfo info : frame.vr)
			{
				m_mVehicleInfo.Set(info.i, info);
			}
		}

		// v1: names and factions ride on every character and vehicle
		if (frame.c)
		{
			foreach (CRF_ReplayChar character : frame.c)
			{
				LearnCharacter(character);
			}
		}

		if (frame.v)
		{
			foreach (CRF_ReplayVeh vehicle : frame.v)
			{
				LearnVehicle(vehicle);
			}
		}

		if (frame.k)
		{
			foreach (CRF_ReplayKill kill : frame.k)
			{
				CRF_ReplayKillEntry entry = new CRF_ReplayKillEntry();
				entry.m_iFrame = frameIndex;
				entry.m_Kill = kill;
				entry.m_iIndex = m_aKills.Count();
				m_aKills.Insert(entry);
				if (kill.ti != 0)
				{
					array<int> deaths = m_mDeathFrames.Get(kill.ti);
					if (!deaths)
					{
						deaths = {};
						m_mDeathFrames.Set(kill.ti, deaths);
					}

					deaths.Insert(frameIndex);
				}
				else if (!kill.tn.IsEmpty())
				{
					array<int> namedDeaths = m_mDeathFramesByName.Get(kill.tn);
					if (!namedDeaths)
					{
						namedDeaths = {};
						m_mDeathFramesByName.Set(kill.tn, namedDeaths);
					}

					namedDeaths.Insert(frameIndex);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LearnCharacter(CRF_ReplayChar character)
	{
		if (character.n.IsEmpty() || m_mCharInfo.Contains(character.i))
			return;

		CRF_ReplayCharInfo info = new CRF_ReplayCharInfo();
		info.i = character.i;
		info.n = character.n;
		info.r = character.r;
		info.f = character.f;
		m_mCharInfo.Set(info.i, info);
	}

	//------------------------------------------------------------------------------------------------
	protected void LearnVehicle(CRF_ReplayVeh vehicle)
	{
		if (vehicle.n.IsEmpty() || m_mVehicleInfo.Contains(vehicle.i))
			return;

		CRF_ReplayVehInfo info = new CRF_ReplayVehInfo();
		info.i = vehicle.i;
		info.n = vehicle.n;
		info.t = vehicle.t;
		m_mVehicleInfo.Set(info.i, info);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnIndexed()
	{
		if (m_aOffsets.IsEmpty())
		{
			m_sError = "This recording has no frames";
			Print(string.Format("[CRF_Replay] No frames found - read %1 lines after the header", m_iLinesRead), LogLevel.ERROR);
			return;
		}

		Print(string.Format("[CRF_Replay] Indexed %1 frames, %2 kills, %3 soldiers, %4 vehicles", m_aOffsets.Count(), m_aKills.Count(), m_mCharInfo.Count(), m_mVehicleInfo.Count()), LogLevel.NORMAL);
		m_fTime = 0;
		m_bPlaying = true;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayFrame ParseLine(string line)
	{
		if (line.StartsWith(","))
			line = line.Substring(1, line.Length() - 1);

		JsonLoadContext context = new JsonLoadContext(false);
		if (!context.LoadFromString(line))
		{
			ReportParseFailure("not valid JSON", line);
			return null;
		}

		CRF_ReplayFrame frame = new CRF_ReplayFrame();
		if (!context.ReadValue("frame", frame))
		{
			ReportParseFailure("no readable \"frame\"", line);
			return null;
		}

		return frame;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReportParseFailure(string reason, string line)
	{
		if (m_bReportedParseFailure)
			return;

		m_bReportedParseFailure = true;
		string start = line;
		if (start.Length() > 160)
			start = start.Substring(0, 160);

		Print(string.Format("[CRF_Replay] Couldn't read a frame (%1) - line of %2 characters: %3", reason, line.Length(), start), LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayFrame GetFrame(int index)
	{
		if (!m_aOffsets.IsIndexValid(index))
			return null;

		CRF_ReplayFrame frame = m_mFrameCache.Get(index);
		if (frame)
			return frame;

		string line;
		m_File.Seek(m_aOffsets[index]);
		m_File.ReadLine(line);
		frame = ParseLine(line.Trim());
		if (!frame)
			return null;

		// Keep the frames around the playhead
		if (m_mFrameCache.Count() >= FRAME_CACHE_SIZE)
		{
			int center = m_fTime;
			array<int> drop = {};
			foreach (int cached, CRF_ReplayFrame unused : m_mFrameCache)
			{
				if (Math.AbsInt(cached - center) > FRAME_CACHE_SIZE / 2)
					drop.Insert(cached);
			}

			foreach (int key : drop)
			{
				m_mFrameCache.Remove(key);
			}
		}

		m_mFrameCache.Set(index, frame);
		return frame;
	}

	// ── Render ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void Render()
	{
		int index = Math.Clamp(Math.Floor(m_fTime), 0, GetLastFrame());
		float alpha = m_fTime - index;
		CRF_ReplayFrame current = GetFrame(index);
		if (!current)
			return;

		CRF_ReplayFrame next = GetFrame(index + 1);

		// Effects of every frame played through since the last render; a seek replays none, and going
		// back removes the effects that hadn't happened yet
		if (m_iLastRendered >= 0 && index < m_iLastRendered)
			RemoveEffectsAfter(index);

		// Back in time, or a jump: no shots from another moment
		if (m_iLastRendered >= 0 && (index < m_iLastRendered || index - m_iLastRendered > EFFECT_JUMP_FRAMES))
			ClearShots();

		if (index > m_iLastRendered && index - m_iLastRendered <= EFFECT_JUMP_FRAMES)
		{
			for (int f = Math.Max(0, m_iLastRendered + 1); f <= index; f++)
			{
				EmitEffects(GetFrame(f), f);
			}
		}

		m_iLastRendered = index;

		UpdateCharacters(current, next, alpha, index);
		UpdateVehicles(current, next, alpha);
		UpdateTracers();
		UpdateEffects();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCharacters(CRF_ReplayFrame current, CRF_ReplayFrame next, float alpha, int index)
	{
		map<int, CRF_ReplayChar> nextById = new map<int, CRF_ReplayChar>();
		if (next && next.c)
		{
			foreach (CRF_ReplayChar n : next.c)
			{
				nextById.Set(n.i, n);
			}
		}

		set<int> seen = new set<int>();
		BaseWorld world = GetGame().GetWorld();
		if (current.c)
		{
			foreach (CRF_ReplayChar c : current.c)
			{
				seen.Insert(c.i);
				CRF_ReplayChar b = nextById.Get(c.i);
				float x = c.x / 10.0, z = c.z / 10.0;
				if (b)
				{
					x = Math.Lerp(x, b.x / 10.0, alpha);
					z = Math.Lerp(z, b.z / 10.0, alpha);
				}

				float y;
				if (m_iVersion >= 3)
				{
					y = c.y / 10.0;
					if (b)
						y = Math.Lerp(y, b.y / 10.0, alpha);
				}
				else
				{
					y = world.GetSurfaceY(x, z);
				}

				// Recorded aim yaw runs the other way to entity yaw
				float yaw = -c.a / 1000.0;
				if (b)
					yaw = LerpAngle(yaw, -b.a / 1000.0, alpha);

				LearnCharacter(c);
				CRF_ReplayCharProxy proxy = m_mChars.Get(c.i);
				int diedAt = LatestDeath(c.i, index);
				if (proxy && proxy.m_bKilled)
				{
					// A body still listed for a moment after the kill stays where it fell
					if (diedAt == proxy.m_iKilledAt && index - diedAt <= LINGER_FRAMES)
						continue;

					// Back before this death, or respawned since: bring them back
					DeleteEntity(proxy.m_Entity);
					m_mChars.Remove(c.i);
					proxy = null;
				}

				if (!proxy || !proxy.m_Entity)
				{
					proxy = SpawnCharacter(c.i, Vector(x, y, z));
					if (!proxy)
						continue;
				}

				bool inVehicle = (c.s & CRF_EVAARCharacterState.IN_VEHICLE) != 0;
				SetHidden(proxy, inVehicle);
				proxy.m_bUnconscious = (c.s & CRF_EVAARCharacterState.UNCONSCIOUS) != 0;
				vector target = Vector(x, y, z);

				// Killed this frame but still listed
				if (diedAt >= 0 && index - diedAt <= LINGER_FRAMES && proxy.m_iKilledAt != diedAt)
				{
					StopWalking(proxy);
					Pose(proxy.m_Entity, target, Vector(yaw, 0, 0));
					KillProxy(proxy, diedAt);
					continue;
				}

				int stance = ECharacterStance.STAND;
				if (m_iVersion >= 3)
					stance = c.t;

				if ((c.s & CRF_EVAARCharacterState.UNCONSCIOUS) != 0)
					stance = ECharacterStance.PRONE;

				ApplyStance(proxy, stance);

				// On foot at walking pace the controller walks them, so the walk / run / crawl cycles play; otherwise
				// (in a vehicle, unconscious, paused, fast playback, too far off the path) they're placed exactly
				bool walked;
				if (b && !inVehicle && !proxy.m_bUnconscious && m_bPlaying && GetSpeed() <= MAX_WALK_PLAYBACK)
				{
					vector velocity = Vector(b.x - c.x, 0, b.z - c.z) * (0.1 / m_fInterval * GetSpeed());
					walked = Walk(proxy, target, velocity, stance);
				}

				if (!walked)
				{
					StopWalking(proxy);
					Pose(proxy.m_Entity, target, Vector(yaw, 0, 0));
				}

				float pitch;
				if (m_iVersion >= 3)
				{
					pitch = c.p / 10.0;
					if (b)
						pitch = Math.Lerp(pitch, b.p / 10.0, alpha);
				}

				if (!inVehicle)
					AimAndFire(proxy, stance, yaw, pitch);
			}
		}

		// Gone from the recording: dead (keep the body) or left - or not born yet after a seek back
		array<int> remove = {};
		foreach (int id, CRF_ReplayCharProxy proxy : m_mChars)
		{
			if (seen.Contains(id))
				continue;

			int deathFrame = LatestDeath(id, index);
			if (deathFrame >= 0 && index - deathFrame < BODY_FRAMES)
			{
				if (!proxy.m_bKilled)
					KillProxy(proxy, deathFrame);

				continue;
			}

			remove.Insert(id);
		}

		foreach (int id : remove)
		{
			CRF_ReplayCharProxy proxy = m_mChars.Get(id);
			if (proxy)
				DeleteEntity(proxy.m_Entity);

			m_mChars.Remove(id);
			if (id == m_iFollowedId)
				Unfollow();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateVehicles(CRF_ReplayFrame current, CRF_ReplayFrame next, float alpha)
	{
		map<int, CRF_ReplayVeh> nextById = new map<int, CRF_ReplayVeh>();
		if (next && next.v)
		{
			foreach (CRF_ReplayVeh n : next.v)
			{
				nextById.Set(n.i, n);
			}
		}

		set<int> seen = new set<int>();
		BaseWorld world = GetGame().GetWorld();
		if (current.v)
		{
			foreach (CRF_ReplayVeh v : current.v)
			{
				seen.Insert(v.i);
				CRF_ReplayVeh b = nextById.Get(v.i);
				float x = v.x / 10.0, z = v.z / 10.0;
				if (b)
				{
					x = Math.Lerp(x, b.x / 10.0, alpha);
					z = Math.Lerp(z, b.z / 10.0, alpha);
				}

				float y;
				if (m_iVersion >= 3)
				{
					y = v.wy / 10.0;
					if (b)
						y = Math.Lerp(y, b.wy / 10.0, alpha);
				}
				else
				{
					y = world.GetSurfaceY(x, z) + v.y;
				}

				float yaw = v.a / 1000.0;
				float pitch = v.pi / 10.0;
				float roll = v.ro / 10.0;
				if (b)
				{
					yaw = LerpAngle(yaw, b.a / 1000.0, alpha);
					pitch = Math.Lerp(pitch, b.pi / 10.0, alpha);
					roll = Math.Lerp(roll, b.ro / 10.0, alpha);
				}

				LearnVehicle(v);
				CRF_ReplayVehicleProxy proxy = m_mVehicles.Get(v.i);
				if (proxy && proxy.m_bWrecked && v.h > 0)
				{
					// Back before it was destroyed
					DeleteEntity(proxy.m_Entity);
					m_mVehicles.Remove(v.i);
					proxy = null;
				}

				if (!proxy || !proxy.m_Entity)
				{
					proxy = SpawnVehicle(v.i, Vector(x, y, z));
					if (!proxy)
						continue;
				}

				proxy.m_iHealth = v.h;
				if (v.h == 0 && !proxy.m_bWrecked)
				{
					proxy.m_bWrecked = true;
					KillEntity(proxy.m_Entity);
				}

				Pose(proxy.m_Entity, Vector(x, y, z), Vector(yaw, pitch, roll));
			}
		}

		array<int> remove = {};
		foreach (int id, CRF_ReplayVehicleProxy proxy : m_mVehicles)
		{
			if (!seen.Contains(id))
				remove.Insert(id);
		}

		foreach (int id : remove)
		{
			CRF_ReplayVehicleProxy proxy = m_mVehicles.Get(id);
			if (proxy)
				DeleteEntity(proxy.m_Entity);

			m_mVehicles.Remove(id);
		}
	}

	// ── Spawning ─────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayCharProxy SpawnCharacter(int id, vector position)
	{
		CRF_ReplayCharInfo info = m_mCharInfo.Get(id);
		ResourceName prefab;
		if (info && !info.pf.IsEmpty())
			prefab = info.pf;

		IEntity entity = SpawnPrefab(prefab, position);
		if (!entity)
		{
			int faction;
			if (info)
				faction = info.f;

			entity = SpawnPrefab(FallbackCharacter(faction), position);
		}

		if (!entity)
			return null;

		SetDamageable(entity, false);
		CRF_ReplayCharProxy proxy = new CRF_ReplayCharProxy();
		proxy.m_iId = id;
		proxy.m_Entity = entity;
		m_mChars.Set(id, proxy);

		// Following someone who just respawned in after a seek
		if (id == m_iFollowedId)
			Follow(id);

		return proxy;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayVehicleProxy SpawnVehicle(int id, vector position)
	{
		CRF_ReplayVehInfo info = m_mVehicleInfo.Get(id);
		ResourceName prefab;
		if (info && !info.pf.IsEmpty())
			prefab = info.pf;

		IEntity entity = SpawnPrefab(prefab, position);
		if (!entity)
		{
			int type;
			if (info)
				type = info.t;

			entity = SpawnPrefab(FallbackVehicle(type), position);
		}

		if (!entity)
			return null;

		// Posed every frame, not simulated
		Physics physics = entity.GetPhysics();
		if (physics)
			physics.ChangeSimulationState(SimulationState.COLLISION);

		SetDamageable(entity, false);
		CRF_ReplayVehicleProxy proxy = new CRF_ReplayVehicleProxy();
		proxy.m_iId = id;
		proxy.m_Entity = entity;
		m_mVehicles.Set(id, proxy);
		return proxy;
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity SpawnPrefab(ResourceName prefab, vector position)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName FallbackCharacter(int faction)
	{
		switch (faction)
		{
			case 1: return FALLBACK_BLUFOR;
			case 2: return FALLBACK_OPFOR;
			case 3: return FALLBACK_INDFOR;
		}

		return FALLBACK_CIV;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName FallbackVehicle(int type)
	{
		switch (type)
		{
			case CRF_EVAARVehicleType.APC: return FALLBACK_APC;
			case CRF_EVAARVehicleType.TANK: return FALLBACK_APC;
			case CRF_EVAARVehicleType.TRUCK: return FALLBACK_TRUCK;
			case CRF_EVAARVehicleType.HELICOPTER: return FALLBACK_HELICOPTER;
		}

		return FALLBACK_CAR;
	}

	//------------------------------------------------------------------------------------------------
	protected void Pose(IEntity entity, vector position, vector angles)
	{
		if (!entity)
			return;

		vector transform[4];
		Math3D.AnglesToMatrix(angles, transform);
		transform[3] = position;

		BaseGameEntity gameEntity = BaseGameEntity.Cast(entity);
		if (gameEntity)
		{
			gameEntity.Teleport(transform);
			return;
		}

		entity.SetWorldTransform(transform);
		entity.Update();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyStance(CRF_ReplayCharProxy proxy, int stance)
	{
		if (proxy.m_iStance == stance || proxy.m_bKilled)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(proxy.m_Entity);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		proxy.m_iStance = stance;
		switch (stance)
		{
			case ECharacterStance.CROUCH:
				controller.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
				break;
			case ECharacterStance.PRONE:
				controller.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
				break;
			default:
				controller.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The weapon on the recorded aim - or on the target of the shots being fired - and the trigger: each
	//! recorded bullet (QueueShot) pulls it once, the next tick lets go, so semi-automatics fire too. Real
	//! shots - muzzle flash, sound, recoil, brass - from a full magazine; replay soldiers take no damage
	//! (SetDamageable), so they hit nobody.
	//! \param[in] yaw / pitch the pose's heading and the recorded aim pitch, degrees
	protected void AimAndFire(CRF_ReplayCharProxy proxy, int stance, float yaw, float pitch)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(proxy.m_Entity);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		// The controller turns the body to its own heading - keep it on the pose's
		controller.SetHeadingAngle(yaw * Math.DEG2RAD);

		bool shooting = m_fClock < proxy.m_fRaisedUntil;
		vector aim = Vector(yaw * Math.DEG2RAD, pitch * Math.DEG2RAD, 0);
		if (shooting)
			aim = AimAt(character, stance, proxy.m_vShotTarget);

		CharacterInputContext input = controller.GetInputContext();
		if (input)
			input.SetAimingAngles(aim);

		if (shooting != proxy.m_bWeaponRaised)
		{
			proxy.m_bWeaponRaised = shooting;
			controller.SetWeaponRaised(shooting);
		}

		if (proxy.m_bTriggerDown)
		{
			proxy.m_bTriggerDown = false;
			controller.SetFireWeaponWanted(false);
			return;
		}

		if (proxy.m_iPendingShots <= 0)
			return;

		proxy.m_iPendingShots--;
		RefillMagazine(controller);
		proxy.m_bTriggerDown = true;
		controller.SetFireWeaponWanted(true);
	}

	//------------------------------------------------------------------------------------------------
	//! A recorded bullet from this soldier: fire one at where it hit
	protected void QueueShot(int shooterId, vector hit)
	{
		CRF_ReplayCharProxy proxy = m_mChars.Get(shooterId);
		if (!proxy || !proxy.m_Entity || proxy.m_bKilled || proxy.m_bHidden)
			return;

		proxy.m_iPendingShots = Math.Min(proxy.m_iPendingShots + 1, MAX_PENDING_SHOTS);
		proxy.m_vShotTarget = hit;
		proxy.m_fRaisedUntil = m_fClock + RAISED_SECONDS;
	}

	//------------------------------------------------------------------------------------------------
	//! A seek: nobody fires shots from another moment
	protected void ClearShots()
	{
		foreach (int id, CRF_ReplayCharProxy proxy : m_mChars)
		{
			proxy.m_iPendingShots = 0;
			proxy.m_fRaisedUntil = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Aiming angles (radians, the controller's) from the soldier's eyes to a point - as vanilla's cinematic track aims
	protected static vector AimAt(ChimeraCharacter character, int stance, vector target)
	{
		float eyes = 1.6;
		if (stance == ECharacterStance.CROUCH)
			eyes = 1.0;
		else if (stance == ECharacterStance.PRONE)
			eyes = 0.3;

		vector from = character.GetOrigin() + Vector(0, eyes, 0);
		vector characterTransform[4], aimTransform[4], aimRotation[3];
		character.GetWorldTransform(characterTransform);
		Math3D.DirectionAndUpMatrix(vector.Direction(from, target), characterTransform[1], aimTransform);
		aimRotation[0] = aimTransform[0];
		aimRotation[1] = aimTransform[1];
		aimRotation[2] = aimTransform[2];
		vector angles = Math3D.MatrixToAngles(aimRotation);
		return Vector(angles[0] * Math.DEG2RAD, angles[1] * Math.DEG2RAD, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Never run dry: the recording fired, so the replay fires
	protected static void RefillMagazine(CharacterControllerComponent controller)
	{
		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
			return;

		BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
		if (magazine)
			magazine.SetAmmoCount(magazine.GetMaxAmmoCount());
	}

	//------------------------------------------------------------------------------------------------
	//! Replay entities only die, wreck or get hit when the recording says so - the shots they fire are real
	protected static void SetDamageable(IEntity entity, bool damageable)
	{
		if (!entity)
			return;

		DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (damage)
			damage.EnableDamageHandling(damageable);
	}

	//------------------------------------------------------------------------------------------------
	//! Walk a soldier along the recording with their own controller, as vanilla's cinematic character track
	//! drives characters: movement input every frame - the recorded velocity plus a pull back onto the path -
	//! and the controller moves them and plays the walk / run / crawl cycle. Teleporting every frame (Pose)
	//! resets the controller, so they idled while sliding.
	//! \param[in] velocity recorded ground velocity at the playback speed, m/s
	//! \return false when they're too far off the path - the caller places them
	protected bool Walk(CRF_ReplayCharProxy proxy, vector target, vector velocity, int stance)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(proxy.m_Entity);
		if (!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return false;

		vector offset = target - character.GetOrigin();
		float rise = Math.AbsFloat(offset[1]);
		offset[1] = 0;
		float drift = offset.Length();
		if (drift > WALK_SNAP_DISTANCE || rise > WALK_SNAP_HEIGHT)
			return false;

		vector wanted = velocity + offset * WALK_CATCH_UP;
		float wantedSpeed = wanted.Length();
		float moveSpeed = MovementSpeed(wantedSpeed, stance);
		vector localDir = vector.Forward;
		if (moveSpeed > 0)
		{
			// Relative to where they face (AimAndFire keeps the heading on the recorded aim): sideways strafes
			vector transform[4];
			character.GetWorldTransform(transform);
			vector worldDir = wanted.Normalized();
			localDir = Vector(vector.Dot(worldDir, transform[0]), 0, vector.Dot(worldDir, transform[2]));
			localDir.Normalize();
		}

		controller.SetMovement(moveSpeed, localDir);
		proxy.m_fMoveSpeed = moveSpeed;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Back to standing before a soldier is placed (once - the controller keeps the last movement)
	protected void StopWalking(CRF_ReplayCharProxy proxy)
	{
		if (proxy.m_fMoveSpeed == 0)
			return;

		proxy.m_fMoveSpeed = 0;
		ChimeraCharacter character = ChimeraCharacter.Cast(proxy.m_Entity);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (controller)
			controller.SetMovement(0, vector.Forward);
	}

	//------------------------------------------------------------------------------------------------
	//! Ground speed (m/s) to the controller's movement speed: 0 standing, 1 walk, 2 run, 3 sprint. Crouched
	//! and prone soldiers cover less ground for the same gait and can't sprint.
	protected static float MovementSpeed(float metresPerSecond, int stance)
	{
		float walk = 1.6;
		float run = 3.6;
		float sprint = 5.5;
		float top = 3;
		if (stance == ECharacterStance.CROUCH)
		{
			walk = 1.1;
			run = 2.6;
			top = 2;
		}
		else if (stance == ECharacterStance.PRONE)
		{
			walk = 0.5;
			run = 1.0;
			top = 2;
		}

		// Standing still, or a recording's jitter
		if (metresPerSecond < walk * 0.2)
			return 0;

		float speed;
		if (metresPerSecond <= walk)
			speed = metresPerSecond / walk;
		else if (metresPerSecond <= run)
			speed = 1 + (metresPerSecond - walk) / (run - walk);
		else
			speed = 2 + (metresPerSecond - run) / (sprint - run);

		// Gaits blend badly below half a walk (see CharacterCinematicTrack)
		return Math.Clamp(speed, 0.5, top);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetHidden(CRF_ReplayCharProxy proxy, bool hidden)
	{
		if (proxy.m_bHidden == hidden || !proxy.m_Entity)
			return;

		proxy.m_bHidden = hidden;
		if (hidden)
			proxy.m_Entity.ClearFlags(EntityFlags.VISIBLE, true);
		else
			proxy.m_Entity.SetFlags(EntityFlags.VISIBLE, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void KillProxy(CRF_ReplayCharProxy proxy, int deathFrame)
	{
		proxy.m_bKilled = true;
		proxy.m_iKilledAt = deathFrame;
		SetHidden(proxy, false);
		KillEntity(proxy.m_Entity);
	}

	//------------------------------------------------------------------------------------------------
	//! The last time this character died at or before the frame, -1 if they haven't yet
	protected int LatestDeath(int id, int index)
	{
		array<int> deaths = m_mDeathFrames.Get(id);
		if (!deaths)
		{
			CRF_ReplayCharInfo info = m_mCharInfo.Get(id);
			if (info && !info.n.IsEmpty())
				deaths = m_mDeathFramesByName.Get(info.n);
		}

		if (!deaths)
			return -1;

		for (int i = deaths.Count() - 1; i >= 0; i--)
		{
			if (deaths[i] <= index)
				return deaths[i];
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected void KillEntity(IEntity entity)
	{
		if (!entity)
			return;

		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (!damage)
			return;

		damage.EnableDamageHandling(true);
		damage.Kill(Instigator.CreateInstigatorGM());
	}

	//------------------------------------------------------------------------------------------------
	protected void DeleteEntity(IEntity entity)
	{
		if (entity)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	// ── Effects ──────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void EmitEffects(CRF_ReplayFrame frame, int frameIndex)
	{
		if (!frame)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (frame.s)
		{
			foreach (CRF_ReplayShot shot : frame.s)
			{
				float sx = shot.sx / 10.0, sz = shot.sz / 10.0, hx = shot.hx / 10.0, hz = shot.hz / 10.0;
				float sy = world.GetSurfaceY(sx, sz) + 1.5;
				float hy = world.GetSurfaceY(hx, hz) + 0.4;
				if (m_iVersion >= 3)
				{
					sy = shot.sy / 10.0;
					hy = shot.hy / 10.0;
				}

				vector hit = Vector(hx, hy, hz);
				if (shot.k == CRF_EVAARShotKind.SHELL)
					SpawnEffect(EXPLOSION_SHELL, hit, frameIndex, BLAST_SECONDS);
				else if (shot.k == CRF_EVAARShotKind.GRENADE)
					SpawnEffect(EXPLOSION_GRENADE, hit, frameIndex, BLAST_SECONDS);
				else
				{
					AddTracer(Vector(sx, sy, sz), hit, ShooterColor(shot.si));
					if (shot.k == CRF_EVAARShotKind.BULLET && GetSpeed() <= MAX_FIRING_SPEED)
						QueueShot(shot.si, hit);
				}
			}
		}

		// A smoke grenade starts smoking where it comes to rest
		if (frame.g)
		{
			foreach (CRF_ReplayGrenade grenade : frame.g)
			{
				if (grenade.t != CRF_EVAARGrenadeType.SMOKE || m_SmokeSpawned.Contains(grenade.i))
					continue;

				float gx = grenade.x / 10.0, gz = grenade.z / 10.0;
				float gy = world.GetSurfaceY(gx, gz);
				if (m_iVersion >= 3)
					gy = grenade.y / 10.0;

				vector position = Vector(gx, gy, gz);
				vector previous;
				if (m_mSmokeLastPos.Find(grenade.i, previous) && vector.Distance(previous, position) < 1)
				{
					m_SmokeSpawned.Insert(grenade.i);
					SpawnEffect(SMOKE_GRENADE, position, frameIndex, SMOKE_SECONDS);
				}

				m_mSmokeLastPos.Set(grenade.i, position);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int ShooterColor(int shooterId)
	{
		CRF_ReplayCharInfo info = m_mCharInfo.Get(shooterId);
		if (info)
		{
			switch (info.f)
			{
				case 1: return 0xFF4096FF;
				case 2: return 0xFFE85050;
				case 3: return 0xFF5CC480;
			}
		}

		return 0xFFFFD166;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddTracer(vector from, vector to, int color)
	{
		if (m_aTracers.Count() >= MAX_TRACERS)
			m_aTracers.Remove(0);

		vector points[2];
		points[0] = from;
		points[1] = to;
		CRF_ReplayTracer tracer = new CRF_ReplayTracer();
		tracer.m_Shape = Shape.CreateLines(color, ShapeFlags.NOZWRITE, points, 2);
		tracer.m_fExpireAt = m_fClock + TRACER_SECONDS / Math.Max(1, GetSpeed() * 0.5);
		m_aTracers.Insert(tracer);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTracers()
	{
		for (int i = m_aTracers.Count() - 1; i >= 0; i--)
		{
			if (m_aTracers[i].m_fExpireAt <= m_fClock)
				m_aTracers.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnEffect(ResourceName effect, vector position, int frameIndex, float seconds)
	{
		ParticleEffectEntitySpawnParams params = new ParticleEffectEntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		ParticleEffectEntity entity = ParticleEffectEntity.SpawnParticleEffect(effect, params);
		if (!entity)
			return;

		CRF_ReplayEffect entry = new CRF_ReplayEffect();
		entry.m_Entity = entity;
		entry.m_iFrame = frameIndex;
		entry.m_fExpireAt = m_fClock + seconds;
		m_aEffects.Insert(entry);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateEffects()
	{
		for (int i = m_aEffects.Count() - 1; i >= 0; i--)
		{
			CRF_ReplayEffect effect = m_aEffects[i];
			if (effect.m_Entity && effect.m_fExpireAt > m_fClock)
				continue;

			DeleteEntity(effect.m_Entity);
			m_aEffects.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveEffectsAfter(int frameIndex)
	{
		for (int i = m_aEffects.Count() - 1; i >= 0; i--)
		{
			if (m_aEffects[i].m_iFrame <= frameIndex)
				continue;

			DeleteEntity(m_aEffects[i].m_Entity);
			m_aEffects.Remove(i);
		}

		m_aTracers.Clear();
		m_SmokeSpawned.Clear();
		m_mSmokeLastPos.Clear();
	}

	// ── Camera ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void SetupCamera(vector focus)
	{
		BaseWorld world = GetGame().GetWorld();
		float ground = world.GetSurfaceY(focus[0], focus[2]);
		vector transform[4];
		Math3D.AnglesToMatrix(Vector(0, -32, 0), transform);
		transform[3] = Vector(focus[0], ground + 140, focus[2] - 200);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;
		m_Camera = GetGame().SpawnEntityPrefab(Resource.Load(COA_EntityHelper.GetSpectatorCameraResource()), world, params);

		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!m_Camera || !cameraManager)
		{
			Print(string.Format("[CRF_Replay] No camera (spectator camera %1, camera manager %2)", m_Camera, cameraManager), LogLevel.ERROR);
			return;
		}

		cameraManager.SetCamera(CameraBase.Cast(m_Camera));
	}

	//------------------------------------------------------------------------------------------------
	void Follow(int characterId)
	{
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_Camera);
		CRF_ReplayCharProxy proxy = m_mChars.Get(characterId);
		if (!camera || !proxy || !proxy.m_Entity)
			return;

		m_iFollowedId = characterId;
		switch (m_eCameraMode)
		{
			case CRF_EReplayCameraMode.HELMET:
				camera.SetOnRailsEntity(proxy.m_Entity, false);
				break;
			case CRF_EReplayCameraMode.EYE:
				camera.SetOnRailsEntityEyeMode(proxy.m_Entity);
				break;
			default:
				camera.SetOnRailsEntity(proxy.m_Entity, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Unfollow()
	{
		m_iFollowedId = 0;
		COA_SpectatorCamera camera = COA_SpectatorCamera.Cast(m_Camera);
		if (!camera)
			return;

		camera.ClearOnRails();

		// Level again after the helmet / eye cam (the spectator does the same)
		vector angles = camera.GetAngles();
		camera.SetAngles(Vector(angles[0], angles[1], 0));
	}

	//------------------------------------------------------------------------------------------------
	//! Helmet cam -> eye cam -> orbit, like the spectator's camera key; the followed soldier switches over
	CRF_EReplayCameraMode CycleCameraMode()
	{
		switch (m_eCameraMode)
		{
			case CRF_EReplayCameraMode.HELMET: SetCameraMode(CRF_EReplayCameraMode.EYE); break;
			case CRF_EReplayCameraMode.EYE: SetCameraMode(CRF_EReplayCameraMode.ORBIT); break;
			default: SetCameraMode(CRF_EReplayCameraMode.HELMET);
		}

		return m_eCameraMode;
	}

	//------------------------------------------------------------------------------------------------
	void SetCameraMode(CRF_EReplayCameraMode mode)
	{
		m_eCameraMode = mode;
		if (m_iFollowedId != 0)
			Follow(m_iFollowedId);
	}

	//------------------------------------------------------------------------------------------------
	CRF_EReplayCameraMode GetCameraMode()
	{
		return m_eCameraMode;
	}

	//------------------------------------------------------------------------------------------------
	int GetFollowedId()
	{
		return m_iFollowedId;
	}

	//------------------------------------------------------------------------------------------------
	//! Free camera over a point (the spectator's teleport: default distance and angle)
	void MoveCameraTo(vector position)
	{
		Unfollow();
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(m_Camera);
		if (!camera)
			return;

		SCR_TeleportToCursorManualCameraComponent teleport = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
		if (teleport)
			teleport.TeleportCamera(position, true, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Next (direction 1) or previous (-1) soldier on foot, players before AI
	void FollowNext(int direction)
	{
		array<int> ids = {};
		array<int> players = {};
		foreach (int id, CRF_ReplayCharProxy proxy : m_mChars)
		{
			if (proxy.m_bKilled || proxy.m_bHidden || !proxy.m_Entity)
				continue;

			CRF_ReplayCharInfo info = m_mCharInfo.Get(id);
			if (info && info.p == 1)
				players.Insert(id);
			else
				ids.Insert(id);
		}

		players.Sort();
		ids.Sort();
		players.InsertAll(ids);
		if (players.IsEmpty())
			return;

		int at = players.Find(m_iFollowedId);
		int nextIndex = 0;
		if (at >= 0)
			nextIndex = (at + direction + players.Count()) % players.Count();
		else if (direction < 0)
			nextIndex = players.Count() - 1;

		Follow(players[nextIndex]);
	}

	// ── Controls & state for the HUD ─────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	bool IsLoading()
	{
		return !m_bIndexed && m_sError.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	string GetStatus()
	{
		if (!m_sError.IsEmpty())
			return m_sError;

		if (!m_bIndexed)
			return string.Format("Reading the recording… %1 min", Math.Floor(m_aOffsets.Count() * m_fInterval / 60));

		return "";
	}

	//------------------------------------------------------------------------------------------------
	string GetTitle()
	{
		return m_sTitle;
	}

	//------------------------------------------------------------------------------------------------
	int GetLastFrame()
	{
		return Math.Max(0, m_aOffsets.Count() - 1);
	}

	//------------------------------------------------------------------------------------------------
	float GetTimeSeconds()
	{
		return m_fTime * m_fInterval;
	}

	//------------------------------------------------------------------------------------------------
	float GetDurationSeconds()
	{
		return GetLastFrame() * m_fInterval;
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlaying()
	{
		return m_bPlaying;
	}

	//------------------------------------------------------------------------------------------------
	void SetPlaying(bool playing)
	{
		if (playing && m_fTime >= GetLastFrame())
			m_fTime = 0;

		m_bPlaying = playing && m_bIndexed;
	}

	//------------------------------------------------------------------------------------------------
	void SeekSeconds(float seconds)
	{
		m_fTime = Math.Clamp(seconds / m_fInterval, 0, GetLastFrame());
	}

	//------------------------------------------------------------------------------------------------
	void SeekBy(float seconds)
	{
		SeekSeconds(GetTimeSeconds() + seconds);
	}

	//------------------------------------------------------------------------------------------------
	float GetSpeed()
	{
		return SPEEDS[m_iSpeedIndex];
	}

	//------------------------------------------------------------------------------------------------
	void ChangeSpeed(int direction)
	{
		m_iSpeedIndex = Math.ClampInt(m_iSpeedIndex + direction, 0, SPEEDS.Count() - 1);
	}

	//------------------------------------------------------------------------------------------------
	string GetFollowedName()
	{
		if (m_iFollowedId == 0)
			return "";

		CRF_ReplayCharInfo info = m_mCharInfo.Get(m_iFollowedId);
		if (!info)
			return "";

		return info.n;
	}

	//------------------------------------------------------------------------------------------------
	//! Kills from the last `seconds` of playback, newest first
	void GetRecentKills(float seconds, out array<CRF_ReplayKillEntry> kills)
	{
		int index = m_fTime;
		int window = seconds / m_fInterval;
		for (int i = m_aKills.Count() - 1; i >= 0; i--)
		{
			CRF_ReplayKillEntry entry = m_aKills[i];
			if (entry.m_iFrame > index)
				continue;

			if (index - entry.m_iFrame > window)
				break;

			kills.Insert(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The kill that put this soldier down most recently, at or before the current frame (null if alive)
	CRF_ReplayKill GetDeathOf(int id)
	{
		int frame = LatestDeath(id, GetFrameIndex());
		if (frame < 0)
			return null;

		CRF_ReplayCharInfo info = m_mCharInfo.Get(id);
		foreach (CRF_ReplayKillEntry entry : m_aKills)
		{
			if (entry.m_iFrame != frame)
				continue;

			CRF_ReplayKill kill = entry.m_Kill;
			if (kill.ti == id || (kill.ti == 0 && info && kill.tn == info.n))
				return kill;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the latest kill at or before the current frame happened (recordings before kill positions have none)
	bool GetLastKillPosition(out vector position)
	{
		int index = GetFrameIndex();
		for (int i = m_aKills.Count() - 1; i >= 0; i--)
		{
			CRF_ReplayKillEntry entry = m_aKills[i];
			if (entry.m_iFrame > index)
				continue;

			CRF_ReplayKill kill = entry.m_Kill;
			if (kill.x == 0 && kill.z == 0)
				continue;

			float x = kill.x / 10.0, z = kill.z / 10.0;
			position = Vector(x, GetGame().GetWorld().GetSurfaceY(x, z), z);
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! The soldiers in the world now, by recording ID (bodies too, m_bKilled)
	map<int, ref CRF_ReplayCharProxy> GetCharacters()
	{
		return m_mChars;
	}

	//------------------------------------------------------------------------------------------------
	map<int, ref CRF_ReplayVehicleProxy> GetVehicles()
	{
		return m_mVehicles;
	}

	//------------------------------------------------------------------------------------------------
	CRF_ReplayCharInfo GetCharInfo(int id)
	{
		return m_mCharInfo.Get(id);
	}

	//------------------------------------------------------------------------------------------------
	CRF_ReplayVehInfo GetVehicleInfo(int id)
	{
		return m_mVehicleInfo.Get(id);
	}

	//------------------------------------------------------------------------------------------------
	int GetFrameIndex()
	{
		return Math.Clamp(Math.Floor(m_fTime), 0, GetLastFrame());
	}

	//------------------------------------------------------------------------------------------------
	//! Seconds between recorded frames
	float GetFrameInterval()
	{
		return m_fInterval;
	}

	//------------------------------------------------------------------------------------------------
	void GetKillFrames(out array<int> frames)
	{
		foreach (CRF_ReplayKillEntry entry : m_aKills)
		{
			frames.Insert(entry.m_iFrame);
		}
	}

	//------------------------------------------------------------------------------------------------
	static float LerpAngle(float a, float b, float t)
	{
		float d = Math.Repeat(b - a + 540, 360) - 180;
		return a + d * t;
	}
}
