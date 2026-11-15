//------------------------------------------------------------------------------------
// CRF_PayloadGamemodeComponent: attackers escort a truck along a route, defenders stop it
//
// The truck moves by itself while attackers are close to it and no defender is. More attackers push it
// faster. Left alone it stops, then rolls back, though never past the last checkpoint reached. Each
// checkpoint adds time to the clock and can move both sides' respawn flags forward, with their vehicle
// spawners. Both sides come back, with their vehicles, at each checkpoint and whenever either side is
// wiped out. Attackers win when the truck reaches the end of the route. Defenders win when the clock
// runs out, unless attackers are still with the truck: then it's overtime until it arrives or they
// leave it.
//
// The truck is a prop, not a vehicle. The server decides how far along the route it is and replicates
// that one distance; every machine places the prop there itself, on the terrain. So nobody can drive
// it, nothing can destroy it, and it can't get stuck on a fence and end the round early. The route has
// to stay on terrain: the truck follows the ground, so it would go under a bridge.
//
// Setup for mission makers (full guide: !Docs/MissionMaking/Gamemodes/CRF_PAYLOAD_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the gamemode entity.
//   2. Place Prefabs/Props/Payload/CRF_PayloadTruck.et (or any static prop) and name it "payload".
//   3. Draw a PolylineShapeEntity along the road, start to finish, and name it "payload_route".
//   4. Place a named entity at each checkpoint and list them in m_aCheckpoints, in route order.
//------------------------------------------------------------------------------------

