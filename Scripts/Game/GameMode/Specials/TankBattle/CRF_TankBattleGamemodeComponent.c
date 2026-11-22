//------------------------------------------------------------------------------------
// CRF_TankBattleGamemodeComponent: one player, one tank, three points (a War Thunder style special)
//
// Each player spawns on foot at their side's flag and picks a tank from a menu; it spawns at one of
// the side's tank spawns and they are put straight in it. One player runs the whole tank: they sit
// in the driver's seat, so driving, the engine and the vehicle's network ownership are vanilla, and
// the main gun is attached to that seat (BaseCompartmentSlot.AttachTurret, the way vanilla's armed
// helicopters give the pilot the rocket pods), so the mouse aims it, left click fires and right
// click goes to its sight. The camera is held in third person. They can't get out. When the tank is
// destroyed, so are they; they come back after Respawn Seconds and pick again.
//
// Capture points are counted by tanks, not people: a point belongs to the side whose tanks sit in
// it alone long enough (more tanks capture faster; tanks from both sides stop it). Every point a
// side holds fills its bar; holding them all fills it faster still. The first bar full wins.
//
// Full guide: !Docs/MissionMaking/Gamemodes/CRF_TANKBATTLE_MISSION_MAKERS_GUIDE.md.
//
// Setup for mission makers (full guide in the doc above):
//   1. Add this component to the gamemode entity. Fill in the two sides: faction, tanks, tank spawns.
//   2. Place a named entity at the centre of each capture point and list them in Points.
//   3. Give each side a respawn flag (players spawn there on foot and pick a tank) and the same
//      number of slots on each side. Leave the mission's own respawns off: this mode respawns
//      players itself.
//------------------------------------------------------------------------------------

enum CRF_ETankBattleState
{
	WAITING,	//!< round not live yet: tanks can be picked but are held
	LIVE,
	ENDED
}

//! What the local player's field repair is doing (server -> that player, CRF_TankBattleGamemodeComponent.UpdateRepairs)
enum CRF_ETankBattleRepair
{
	NONE,
	REPAIRING,		//!< this player's vehicle is repairing a friendly one
	BEING_REPAIRED,	//!< a friendly repair vehicle is repairing this player's
	REPAIRED,		//!< this player's vehicle was just repaired
	REPAIR_DONE		//!< this player just finished repairing a friendly vehicle
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class CRF_TankBattleTank
{
	[Attribute(desc: "Name shown in the tank menu, e.g. \"T-72A\"")]
	string m_sName;

	[Attribute(desc: "One line under the name, e.g. \"Main battle tank\"")]
	string m_sDescription;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Vehicle prefab. Tracked or wheeled, with a turret.", "et")]
	ResourceName m_sPrefab;

	[Attribute("0", UIWidgets.EditBox, "How many of these one side can have out at once (0 = no limit)", params: "0 64 1")]
	int m_iLimit;

	[Attribute("0", UIWidgets.EditBox, "Which turret the driver controls, counting the vehicle's turret seats from 0. The main gun is usually 0; try 1 if the mouse turns the commander's machine gun instead.", params: "0 8 1")]
	int m_iTurretSeat;

	[Attribute("0", UIWidgets.CheckBox, "Field repair: stopped next to a friendly vehicle whose engine or gearbox is destroyed, this one repairs it (the light IFVs, APCs and scout cars)")]
	bool m_bCanRepair;

	protected int m_iAvailable = -1;	//!< -1 not checked yet, 0 no, 1 yes

	//------------------------------------------------------------------------------------------------
	//! Can this vehicle be spawned with the mods that are loaded? Checked once, on every machine (the
	//! server and its clients run the same mods, so the menu and the server agree).
	//! A Tank Battle prefab is a CRF child of a mod's vehicle: without that mod the child itself still
	//! loads, but its parent is missing. So: the prefab loads and has a parent prefab.
	bool IsAvailable()
	{
		if (m_iAvailable < 0)
			m_iAvailable = CheckAvailable();

		return m_iAvailable == 1;
	}

	//------------------------------------------------------------------------------------------------
	protected int CheckAvailable()
	{
		if (m_sPrefab.IsEmpty())
			return 0;

		Resource resource = Resource.Load(m_sPrefab);
		if (!resource || !resource.IsValid())
			return 0;

		BaseResourceObject resourceObject = resource.GetResource();
		if (!resourceObject)
			return 0;

		BaseContainer prefab = resourceObject.ToBaseContainer();
		if (!prefab || !prefab.GetAncestor())
			return 0;

		return 1;
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sFactionKey")]
class CRF_TankBattleSide
{
	[Attribute("BLUFOR", desc: "Faction key")]
	string m_sFactionKey;

	[Attribute(desc: "Tanks this side can pick from, in menu order")]
	ref array<ref CRF_TankBattleTank> m_aTanks;

	[Attribute(desc: "Names of entities where this side's tanks spawn, facing the way the entity faces. A spawn with a tank on it is skipped while another is free. None: near the player.")]
	ref array<string> m_aSpawnEntities;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class CRF_TankBattlePoint
{
	[Attribute("A", desc: "What players call it")]
	string m_sName;

	[Attribute(desc: "Name of an entity at the point's centre")]
	string m_sEntityName;

	[Attribute("60", UIWidgets.EditBox, "Metres from the centre a tank counts as in the point", params: "10 500 1")]
	float m_fRadius;
}

//------------------------------------------------------------------------------------------------
//! Server: one player's state in the mode
class CRF_TankBattlePlayer
{
	int m_iSide = -1;
	RplId m_TankId = RplId.Invalid();
	Vehicle m_Tank;
	int m_iOption = -1;
	bool m_bWasSeated;
	int m_iSeatTries;
	int m_iSeatTriedAtMs;
	int m_iDeadSinceMs = -1;
	int m_iRequestedAtMs = -100000;
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Tank Battle: one player per tank, third person, three capture points and a score bar per side.")]
class CRF_TankBattleGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_TankBattleGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute(desc: "The two sides. The first is drawn on the left of the score bar for spectators.", category: "Tank Battle - Sides")]
	protected ref array<ref CRF_TankBattleSide> m_aSides;

	[Attribute(desc: "The capture points (three is the classic layout, up to five fit on the HUD)", category: "Tank Battle - Points")]
	protected ref array<ref CRF_TankBattlePoint> m_aPoints;

	[Attribute("20", UIWidgets.EditBox, "Seconds for one tank to take a neutral point. An enemy point takes twice as long: it is neutralised first.", params: "1 600 1", category: "Tank Battle - Points")]
	protected float m_fCaptureSeconds;

	[Attribute("0.5", UIWidgets.EditBox, "How much faster each extra tank captures (0.5 = two tanks capture 1.5x as fast)", params: "0 5 0.05", category: "Tank Battle - Points")]
	protected float m_fExtraTankCaptureSpeed;

	[Attribute("2", UIWidgets.EditBox, "Fastest a point can be captured, as a multiple of one tank's speed", params: "1 10 0.1", category: "Tank Battle - Points")]
	protected float m_fMaxCaptureMultiplier;

	[Attribute("1000", UIWidgets.EditBox, "Score that fills a side's bar and wins", params: "10 100000 10", category: "Tank Battle - Score")]
	protected int m_iScoreLimit;

	[Attribute("2", UIWidgets.EditBox, "Score per second for each point a side holds", params: "0 1000 0.1", category: "Tank Battle - Score")]
	protected float m_fScorePerPoint;

	[Attribute("4", UIWidgets.EditBox, "Extra score per second while a side holds every point", params: "0 1000 0.1", category: "Tank Battle - Score")]
	protected float m_fAllPointsBonus;

	[Attribute("0", UIWidgets.EditBox, "Score a side gets for each enemy it kills (0 = kills don't score)", params: "0 10000 1", category: "Tank Battle - Score")]
	protected int m_iScorePerKill;

	[Attribute("0", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when a bar fills. Off (default, it's a special): the result is announced, the tanks are held, and an admin ends the mission.", category: "Tank Battle - Score")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("10", UIWidgets.EditBox, "Seconds after dying before a player is back at their flag to pick another tank", params: "0 300 1", category: "Tank Battle - Players")]
	protected float m_fRespawnSeconds;

	[Attribute("1", UIWidgets.CheckBox, "Respawn players after Respawn Seconds. Off: the mission's own respawn settings decide.", category: "Tank Battle - Players")]
	protected bool m_bRespawnPlayers;

	[Attribute("1", UIWidgets.CheckBox, "Players can't get out of their tank or change seats", category: "Tank Battle - Players")]
	protected bool m_bLockCrewIn;

	[Attribute("1", UIWidgets.CheckBox, "Hold the camera in third person while in a tank. The gun sights still work.", category: "Tank Battle - Players")]
	protected bool m_bForceThirdPerson;

	[Attribute("1", UIWidgets.CheckBox, "Tanks are held (engine off, handbrake on) until the round goes live", category: "Tank Battle - Players")]
	protected bool m_bHoldUntilLive;

	[Attribute("60", UIWidgets.EditBox, "Seconds a wreck, or a tank whose player is gone, stays before it's removed", params: "0 600 1", category: "Tank Battle - Players")]
	protected float m_fWreckSeconds;

	[Attribute("15", UIWidgets.EditBox, "Seconds a repair vehicle (Can Repair on its tank option) needs, stopped next to a friendly vehicle with a destroyed engine or gearbox, to repair it (0 = no field repair)", params: "0 120 1", category: "Tank Battle - Repair")]
	protected float m_fRepairSeconds;

	[Attribute("15", UIWidgets.EditBox, "How close the repair vehicle has to be: metres between the two vehicles' centres", params: "5 50 1", category: "Tank Battle - Repair")]
	protected float m_fRepairRange;

	[Attribute("1.5", UIWidgets.EditBox, "Both vehicles have to be (nearly) stopped: metres per second", params: "0 10 0.1", category: "Tank Battle - Repair")]
	protected float m_fRepairMaxSpeed;

	[Attribute("1.6", UIWidgets.EditBox, "Third person camera distance in a tank, as a multiple of the tank's own (1 = vanilla)", params: "1 4 0.05", category: "Tank Battle - Camera")]
	protected float m_fCameraDistanceScale;

	[Attribute("10", UIWidgets.EditBox, "Degrees added to the third person field of view in a tank (0 = vanilla)", params: "0 40 1", category: "Tank Battle - Camera")]
	protected float m_fCameraExtraFov;

	[Attribute("2.5", UIWidgets.EditBox, "Metres the third person camera is raised in a tank. It looks down at where the gun points, so the aim point stays in the middle of the screen.", params: "0 15 0.1", category: "Tank Battle - Camera")]
	protected float m_fCameraExtraHeight;

	[Attribute("1", UIWidgets.CheckBox, "Third person camera looks at where the gun points (the crosshair stays in the middle of the screen). Off: vanilla, along the gun.", category: "Tank Battle - Camera")]
	protected bool m_bCameraLookAtAim;

	[Attribute("1", UIWidgets.CheckBox, "Crosshair where the gun points, in third person", category: "Tank Battle - HUD")]
	protected bool m_bShowCrosshair;

	[Attribute("1", UIWidgets.CheckBox, "Minimap in the bottom right while the player sits in their tank: the capture zones and friendly tanks", category: "Tank Battle - HUD")]
	protected bool m_bShowMinimap;

	[Attribute("1", UIWidgets.CheckBox, "Draw the map's satellite image under the minimap", category: "Tank Battle - HUD")]
	protected bool m_bMinimapSatellite;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Image under the minimap instead of the world's own map satellite image (it must cover the whole terrain, north up). Needed on maps that set the image on their placed map entity rather than its prefab, e.g. Chernarus Minus: {096230E40A49DAC9}Worlds/MapData/ChernarusMinusMap.edds", "edds", category: "Tank Battle - HUD")]
	protected ResourceName m_sMinimapImage;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker for the capture points", "edds", category: "Tank Battle - Map")]
	protected ResourceName m_sPointMarkerIcon;

	// Replicated
	[RplProp()]
	protected int m_iState;
	[RplProp()]
	protected int m_iScore0;
	[RplProp()]
	protected int m_iScore1;
	[RplProp()]
	protected int m_iWinnerSide = -1;
	[RplProp()]
	protected ref array<int> m_aPointOwner = {};		//!< -1 nobody, else side index
	[RplProp()]
	protected ref array<int> m_aPointProgress = {};		//!< -100 (side 1 holds it) to 100 (side 0)
	[RplProp()]
	protected ref array<int> m_aPointCapturing = {};	//!< -1 nobody in it, side index, or CONTESTED
	[RplProp()]
	protected ref array<int> m_aTankPlayers = {};		//!< who has a tank, with the three lists below
	[RplProp()]
	protected ref array<RplId> m_aTankIds = {};
	[RplProp()]
	protected ref array<int> m_aTankSides = {};
	[RplProp()]
	protected ref array<int> m_aTankOptions = {};
	// Server
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_Safestart;
	protected bool m_bEndRequested;
	protected bool m_bDirty;
	protected ref map<int, ref CRF_TankBattlePlayer> m_mPlayers = new map<int, ref CRF_TankBattlePlayer>();
	protected ref array<float> m_aProgress = {};		//!< -1 to 1, as m_aPointProgress
	protected float m_fScore0;
	protected float m_fScore1;
	protected ref array<RplId> m_aWreckIds = {};
	protected ref array<int> m_aWreckRemoveAtMs = {};
	protected int m_iPositionTicks;
	protected ref map<RplId, float> m_mRepairProgress = new map<RplId, float>();	//!< seconds of field repair each broken vehicle has had

	// Every machine
	protected ref array<IEntity> m_aPointEntities = {};
	protected ref array<IEntity> m_aAttachLogged = {};	//!< tanks whose turret attachment has been logged (once each)
	protected ref array<IEntity> m_aNoTurretWarned = {};
	protected bool m_bPointsFound;

	// Client
	protected Vehicle m_LocalTank;
	protected bool m_bLocalHeld;			//!< this machine put the local tank's handbrake on for the hold
	protected int m_iEngineStartAtMs;
	protected int m_iMenuSnoozeUntilMs;
	protected int m_iTurretCheckAtMs;
	protected ref map<RplId, int> m_mFriendlyTankPositions = new map<RplId, int>();	//!< own side's tanks (PackPosition) from the server's last SendTankPositions
	protected int m_iFriendlyPositionsAtMs = -100000;
	protected bool m_bMouseSteeringHeld;		//!< HoldMouseSteeringOff has the player's mouse steering off
	protected bool m_bSavedMouseControlVehicle;
	protected float m_fSavedMouseSteeringSensitivity = 1;
	protected Vehicle m_RecoverTank;			//!< RecoverLocalTurret: the tank its tries are counted for
	protected int m_iRecoverTries;
	protected bool m_bRecoverSeated;			//!< ...and whether the current seating has been timed yet
	protected int m_iRecoverSeatedAtMs;
	protected int m_iRecoverCheckAtMs;
	protected int m_iRepairStatus;				//!< CRF_ETankBattleRepair, from the server (SetRepairStatus)
	protected int m_iRepairPercent;
	protected int m_iRepairStatusAtMs = -100000;
	protected ref TraceParam m_GunTrace;
	protected bool m_bGunRayCached;
	protected bool m_bGunRayValid;
	protected int m_iGunRayAtMs;
	protected vector m_vGunRayOrigin;
	protected vector m_vGunRayDirection;
	protected float m_fGunRayDistance;

	static const float GUN_TRACE_RANGE = 2500;		//!< m traced along the gun
	protected static const float GUN_TRACE_START = 3;	//!< m past the muzzle the trace starts (clear of the barrel)
	protected ref CRF_SpecialsMarkerSet m_Markers;
	protected ref CRF_TankBattleHUD m_Hud;

	static const int CONTESTED = 2;

	protected static const int TICK_MS = 500;
	protected static const float TICK_SECONDS = 0.5;
	protected static const int POSITION_EVERY_TICKS = 2;	//!< SendTankPositions: every second
	protected static const int POSITION_STALE_MS = 3500;	//!< client: drop the server's positions this long after the last arrived
	protected static const int SEAT_MAX_TRIES = 5;
	protected static const int SEAT_RETRY_MS = 1500;
	protected static const int REQUEST_COOLDOWN_MS = 1500;
	protected static const int ENGINE_RETRY_MS = 2000;
	protected static const int TURRET_CHECK_MS = 250;
	protected static const int RECOVER_CHECK_MS = 500;	//!< how often a seated driver's missing gun connection is looked at
	protected static const int RECOVER_WAIT_MS = 1500;	//!< seated this long without the gun before being re-seated
	protected static const int RECOVER_MAX_TRIES = 3;
	protected static const int MENU_SNOOZE_MS = 5000;
	protected static const float SPAWN_CLEAR_RADIUS = 9;	//!< a tank spawn with a tank this close is taken
	protected static const string LOG = "[CRF_TankBattle]";

	protected static CRF_TankBattleGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_TankBattleGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_TankBattleGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_TankBattleGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		SetEventMask(owner, EntityEvent.FRAME);

		if (!System.IsConsoleApp())
		{
			m_Markers = new CRF_SpecialsMarkerSet();
			m_Hud = new CRF_TankBattleHUD();
			GetGame().GetCallqueue().CallLater(UpdateMarkers, 1000, true);
			GetGame().GetCallqueue().CallLater(UpdateUi, 250, true);
		}

		if (Replication.IsServer())
		{
			InitPointState();
			GetGame().GetCallqueue().CallLater(HookGamemode, 1000, false);
		}
	}

	override void OnDelete(IEntity owner)
	{
		// Leaving the mission from inside a tank: the player's own mouse steering settings back
		RestoreMouseSteering();

		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(HookGamemode);
			GetGame().GetCallqueue().Remove(UpdateMarkers);
			GetGame().GetCallqueue().Remove(UpdateUi);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Safestart && m_Safestart.m_OnRoundLive)
			m_Safestart.m_OnRoundLive.Remove(OnRoundLive);

		CRF_TankBattleMenu.CloseMenu();

		if (m_Markers)
			m_Markers.Clear();

		if (m_Hud)
			m_Hud.Destroy();

		super.OnDelete(owner);
	}

	protected void InitPointState()
	{
		m_aPointOwner.Clear();
		m_aPointProgress.Clear();
		m_aPointCapturing.Clear();
		m_aProgress.Clear();

		if (!m_aPoints)
			return;

		for (int i = 0; i < m_aPoints.Count(); i++)
		{
			m_aPointOwner.Insert(-1);
			m_aPointProgress.Insert(0);
			m_aPointCapturing.Insert(-1);
			m_aProgress.Insert(0);
		}
	}

	//! Server, once: the vehicles left out of the menu because their mod isn't loaded
	protected void LogUnavailableTanks()
	{
		if (!m_aSides)
			return;

		foreach (CRF_TankBattleSide side : m_aSides)
		{
			if (!side.m_aTanks)
				continue;

			int available;
			foreach (CRF_TankBattleTank tank : side.m_aTanks)
			{
				if (tank.IsAvailable())
				{
					available++;
					continue;
				}

				Print(string.Format("%1 %2: '%3' isn't offered - its prefab or the mod it comes from isn't loaded (%4).", LOG, side.m_sFactionKey, tank.m_sName, tank.m_sPrefab), LogLevel.WARNING);
			}

			if (available == 0)
				Print(string.Format("%1 %2 has no vehicle it can spawn: its players can't play.", LOG, side.m_sFactionKey), LogLevel.ERROR);
		}
	}

	protected void HookGamemode()
	{
		if (!m_aSides || m_aSides.Count() != 2)
			Print(string.Format("%1 Needs exactly two sides, has %2.", LOG, SideCount()), LogLevel.ERROR);

		if (!m_aPoints || m_aPoints.IsEmpty())
			Print(string.Format("%1 No capture points: nobody can score.", LOG), LogLevel.WARNING);

		LogUnavailableTanks();

		m_Gamemode = COA_Gamemode.GetInstance();
		if (!m_Gamemode)
		{
			Print(string.Format("%1 No COA_Gamemode: Tank Battle is not running.", LOG), LogLevel.WARNING);
			return;
		}

		m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		m_Safestart = COA_SafestartManager.GetInstance();
		if (m_Safestart && m_Safestart.m_OnRoundLive)
			m_Safestart.m_OnRoundLive.Insert(OnRoundLive);

		// Players are looked after (tanks, seats, respawns) from now on, safestart included
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);

		OnGamemodeStateChanged();
	}

	protected void OnRoundLive()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode)
			return;

		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
		{
			// Missions with safestart start on OnRoundLive
			if (m_iState == CRF_ETankBattleState.WAITING && (!m_Safestart || m_Safestart.IsRoundLive()))
				StartRound();
			return;
		}

		// The mission ended, whoever decided it
		if (m_iState == CRF_ETankBattleState.LIVE)
			SetEnded(-1);
	}

	protected void StartRound()
	{
		if (m_iState != CRF_ETankBattleState.WAITING)
			return;

		m_iState = CRF_ETankBattleState.LIVE;
		Replication.BumpMe();

		CheckSidesEven();

		string pointNames = PointNames();
		CRF_RadioTransmissionEvents.PopupToFaction("", "TANK BATTLE", string.Format("Hold %1. The more you hold, the faster your bar fills.", pointNames), 8);
	}

	//! 1:1 is the mission's slots: this only says so when the sides are uneven at the start
	protected void CheckSidesEven()
	{
		if (SideCount() != 2)
			return;

		int count0 = CountSidePlayers(0);
		int count1 = CountSidePlayers(1);
		if (count0 == count1)
			return;

		string text = string.Format("%1 %2 v %3 %4", CRF_SpecialsHelper.GetFactionName(m_aSides[0].m_sFactionKey), count0, count1, CRF_SpecialsHelper.GetFactionName(m_aSides[1].m_sFactionKey));
		Print(string.Format("%1 Uneven sides at the start: %2", LOG, text), LogLevel.WARNING);
		CRF_RadioTransmissionEvents.PopupToFaction("", "UNEVEN SIDES", text, 8);
	}

	protected int CountSidePlayers(int side)
	{
		array<int> players = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_aSides[side].m_sFactionKey, players);
		return players.Count();
	}