enum CRF_EPayloadState
{
	WAITING,		//!< the round isn't live yet
	STOPPED,		//!< no attackers with it
	MOVING,
	CONTESTED,		//!< attackers and defenders both with it
	ROLLING_BACK,
	ENDED
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class CRF_PayloadCheckpoint
{
	[Attribute(desc: "What players call it, e.g. \"Alpha\"")]
	string m_sName;

	[Attribute(desc: "Name of an entity placed at the checkpoint. It counts as reached at the nearest point on the route.")]
	string m_sEntityName;

	[Attribute("300", UIWidgets.EditBox, "Seconds added to the clock when the truck gets here", params: "0 3600 1")]
	int m_iBonusSeconds;

	[Attribute(desc: "Name of an entity the attackers' respawn flag moves to when the truck gets here (empty = stays put)")]
	string m_sAttackerSpawnEntity;

	[Attribute(desc: "Name of an entity the defenders' respawn flag moves to when the truck gets here (empty = stays put)")]
	string m_sDefenderSpawnEntity;
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Payload: attackers escort a truck along a route, defenders stop it.")]
class CRF_PayloadGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_PayloadGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the side escorting the truck", category: "Payload - Factions")]
	protected string m_sAttackerFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the side stopping it", category: "Payload - Factions")]
	protected string m_sDefenderFaction;

	[Attribute("payload", UIWidgets.EditBox, "Name of the prop that moves. Must be a static prop, not a vehicle.", category: "Payload - Truck")]
	protected string m_sPayloadName;

	[Attribute("payload_route", UIWidgets.EditBox, "Name of the PolylineShapeEntity the truck follows, drawn from start to finish", category: "Payload - Truck")]
	protected string m_sRouteName;

	[Attribute("0", UIWidgets.EditBox, "Metres the prop's origin sits above the ground. CRF_PayloadTruck.et needs about 1.1.", params: "-5 10 0.05", category: "Payload - Truck")]
	protected float m_fHeightOffset;

	[Attribute("5", UIWidgets.EditBox, "Metres between the prop's front and rear axles, for how it turns and climbs", params: "1 30 0.5", category: "Payload - Truck")]
	protected float m_fWheelbase;

	[Attribute("30", UIWidgets.EditBox, "Metres from the truck a player counts as being with it", params: "5 200 1", category: "Payload - Movement")]
	protected float m_fEscortRadius;

	[Attribute("1", UIWidgets.EditBox, "Attackers needed to move it", params: "1 50 1", category: "Payload - Movement")]
	protected int m_iMinAttackers;

	[Attribute("1", UIWidgets.EditBox, "Defenders needed to stop it", params: "1 50 1", category: "Payload - Movement")]
	protected int m_iMinDefenders;

	[Attribute("2.5", UIWidgets.EditBox, "Metres per second with the fewest attackers", params: "0.5 15 0.1", category: "Payload - Movement")]
	protected float m_fBaseSpeed;

	[Attribute("0.25", UIWidgets.EditBox, "Metres per second added by each extra attacker", params: "0 5 0.05", category: "Payload - Movement")]
	protected float m_fSpeedPerAttacker;

	[Attribute("4", UIWidgets.EditBox, "Fastest it goes, in metres per second", params: "0.5 15 0.1", category: "Payload - Movement")]
	protected float m_fMaxSpeed;

	[Attribute("30", UIWidgets.EditBox, "Seconds without attackers before it rolls back (0 = it never does)", params: "0 600 1", category: "Payload - Movement")]
	protected float m_fRollbackDelay;

	[Attribute("1", UIWidgets.EditBox, "Metres per second it rolls back", params: "0.1 10 0.1", category: "Payload - Movement")]
	protected float m_fRollbackSpeed;

	[Attribute(desc: "The checkpoints, in route order", category: "Payload - Checkpoints")]
	protected ref array<ref CRF_PayloadCheckpoint> m_aCheckpoints;

	[Attribute("1", UIWidgets.CheckBox, "Bring both sides' dead back when a checkpoint is reached", category: "Payload - Respawns")]
	protected bool m_bRespawnOnCheckpoint;

	[Attribute("1", UIWidgets.CheckBox, "Bring both sides' dead back when either side has nobody left alive", category: "Payload - Respawns")]
	protected bool m_bRespawnOnWipe;

	[Attribute("5", UIWidgets.EditBox, "Seconds after a side is wiped out before both sides come back", params: "0 120 1", category: "Payload - Respawns")]
	protected float m_fWipeRespawnDelay;

	[Attribute("1", UIWidgets.CheckBox, "Each side's vehicle spawners move with its respawn flag, keeping their place around it. Place them around the side's starting flag.", category: "Payload - Respawns")]
	protected bool m_bVehiclesFollowSpawns;

	[Attribute("1", UIWidgets.CheckBox, "When both sides come back, so do their vehicles: wrecks, and vehicles left empty away from their spawn, are replaced at the spawn. Vehicles with someone in them are left alone. Only spawners set to respawn on a side respawn.", category: "Payload - Respawns")]
	protected bool m_bRespawnVehicles;

	[Attribute("1500", UIWidgets.EditBox, "Seconds on the clock when the round starts", params: "60 14400 1", category: "Payload - Ending")]
	protected int m_iStartSeconds;

	[Attribute("10", UIWidgets.EditBox, "In overtime, seconds without attackers before the defenders win", params: "0 120 1", category: "Payload - Ending")]
	protected float m_fOvertimeGrace;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Payload - Ending")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("{9EBE212C91B36BBE}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Truck.edds", UIWidgets.ResourcePickerThumbnail, "Map marker for the truck", "edds", category: "Payload - Map")]
	protected ResourceName m_sTruckMarkerIcon;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker for the checkpoints and the finish", "edds", category: "Payload - Map")]
	protected ResourceName m_sCheckpointMarkerIcon;

	[Attribute("1", UIWidgets.CheckBox, "Show a minimap of the route on the HUD, with how far the truck has come and where the player is (see CRF_PayloadMinimap)", category: "Payload - Map")]
	protected bool m_bShowMinimap;

	// Replicated: all a client needs to place the truck and draw the panel
	[RplProp()]
	protected float m_fDistance;
	[RplProp()]
	protected float m_fSpeed;
	[RplProp()]
	protected int m_iState;
	[RplProp()]
	protected int m_iSecondsLeft;
	[RplProp()]
	protected int m_iCheckpointsReached;
	[RplProp()]
	protected bool m_bOvertime;
	[RplProp()]
	protected bool m_bAttackersWon;

	// Every machine: the route, built from the world
	protected ref array<vector> m_aRoute = {};
	protected ref array<float> m_aRouteDistances = {};
	protected ref array<float> m_aCheckpointDistances = {};
	protected float m_fRouteLength;
	protected IEntity m_Payload;
	protected ref array<IEntity> m_aPayloadParts = {};	//!< the prop and everything in its slots (cargo bed, wheels)
	protected bool m_bRouteReady;
	protected bool m_bWarnedNoRoute;

	// Every machine: where the prop is drawn, which eases towards m_fDistance between updates
	protected float m_fShownDistance;
	protected float m_fAppliedDistance = -1;
	protected ref CRF_SpecialsMarkerSet m_Markers;
	protected ref CRF_PayloadHUD m_Hud;

	// Server
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_Safestart;
	protected bool m_bRoundLive;
	protected bool m_bEndRequested;
	protected float m_fFloorDistance;	//!< it rolls back no further than this
	protected float m_fIdleSeconds;
	protected float m_fOvertimeIdleSeconds;
	protected float m_fClockCarry;
	protected bool m_bRespawnPending;

	protected static const int TICK_MS = 500;
	protected static const float TICK_SECONDS = 0.5;
	protected static const float SNAP_DISTANCE = 30;	//!< further than this from the real position, jump (joining mid-round)
	protected static const float CATCH_UP = 2;
	protected static const float HALF_WIDTH = 1.2;		//!< how far either side of the truck the ground is measured, for its roll
	protected static const float VEHICLE_SEARCH_RADIUS = 30;	//!< how far a moved vehicle spawner looks for open ground
	protected static const float VEHICLE_CLEAR_RADIUS = 5;		//!< CRF_VehicleGearscriptManager won't spawn on a pad with less room than this
	protected static const float VEHICLE_PARKED_RADIUS = 25;	//!< an empty vehicle this close to its spawner is still waiting there
	protected static const float RIDER_TRACE_UP = 0.5;		//!< a rider's feet are looked for from this far above their origin...
	protected static const float RIDER_TRACE_DOWN = 0.75;	//!< ...to this far below it
	protected static const float RIDER_TRACE_RADIUS = 0.25;

	protected static CRF_PayloadGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_PayloadGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_PayloadGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_PayloadGamemodeComponent GetInstance()
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
			m_Hud = new CRF_PayloadHUD();
			GetGame().GetCallqueue().CallLater(UpdateMarkers, 1000, true);
			GetGame().GetCallqueue().CallLater(UpdateHud, 250, true);
		}

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(HookGamemode, 1000, false);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(HookGamemode);
			GetGame().GetCallqueue().Remove(UpdateMarkers);
			GetGame().GetCallqueue().Remove(UpdateHud);
			GetGame().GetCallqueue().Remove(RespawnBothSides);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Markers)
			m_Markers.Clear();

		if (m_Hud)
			m_Hud.Destroy();

		super.OnDelete(owner);
	}

	protected void HookGamemode()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		if (!m_Gamemode)
		{
			Print("[CRF_Payload] No COA_Gamemode: payload is not running.", LogLevel.WARNING);
			return;
		}

		m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		// The clock starts when safestart ends, not when it begins
		m_Safestart = COA_SafestartManager.GetInstance();
		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Insert(OnSafestartChanged);

		m_iSecondsLeft = m_iStartSeconds;
		Replication.BumpMe();

		OnGamemodeStateChanged();
	}

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive && m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode)
			return;

		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
		{
			// Missions with safestart start on OnSafestartChanged
			if (!m_bRoundLive && (!m_Safestart || m_Safestart.IsRoundLive()))
				StartRound();
			return;
		}

		// The mission ended, whoever decided it: the truck stops
		if (m_bRoundLive && m_iState != CRF_EPayloadState.ENDED)
		{
			GetGame().GetCallqueue().Remove(Tick);
			SetEnded(false);
		}
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		if (!m_bRouteReady && !BuildRoute())
		{
			Print("[CRF_Payload] The round went live without a route: the truck won't move.", LogLevel.ERROR);
			return;
		}

		m_bRoundLive = true;
		m_iState = CRF_EPayloadState.STOPPED;
		Replication.BumpMe();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackerFaction, "ESCORT THE TRUCK", "Stay with it and it moves. Get it to the end of the route.", 8);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFaction, "STOP THE TRUCK", "Get to it to stop it. Hold it until the clock runs out.", 8);

		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//===================================================================================
	// SERVER: THE ROUND
	//===================================================================================

	protected void Tick()
	{
		if (m_bEndRequested || !m_Payload)
			return;

		int attackers = CountNearTruck(m_sAttackerFaction);
		int defenders = CountNearTruck(m_sDefenderFaction);
		bool escorted = attackers >= m_iMinAttackers;

		if (escorted)
			m_fIdleSeconds = 0;
		else
			m_fIdleSeconds += TICK_SECONDS;

		if (escorted && defenders >= m_iMinDefenders)
		{
			m_iState = CRF_EPayloadState.CONTESTED;
			m_fSpeed = 0;
		}
		else if (escorted)
		{
			m_iState = CRF_EPayloadState.MOVING;
			m_fSpeed = Math.Min(m_fMaxSpeed, m_fBaseSpeed + (attackers - m_iMinAttackers) * m_fSpeedPerAttacker);
		}
		else if (m_fRollbackDelay > 0 && m_fIdleSeconds >= m_fRollbackDelay && m_fDistance > m_fFloorDistance)
		{
			m_iState = CRF_EPayloadState.ROLLING_BACK;
			m_fSpeed = -m_fRollbackSpeed;
		}
		else
		{
			m_iState = CRF_EPayloadState.STOPPED;
			m_fSpeed = 0;
		}

		m_fDistance = Math.Clamp(m_fDistance + m_fSpeed * TICK_SECONDS, m_fFloorDistance, m_fRouteLength);

		CheckCheckpoints();

		if (m_fDistance >= m_fRouteLength)
		{
			EndRound(true, "THE TRUCK GOT THROUGH", string.Format("%1 reached the end of the route.", CRF_SpecialsHelper.GetFactionName(m_sAttackerFaction)));
			return;
		}

		if (m_bRespawnOnWipe)
			CheckWipe();

		UpdateClock(escorted);
		Replication.BumpMe();
	}

	protected int CountNearTruck(string factionKey)
	{
		array<int> players = {};
		CRF_SpecialsHelper.GetAlivePlayers(factionKey, players);

		vector truckPosition = GetRoutePoint(m_fDistance);
		int count;
		foreach (int playerId : players)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (entity && vector.DistanceXZ(entity.GetOrigin(), truckPosition) <= m_fEscortRadius)
				count++;
		}

		return count;
	}

	protected void CheckCheckpoints()
	{
		while (m_iCheckpointsReached < m_aCheckpointDistances.Count() && m_fDistance >= m_aCheckpointDistances[m_iCheckpointsReached])
		{
			ReachCheckpoint(m_iCheckpointsReached);
			m_iCheckpointsReached++;
		}
	}

	protected void ReachCheckpoint(int index)
	{
		CRF_PayloadCheckpoint checkpoint = m_aCheckpoints[index];

		m_fFloorDistance = m_aCheckpointDistances[index];
		m_iSecondsLeft += checkpoint.m_iBonusSeconds;

		MoveSpawn(m_sAttackerFaction, checkpoint.m_sAttackerSpawnEntity);
		MoveSpawn(m_sDefenderFaction, checkpoint.m_sDefenderSpawnEntity);

		// After the flags have moved, so the dead come back at the new ones. This also takes over a
		// wipe respawn that hasn't happened yet, so the two don't run back to back.
		if (m_bRespawnOnCheckpoint)
			ScheduleRespawn(500);

		string text = "The truck can't roll back past here.";
		if (checkpoint.m_iBonusSeconds > 0)
			text = string.Format("+%1 on the clock. The truck can't roll back past here.", CRF_SpecialsHelper.FormatSeconds(checkpoint.m_iBonusSeconds));

		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("CHECKPOINT %1 REACHED", checkpoint.m_sName), text, 8);
	}

	//! The same way Rush moves its flags
	protected void MoveSpawn(string factionKey, string entityName)
	{
		if (entityName.IsEmpty())
			return;

		IEntity marker = CRF_SpecialsHelper.FindNamedEntity(entityName, "[CRF_Payload]");
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!marker || !respawnManager)
			return;

		array<COA_SpawnPointData> spawns = respawnManager.GetFactionSpawnpoints(factionKey);
		if (!spawns || spawns.IsEmpty())
		{
			Print(string.Format("[CRF_Payload] No spawn points for %1 to move.", factionKey), LogLevel.WARNING);
			return;
		}

		IEntity flag = COA_EntityHelper.GetEntityFromRplId(spawns[0].GetSpawnPointEntity());
		if (!flag)
			return;

		vector oldFlag[4];
		flag.GetWorldTransform(oldFlag);
		vector oldAngles = flag.GetAngles();
		float oldYaw = oldAngles[1];

		CRF_SpecialsHelper.MoveSpawnPoint(flag, marker);

		if (m_bVehiclesFollowSpawns)
			MoveVehicleSpawners(factionKey, oldFlag, oldYaw, flag);
	}

	//! Keeps each spawner where it was around the flag: as far ahead or behind it and to the side,
	//! turned as much as the flag turned. Then onto open ground nearby, since the same spot by the
	//! next flag can be a house or a tree, and a blocked pad never spawns its vehicle.
	protected void MoveVehicleSpawners(string factionKey, vector oldFlag[4], float oldYaw, notnull IEntity flag)
	{
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		BaseWorld world = GetGame().GetWorld();
		if (!respawnManager || !world)
			return;

		array<COA_VehicleSpawner> spawners = respawnManager.GetVehicleSpawners();
		if (!spawners)
			return;

		vector newFlag[4];
		flag.GetWorldTransform(newFlag);
		vector newAngles = flag.GetAngles();
		float turn = newAngles[1] - oldYaw;

		vector oldRight = FlatAxis(oldFlag[0]);
		vector oldForward = FlatAxis(oldFlag[2]);
		vector newRight = FlatAxis(newFlag[0]);
		vector newForward = FlatAxis(newFlag[2]);

		foreach (COA_VehicleSpawner spawner : spawners)
		{
			if (!spawner || spawner.m_sFactionKey != factionKey)
				continue;

			vector offset = spawner.GetOrigin() - oldFlag[3];
			float across = vector.Dot(offset, oldRight);
			float along = vector.Dot(offset, oldForward);
			vector target = newFlag[3] + newRight * across + newForward * along;

			vector position;
			if (!SCR_WorldTools.FindEmptyTerrainPosition(position, target, VEHICLE_SEARCH_RADIUS, VEHICLE_CLEAR_RADIUS, 3))
				Print(string.Format("[CRF_Payload] No open ground for a %1 vehicle near %2: it may not spawn until the spot is clear.", factionKey, target), LogLevel.WARNING);

			position[1] = world.GetSurfaceY(position[0], position[2]);
			spawner.SetOrigin(position);

			vector angles = spawner.GetAngles();
			angles[1] = angles[1] + turn;
			spawner.SetAngles(angles);
		}
	}

	protected vector FlatAxis(vector axis)
	{
		vector flat = Vector(axis[0], 0, axis[2]);
		return flat.Normalized();
	}

	//! Server: a side with players slotted and none of them alive
	protected bool IsWipedOut(string factionKey)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		bool anyPlayers;
		foreach (int playerId : playerIds)
		{
			if (!slottingManager.IsPlayerInASlot(playerId))
				continue;

			Faction faction = slottingManager.GetPlayerSlotFaction(playerId, true);
			if (!faction || faction.GetFactionKey() != factionKey)
				continue;

			if (!slottingManager.IsPlayerConsideredDead(playerId))
				return false;

			anyPlayers = true;
		}

		return anyPlayers;
	}

	protected void CheckWipe()
	{
		if (m_bRespawnPending)
			return;

		string wipedFaction;
		if (IsWipedOut(m_sAttackerFaction))
			wipedFaction = m_sAttackerFaction;
		else if (IsWipedOut(m_sDefenderFaction))
			wipedFaction = m_sDefenderFaction;
		else
			return;

		int delaySeconds = m_fWipeRespawnDelay;
		ScheduleRespawn(delaySeconds * 1000);

		string text = "Both sides are back at their spawns.";
		if (delaySeconds > 0)
			text = string.Format("Both sides come back at their spawns in %1 seconds.", delaySeconds);

		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("%1 WIPED OUT", CRF_SpecialsHelper.GetFactionName(wipedFaction)), text, 8);
	}

	//! One respawn at a time: a later request replaces the one waiting
	protected void ScheduleRespawn(int delayMs)
	{
		m_bRespawnPending = true;
		GetGame().GetCallqueue().Remove(RespawnBothSides);
		GetGame().GetCallqueue().CallLater(RespawnBothSides, delayMs, false);
	}

	protected void RespawnBothSides()
	{
		m_bRespawnPending = false;

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager || m_bEndRequested)
			return;

		// Before the players: RespawnSide charges tickets for every vehicle that needs replacing
		if (m_bRespawnVehicles)
		{
			ReplaceVehicles(m_sAttackerFaction);
			ReplaceVehicles(m_sDefenderFaction);
		}

		respawnManager.RespawnSide(m_sAttackerFaction);
		respawnManager.RespawnSide(m_sDefenderFaction);
	}

	//! RespawnSide only brings back vehicles that are destroyed or gone. This also replaces ones
	//! left empty somewhere else (like back down the road, when the spawn has moved up), so a side
	//! that respawns has its vehicles at the spawn.
	protected void ReplaceVehicles(string factionKey)
	{
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!respawnManager || !gearscriptManager)
			return;

		array<COA_VehicleSpawner> spawners = respawnManager.GetVehicleSpawners();
		if (!spawners)
			return;

		int tickets = respawnManager.GetFactionTickets(factionKey);
		foreach (COA_VehicleSpawner spawner : spawners)
		{
			if (!spawner || spawner.m_sFactionKey != factionKey || !spawner.m_bShouldRespawnOnSideRespawn)
				continue;

			// -1 is unlimited and 0 means tickets aren't used, as in Rush and Cache Hunt. Not
			// CRF_COA_RespawnManager.RespawnSideVehicles' check, which skips every vehicle on -1.
			if (tickets > 0 && tickets < spawner.m_iTicketsPerRespawn)
				continue;

			IEntity vehicle = GetSpawnerVehicle(spawner);
			if (vehicle && !IsWrecked(vehicle))
			{
				if (IsCrewed(vehicle) || vector.DistanceXZ(vehicle.GetOrigin(), spawner.GetOrigin()) <= VEHICLE_PARKED_RADIUS)
					continue;
			}

			// Removed here rather than by the gearscript manager, which waits 15 seconds after
			// removing one before it spawns the next
			if (vehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(vehicle);

			spawner.m_eVehicle = null;
			spawner.m_VehicleRplId = RplId.Invalid();
			gearscriptManager.QueueSpawn(spawner);
		}
	}

	protected IEntity GetSpawnerVehicle(notnull COA_VehicleSpawner spawner)
	{
		if (!spawner.m_VehicleRplId.IsValid())
			return null;

		RplComponent rpl = RplComponent.Cast(Replication.FindItem(spawner.m_VehicleRplId));
		if (!rpl)
			return null;

		IEntity vehicle = rpl.GetEntity();
		if (!vehicle || vehicle.IsDeleted())
			return null;

		return vehicle;
	}

	protected bool IsWrecked(notnull IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		return damageManager != null && damageManager.GetState() == EDamageState.DESTROYED;
	}

	//! Someone alive in a seat
	protected bool IsCrewed(notnull IEntity vehicle)
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

	protected void UpdateClock(bool escorted)
	{
		m_fClockCarry += TICK_SECONDS;
		if (m_fClockCarry >= 1)
		{
			m_fClockCarry -= 1;
			if (m_iSecondsLeft > 0)
				m_iSecondsLeft--;
		}

		// A checkpoint in overtime puts time back on the clock
		if (m_iSecondsLeft > 0)
		{
			m_bOvertime = false;
			return;
		}

		if (!escorted)
		{
			m_fOvertimeIdleSeconds += TICK_SECONDS;
			if (m_fOvertimeIdleSeconds >= m_fOvertimeGrace)
				EndRound(false, "THE TRUCK WAS STOPPED", string.Format("%1 held it until the clock ran out.", CRF_SpecialsHelper.GetFactionName(m_sDefenderFaction)));
			return;
		}

		m_fOvertimeIdleSeconds = 0;
		if (m_bOvertime)
			return;

		m_bOvertime = true;
		CRF_RadioTransmissionEvents.PopupToFaction("", "OVERTIME", "The clock is out, but the attackers are still with the truck. It's over if they leave it.", 8);
	}

	protected void EndRound(bool attackersWon, string title, string text)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		GetGame().GetCallqueue().Remove(Tick);
		SetEnded(attackersWon);

		CRF_RadioTransmissionEvents.PopupToFaction("", title, text, 10);

		if (!m_bEndMissionOnVictory)
			return;

		string winner = m_sDefenderFaction;
		if (attackersWon)
			winner = m_sAttackerFaction;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	protected void SetEnded(bool attackersWon)
	{
		m_iState = CRF_EPayloadState.ENDED;
		m_fSpeed = 0;
		m_bOvertime = false;
		m_bAttackersWon = attackersWon;
		Replication.BumpMe();
	}

	//===================================================================================
	// EVERY MACHINE: THE ROUTE
	//===================================================================================

	//! Reads the route, the prop and the checkpoints from the world.
	//! \return false if they aren't there (yet)
	protected bool BuildRoute()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		ShapeEntity route = ShapeEntity.Cast(world.FindEntityByName(m_sRouteName));
		m_Payload = world.FindEntityByName(m_sPayloadName);
		if (!route || !m_Payload)
		{
			if (!m_bWarnedNoRoute)
				Print(string.Format("[CRF_Payload] Need a shape named '%1' and a prop named '%2' in the world.", m_sRouteName, m_sPayloadName), LogLevel.WARNING);

			m_bWarnedNoRoute = true;
			return false;
		}

		if (Vehicle.Cast(m_Payload))
			Print(string.Format("[CRF_Payload] '%1' is a vehicle. The truck has to be a static prop: its physics will fight the movement.", m_sPayloadName), LogLevel.ERROR);

		array<vector> points = {};
		route.GetPointsPositions(points);
		if (points.Count() < 2)
		{
			if (!m_bWarnedNoRoute)
				Print(string.Format("[CRF_Payload] '%1' needs at least two points.", m_sRouteName), LogLevel.WARNING);

			m_bWarnedNoRoute = true;
			return false;
		}

		m_aRoute.Clear();
		m_aRouteDistances.Clear();
		m_fRouteLength = 0;
		foreach (int i, vector point : points)
		{
			vector worldPoint = route.CoordToParent(point);
			if (i > 0)
				m_fRouteLength += vector.DistanceXZ(m_aRoute[i - 1], worldPoint);

			m_aRoute.Insert(worldPoint);
			m_aRouteDistances.Insert(m_fRouteLength);
		}

		BuildCheckpoints();

		// Placed props are flagged static, which stops them moving
		m_Payload.ClearFlags(EntityFlags.STATIC, true);

		m_aPayloadParts.Clear();
		CollectPayloadParts(m_Payload);

		m_bRouteReady = true;
		return true;
	}

	protected void BuildCheckpoints()
	{
		m_aCheckpointDistances.Clear();
		if (!m_aCheckpoints)
			return;

		foreach (int i, CRF_PayloadCheckpoint checkpoint : m_aCheckpoints)
		{
			float distance = m_fRouteLength;
			IEntity entity = CRF_SpecialsHelper.FindNamedEntity(checkpoint.m_sEntityName, "[CRF_Payload]");
			if (entity)
				distance = GetDistanceAlongRoute(entity.GetOrigin());

			if (i > 0 && distance < m_aCheckpointDistances[i - 1])
				Print(string.Format("[CRF_Payload] Checkpoint '%1' comes before the one listed ahead of it on the route.", checkpoint.m_sName), LogLevel.WARNING);

			m_aCheckpointDistances.Insert(distance);
		}
	}

	//! How far along the route the point nearest position is
	protected float GetDistanceAlongRoute(vector position)
	{
		float best = float.MAX;
		float bestDistance;
		int count = m_aRoute.Count();
		for (int i = 0; i < count - 1; i++)
		{
			vector from = m_aRoute[i];
			vector to = m_aRoute[i + 1];
			float segmentX = to[0] - from[0];
			float segmentZ = to[2] - from[2];
			float lengthSquared = segmentX * segmentX + segmentZ * segmentZ;

			float along = 0;
			if (lengthSquared > 0)
				along = Math.Clamp(((position[0] - from[0]) * segmentX + (position[2] - from[2]) * segmentZ) / lengthSquared, 0, 1);

			float offsetX = from[0] + along * segmentX - position[0];
			float offsetZ = from[2] + along * segmentZ - position[2];
			float offset = offsetX * offsetX + offsetZ * offsetZ;
			if (offset < best)
			{
				best = offset;
				bestDistance = m_aRouteDistances[i] + along * (m_aRouteDistances[i + 1] - m_aRouteDistances[i]);
			}
		}

		return bestDistance;
	}

	//! The point this far along the route
	protected vector GetRoutePoint(float distance)
	{
		int last = m_aRoute.Count() - 1;
		if (last < 1)
			return vector.Zero;

		if (distance <= 0)
			return m_aRoute[0];

		if (distance >= m_fRouteLength)
			return m_aRoute[last];

		int low = 0;
		int high = last;
		while (high - low > 1)
		{
			int middle = (low + high) / 2;
			if (m_aRouteDistances[middle] <= distance)
				low = middle;
			else
				high = middle;
		}

		float length = m_aRouteDistances[high] - m_aRouteDistances[low];
		if (length <= 0)
			return m_aRoute[low];

		float along = (distance - m_aRouteDistances[low]) / length;
		vector from = m_aRoute[low];
		vector to = m_aRoute[high];
		return from + (to - from) * along;
	}

	//===================================================================================
	// EVERY MACHINE: PLACING THE TRUCK
	//===================================================================================

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bRouteReady && !BuildRoute())
			return;

		if (!m_Payload)
			return;

		float error = m_fDistance - m_fShownDistance;
		if (m_fAppliedDistance < 0 || Math.AbsFloat(error) > SNAP_DISTANCE)
			m_fShownDistance = m_fDistance;
		else
			m_fShownDistance += (m_fSpeed + error * CATCH_UP) * timeSlice;

		m_fShownDistance = Math.Clamp(m_fShownDistance, 0, m_fRouteLength);

		if (Math.AbsFloat(m_fShownDistance - m_fAppliedDistance) < 0.001)
			return;

		m_fAppliedDistance = m_fShownDistance;
		PlaceTruck(m_fShownDistance);
	}

	//! On the ground, facing along the route, pitched and rolled to the slope under it
	protected void PlaceTruck(float distance)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float halfWheelbase = m_fWheelbase * 0.5;
		vector front = GetRoutePoint(distance + halfWheelbase);
		vector rear = GetRoutePoint(distance - halfWheelbase);
		front[1] = world.GetSurfaceY(front[0], front[2]);
		rear[1] = world.GetSurfaceY(rear[0], rear[2]);

		vector forward = front - rear;
		if (forward.LengthSq() < 0.01)
			return;

		forward = forward.Normalized();

		vector position = GetRoutePoint(distance);
		vector side = Vector(forward[2], 0, -forward[0]);
		side = side.Normalized();

		vector rightPoint = position + side * HALF_WIDTH;
		vector leftPoint = position - side * HALF_WIDTH;
		rightPoint[1] = world.GetSurfaceY(rightPoint[0], rightPoint[2]);
		leftPoint[1] = world.GetSurfaceY(leftPoint[0], leftPoint[2]);

		vector right = rightPoint - leftPoint;
		right = right.Normalized();

		vector up = Cross(forward, right);
		up = up.Normalized();
		right = Cross(up, forward);

		// On the line between the axles, so it doesn't sink into a dip between them
		position[1] = (front[1] + rear[1]) * 0.5 + m_fHeightOffset;

		vector transform[4];
		transform[0] = right;
		transform[1] = up;
		transform[2] = forward;
		transform[3] = position;

		// Where on the truck the local player is standing, if they are, before it moves
		ChimeraCharacter rider = GetLocalRider();
		vector riderOnTruck;
		if (rider)
			riderOnTruck = m_Payload.CoordToLocal(rider.GetOrigin());

		m_Payload.SetWorldTransform(transform);
		m_Payload.Update();
		UpdateChildren(m_Payload);

		if (rider)
			CarryRider(rider, m_Payload.CoordToParent(riderOnTruck));
	}

	//===================================================================================
	// EVERY CLIENT: RIDING ON THE TRUCK
	//
	// The truck is a static prop placed by script, so physics gives it no velocity and a player standing on
	// it would be left behind as it moves. Instead each client moves its own player with it: character
	// movement is owned by the client, and the move replicates like any other step they take.
	//===================================================================================

	//! The local player, if they are standing (or have just jumped) on the truck
	protected ChimeraCharacter GetLocalRider()
	{
		if (System.IsConsoleApp())
			return null;

		// The slots (cargo bed, wheels) may fill in after the route was read
		if (m_aPayloadParts.Count() < 2 && m_Payload.GetChildren())
		{
			m_aPayloadParts.Clear();
			CollectPayloadParts(m_Payload);
		}

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character || character.IsInVehicle())
			return null;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return null;

		vector origin = character.GetOrigin();
		if (vector.DistanceSqXZ(origin, m_Payload.GetOrigin()) > 100)
			return null;

		TraceSphere trace = new TraceSphere();
		trace.Start = origin + vector.Up * RIDER_TRACE_UP;
		trace.End = origin - vector.Up * RIDER_TRACE_DOWN;
		trace.Radius = RIDER_TRACE_RADIUS;
		trace.Flags = TraceFlags.ENTS;
		trace.IncludeArray = m_aPayloadParts;

		float hit = character.GetWorld().TraceMove(trace, null);
		if (!trace.TraceEnt)
			return null;

		// Under their feet, not a wheel they're standing beside
		float hitHeight = trace.Start[1] - (RIDER_TRACE_UP + RIDER_TRACE_DOWN) * hit;
		if (hitHeight > origin[1] + 0.15)
			return null;

		return character;
	}

	//! Moves the rider to where the truck has taken the spot they were standing on
	protected void CarryRider(notnull ChimeraCharacter rider, vector position)
	{
		vector riderTransform[4];
		rider.GetWorldTransform(riderTransform);
		if (vector.DistanceSq(riderTransform[3], position) < 0.000001)
			return;

		riderTransform[3] = position;
		rider.Teleport(riderTransform);
	}

	//! The prop and every entity hanging off it in slots, for the rider trace
	protected void CollectPayloadParts(IEntity entity)
	{
		m_aPayloadParts.Insert(entity);
		IEntity child = entity.GetChildren();
		while (child)
		{
			CollectPayloadParts(child);
			child = child.GetSibling();
		}
	}

	//! The wheels and cargo bed hang off the truck in slots
	protected void UpdateChildren(IEntity parent)
	{
		IEntity child = parent.GetChildren();
		while (child)
		{
			child.Update();
			UpdateChildren(child);
			child = child.GetSibling();
		}
	}

	protected vector Cross(vector a, vector b)
	{
		return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
	}

	//===================================================================================
	// CLIENT: MAP AND PANEL
	//===================================================================================

	protected void UpdateMarkers()
	{
		if (!m_bRouteReady || !m_Markers)
			return;

		m_Markers.Begin();

		vector truck = GetRoutePoint(m_fDistance);
		m_Markers.Want(Vector(Math.Round(truck[0]), 0, Math.Round(truck[2])), "Truck", m_sTruckMarkerIcon, ARGB(255, 255, 165, 0));

		if (m_aCheckpoints)
		{
			foreach (int i, CRF_PayloadCheckpoint checkpoint : m_aCheckpoints)
			{
				if (!m_aCheckpointDistances.IsIndexValid(i))
					continue;

				int color = ARGB(255, 255, 255, 255);
				if (i < m_iCheckpointsReached)
					color = ARGB(255, 90, 90, 90);

				m_Markers.Want(GetRoutePoint(m_aCheckpointDistances[i]), checkpoint.m_sName, m_sCheckpointMarkerIcon, color);
			}
		}

		m_Markers.Want(GetRoutePoint(m_fRouteLength), "Finish", m_sCheckpointMarkerIcon, ARGB(255, 225, 25, 0));
		m_Markers.Apply();
	}

	protected void UpdateHud()
	{
		if (m_Hud)
			m_Hud.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	CRF_EPayloadState GetPayloadState()
	{
		return m_iState;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	int GetSecondsLeft()
	{
		return m_iSecondsLeft;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	bool IsOvertime()
	{
		return m_bOvertime;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	bool DidAttackersWin()
	{
		return m_bAttackersWon;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	int GetCheckpointsReached()
	{
		return m_iCheckpointsReached;
	}

	//------------------------------------------------------------------------------------------------
	int GetCheckpointCount()
	{
		return m_aCheckpointDistances.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! 0-1 along the route
	float GetProgress()
	{
		if (m_fRouteLength <= 0)
			return 0;

		return m_fDistance / m_fRouteLength;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the truck is on this machine
	vector GetTruckPosition()
	{
		return GetRoutePoint(m_fDistance);
	}

	//------------------------------------------------------------------------------------------------
	//! How far along the route the prop is drawn on this machine, which eases between updates
	float GetShownDistance()
	{
		return m_fShownDistance;
	}

	//------------------------------------------------------------------------------------------------
	//! True once the route has been read from the world on this machine
	bool IsRouteReady()
	{
		return m_bRouteReady;
	}

	//------------------------------------------------------------------------------------------------
	//! The route's points in world space, start first. Read only.
	array<vector> GetRoute()
	{
		return m_aRoute;
	}

	//------------------------------------------------------------------------------------------------
	//! How far along the route each of GetRoute()'s points is. Read only.
	array<float> GetRouteDistances()
	{
		return m_aRouteDistances;
	}

	//------------------------------------------------------------------------------------------------
	//! How far along the route each checkpoint is, in order. Read only.
	array<float> GetCheckpointDistances()
	{
		return m_aCheckpointDistances;
	}

	//------------------------------------------------------------------------------------------------
	bool ShowMinimap()
	{
		return m_bShowMinimap;
	}

	//------------------------------------------------------------------------------------------------
	string GetAttackerFactionKey()
	{
		return m_sAttackerFaction;
	}

	//------------------------------------------------------------------------------------------------
	string GetDefenderFactionKey()
	{
		return m_sDefenderFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iState == CRF_EPayloadState.WAITING)
		{
			lines.Insert("Payload: the truck moves when the round goes live");
			return;
		}

		if (m_iState == CRF_EPayloadState.ENDED)
		{
			lines.Insert("Payload: round over");
			return;
		}

		int percent = Math.Round(GetProgress() * 100);
		lines.Insert(string.Format("Payload: %1 percent of the route, checkpoint %2 of %3", percent, m_iCheckpointsReached, GetCheckpointCount()));
		if (m_bOvertime)
			lines.Insert("Payload: overtime");
		else
			lines.Insert(string.Format("Payload: %1 left", SCR_FormatHelper.FormatTime(m_iSecondsLeft)));
	}
}