	//===================================================================================
	// SERVER: THE TICK
	//===================================================================================

	protected void Tick()
	{
		int now = GetGame().GetWorld().GetWorldTime();

		ManagePlayers(now);
		RemoveWrecks(now);

		m_iPositionTicks++;
		if (m_iPositionTicks >= POSITION_EVERY_TICKS)
		{
			m_iPositionTicks = 0;
			SendTankPositions();
		}

		if (m_iState == CRF_ETankBattleState.LIVE && !m_bEndRequested)
		{
			UpdatePoints();
			UpdateScore();
			UpdateRepairs();
		}

		if (m_bDirty)
		{
			m_bDirty = false;
			Replication.BumpMe();
		}
	}

	//===================================================================================
	// SERVER: PLAYERS AND THEIR TANKS
	//===================================================================================

	protected CRF_TankBattlePlayer GetPlayerState(int playerId)
	{
		CRF_TankBattlePlayer state = m_mPlayers.Get(playerId);
		if (!state)
		{
			state = new CRF_TankBattlePlayer();
			m_mPlayers.Set(playerId, state);
		}

		return state;
	}

	protected void ManagePlayers(int now)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			int side = GetSideOfFaction(CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId));
			if (side < 0)
				continue;

			CRF_TankBattlePlayer state = GetPlayerState(playerId);
			state.m_iSide = side;

			ChimeraCharacter character = ChimeraCharacter.Cast(CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId));
			if (character && COA_EntityHelper.IsSpectator(character))
				character = null;

			if (!character)
			{
				if (state.m_Tank || state.m_TankId.IsValid())
					ReleaseTank(state, now);

				if (slottingManager.IsPlayerInASlot(playerId) && slottingManager.IsPlayerConsideredDead(playerId))
					UpdateRespawn(playerId, state, now);
				else
					state.m_iDeadSinceMs = -1;

				continue;
			}

			state.m_iDeadSinceMs = -1;
			ManageTank(playerId, state, character, now);
		}
	}

	//! Server: a living player and their tank, if they have one
	protected void ManageTank(int playerId, notnull CRF_TankBattlePlayer state, notnull ChimeraCharacter character, int now)
	{
		Vehicle tank = GetStateTank(state);
		if (!tank)
		{
			if (state.m_TankId.IsValid())
				ReleaseTank(state, now);	// deleted under them
			return;
		}

		CharacterControllerComponent controller = character.GetCharacterController();

		// Their tank went up: so do they
		if (IsWrecked(tank))
		{
			if (character.IsInVehicle() || state.m_bWasSeated)
				KillCharacter(character);

			ReleaseTank(state, now);
			return;
		}

		if (character.IsInVehicle() && character.GetRootParent() == tank)
		{
			state.m_bWasSeated = true;
			state.m_iSeatTries = 0;

			// No revives in a tank
			if (controller && controller.GetLifeState() == ECharacterLifeState.INCAPACITATED)
				KillCharacter(character);

			return;
		}

		// Not in it (yet): the seat change runs on the player's own machine, which may not have the
		// new tank yet, so it's tried a few times
		if (state.m_iSeatTries >= SEAT_MAX_TRIES)
		{
			if (now - state.m_iSeatTriedAtMs < SEAT_RETRY_MS)
				return;

			if (state.m_bWasSeated)
			{
				// They got out somehow and couldn't be put back
				KillCharacter(character);
			}
			else
			{
				Print(string.Format("%1 Couldn't seat player %2 in their tank: removed it so they can pick again.", LOG, playerId), LogLevel.WARNING);
				CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "TANK LOST", "Couldn't put you in it. Pick again.", 5);
			}

			ReleaseTank(state, now, true);
			return;
		}

		if (state.m_iSeatTries > 0 && now - state.m_iSeatTriedAtMs < SEAT_RETRY_MS)
			return;

		if (character.IsInVehicle())
			return;	// in some other vehicle: wait for them to be out

		state.m_iSeatTries++;
		state.m_iSeatTriedAtMs = now;
		SeatCharacter(character, tank, state.m_iSide, state.m_iOption);
	}

	protected void SeatCharacter(notnull ChimeraCharacter character, notnull Vehicle tank, int side, int option)
	{
		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(character.GetCompartmentAccessComponent());
		if (!access)
			return;

		AttachMainTurret(tank);

		BaseCompartmentSlot seat = GetDriverSeat(tank);
		if (!seat || seat.GetOccupant() || !access.MoveInVehicle(tank, ECompartmentType.PILOT, false, seat))
			Print(string.Format("%1 No free driver's seat in %2.", LOG, tank), LogLevel.WARNING);
	}

	//! The vehicle's driver's seat, null if it has none
	static BaseCompartmentSlot GetDriverSeat(notnull Vehicle vehicle)
	{
		VehicleControllerComponent controller = vehicle.GetVehicleController();
		if (!controller)
			return null;

		return controller.GetPilotCompartmentSlot();
	}

	//! The turret the driver controls: the index'th turret seat's turret (m_iTurretSeat), null if there isn't one
	static TurretControllerComponent GetMainTurret(notnull IEntity vehicle, int index)
	{
		array<BaseCompartmentSlot> seats = {};
		CollectSeats(vehicle, ECompartmentType.TURRET, seats);
		if (!seats.IsIndexValid(index))
			return null;

		return TurretControllerComponent.Cast(seats[index].GetController());
	}

	//! The turret in entity or anything slotted on it whose controller has this UniqueTurretName
	static TurretControllerComponent FindTurretByName(notnull IEntity entity, string uniqueName)
	{
		TurretControllerComponent turret = TurretControllerComponent.Cast(entity.FindComponent(TurretControllerComponent));
		if (turret && turret.GetUniqueName() == uniqueName)
			return turret;

		IEntity child = entity.GetChildren();
		while (child)
		{
			turret = FindTurretByName(child, uniqueName);
			if (turret)
				return turret;

			child = child.GetSibling();
		}

		return null;
	}

	//! Seats of a type in entity and the entities slotted on it, in vanilla's order (the hull's own, then
	//! each child's, depth first). A compartment manager only lists its own entity's seats: a tank's
	//! gunner sits on the turret entity (see SCR_CompartmentAccessComponent.FindFreeAndAccessibleCompartment).
	static void CollectSeats(notnull IEntity entity, ECompartmentType type, notnull array<BaseCompartmentSlot> seats)
	{
		BaseCompartmentManagerComponent manager = BaseCompartmentManagerComponent.Cast(entity.FindComponent(BaseCompartmentManagerComponent));
		if (manager)
		{
			array<BaseCompartmentSlot> own = {};
			manager.GetCompartments(own);
			foreach (BaseCompartmentSlot seat : own)
			{
				if (seat.GetType() == type)
					seats.Insert(seat);
			}
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			CollectSeats(child, type, seats);
			child = child.GetSibling();
		}
	}

	//! Every machine: the main gun answers to the driver's seat. Aim (mouse), sight, fire and zeroing,
	//! not TurretContext: that holds the W/A/S/D turret keys, which drive here. Bits are
	//! 1 << ETurretContextID, as in vanilla's AutoConnectContextIndices (armed Mi-8: 0x8, fire only).
	protected void AttachMainTurret(notnull Vehicle tank)
	{
		BaseCompartmentSlot driverSeat = GetDriverSeat(tank);
		if (!driverSeat)
			return;

		int seatIndex;
		CRF_TankBattleTank tankOption;
		int index = m_aTankIds.Find(GetRplId(tank));
		if (index >= 0 && m_aTankOptions.IsIndexValid(index) && m_aTankSides.IsIndexValid(index))
			tankOption = GetTankOption(m_aTankSides[index], m_aTankOptions[index]);
		if (tankOption)
			seatIndex = tankOption.m_iTurretSeat;

		// The turret the seat auto-connects to (Tank Battle prefabs name it), else the Turret Seat'th turret
		TurretControllerComponent turret;
		string autoConnect = driverSeat.GetAutoConnectTurret();
		if (!autoConnect.IsEmpty())
			turret = FindTurretByName(tank, autoConnect);

		if (!turret)
			turret = GetMainTurret(tank, seatIndex);

		if (!turret)
		{
			if (!m_aNoTurretWarned.Contains(tank))
			{
				m_aNoTurretWarned.Insert(tank);
				Print(string.Format("%1 %2 has no turret seat %3: the driver can't fire.", LOG, tank, seatIndex), LogLevel.WARNING);
			}
			return;
		}

		// Never to an empty seat
		if (!driverSeat.GetOccupant())
			return;

		// A Tank Battle prefab's seat connects the gun natively as the driver gets in (AutoConnectToTurret), and
		// only that connection is wired to the driver's input. It lands a moment after the seat reports its
		// occupant (vanilla allows for the same gap in SCR_CompartmentAccessComponent.ToggleTurretFireModeControlls).
		// A scripted AttachTurret in that gap - or any other time - leaves the seat reporting the gun attached
		// with the mouse steering the hull instead, which is what happened "sometimes" on spawning. So such a
		// seat is never attached from script: if the native connection is missing, the driver's own machine
		// re-seats them (RecoverLocalTurret). Scripted attaching is only for a prefab without AutoConnectToTurret,
		// whose turret must allow it (CanBeOverridden) or nothing happens.
		bool connectsNatively = !autoConnect.IsEmpty();
		if (!connectsNatively && driverSeat.GetAttachedTurret() != turret)
			driverSeat.AttachTurret(turret, GetAttachContexts());

		// Not connected yet: nothing to report until it is
		if (connectsNatively && !driverSeat.GetAttachedTurret())
			return;

		// Once per tank, with the driver in: whether it worked
		if (m_aAttachLogged.Contains(tank))
			return;

		m_aAttachLogged.Insert(tank);
		TurretControllerComponent attached = driverSeat.GetAttachedTurret();
		if (attached == turret)
		{
			Print(string.Format("%1 %2: the driver controls the main gun %3 (contexts %4).", LOG, tank, turret.GetOwner(), attached.GetContextIDs()), LogLevel.NORMAL);
			return;
		}

		Print(string.Format("%1 %2: the main gun %3 couldn't be attached to the driver's seat (attached: %4). Use a Tank Battle prefab: the turret needs CanBeOverridden, the driver's seat AutoConnectToTurret (see the guide).", LOG, tank, turret.GetOwner(), attached), LogLevel.WARNING);
	}

	protected static RplId GetRplId(notnull IEntity entity)
	{
		RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rpl)
			return RplId.Invalid();

		return rpl.Id();
	}

	//! Player dead: back at the flag after m_fRespawnSeconds
	protected void UpdateRespawn(int playerId, notnull CRF_TankBattlePlayer state, int now)
	{
		if (!m_bRespawnPlayers || m_iState == CRF_ETankBattleState.ENDED || !m_Gamemode || m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		if (state.m_iDeadSinceMs < 0)
		{
			state.m_iDeadSinceMs = now;
			int seconds = m_fRespawnSeconds;
			if (seconds > 0)
				CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "TANK DESTROYED", string.Format("Back in %1 seconds.", seconds), 4);
			return;
		}

		if (now - state.m_iDeadSinceMs < m_fRespawnSeconds * 1000)
			return;

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return;

		state.m_iDeadSinceMs = -1;
		respawnManager.RespawnPlayer(playerId);
	}

	//! The player no longer has this tank. It's removed after m_fWreckSeconds (straight away if
	//! removeNow), unless someone is still in it.
	protected void ReleaseTank(notnull CRF_TankBattlePlayer state, int now, bool removeNow = false)
	{
		if (state.m_TankId.IsValid())
		{
			int delayMs = m_fWreckSeconds * 1000;
			if (removeNow)
				delayMs = 0;

			m_aWreckIds.Insert(state.m_TankId);
			m_aWreckRemoveAtMs.Insert(now + delayMs);
		}

		state.m_TankId = RplId.Invalid();
		state.m_Tank = null;
		state.m_iOption = -1;
		state.m_bWasSeated = false;
		state.m_iSeatTries = 0;
		RebuildTankList();
	}

	protected void RemoveWrecks(int now)
	{
		for (int i = m_aWreckIds.Count() - 1; i >= 0; i--)
		{
			if (now < m_aWreckRemoveAtMs[i])
				continue;

			IEntity wreck = COA_EntityHelper.GetEntityFromRplId(m_aWreckIds[i]);
			if (wreck && !IsWrecked(wreck) && HasLivingOccupant(wreck))
				continue;	// someone got in: leave it

			if (wreck)
				SCR_EntityHelper.DeleteEntityAndChildren(wreck);

			m_aWreckIds.Remove(i);
			m_aWreckRemoveAtMs.Remove(i);
		}
	}

	protected void KillCharacter(notnull ChimeraCharacter character)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(character.GetDamageManager());
		if (damageManager && damageManager.GetState() != EDamageState.DESTROYED)
			damageManager.Kill(Instigator.CreateInstigator(null));
	}

	//! The replicated list of who has which tank, from m_mPlayers
	protected void RebuildTankList()
	{
		m_aTankPlayers.Clear();
		m_aTankIds.Clear();
		m_aTankSides.Clear();
		m_aTankOptions.Clear();

		foreach (int playerId, CRF_TankBattlePlayer state : m_mPlayers)
		{
			if (!state.m_TankId.IsValid())
				continue;

			m_aTankPlayers.Insert(playerId);
			m_aTankIds.Insert(state.m_TankId);
			m_aTankSides.Insert(state.m_iSide);
			m_aTankOptions.Insert(state.m_iOption);
		}

		m_bDirty = true;
	}

	//! Server, every second: each side's tank positions to that side's players only, for the minimap's
	//! dots past the streaming range (the server's network view distance can be shorter than the
	//! field). Not an RplProp: they're stale in a second, nobody joining needs the old ones, and an
	//! RplProp reaches every client, so enemy positions would too. Unreliable: a lost one is replaced
	//! by the next.
	protected void SendTankPositions()
	{
		array<RplId> ids0 = {};
		array<int> positions0 = {};
		array<RplId> ids1 = {};
		array<int> positions1 = {};

		foreach (int i, RplId tankId : m_aTankIds)
		{
			if (!m_aTankSides.IsIndexValid(i))
				break;

			IEntity tank = COA_EntityHelper.GetEntityFromRplId(tankId);
			if (!tank || IsWrecked(tank))
				continue;

			if (m_aTankSides[i] == 0)
			{
				ids0.Insert(tankId);
				positions0.Insert(PackPosition(tank.GetOrigin()));
			}
			else if (m_aTankSides[i] == 1)
			{
				ids1.Insert(tankId);
				positions1.Insert(PackPosition(tank.GetOrigin()));
			}
		}

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();	// a hosting player has every tank already
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (playerId == localPlayerId)
				continue;

			int side = GetSideOfFaction(CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId));
			if (side < 0)
				continue;

			COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
			if (!ownerManager)
				continue;

			if (side == 0)
				ownerManager.ReceiveTankBattlePositions(ids0, positions0);
			else
				ownerManager.ReceiveTankBattlePositions(ids1, positions1);
		}
	}

	//! Client, from SendTankPositions (via COA_PlayerRplToOwnerManager)
	void SetFriendlyTankPositions(notnull array<RplId> tankIds, notnull array<int> positions)
	{
		m_mFriendlyTankPositions.Clear();
		foreach (int i, RplId tankId : tankIds)
		{
			if (positions.IsIndexValid(i))
				m_mFriendlyTankPositions.Set(tankId, positions[i]);
		}

		m_iFriendlyPositionsAtMs = GetGame().GetWorld().GetWorldTime();
	}

	//===================================================================================
	// SERVER: FIELD REPAIR
	//===================================================================================

	//! Server, twice a second while live: a repair vehicle (its tank option has Can Repair) stopped
	//! within m_fRepairRange of a stopped friendly mode vehicle whose engine or gearbox is destroyed
	//! repairs it in m_fRepairSeconds. One repair at a time per repair vehicle, the nearest broken
	//! one first; a repair vehicle can't repair itself. Progress drains while nobody is repairing.
	protected void UpdateRepairs()
	{
		if (m_fRepairSeconds <= 0)
			return;

		array<Vehicle> repairers = {};
		array<int> repairerIndices = {};
		array<Vehicle> broken = {};
		array<int> brokenIndices = {};
		foreach (int i, RplId tankId : m_aTankIds)
		{
			if (!m_aTankSides.IsIndexValid(i) || !m_aTankOptions.IsIndexValid(i) || !m_aTankPlayers.IsIndexValid(i))
				break;

			Vehicle tank = Vehicle.Cast(COA_EntityHelper.GetEntityFromRplId(tankId));
			if (!tank || IsWrecked(tank) || GetSpeed(tank) > m_fRepairMaxSpeed)
				continue;

			if (HasBrokenDrive(tank))
			{
				broken.Insert(tank);
				brokenIndices.Insert(i);
			}

			CRF_TankBattleTank option = GetTankOption(m_aTankSides[i], m_aTankOptions[i]);
			if (option && option.m_bCanRepair)
			{
				repairers.Insert(tank);
				repairerIndices.Insert(i);
			}
		}

		array<RplId> worked = {};
		if (!repairers.IsEmpty())
		{
			array<int> busy = {};
			float range = m_fRepairRange * m_fRepairRange;
			foreach (int b, Vehicle target : broken)
			{
				int targetIndex = brokenIndices[b];
				int best = -1;
				float bestDistance = range;
				foreach (int r, Vehicle repairer : repairers)
				{
					int repairerIndex = repairerIndices[r];
					if (repairerIndex == targetIndex || busy.Contains(repairerIndex) || m_aTankSides[repairerIndex] != m_aTankSides[targetIndex])
						continue;

					float distance = vector.DistanceSq(target.GetOrigin(), repairer.GetOrigin());
					if (distance <= bestDistance)
					{
						bestDistance = distance;
						best = repairerIndex;
					}
				}

				if (best < 0)
					continue;

				busy.Insert(best);
				RplId targetId = m_aTankIds[targetIndex];
				worked.Insert(targetId);

				float progress;
				m_mRepairProgress.Find(targetId, progress);
				progress += TICK_SECONDS;
				if (progress >= m_fRepairSeconds)
				{
					RepairDrive(target);
					m_mRepairProgress.Remove(targetId);
					SendRepairStatus(m_aTankPlayers[targetIndex], CRF_ETankBattleRepair.REPAIRED, 100);
					SendRepairStatus(m_aTankPlayers[best], CRF_ETankBattleRepair.REPAIR_DONE, 100);
					Print(string.Format("[CRF_TankBattle] Player %1 repaired player %2's vehicle", m_aTankPlayers[best], m_aTankPlayers[targetIndex]), LogLevel.NORMAL);
					continue;
				}

				m_mRepairProgress.Set(targetId, progress);
				int percent = progress * 100 / m_fRepairSeconds;
				SendRepairStatus(m_aTankPlayers[targetIndex], CRF_ETankBattleRepair.BEING_REPAIRED, percent);
				SendRepairStatus(m_aTankPlayers[best], CRF_ETankBattleRepair.REPAIRING, percent);
			}
		}

		// Nobody working on it (it or the repair vehicle moved off, or left): the progress drains
		array<RplId> idle = {};
		foreach (RplId idleId, float idleProgress : m_mRepairProgress)
		{
			if (!worked.Contains(idleId))
				idle.Insert(idleId);
		}

		foreach (RplId drainId : idle)
		{
			float drained = m_mRepairProgress.Get(drainId) - TICK_SECONDS;
			if (drained <= 0 || !m_aTankIds.Contains(drainId))
				m_mRepairProgress.Remove(drainId);
			else
				m_mRepairProgress.Set(drainId, drained);
		}
	}

	//! Server: engine and gearbox back to full health (the damage manager turns them back on; the
	//! driver's machine restarts the engine by itself, UpdateLocalTank)
	protected void RepairDrive(notnull Vehicle vehicle)
	{
		SCR_VehicleDamageManagerComponent damageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!damageManager)
			return;

		array<HitZone> hitZones = {};
		damageManager.GetAllHitZonesInHierarchy(hitZones);
		foreach (HitZone hitZone : hitZones)
		{
			if (SCR_EngineHitZone.Cast(hitZone) || SCR_GearboxHitZone.Cast(hitZone))
				hitZone.SetHealthScaled(1);
		}
	}

	protected void SendRepairStatus(int playerId, CRF_ETankBattleRepair status, int percent)
	{
		if (playerId <= 0)
			return;

		// A hosting player: straight in
		if (playerId == SCR_PlayerController.GetLocalPlayerId())
		{
			SetRepairStatus(status, percent);
			return;
		}

		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (ownerManager)
			ownerManager.ReceiveTankBattleRepair(status, percent);
	}

	//! Every machine: is this vehicle's engine or gearbox destroyed (it can't drive, but isn't a wreck)?
	//! The damage manager keeps both flags up to date from its hit zones on every machine.
	bool HasBrokenDrive(notnull IEntity vehicle)
	{
		SCR_VehicleDamageManagerComponent damageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		return damageManager && (!damageManager.GetEngineFunctional() || !damageManager.GetGearboxFunctional());
	}

	protected static float GetSpeed(notnull IEntity vehicle)
	{
		Physics physics = vehicle.GetPhysics();
		if (!physics)
			return 0;

		return physics.GetVelocity().Length();
	}

	//! Client, from SendRepairStatus (via COA_PlayerRplToOwnerManager)
	void SetRepairStatus(int status, int percent)
	{
		m_iRepairStatus = status;
		m_iRepairPercent = percent;
		BaseWorld world = GetGame().GetWorld();
		if (world)
			m_iRepairStatusAtMs = world.GetWorldTime();
	}

	//! Client: what the local player's field repair is doing (CRF_ETankBattleRepair; NONE once the
	//! server stops saying - progress comes twice a second, the "done" ones are shown for 4 s)
	int GetRepairStatus(out int percent)
	{
		percent = m_iRepairPercent;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return CRF_ETankBattleRepair.NONE;

		int age = world.GetWorldTime() - m_iRepairStatusAtMs;
		if (m_iRepairStatus == CRF_ETankBattleRepair.REPAIRED || m_iRepairStatus == CRF_ETankBattleRepair.REPAIR_DONE)
		{
			if (age < 4000)
				return m_iRepairStatus;

			return CRF_ETankBattleRepair.NONE;
		}

		if (age < 1500)
			return m_iRepairStatus;

		return CRF_ETankBattleRepair.NONE;
	}

	//! Client: can the local player's vehicle do field repairs?
	bool CanLocalRepair()
	{
		if (!m_LocalTank || m_fRepairSeconds <= 0)
			return false;

		RplComponent rpl = RplComponent.Cast(m_LocalTank.FindComponent(RplComponent));
		if (!rpl)
			return false;

		int index = m_aTankIds.Find(rpl.Id());
		if (index < 0 || !m_aTankSides.IsIndexValid(index) || !m_aTankOptions.IsIndexValid(index))
			return false;

		CRF_TankBattleTank option = GetTankOption(m_aTankSides[index], m_aTankOptions[index]);
		return option && option.m_bCanRepair;
	}

	//! Client: the nearest friendly vehicle with a destroyed engine or gearbox that this machine has
	//! (streamed in), and how far; -1 if none within maxDistance
	float GetNearestBrokenFriendly(float maxDistance)
	{
		if (!m_LocalTank)
			return -1;

		int localSide = GetLocalSide();
		vector origin = m_LocalTank.GetOrigin();
		float best = maxDistance * maxDistance;
		bool found;
		foreach (int i, RplId tankId : m_aTankIds)
		{
			if (!m_aTankSides.IsIndexValid(i) || m_aTankSides[i] != localSide)
				continue;

			IEntity tank = COA_EntityHelper.GetEntityFromRplId(tankId);
			if (!tank || tank == m_LocalTank || IsWrecked(tank) || !HasBrokenDrive(tank))
				continue;

			float distance = vector.DistanceSq(origin, tank.GetOrigin());
			if (distance < best)
			{
				best = distance;
				found = true;
			}
		}

		if (!found)
			return -1;

		return Math.Sqrt(best);
	}

	float GetRepairRange()
	{
		return m_fRepairRange;
	}

	//! x and z to the metre in one int (z in the high 16 bits): maps up to 65 km
	static int PackPosition(vector position)
	{
		int x = (int)Math.Clamp(Math.Round(position[0]), 0, 65535);
		int z = (int)Math.Clamp(Math.Round(position[2]), 0, 65535);
		return (z << 16) | x;
	}

	static vector UnpackPosition(int packed)
	{
		// Unpacked as ints first: Vector() takes floats and won't take the bitwise expressions directly
		int x = packed & 65535;
		int z = (packed >> 16) & 65535;
		float fx = x;
		float fz = z;
		return Vector(fx, 0, fz);
	}

	protected Vehicle GetStateTank(notnull CRF_TankBattlePlayer state)
	{
		if (state.m_Tank && !state.m_Tank.IsDeleted())
			return state.m_Tank;

		if (!state.m_TankId.IsValid())
			return null;

		state.m_Tank = Vehicle.Cast(COA_EntityHelper.GetEntityFromRplId(state.m_TankId));
		return state.m_Tank;
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		CRF_TankBattlePlayer state = m_mPlayers.Get(playerId);
		if (!state)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		ReleaseTank(state, now);
		m_mPlayers.Remove(playerId);
		RebuildTankList();
	}

	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || m_iScorePerKill <= 0 || m_iState != CRF_ETankBattleState.LIVE || m_bEndRequested)
			return;

		int victimSide = GetSideOfFaction(CRF_RadioTransmissionEvents.GetPlayerFactionKey(instigatorContextData.GetVictimPlayerID()));
		int killerSide = GetSideOfFaction(CRF_RadioTransmissionEvents.GetPlayerFactionKey(instigatorContextData.GetKillerPlayerID()));
		if (killerSide < 0 || victimSide < 0 || killerSide == victimSide)
			return;

		AddScore(killerSide, m_iScorePerKill);
		CheckWin();
		m_bDirty = true;
	}

	//===================================================================================
	// SERVER: PICKING A TANK
	//===================================================================================

	//! Server, from the player's tank menu (COA_PlayerRplToAuthorityManager.RequestTankBattleTank)
	void RequestTank(int playerId, int option)
	{
		if (!Replication.IsServer() || m_iState == CRF_ETankBattleState.ENDED || !m_Gamemode || m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		int side = GetSideOfFaction(CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId));
		CRF_TankBattleTank tankOption = GetTankOption(side, option);
		if (!tankOption || !tankOption.IsAvailable())
			return;	// its mod isn't loaded (the menu doesn't offer it)

		CRF_TankBattlePlayer state = GetPlayerState(playerId);
		int now = GetGame().GetWorld().GetWorldTime();
		if (now - state.m_iRequestedAtMs < REQUEST_COOLDOWN_MS)
			return;

		state.m_iRequestedAtMs = now;
		state.m_iSide = side;

		ChimeraCharacter character = ChimeraCharacter.Cast(CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId));
		if (!character || COA_EntityHelper.IsSpectator(character) || character.IsInVehicle())
			return;

		Vehicle current = GetStateTank(state);
		if (current && !IsWrecked(current))
			return;	// already has one

		if (current || state.m_TankId.IsValid())
			ReleaseTank(state, now);	// the old wreck is cleared away as usual

		if (tankOption.m_iLimit > 0 && CountTanks(side, option) >= tankOption.m_iLimit)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "TANK FULL", string.Format("Your side already has %1 %2. Pick another.", tankOption.m_iLimit, tankOption.m_sName), 4);
			return;
		}

		vector spawnPosition;
		float spawnYaw;
		GetSpawnPosition(side, character.GetOrigin(), spawnPosition, spawnYaw);

		// Kept until the spawn is done: a load result freed mid-spawn crashes
		Resource resource = Resource.Load(tankOption.m_sPrefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("%1 Can't load tank '%2'.", LOG, tankOption.m_sPrefab), LogLevel.ERROR);
			return;
		}

		Vehicle tank = Vehicle.Cast(CRF_SpecialsHelper.SpawnOnGround(tankOption.m_sPrefab, spawnPosition, spawnYaw, 15, 4, 0.5));
		if (!tank)
		{
			Print(string.Format("%1 '%2' didn't spawn as a vehicle.", LOG, tankOption.m_sPrefab), LogLevel.ERROR);
			return;
		}

		RplComponent tankRpl = RplComponent.Cast(tank.FindComponent(RplComponent));
		if (!tankRpl)
		{
			Print(string.Format("%1 '%2' has no RplComponent.", LOG, tankOption.m_sPrefab), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(tank);
			return;
		}

		state.m_TankId = tankRpl.Id();
		state.m_Tank = tank;
		state.m_iOption = option;
		state.m_bWasSeated = false;
		state.m_iSeatTries = 0;
		RebuildTankList();

		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, tankOption.m_sName, "W/A/S/D drive. Mouse aims, left click fires, right click for the sight.", 8);
	}

	//! Where side's next tank spawns: a free tank spawn, else the least busy, else near the player
	protected void GetSpawnPosition(int side, vector near, out vector position, out float yaw)
	{
		position = near;
		yaw = 0;

		CRF_TankBattleSide sideData = m_aSides[side];
		if (!sideData.m_aSpawnEntities || sideData.m_aSpawnEntities.IsEmpty())
			return;

		IEntity best;
		int bestCount = int.MAX;
		foreach (string entityName : sideData.m_aSpawnEntities)
		{
			IEntity spawn = CRF_SpecialsHelper.FindNamedEntity(entityName, LOG);
			if (!spawn)
				continue;

			int count = CountTanksNear(spawn.GetOrigin(), SPAWN_CLEAR_RADIUS);
			if (count < bestCount)
			{
				best = spawn;
				bestCount = count;
			}

			if (count == 0)
				break;
		}

		if (!best)
			return;

		position = best.GetOrigin();
		vector angles = best.GetAngles();
		yaw = angles[1];
	}

	protected int CountTanksNear(vector position, float radius)
	{
		int count;
		foreach (int statePlayerId, CRF_TankBattlePlayer state : m_mPlayers)
		{
			Vehicle tank = GetStateTank(state);
			if (tank && vector.DistanceXZ(tank.GetOrigin(), position) <= radius)
				count++;
		}

		return count;
	}

	//===================================================================================
	// SERVER: POINTS AND SCORE
	//===================================================================================

	protected void UpdatePoints()
	{
		if (!m_aPoints || !FindPoints())
			return;

		foreach (int i, CRF_TankBattlePoint point : m_aPoints)
		{
			IEntity pointEntity = m_aPointEntities[i];
			if (!pointEntity)
				continue;

			int count0;
			int count1;
			CountTanksInPoint(pointEntity.GetOrigin(), point.m_fRadius, count0, count1);

			int capturing = -1;
			float progress = m_aProgress[i];
			if (count0 > 0 && count1 > 0)
				capturing = CONTESTED;
			else if (count0 > 0)
			{
				capturing = 0;
				progress += CaptureRate(count0) * TICK_SECONDS;
			}
			else if (count1 > 0)
			{
				capturing = 1;
				progress -= CaptureRate(count1) * TICK_SECONDS;
			}

			progress = Math.Clamp(progress, -1, 1);
			m_aProgress[i] = progress;

			int owner = m_aPointOwner[i];
			int newOwner = owner;
			if (progress >= 1)
				newOwner = 0;
			else if (progress <= -1)
				newOwner = 1;
			else if ((owner == 0 && progress <= 0) || (owner == 1 && progress >= 0))
				newOwner = -1;	// neutralised

			if (newOwner != owner)
				AnnouncePoint(point, owner, newOwner);

			int shownProgress = (int)Math.Round(progress * 100);
			if (newOwner != owner || shownProgress != m_aPointProgress[i] || capturing != m_aPointCapturing[i])
			{
				m_aPointOwner[i] = newOwner;
				m_aPointProgress[i] = shownProgress;
				m_aPointCapturing[i] = capturing;
				m_bDirty = true;
			}
		}
	}

	//! Points per second: 1/m_fCaptureSeconds for one tank, faster with more
	protected float CaptureRate(int tanks)
	{
		if (m_fCaptureSeconds <= 0)
			return 1;

		float multiplier = Math.Min(1 + (tanks - 1) * m_fExtraTankCaptureSpeed, m_fMaxCaptureMultiplier);
		return multiplier / m_fCaptureSeconds;
	}

	//! Seated players' living tanks in the point, per side
	protected void CountTanksInPoint(vector center, float radius, out int count0, out int count1)
	{
		count0 = 0;
		count1 = 0;
		foreach (int statePlayerId, CRF_TankBattlePlayer state : m_mPlayers)
		{
			if (!state.m_bWasSeated)
				continue;

			Vehicle tank = GetStateTank(state);
			if (!tank || IsWrecked(tank) || vector.DistanceXZ(tank.GetOrigin(), center) > radius)
				continue;

			if (state.m_iSide == 0)
				count0++;
			else if (state.m_iSide == 1)
				count1++;
		}
	}

	protected void AnnouncePoint(notnull CRF_TankBattlePoint point, int oldOwner, int newOwner)
	{
		if (newOwner >= 0)
		{
			CRF_RadioTransmissionEvents.PopupToFaction(m_aSides[newOwner].m_sFactionKey, string.Format("%1 CAPTURED", point.m_sName), "", 4);
			CRF_RadioTransmissionEvents.PopupToFaction(m_aSides[1 - newOwner].m_sFactionKey, string.Format("%1 LOST", point.m_sName), "The enemy holds it.", 4);
			return;
		}

		if (oldOwner >= 0)
		{
			CRF_RadioTransmissionEvents.PopupToFaction(m_aSides[oldOwner].m_sFactionKey, string.Format("%1 LOST", point.m_sName), "It's neutral: take it back.", 4);
			CRF_RadioTransmissionEvents.PopupToFaction(m_aSides[1 - oldOwner].m_sFactionKey, string.Format("%1 NEUTRALISED", point.m_sName), "Stay on it to capture it.", 4);
		}
	}

	protected void UpdateScore()
	{
		if (SideCount() != 2)
			return;

		int pointCount = m_aPointOwner.Count();
		for (int side = 0; side < 2; side++)
		{
			int held;
			foreach (int owner : m_aPointOwner)
			{
				if (owner == side)
					held++;
			}

			if (held == 0)
				continue;

			float rate = m_fScorePerPoint * held;
			if (held == pointCount)
				rate += m_fAllPointsBonus;

			AddScore(side, rate * TICK_SECONDS);
		}

		CheckWin();
	}

	protected void AddScore(int side, float amount)
	{
		if (side == 0)
			m_fScore0 = Math.Min(m_fScore0 + amount, m_iScoreLimit);
		else
			m_fScore1 = Math.Min(m_fScore1 + amount, m_iScoreLimit);

		int score0 = m_fScore0;
		int score1 = m_fScore1;
		if (score0 != m_iScore0 || score1 != m_iScore1)
		{
			m_iScore0 = score0;
			m_iScore1 = score1;
			m_bDirty = true;
		}
	}

	protected void CheckWin()
	{
		if (m_bEndRequested)
			return;

		bool full0 = m_fScore0 >= m_iScoreLimit;
		bool full1 = m_fScore1 >= m_iScoreLimit;
		if (!full0 && !full1)
			return;

		int winner = 0;
		if (full1 && (!full0 || m_fScore1 > m_fScore0))
			winner = 1;

		EndRound(winner);
	}

	protected void EndRound(int winnerSide)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		SetEnded(winnerSide);

		string winnerFaction = m_aSides[winnerSide].m_sFactionKey;
		int winnerScore = m_iScore0;
		int loserScore = m_iScore1;
		if (winnerSide == 1)
		{
			winnerScore = m_iScore1;
			loserScore = m_iScore0;
		}

		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("%1 WINS", CRF_SpecialsHelper.GetFactionName(winnerFaction)), string.Format("Their bar filled first: %1 to %2.", winnerScore, loserScore), 10);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winnerFaction, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	protected void SetEnded(int winnerSide)
	{
		m_iState = CRF_ETankBattleState.ENDED;
		m_iWinnerSide = winnerSide;
		m_bDirty = false;
		Replication.BumpMe();
	}

	//===================================================================================
	// DRIVING
	//===================================================================================

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		int now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_iTurretCheckAtMs)
		{
			m_iTurretCheckAtMs = now + TURRET_CHECK_MS;
			AttachMainTurrets();
		}

		if (!System.IsConsoleApp())
		{
			UpdateLocalTank(now);
			if (m_Hud)
				m_Hud.UpdateFrame(this);
		}
	}

	//! Every machine: each mode tank's main gun attached to its driver's seat (the tank list is replicated,
	//! so clients do it as their copies of the tanks stream in)
	protected void AttachMainTurrets()
	{
		// Deleted tanks leave nulls behind in the once-per-tank logs: a 40-minute round goes through hundreds
		PruneNulls(m_aAttachLogged);
		PruneNulls(m_aNoTurretWarned);

		foreach (RplId tankId : m_aTankIds)
		{
			Vehicle tank = Vehicle.Cast(COA_EntityHelper.GetEntityFromRplId(tankId));
			if (tank && !IsWrecked(tank))
				AttachMainTurret(tank);
		}
	}

	protected static void PruneNulls(notnull array<IEntity> entities)
	{
		for (int i = entities.Count() - 1; i >= 0; i--)
		{
			if (!entities[i])
				entities.Remove(i);
		}
	}

	//! Client, every frame: camera held in third person, and on the machine that drives the tank (the
	//! driver's own), the hold before the round goes live and the engine started once it is
	protected void UpdateLocalTank(int now)
	{
		m_LocalTank = FindLocalTank();
		if (!m_LocalTank)
		{
			m_bLocalHeld = false;
			m_bRecoverSeated = false;
			RestoreMouseSteering();
			return;
		}

		// The mouse is the gun's or nobody's, never the hull's
		BaseCompartmentSlot driverSeat = GetDriverSeat(m_LocalTank);
		bool gunConnected = driverSeat && driverSeat.GetAttachedTurret();
		HoldMouseSteeringOff(gunConnected);
		if (!gunConnected && driverSeat)
			RecoverLocalTurret(driverSeat, now);

		if (m_bForceThirdPerson)
			HoldThirdPerson();

		VehicleControllerComponent controller = m_LocalTank.GetVehicleController();
		if (!controller)
			return;

		CarControllerComponent carController = CarControllerComponent.Cast(controller);
		bool hold = m_iState == CRF_ETankBattleState.ENDED || (m_bHoldUntilLive && m_iState != CRF_ETankBattleState.LIVE);
		if (hold)
		{
			if (controller.IsEngineOn())
				controller.StopEngine(false);

			if (carController && !carController.GetPersistentHandBrake())
				carController.SetPersistentHandBrake(true);

			m_bLocalHeld = true;
			return;
		}

		if (m_bLocalHeld)
		{
			m_bLocalHeld = false;
			if (carController)
				carController.SetPersistentHandBrake(false);
		}

		// Vanilla starts the engine from the driver's seat (SCR_EngineAction): same here, so nobody has to
		if (!controller.IsEngineOn() && now >= m_iEngineStartAtMs)
		{
			m_iEngineStartAtMs = now + ENGINE_RETRY_MS;
			controller.StartEngine();
		}
	}

	//! The turret contexts the driver gets: aim, sight, fire, weapon aim (not TURRET_CONTEXT, which would
	//! put W/A/S/D on the traverse) - as the Tank Battle prefabs' AutoConnectContextIndices 0x1e
	static ETurretContextID GetAttachContexts()
	{
		return (1 << ETurretContextID.TURRET_MOVEMENTCONTEXT) | (1 << ETurretContextID.TURRET_OPSCONTEXT) | (1 << ETurretContextID.TURRET_WEAPONCONTEXT) | (1 << ETurretContextID.TURRET_WEAPONAIMCONTEXT);
	}

	//! Client, on the driver's own machine: the gun isn't connected to the seat. The native connection
	//! (AutoConnectToTurret) is made as the driver gets in and is the only one wired to their input, so if
	//! it didn't happen - the turret is its own replicated entity and can stream in after the hull the
	//! driver was seated in - the driver is put out once the turret is here, and the server seats them
	//! again on its next tick (ManageTank), which connects it. Checked every RECOVER_CHECK_MS, a few tries
	//! per tank.
	protected void RecoverLocalTurret(notnull BaseCompartmentSlot driverSeat, int now)
	{
		if (now < m_iRecoverCheckAtMs)
			return;

		m_iRecoverCheckAtMs = now + RECOVER_CHECK_MS;

		if (!m_bRecoverSeated)
		{
			m_bRecoverSeated = true;
			m_iRecoverSeatedAtMs = now;
			if (m_LocalTank != m_RecoverTank)
			{
				m_RecoverTank = m_LocalTank;
				m_iRecoverTries = 0;
			}

			return;
		}

		string turretName = driverSeat.GetAutoConnectTurret();
		if (turretName.IsEmpty() || m_iRecoverTries >= RECOVER_MAX_TRIES || now - m_iRecoverSeatedAtMs < RECOVER_WAIT_MS)
			return;

		TurretControllerComponent turret = FindTurretByName(m_LocalTank, turretName);
		if (!turret || !turret.GetTurretComponent())
			return;		// the turret hasn't streamed in yet: keep waiting

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return;

		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access || access.IsGettingIn() || access.IsGettingOut())
			return;

		m_iRecoverTries++;
		m_bRecoverSeated = false;
		Print(string.Format("%1 %2: the main gun isn't connected to the local driver's seat; re-seating them (try %3 of %4).", LOG, m_LocalTank, m_iRecoverTries, RECOVER_MAX_TRIES), LogLevel.WARNING);
		access.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false);
	}

	//! Client, every frame in a mode vehicle: the mouse aims the gun or does nothing; W/A/S/D drive. What the
	//! mouse does in a driver's seat is decided by two global player options (not per seat):
	//! - "mouse steering in vehicles" (SetMouseControlVehicle). OFF is vanilla's default: the engine forces
	//!   mouse free look while driving, so the mouse looks around and can't steer - but the free look
	//!   context also takes the mouse before the turret's does, so a connected gun couldn't be aimed. ON
	//!   lets the turret have it, and the hull if there is no turret taking it.
	//!   So it is ON only while the gun is connected to the seat, and OFF otherwise: with no gun the mouse
	//!   free-looks instead of steering.
	//! - its sensitivity, held at 0 as well.
	//! Re-applied if a settings change puts them back (two engine calls, no loop); the player's own values
	//! are put back on leaving.
	//! \param[in] gunConnected the main gun is connected to the local driver's seat
	protected void HoldMouseSteeringOff(bool gunConnected)
	{
		if (!m_bMouseSteeringHeld)
		{
			m_bMouseSteeringHeld = true;
			m_bSavedMouseControlVehicle = CharacterControllerComponent.GetMouseControlVehicle();
			m_fSavedMouseSteeringSensitivity = CharacterControllerComponent.GetMouseSteeringSensitivity();
		}

		if (CharacterControllerComponent.GetMouseControlVehicle() != gunConnected)
			CharacterControllerComponent.SetMouseControlVehicle(gunConnected);

		if (CharacterControllerComponent.GetMouseSteeringSensitivity() != 0)
			CharacterControllerComponent.SetMouseSteeringSensitivity(0);
	}

	protected void RestoreMouseSteering()
	{
		if (!m_bMouseSteeringHeld)
			return;

		m_bMouseSteeringHeld = false;
		CharacterControllerComponent.SetMouseControlVehicle(m_bSavedMouseControlVehicle);
		CharacterControllerComponent.SetMouseSteeringSensitivity(m_fSavedMouseSteeringSensitivity);
	}

	protected void HoldThirdPerson()
	{
		IEntity local = SCR_PlayerController.GetLocalControlledEntity();
		if (!local)
			return;

		SCR_CharacterCameraHandlerComponent cameraHandler = SCR_CharacterCameraHandlerComponent.Cast(local.FindComponent(SCR_CharacterCameraHandlerComponent));
		if (cameraHandler && !cameraHandler.IsInThirdPerson())
			cameraHandler.SetThirdPerson(true);
	}

	//! The local player's own tank, if they are sitting in it
	protected Vehicle FindLocalTank()
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character || !character.IsInVehicle())
			return null;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.IsDead())
			return null;

		Vehicle tank = Vehicle.Cast(character.GetRootParent());
		if (!tank || GetTankPlayer(tank) != SCR_PlayerController.GetLocalPlayerId() || IsWrecked(tank))
			return null;

		return tank;
	}

	//===================================================================================
	// CLIENT: MENU, HUD AND MAP
	//===================================================================================

	protected void UpdateUi()
	{
		UpdateMenu();

		if (m_Hud)
			m_Hud.Update(this);
	}

	//! Open while the local player is alive on foot without a tank
	protected void UpdateMenu()
	{
		int side = GetLocalSide();
		bool want = side >= 0 && m_iState != CRF_ETankBattleState.ENDED && GetTankIndexOfPlayer(SCR_PlayerController.GetLocalPlayerId()) < 0;

		if (want)
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
			want = character && !COA_EntityHelper.IsSpectator(character) && !character.IsInVehicle();
			if (want)
			{
				CharacterControllerComponent controller = character.GetCharacterController();
				want = controller && !controller.IsDead();
			}
		}

		if (!want)
		{
			CRF_TankBattleMenu.CloseMenu();
			return;
		}

		if (CRF_TankBattleMenu.IsOpen())
		{
			CRF_TankBattleMenu.Refresh(this, side);
			return;
		}

		int now = GetGame().GetWorld().GetWorldTime();
		if (now < m_iMenuSnoozeUntilMs)
			return;

		// Only over the game, not over the map, the pause menu or the lobby
		if (GetGame().GetMenuManager().GetTopMenu())
			return;

		CRF_TankBattleMenu.Open(this, side);
	}

	//! Client: the menu was closed with Esc - it comes back after a few seconds
	void SnoozeMenu()
	{
		m_iMenuSnoozeUntilMs = GetGame().GetWorld().GetWorldTime() + MENU_SNOOZE_MS;
	}

	protected void UpdateMarkers()
	{
		if (!m_Markers || !m_aPoints || !FindPoints())
			return;

		int localSide = GetLocalSide();

		m_Markers.Begin();
		foreach (int i, CRF_TankBattlePoint point : m_aPoints)
		{
			IEntity pointEntity = m_aPointEntities[i];
			if (!pointEntity)
				continue;

			vector position = pointEntity.GetOrigin();
			m_Markers.Want(Vector((int)Math.Round(position[0]), 0, (int)Math.Round(position[2])), point.m_sName, m_sPointMarkerIcon, GetOwnerColor(GetPointOwner(i), localSide));
		}

		m_Markers.Apply();
	}

	//! Blue for the local side, red for the enemy, white for nobody (side 0 is blue for spectators)
	static int GetOwnerColor(int ownerSide, int localSide)
	{
		if (ownerSide < 0)
			return ARGB(255, 230, 230, 230);

		if (localSide < 0)
			localSide = 0;

		if (ownerSide == localSide)
			return ARGB(255, 60, 140, 255);

		return ARGB(255, 225, 45, 35);
	}

	//===================================================================================
	// EVERY MACHINE
	//===================================================================================

	//! Finds the points' entities by name. False until all of them are found.
	protected bool FindPoints()
	{
		if (m_bPointsFound)
			return true;

		BaseWorld world = GetGame().GetWorld();
		if (!world || !m_aPoints)
			return false;

		m_aPointEntities.Clear();
		bool all = true;
		foreach (CRF_TankBattlePoint point : m_aPoints)
		{
			IEntity pointEntity = world.FindEntityByName(point.m_sEntityName);
			if (!pointEntity)
				all = false;

			m_aPointEntities.Insert(pointEntity);
		}

		if (!all)
			Print(string.Format("%1 Not every point's entity is in the world (%2).", LOG, PointNames()), LogLevel.WARNING);

		// Missing ones stay missing: don't search again every tick
		m_bPointsFound = true;
		return true;
	}

	protected string PointNames()
	{
		string names;
		if (!m_aPoints)
			return names;

		foreach (int i, CRF_TankBattlePoint point : m_aPoints)
		{
			if (i > 0 && i == m_aPoints.Count() - 1)
				names += " and ";
			else if (i > 0)
				names += ", ";

			names += point.m_sName;
		}

		return names;
	}

	protected bool IsWrecked(notnull IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		return damageManager != null && damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected bool HasLivingOccupant(notnull IEntity vehicle)
	{
		SCR_BaseCompartmentManagerComponent compartmentManager = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartmentManager)
			return false;

		array<IEntity> occupants = {};
		compartmentManager.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			if (occupant && !IsWrecked(occupant))
				return true;
		}

		return false;
	}

	//! -1 if factionKey isn't one of the two sides
	int GetSideOfFaction(string factionKey)
	{
		if (!m_aSides || factionKey.IsEmpty())
			return -1;

		foreach (int i, CRF_TankBattleSide side : m_aSides)
		{
			if (side.m_sFactionKey == factionKey)
				return i;
		}

		return -1;
	}

	//! Client: the local player's side, -1 if they aren't on one
	int GetLocalSide()
	{
		return GetSideOfFaction(CRF_SpecialsHelper.GetLocalFactionKey());
	}

	int SideCount()
	{
		if (!m_aSides)
			return 0;

		return m_aSides.Count();
	}

	CRF_TankBattleSide GetSide(int side)
	{
		if (!m_aSides || !m_aSides.IsIndexValid(side))
			return null;

		return m_aSides[side];
	}

	CRF_TankBattleTank GetTankOption(int side, int option)
	{
		CRF_TankBattleSide sideData = GetSide(side);
		if (!sideData || !sideData.m_aTanks || !sideData.m_aTanks.IsIndexValid(option))
			return null;

		return sideData.m_aTanks[option];
	}

	//! How many of a side's tanks are this option (replicated)
	int CountTanks(int side, int option)
	{
		int count;
		foreach (int i, int tankSide : m_aTankSides)
		{
			if (tankSide == side && m_aTankOptions.IsIndexValid(i) && m_aTankOptions[i] == option)
				count++;
		}

		return count;
	}

	//! Index in the replicated tank list, -1 if the player has no tank
	protected int GetTankIndexOfPlayer(int playerId)
	{
		return m_aTankPlayers.Find(playerId);
	}

	//! The player a mode tank belongs to (replicated), -1 if it isn't one
	int GetTankPlayer(notnull IEntity vehicle)
	{
		RplComponent rpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!rpl)
			return -1;

		int index = m_aTankIds.Find(rpl.Id());
		if (index < 0 || !m_aTankPlayers.IsIndexValid(index))
			return -1;

		return m_aTankPlayers[index];
	}

	//! A character sitting in a mode tank that is still in one piece
	bool IsInModeTank(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character || !character.IsInVehicle())
			return false;

		IEntity vehicle = character.GetRootParent();
		if (!vehicle || vehicle == character)
			return false;

		return GetTankPlayer(vehicle) >= 0 && !IsWrecked(vehicle);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla overrides: is this character kept in their tank (no getting out, no changing seats)
	static bool IsLockedInTank(IEntity character)
	{
		return s_Instance && s_Instance.m_bLockCrewIn && s_Instance.IsInModeTank(character);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla overrides: the camera switch key does nothing for this character
	static bool IsThirdPersonLocked(IEntity character)
	{
		return s_Instance && s_Instance.m_bForceThirdPerson && s_Instance.m_LocalTank && character == SCR_PlayerController.GetLocalControlledEntity();
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla overrides: this character sits in a mode tank (the camera treats its driver as the gunner)
	static bool IsInTankBattleTank(IEntity character)
	{
		return s_Instance && s_Instance.IsInModeTank(character);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla overrides: W/A/S/D drive this turret's tank rather than turn the turret
	static bool IsLocalTankTurret(notnull TurretControllerComponent turret)
	{
		return s_Instance && s_Instance.m_LocalTank && turret.GetOwner().GetRootParent() == s_Instance.m_LocalTank;
	}

	//------------------------------------------------------------------------------------------------
	CRF_ETankBattleState GetBattleState()
	{
		return m_iState;
	}

	int GetScore(int side)
	{
		if (side == 0)
			return m_iScore0;

		return m_iScore1;
	}

	int GetScoreLimit()
	{
		return m_iScoreLimit;
	}

	int GetWinnerSide()
	{
		return m_iWinnerSide;
	}

	int GetPointCount()
	{
		return m_aPointOwner.Count();
	}

	string GetPointName(int index)
	{
		if (!m_aPoints || !m_aPoints.IsIndexValid(index))
			return "?";

		return m_aPoints[index].m_sName;
	}

	int GetPointOwner(int index)
	{
		if (!m_aPointOwner.IsIndexValid(index))
			return -1;

		return m_aPointOwner[index];
	}

	//! -100 (side 1) to 100 (side 0)
	int GetPointProgress(int index)
	{
		if (!m_aPointProgress.IsIndexValid(index))
			return 0;

		return m_aPointProgress[index];
	}

	//! -1 nobody, a side index, or CONTESTED
	int GetPointCapturing(int index)
	{
		if (!m_aPointCapturing.IsIndexValid(index))
			return -1;

		return m_aPointCapturing[index];
	}

	//! The point position (here) is within, -1 if none
	int GetPointAt(vector position)
	{
		if (!m_aPoints || !FindPoints())
			return -1;

		foreach (int i, CRF_TankBattlePoint point : m_aPoints)
		{
			IEntity pointEntity = m_aPointEntities[i];
			if (pointEntity && vector.DistanceXZ(pointEntity.GetOrigin(), position) <= point.m_fRadius)
				return i;
		}

		return -1;
	}

	//! Client: the local player's tank while they sit in it
	Vehicle GetLocalTank()
	{
		return m_LocalTank;
	}

	//! Client: the gun the local player controls (attached to their driver's seat), null if none
	TurretControllerComponent GetLocalTurret()
	{
		if (!m_LocalTank)
			return null;

		BaseCompartmentSlot driverSeat = GetDriverSeat(m_LocalTank);
		if (!driverSeat)
			return null;

		return driverSeat.GetAttachedTurret();
	}

	//! Client: where the local gun points (world), zero if there's no gun
	vector GetLocalGunDirection()
	{
		TurretControllerComponent turret = GetLocalTurret();
		if (!turret || !turret.GetTurretComponent())
			return vector.Zero;

		return turret.GetTurretComponent().GetAimingDirectionWorld();
	}

	//! The point's centre entity (null if missing or not found yet)
	IEntity GetPointEntity(int index)
	{
		if (!FindPoints() || !m_aPointEntities.IsIndexValid(index))
			return null;

		return m_aPointEntities[index];
	}

	float GetPointRadius(int index)
	{
		if (!m_aPoints || !m_aPoints.IsIndexValid(index))
			return 0;

		return m_aPoints[index].m_fRadius;
	}

	//! Every mode tank in one piece except skip, with its side. Where this machine has the tank (a
	//! client: streamed in) its live position, else, for its own side's tanks only, the server's from
	//! SendTankPositions (about a second old).
	void GetTankPositions(notnull array<vector> positions, notnull array<int> sides, notnull array<bool> broken, IEntity skip = null)
	{
		BaseWorld world = GetGame().GetWorld();
		bool haveServerPositions = world && world.GetWorldTime() - m_iFriendlyPositionsAtMs < POSITION_STALE_MS;
		int packed;

		foreach (int i, RplId tankId : m_aTankIds)
		{
			if (!m_aTankSides.IsIndexValid(i))
				break;

			IEntity tank = COA_EntityHelper.GetEntityFromRplId(tankId);
			if (tank)
			{
				if (tank == skip || IsWrecked(tank))
					continue;

				positions.Insert(tank.GetOrigin());
				broken.Insert(HasBrokenDrive(tank));
			}
			else if (haveServerPositions && m_mFriendlyTankPositions.Find(tankId, packed))
			{
				positions.Insert(UnpackPosition(packed));
				broken.Insert(false);	// only known for the vehicles this machine has
			}
			else
			{
				continue;
			}

			sides.Insert(m_aTankSides[i]);
		}
	}

	//! Positions of both sides' tank spawns that are in the world
	void GetSpawnPositions(notnull array<vector> positions)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world || !m_aSides)
			return;

		foreach (CRF_TankBattleSide side : m_aSides)
		{
			if (!side.m_aSpawnEntities)
				continue;

			foreach (string entityName : side.m_aSpawnEntities)
			{
				IEntity spawn = world.FindEntityByName(entityName);
				if (spawn)
					positions.Insert(spawn.GetOrigin());
			}
		}
	}

	bool ShowMinimap()
	{
		return m_bShowMinimap;
	}

	bool ShowCrosshair()
	{
		return m_bShowCrosshair;
	}

	bool ShowMinimapSatellite()
	{
		return m_bMinimapSatellite;
	}

	//! The image under the minimap: Minimap Image, else the world map's "Satellite background image"
	//! (vanilla's map entities set it, e.g. MapEntity_Cain.et: CainRasterized.edds). Empty: none.
	ResourceName GetMinimapImage()
	{
		if (!m_bMinimapSatellite)
			return ResourceName.Empty;

		if (!m_sMinimapImage.IsEmpty())
			return m_sMinimapImage;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.GetPrefabData())
			return ResourceName.Empty;

		BaseContainer prefab = mapEntity.GetPrefabData().GetPrefab();
		if (!prefab)
			return ResourceName.Empty;

		ResourceName image;
		prefab.Get("Satellite background image", image);
		return image;
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla overrides (CharacterCamera3rdPersonTurret): third person distance multiplier for this
	//! character's camera, 1 outside a mode tank
	static float GetCameraDistanceScale(IEntity character)
	{
		if (!s_Instance || !s_Instance.IsInModeTank(character))
			return 1;

		return s_Instance.m_fCameraDistanceScale;
	}

	//! Vanilla overrides: degrees added to this character's third person field of view, 0 outside a mode tank
	static float GetCameraExtraFov(IEntity character)
	{
		if (!s_Instance || !s_Instance.IsInModeTank(character))
			return 0;

		return s_Instance.m_fCameraExtraFov;
	}

	//! Vanilla overrides: this character's third person camera looks at the gun's aim point
	static bool IsCameraLookAtAim(IEntity character)
	{
		return s_Instance && s_Instance.m_bCameraLookAtAim && s_Instance.IsInModeTank(character);
	}

	//! Vanilla overrides: metres this character's third person camera is raised, 0 outside a mode tank
	static float GetCameraExtraHeight(IEntity character)
	{
		if (!s_Instance || !s_Instance.IsInModeTank(character))
			return 0;

		return s_Instance.m_fCameraExtraHeight;
	}

	//! Vanilla overrides: the local player's gun ray, for a camera that looks at the aim point. False
	//! (camera stays vanilla) for anyone else, outside a mode tank, or with Camera Look At Aim off.
	static bool GetCameraAimRay(IEntity character, out vector origin, out vector direction, out float distance)
	{
		if (!s_Instance || !s_Instance.m_bCameraLookAtAim || !character || character != SCR_PlayerController.GetLocalControlledEntity())
			return false;

		if (!s_Instance.IsInModeTank(character))
			return false;

		return s_Instance.GetLocalGunRay(origin, direction, distance);
	}

	//! Client: the local gun's ray - from the muzzle (or the turret) along the gun, to what it would hit
	//! (or CROSSHAIR_RANGE). Traced once a frame; the camera and the crosshair share it.
	bool GetLocalGunRay(out vector origin, out vector direction, out float distance)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		int now = world.GetWorldTime();
		if (m_bGunRayCached && now == m_iGunRayAtMs)
		{
			origin = m_vGunRayOrigin;
			direction = m_vGunRayDirection;
			distance = m_fGunRayDistance;
			return m_bGunRayValid;
		}

		m_bGunRayCached = true;
		m_iGunRayAtMs = now;
		m_bGunRayValid = TraceLocalGun(world);

		origin = m_vGunRayOrigin;
		direction = m_vGunRayDirection;
		distance = m_fGunRayDistance;
		return m_bGunRayValid;
	}

	protected bool TraceLocalGun(notnull BaseWorld world)
	{
		Vehicle tank = GetLocalTank();
		TurretControllerComponent turret = GetLocalTurret();
		if (!tank || !turret || !turret.GetTurretComponent())
			return false;

		vector direction = turret.GetTurretComponent().GetAimingDirectionWorld();
		if (direction == vector.Zero)
			return false;

		// From the muzzle if the gun has one, else from the turret
		vector origin = turret.GetOwner().GetOrigin() + Vector(0, 1, 0);
		BaseWeaponManagerComponent weaponManager = turret.GetWeaponManager();
		if (weaponManager)
		{
			vector muzzle[4];
			if (weaponManager.GetCurrentMuzzleTransform(muzzle))
				origin = muzzle[3];
		}

		if (!m_GunTrace)
			m_GunTrace = new TraceParam();

		m_GunTrace.Start = origin + direction * GUN_TRACE_START;
		m_GunTrace.End = origin + direction * GUN_TRACE_RANGE;
		m_GunTrace.Flags = TraceFlags.ENTS | TraceFlags.WORLD | TraceFlags.OCEAN;
		m_GunTrace.Exclude = tank;
		float fraction = world.TraceMove(m_GunTrace, null);

		m_vGunRayOrigin = origin;
		m_vGunRayDirection = direction;
		m_fGunRayDistance = GUN_TRACE_START + (GUN_TRACE_RANGE - GUN_TRACE_START) * fraction;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (SideCount() != 2)
			return;

		string name0 = CRF_SpecialsHelper.GetFactionName(m_aSides[0].m_sFactionKey);
		string name1 = CRF_SpecialsHelper.GetFactionName(m_aSides[1].m_sFactionKey);

		if (m_iState == CRF_ETankBattleState.WAITING)
		{
			lines.Insert(string.Format("Tank Battle: first to %1 wins. Points start when the round goes live.", m_iScoreLimit));
			return;
		}

		lines.Insert(string.Format("Tank Battle: %1 %2, %3 %4 (first to %5)", name0, m_iScore0, name1, m_iScore1, m_iScoreLimit));
		if (m_iState == CRF_ETankBattleState.ENDED && m_iWinnerSide >= 0)
			lines.Insert(string.Format("Tank Battle: %1 won", CRF_SpecialsHelper.GetFactionName(m_aSides[m_iWinnerSide].m_sFactionKey)));
	}
}
