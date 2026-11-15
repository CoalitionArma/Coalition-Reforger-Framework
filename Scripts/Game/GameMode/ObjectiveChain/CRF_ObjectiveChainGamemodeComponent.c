//------------------------------------------------------------------------------------
// CRF_ObjectiveChainGamemodeComponent: the attackers take a chain of objectives, one after another
//
// Each objective is a zone the attackers have to control: enough of them inside it and few enough
// conscious defenders, for long enough. An objective can also list breach points - wall sections an
// attacker blows open with a breaching charge (a satchel, an M112 block or a Bangalore from their
// inventory, see CRF_BreachChargeAction) - and then can't be taken until enough of them are breached.
// When an objective falls the next one opens, the factions' respawn points can move up, and every dead
// player of both sides comes back (COA_RespawnManager.RespawnSide, as Rush does on a zone advance).
// The attackers win when the last objective falls; the defenders when the clock runs out or the
// attackers are wiped out with no way back.
//
// The server decides everything. Clients get the current objective, the capture progress and which
// breach points are armed or blown; every machine deletes its own copy of a blown wall section (they're
// plain static world entities), and the explosion itself is a replicated prefab spawned by the server.
//
// Setup for mission makers (full guide: !Docs/MissionMaking/Gamemodes/CRF_OBJECTIVE_CHAIN_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the COA_Lobby gamemode entity and list the objectives in order.
//   2. Place a named entity at the centre of every objective zone.
//   3. For a breach, place CRF_BreachWall_4m.et (or any prefab with CRF_BreachChargeAction) and name it;
//      list the names under the objective's breach points.
//------------------------------------------------------------------------------------

enum CRF_EObjectiveChainPhase
{
	WAITING,	//!< the round isn't live yet
	RUNNING,
	ENDED
}

//------------------------------------------------------------------------------------------------
//! Where a faction's (default) respawn point moves when an objective falls
[BaseContainerProps()]
class CRF_ObjectiveChainRespawnMove
{
	[Attribute("", UIWidgets.EditBox, "Name of the entity the faction's respawn point moves to (position and facing)")]
	string m_sMarkerEntityName;

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key whose default respawn point moves")]
	string m_sFaction;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTitle")]
class CRF_ObjectiveChainObjective
{
	[Attribute("Objective", UIWidgets.EditBox, "The attackers' task title")]
	string m_sTitle;

	[Attribute("", UIWidgets.EditBoxMultiline, "The attackers' task description")]
	string m_sDescription;

	[Attribute("", UIWidgets.EditBox, "The defenders' task title. Empty: \"Hold: <title>\"")]
	string m_sDefenderTitle;

	[Attribute("", UIWidgets.EditBoxMultiline, "The defenders' task description")]
	string m_sDefenderDescription;

	[Attribute("", UIWidgets.EditBox, "Name of the entity at the centre of the zone")]
	string m_sZoneEntityName;

	[Attribute("75", UIWidgets.EditBox, "Zone radius (metres, on the map)", params: "5 1000 1")]
	float m_fRadius;

	[Attribute("60", UIWidgets.EditBox, "Seconds the attackers must control the zone", params: "0 1800 1")]
	float m_fHoldSeconds;

	[Attribute("2", UIWidgets.EditBox, "Conscious attackers needed in the zone", params: "1 100 1")]
	int m_iMinAttackers;

	[Attribute("1", UIWidgets.EditBox, "The zone is controlled while at most this many conscious defenders are in it", params: "0 100 1")]
	int m_iMaxDefenders;

	[Attribute(desc: "Names of breach point entities (CRF_BreachChargeAction) that belong to this objective")]
	ref array<string> m_aBreachPointNames;

	[Attribute("1", UIWidgets.EditBox, "Breach points that must be blown before the zone can be taken (ignored without breach points)", params: "0 32 1")]
	int m_iBreachesRequired;

	[Attribute(desc: "Respawn points that move when this objective falls")]
	ref array<ref CRF_ObjectiveChainRespawnMove> m_aRespawnMoves;

	[Attribute("1", UIWidgets.CheckBox, "When this objective falls, every dead player of both sides respawns")]
	bool m_bRespawnDeadOnComplete;
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Objective chain: the attackers take a list of zones in order, some behind walls they have to breach.")]
class CRF_ObjectiveChainGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_ObjectiveChainGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the attackers", category: "Objective Chain")]
	protected string m_sAttackerFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the defenders", category: "Objective Chain")]
	protected string m_sDefenderFaction;

	[Attribute(desc: "The objectives, in the order they're taken", category: "Objective Chain")]
	protected ref array<ref CRF_ObjectiveChainObjective> m_aObjectives;

	[Attribute("60", UIWidgets.EditBox, "Minutes from the round going live until the defenders win. 0: no limit.", params: "0 600 1", category: "Objective Chain")]
	protected int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Objective Chain")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("1", UIWidgets.CheckBox, "Assign every player to their side's open objective task (each player once per objective; anyone who unassigns stays unassigned), so the HUD shows it without opening the task list", category: "Objective Chain")]
	protected bool m_bAutoAssignTasks;

	[Attribute("10", UIWidgets.EditBox, "Seconds from setting a breaching charge to the wall section blowing", params: "1 120 1", category: "Breaching")]
	protected float m_fBreachFuseSeconds;

	[Attribute("6", UIWidgets.EditBox, "How close (metres) an attacker must be to a breach point to set a charge on it", params: "2 30 0.5", category: "Breaching")]
	protected float m_fBreachReach;

	[Attribute(desc: "An inventory item counts as a breaching charge if its prefab path, or a parent prefab's, contains one of these", category: "Breaching")]
	protected ref array<string> m_aBreachChargeKeywords;

	[Attribute("{2F690C7C59FB4DBF}Prefabs/Weapons/Warheads/Explosions/Explosion_Tnt_Small.et", UIWidgets.ResourceNamePicker, "Explosion spawned (server, replicated) when a breach blows", params: "et", category: "Breaching")]
	protected ResourceName m_rBreachExplosion;

	// Replicated
	[RplProp()]
	protected int m_iPhase;
	[RplProp()]
	protected int m_iCurrent;			//!< index of the open objective (== count once all have fallen)
	[RplProp()]
	protected float m_fProgress01;		//!< capture progress of the open objective, 0..1
	[RplProp(onRplName: "OnBreachStateReplicated")]
	protected int m_iBreachedMask;		//!< bit per breach point (global index), blown
	[RplProp(onRplName: "OnBreachStateReplicated")]
	protected int m_iArmedMask;			//!< bit per breach point, charge set and burning
	[RplProp()]
	protected bool m_bAttackersWon;

	// Every machine: breach points flattened across objectives (built from the attributes)
	protected ref array<string> m_aBreachNames = {};
	protected ref array<int> m_aBreachObjective = {};
	protected int m_iLocallyRemovedMask;

	// Client
	protected ref CRF_SpecialsMarkerSet m_Markers;

	// Server
	protected COA_Gamemode m_Gamemode;
	protected bool m_bRoundLive;
	protected bool m_bEndRequested;
	protected float m_fDeadline;		//!< world time (ms) the clock runs out, 0 = none
	protected float m_fProgress;		//!< seconds held on the open objective
	protected float m_fLastSentProgress = -1;
	protected float m_fWipeSeconds;
	protected ref array<vector> m_aZoneCenters = {};
	protected ref array<vector> m_aBreachPositions = {};
	protected ref array<string> m_aAttackerTasks = {};
	protected ref array<string> m_aDefenderTasks = {};
	protected ref array<IEntity> m_aQueryResults = {};
	protected int m_iLastBreachCount = -1;

	protected static const int TICK_MS = 500;
	protected static const float TICK_SECONDS = 0.5;
	protected static const float WIPE_SECONDS = 60;
	protected static const float QUERY_MARGIN = 30;
	protected static const int MAX_BREACH_POINTS = 31;
	protected static const int MARKER_POLL_MS = 2000;
	protected static const ResourceName OBJECTIVE_ICON = "{1B3B38AAA1577734}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_Defend.edds";
	protected static const ResourceName BREACH_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
	protected static const string LOG = "[CRF_ObjectiveChain]";
	protected static const string MODE = "ObjectiveChain";

	protected static CRF_ObjectiveChainGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_ObjectiveChainGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_ObjectiveChainGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_ObjectiveChainGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		BuildBreachList();

		// Every machine: the action checks the player's inventory locally before asking the server
		if (!m_aBreachChargeKeywords || m_aBreachChargeKeywords.IsEmpty())
		{
			m_aBreachChargeKeywords = new array<string>();
			m_aBreachChargeKeywords.Insert("Explosive_Satchels");
			m_aBreachChargeKeywords.Insert("DemoBlock_M112");
			m_aBreachChargeKeywords.Insert("Bangalore");
		}

		if (!System.IsConsoleApp())
		{
			m_Markers = new CRF_SpecialsMarkerSet();
			GetGame().GetCallqueue().CallLater(UpdateLocal, MARKER_POLL_MS, true);
		}

		if (!Replication.IsServer())
			return;

		if (!m_aObjectives || m_aObjectives.IsEmpty())
		{
			Print(LOG + " No objectives configured: the objective chain is not running.", LogLevel.ERROR);
			return;
		}

		// Polled: the Lobby managers may not exist yet this early, and a mission without safestart
		// goes live on the gamemode state instead
		GetGame().GetCallqueue().CallLater(WaitForRoundLive, 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(WaitForRoundLive);
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(UpdateLocal);
			GetGame().GetCallqueue().Remove(DetonateBreach);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Markers)
			m_Markers.Clear();

		super.OnDelete(owner);
	}

	//! Global breach index -> name / objective, the same on every machine (from the attributes)
	protected void BuildBreachList()
	{
		m_aBreachNames.Clear();
		m_aBreachObjective.Clear();
		if (!m_aObjectives)
			return;

		foreach (int index, CRF_ObjectiveChainObjective objective : m_aObjectives)
		{
			if (!objective || !objective.m_aBreachPointNames)
				continue;

			foreach (string breachName : objective.m_aBreachPointNames)
			{
				if (breachName.IsEmpty())
					continue;

				if (m_aBreachNames.Count() >= MAX_BREACH_POINTS)
				{
					Print(string.Format("%1 More than %2 breach points: '%3' and the rest are ignored.", LOG, MAX_BREACH_POINTS, breachName), LogLevel.ERROR);
					return;
				}

				m_aBreachNames.Insert(breachName);
				m_aBreachObjective.Insert(index);
			}
		}
	}

	protected void WaitForRoundLive()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return;

		COA_SafestartManager safestart = COA_SafestartManager.GetInstance();
		if (safestart)
		{
			if (!safestart.IsRoundLive())
				return;
		}
		else if (gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
		{
			return;
		}

		GetGame().GetCallqueue().Remove(WaitForRoundLive);
		m_Gamemode = gamemode;
		m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
		StartRound();
	}

	//! The mission left the game state some other way (an admin ended it)
	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode || m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			return;

		if (m_bRoundLive && m_iPhase != CRF_EObjectiveChainPhase.ENDED)
		{
			m_bEndRequested = true;
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(DetonateBreach);
			m_iPhase = CRF_EObjectiveChainPhase.ENDED;
			Replication.BumpMe();
		}
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		m_aZoneCenters.Clear();
		foreach (CRF_ObjectiveChainObjective objective : m_aObjectives)
		{
			IEntity zone = CRF_SpecialsHelper.FindNamedEntity(objective.m_sZoneEntityName, LOG);
			if (!zone)
			{
				Print(string.Format("%1 Objective '%2' has no zone entity: the objective chain is not running.", LOG, objective.m_sTitle), LogLevel.ERROR);
				return;
			}

			m_aZoneCenters.Insert(zone.GetOrigin());
		}

		m_aBreachPositions.Clear();
		foreach (string breachName : m_aBreachNames)
		{
			IEntity breach = CRF_SpecialsHelper.FindNamedEntity(breachName, LOG);
			if (breach)
				m_aBreachPositions.Insert(breach.GetOrigin());
			else
				m_aBreachPositions.Insert(vector.Zero);
		}

		m_bRoundLive = true;
		if (m_iTimeLimitMinutes > 0)
			m_fDeadline = GetGame().GetWorld().GetWorldTime() + m_iTimeLimitMinutes * 60000;

		m_iPhase = CRF_EObjectiveChainPhase.RUNNING;
		Replication.BumpMe();

		OpenObjective(0);
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//===================================================================================
	// SERVER: OBJECTIVES
	//===================================================================================

	protected void OpenObjective(int index)
	{
		m_iCurrent = index;
		m_fProgress = 0;
		m_fProgress01 = 0;
		m_fLastSentProgress = -1;
		m_iLastBreachCount = -1;
		Replication.BumpMe();

		CRF_ObjectiveChainObjective objective = m_aObjectives[index];
		vector center = m_aZoneCenters[index];

		string attackerTask = CRF_ObjectiveTasks.MakeId(MODE, "Objective" + index.ToString(), m_sAttackerFaction);
		CRF_ObjectiveTasks.Create(attackerTask, m_sAttackerFaction, AttackerTaskTitle(index), objective.m_sDescription,
			CRF_ObjectiveTasks.ICON_SEIZE, SCR_ETaskState.FAILED, center);

		string defenderTitle = objective.m_sDefenderTitle;
		if (defenderTitle.IsEmpty())
			defenderTitle = "Hold: " + objective.m_sTitle;

		string defenderTask = CRF_ObjectiveTasks.MakeId(MODE, "Hold" + index.ToString(), m_sDefenderFaction);
		CRF_ObjectiveTasks.Create(defenderTask, m_sDefenderFaction, defenderTitle, objective.m_sDefenderDescription,
			CRF_ObjectiveTasks.ICON_DEFEND, SCR_ETaskState.COMPLETED, center);

		m_aAttackerTasks.Insert(attackerTask);
		m_aDefenderTasks.Insert(defenderTask);

		if (m_bAutoAssignTasks)
		{
			CRF_ObjectiveTasks.AutoAssign(attackerTask, m_sAttackerFaction);
			CRF_ObjectiveTasks.AutoAssign(defenderTask, m_sDefenderFaction);
		}

		string step = string.Format("OBJECTIVE %1 OF %2", index + 1, m_aObjectives.Count());
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackerFaction, step, objective.m_sTitle, 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFaction, step, defenderTitle, 10);
		Print(string.Format("%1 Objective %2 open: %3", LOG, index + 1, objective.m_sTitle), LogLevel.NORMAL);
	}

	//! The attackers' title, with the breach count while the objective still needs breaching
	protected string AttackerTaskTitle(int index)
	{
		CRF_ObjectiveChainObjective objective = m_aObjectives[index];
		int required = BreachesRequired(index);
		if (required <= 0)
			return objective.m_sTitle;

		int breached = Math.Min(CountBreached(index), required);
		return string.Format("%1 (breached %2/%3)", objective.m_sTitle, breached, required);
	}

	protected void Tick()
	{
		if (m_bEndRequested)
			return;

		if (m_fDeadline > 0 && GetGame().GetWorld().GetWorldTime() >= m_fDeadline)
		{
			EndRound(false, "Time is up: the attack has stalled.");
			return;
		}

		TickWipe();
		if (m_bEndRequested)
			return;

		TickObjective();
	}

	protected void TickObjective()
	{
		if (m_iCurrent >= m_aObjectives.Count())
			return;

		CRF_ObjectiveChainObjective objective = m_aObjectives[m_iCurrent];
		string attackerTask = m_aAttackerTasks[m_iCurrent];

		int breached = CountBreached(m_iCurrent);
		if (breached != m_iLastBreachCount)
		{
			m_iLastBreachCount = breached;
			CRF_ObjectiveTasks.SetText(attackerTask, AttackerTaskTitle(m_iCurrent), objective.m_sDescription);
		}

		if (breached < BreachesRequired(m_iCurrent))
			return;

		int attackers, defenders;
		CountInZone(m_aZoneCenters[m_iCurrent], objective.m_fRadius, attackers, defenders);

		// Controlled: progress. Contested: it holds. Nobody there to hold it: it drains.
		if (attackers >= objective.m_iMinAttackers && defenders <= objective.m_iMaxDefenders)
			m_fProgress += TICK_SECONDS;
		else if (attackers == 0)
			m_fProgress = Math.Max(0, m_fProgress - TICK_SECONDS);

		float fraction = 1;
		if (objective.m_fHoldSeconds > 0)
			fraction = Math.Clamp(m_fProgress / objective.m_fHoldSeconds, 0, 1);

		// Replicate and show it in steps, not every tick
		if (Math.AbsFloat(fraction - m_fLastSentProgress) >= 0.02 || (fraction == 0 && m_fLastSentProgress != 0))
		{
			m_fLastSentProgress = fraction;
			m_fProgress01 = fraction;
			Replication.BumpMe();
			CRF_ObjectiveTasks.SetProgress(attackerTask, fraction);
			CRF_ObjectiveTasks.SetProgress(m_aDefenderTasks[m_iCurrent], fraction);
		}

		if (fraction >= 1)
			CompleteObjective(m_iCurrent);
	}

	protected void CompleteObjective(int index)
	{
		CRF_ObjectiveChainObjective objective = m_aObjectives[index];
		CRF_ObjectiveTasks.Finish(m_aAttackerTasks[index], SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_aDefenderTasks[index], SCR_ETaskState.FAILED);
		Print(string.Format("%1 Objective %2 taken: %3", LOG, index + 1, objective.m_sTitle), LogLevel.NORMAL);

		if (index + 1 >= m_aObjectives.Count())
		{
			m_iCurrent = m_aObjectives.Count();
			Replication.BumpMe();
			EndRound(true, objective.m_sTitle + ": done. Every objective has been taken.");
			return;
		}

		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackerFaction, "OBJECTIVE TAKEN", objective.m_sTitle, 8);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFaction, "OBJECTIVE LOST", objective.m_sTitle + ". Fall back to the next position.", 8);

		MoveRespawns(objective);
		if (objective.m_bRespawnDeadOnComplete)
		{
			COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
			if (respawnManager)
			{
				respawnManager.RespawnSide(m_sAttackerFaction);
				respawnManager.RespawnSide(m_sDefenderFaction);
			}
		}

		OpenObjective(index + 1);
	}

	//! As Rush: the faction's first (default) respawn point goes to the marker
	protected void MoveRespawns(notnull CRF_ObjectiveChainObjective objective)
	{
		if (!objective.m_aRespawnMoves)
			return;

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return;

		foreach (CRF_ObjectiveChainRespawnMove move : objective.m_aRespawnMoves)
		{
			if (!move)
				continue;

			IEntity marker = CRF_SpecialsHelper.FindNamedEntity(move.m_sMarkerEntityName, LOG);
			if (!marker)
				continue;

			array<COA_SpawnPointData> spawns = respawnManager.GetFactionSpawnpoints(move.m_sFaction);
			if (!spawns || spawns.IsEmpty())
			{
				Print(string.Format("%1 %2 has no respawn point to move to '%3'.", LOG, move.m_sFaction, move.m_sMarkerEntityName), LogLevel.WARNING);
				continue;
			}

			IEntity spawnEntity = COA_EntityHelper.GetEntityFromRplId(spawns[0].GetSpawnPointEntity());
			if (!spawnEntity)
				continue;

			CRF_SpecialsHelper.MoveSpawnPoint(spawnEntity, marker);
			Print(string.Format("%1 %2 respawn point moved to '%3' at %4", LOG, move.m_sFaction, move.m_sMarkerEntityName, marker.GetOrigin()), LogLevel.NORMAL);
		}
	}

	//! No living attackers for a while, and none of them can come back
	protected void TickWipe()
	{
		if (!HasSlottedPlayers(m_sAttackerFaction))
		{
			m_fWipeSeconds = 0;
			return;
		}

		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sAttackerFaction, alive);
		if (!alive.IsEmpty() || CanFactionRespawn(m_sAttackerFaction))
		{
			m_fWipeSeconds = 0;
			return;
		}

		m_fWipeSeconds += TICK_SECONDS;
		if (m_fWipeSeconds >= WIPE_SECONDS)
			EndRound(false, string.Format("%1 has nobody left alive and no respawns.", CRF_SpecialsHelper.GetFactionName(m_sAttackerFaction)));
	}

	//! The Lobby's own respawn gates (COA_RespawnManager.CanPlayerRespawn), minus the per-entity spectator
	//! check, which every dead player would fail
	protected bool CanFactionRespawn(string factionKey)
	{
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!respawnManager || !gamemode)
			return true;	// can't tell: leave it to the clock

		if (!respawnManager.m_bCurrentRespawnEnabled || !respawnManager.IsRespawnTimeAllowed())
			return false;

		if (respawnManager.GetFactionSpawnpoints(factionKey).IsEmpty())
			return false;

		if (gamemode.m_eRespawnMode != COA_ERespawnMode.SLOT)
			return respawnManager.TicketsRemaining(factionKey);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) == factionKey && respawnManager.SlotRespawnsRemaining(playerId))
				return true;
		}

		return false;
	}

	protected bool HasSlottedPlayers(string factionKey)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) == factionKey)
				return true;
		}

		return false;
	}

	//! Conscious characters of both sides in the zone, AI and players alike (captives are CIV to ACE)
	protected void CountInZone(vector center, float radius, out int attackers, out int defenders)
	{
		attackers = 0;
		defenders = 0;
		m_aQueryResults.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius + QUERY_MARGIN, AddCharacterResult, null, EQueryEntitiesFlags.DYNAMIC);

		foreach (IEntity entity : m_aQueryResults)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character || COA_EntityHelper.IsSpectator(character))
				continue;

			if (vector.DistanceXZ(character.GetOrigin(), center) > radius)
				continue;

			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE)
				continue;

			string faction = character.GetFactionKey();
			if (faction == m_sAttackerFaction)
				attackers++;
			else if (faction == m_sDefenderFaction)
				defenders++;
		}

		m_aQueryResults.Clear();
	}

	//! Characters, and whoever sits in a vehicle (a seated character may not come up in the query itself)
	protected bool AddCharacterResult(IEntity entity)
	{
		if (SCR_ChimeraCharacter.Cast(entity))
		{
			if (!m_aQueryResults.Contains(entity))
				m_aQueryResults.Insert(entity);
			return true;
		}

		if (!Vehicle.Cast(entity))
			return true;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(entity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
			return true;

		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			if (SCR_ChimeraCharacter.Cast(occupant) && !m_aQueryResults.Contains(occupant))
				m_aQueryResults.Insert(occupant);
		}

		return true;
	}

	//===================================================================================
	// SERVER: BREACHING
	//===================================================================================

	//! From CRF_BreachChargeAction through COA_PlayerRplToAuthorityManager.RequestObjectiveChainBreach.
	//! Everything is checked again here: the action's own checks ran on the player's machine.
	void ServerArmBreach(int playerId, int breachIndex)
	{
		if (m_bEndRequested || m_iPhase != CRF_EObjectiveChainPhase.RUNNING || !CanBreachNow(breachIndex))
			return;

		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sAttackerFaction)
			return;

		IEntity user = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!user || COA_EntityHelper.IsSpectator(user))
			return;

		vector breachPosition = m_aBreachPositions[breachIndex];
		if (vector.Distance(user.GetOrigin(), breachPosition) > m_fBreachReach)
			return;

		IEntity charge = FindBreachCharge(user, m_aBreachChargeKeywords);
		if (!charge)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "NO BREACHING CHARGE", "You need a satchel charge, an M112 block or a Bangalore torpedo.", 5);
			return;
		}

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory || !inventory.TryDeleteItem(charge))
			SCR_EntityHelper.DeleteEntityAndChildren(charge);

		m_iArmedMask = m_iArmedMask | (1 << breachIndex);
		Replication.BumpMe();

		int fuse = Math.Round(m_fBreachFuseSeconds);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackerFaction, "BREACHING CHARGE SET", string.Format("The wall blows in %1 seconds. Get clear!", fuse), 6);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFaction, "BREACHING CHARGE ON THE WALL", "The enemy is blowing a gap in the wall.", 6);
		Print(string.Format("%1 Player %2 set a breaching charge on '%3'.", LOG, playerId, m_aBreachNames[breachIndex]), LogLevel.NORMAL);

		int fuseMs = m_fBreachFuseSeconds * 1000;
		GetGame().GetCallqueue().CallLater(DetonateBreach, fuseMs, false, breachIndex);
	}

	protected void DetonateBreach(int breachIndex)
	{
		if (m_bEndRequested || breachIndex < 0 || breachIndex >= m_aBreachNames.Count())
			return;

		int bit = 1 << breachIndex;
		m_iArmedMask = m_iArmedMask & ~bit;
		m_iBreachedMask = m_iBreachedMask | bit;
		Replication.BumpMe();

		vector position = m_aBreachPositions[breachIndex];
		IEntity wall = GetGame().GetWorld().FindEntityByName(m_aBreachNames[breachIndex]);
		if (wall)
			position = wall.GetOrigin();

		SpawnBreachExplosion(position);
		ApplyBreachedLocally();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackerFaction, "WALL BREACHED", "There's a gap in the wall. Push through!", 6);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFaction, "THE WALL IS BREACHED", "Stop them coming through the gap!", 6);
		Print(string.Format("%1 '%2' is breached.", LOG, m_aBreachNames[breachIndex]), LogLevel.NORMAL);
	}

	protected void SpawnBreachExplosion(vector position)
	{
		if (m_rBreachExplosion.IsEmpty())
			return;

		Resource resource = Resource.Load(m_rBreachExplosion);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		// A sunk wall section's origin is underground: put the blast at the surface, not under it
		vector blast = position;
		blast[1] = Math.Max(position[1] + 1, GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1);
		spawnParams.Transform[3] = blast;
		GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	//===================================================================================
	// EVERY MACHINE: BREACH STATE
	//===================================================================================

	protected void OnBreachStateReplicated()
	{
		ApplyBreachedLocally();
	}

	//! Blown wall sections are plain static entities: every machine deletes its own copy
	protected void ApplyBreachedLocally()
	{
		int pending = m_iBreachedMask & ~m_iLocallyRemovedMask;
		if (pending == 0)
			return;

		BaseWorld world = GetGame().GetWorld();
		foreach (int index, string breachName : m_aBreachNames)
		{
			int bit = 1 << index;
			if ((pending & bit) == 0)
				continue;

			IEntity wall = world.FindEntityByName(breachName);
			if (wall)
				SCR_EntityHelper.DeleteEntityAndChildren(wall);

			m_iLocallyRemovedMask = m_iLocallyRemovedMask | bit;
		}
	}

	//! Breach index of a world entity by its name, -1 if it's not a breach point
	int GetBreachIndex(IEntity entity)
	{
		if (!entity)
			return -1;

		return m_aBreachNames.Find(entity.GetName());
	}

	//! The breach point belongs to the open objective and is neither armed nor blown (replicated state)
	bool CanBreachNow(int breachIndex)
	{
		if (m_iPhase != CRF_EObjectiveChainPhase.RUNNING || breachIndex < 0 || breachIndex >= m_aBreachNames.Count())
			return false;

		if (m_aBreachObjective[breachIndex] != m_iCurrent)
			return false;

		int bit = 1 << breachIndex;
		return (m_iBreachedMask & bit) == 0 && (m_iArmedMask & bit) == 0;
	}

	bool IsBreachArmed(int breachIndex)
	{
		return breachIndex >= 0 && (m_iArmedMask & (1 << breachIndex)) != 0;
	}

	protected int CountBreached(int objectiveIndex)
	{
		int count;
		foreach (int index, int owner : m_aBreachObjective)
		{
			if (owner == objectiveIndex && (m_iBreachedMask & (1 << index)) != 0)
				count++;
		}

		return count;
	}

	protected int CountBreachPoints(int objectiveIndex)
	{
		int count;
		foreach (int owner : m_aBreachObjective)
		{
			if (owner == objectiveIndex)
				count++;
		}

		return count;
	}

	protected int BreachesRequired(int objectiveIndex)
	{
		int points = CountBreachPoints(objectiveIndex);
		if (points == 0)
			return 0;

		return Math.Min(m_aObjectives[objectiveIndex].m_iBreachesRequired, points);
	}

	//! An inventory item counts as a charge if its prefab, or one of its parents, matches a keyword
	static IEntity FindBreachCharge(IEntity user, array<string> keywords)
	{
		if (!user || !keywords)
			return null;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return null;

		array<IEntity> items = {};
		inventory.GetItems(items);
		foreach (IEntity item : items)
		{
			if (item && IsBreachCharge(item, keywords))
				return item;
		}

		return null;
	}

	protected static bool IsBreachCharge(notnull IEntity item, notnull array<string> keywords)
	{
		EntityPrefabData prefabData = item.GetPrefabData();
		if (!prefabData)
			return false;

		BaseContainer prefab = prefabData.GetPrefab();
		while (prefab)
		{
			string resourceName = prefab.GetResourceName();
			foreach (string keyword : keywords)
			{
				if (!keyword.IsEmpty() && resourceName.Contains(keyword))
					return true;
			}

			prefab = prefab.GetAncestor();
		}

		return false;
	}

	array<string> GetBreachChargeKeywords()
	{
		return m_aBreachChargeKeywords;
	}

	//===================================================================================
	// SERVER: THE END
	//===================================================================================

	protected void EndRound(bool attackersWon, string reason)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().Remove(DetonateBreach);

		m_iPhase = CRF_EObjectiveChainPhase.ENDED;
		m_bAttackersWon = attackersWon;
		Replication.BumpMe();

		string winner = m_sDefenderFaction;
		if (attackersWon)
			winner = m_sAttackerFaction;

		// Whatever is still open: the winner's done, the loser's failed
		SCR_ETaskState attackerState = SCR_ETaskState.FAILED;
		SCR_ETaskState defenderState = SCR_ETaskState.COMPLETED;
		if (attackersWon)
		{
			attackerState = SCR_ETaskState.COMPLETED;
			defenderState = SCR_ETaskState.FAILED;
		}

		foreach (string task : m_aAttackerTasks)
			CRF_ObjectiveTasks.Finish(task, attackerState);
		foreach (string task : m_aDefenderTasks)
			CRF_ObjectiveTasks.Finish(task, defenderState);

		AnnounceResult(attackersWon, reason);
		Print(string.Format("%1 Round over, %2 wins: %3", LOG, winner, reason), LogLevel.NORMAL);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	protected void AnnounceResult(bool attackersWon, string reason)
	{
		string attackerTitle = "MISSION FAILED";
		string defenderTitle = "VICTORY";
		string winnerName = CRF_SpecialsHelper.GetFactionName(m_sDefenderFaction);
		if (attackersWon)
		{
			attackerTitle = "MISSION COMPLETE";
			defenderTitle = "DEFEAT";
			winnerName = CRF_SpecialsHelper.GetFactionName(m_sAttackerFaction);
		}

		string neutralTitle = string.Format("Winner: %1", winnerName);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string faction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string title = neutralTitle;
			if (faction == m_sAttackerFaction)
				title = attackerTitle;
			else if (faction == m_sDefenderFaction)
				title = defenderTitle;

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, title, reason, 12);
		}
	}

	//===================================================================================
	// CLIENT: MAP MARKERS
	//===================================================================================

	//! Polled: the local player's faction changes with slotting, and a JIP client may miss the first
	//! breach replication callback. The open objective for both sides, its unbroken breach points for
	//! the attackers.
	protected void UpdateLocal()
	{
		ApplyBreachedLocally();

		if (!m_Markers)
			return;

		m_Markers.Begin();
		string localFaction = CRF_SpecialsHelper.GetLocalFactionKey();
		bool involved = localFaction == m_sAttackerFaction || localFaction == m_sDefenderFaction;
		if (involved && m_iPhase == CRF_EObjectiveChainPhase.RUNNING && m_aObjectives && m_iCurrent < m_aObjectives.Count())
		{
			CRF_ObjectiveChainObjective objective = m_aObjectives[m_iCurrent];
			IEntity zone = GetGame().GetWorld().FindEntityByName(objective.m_sZoneEntityName);
			if (zone)
				m_Markers.Want(zone.GetOrigin(), objective.m_sTitle, OBJECTIVE_ICON, ARGB(255, 225, 25, 0));

			if (localFaction == m_sAttackerFaction)
			{
				foreach (int index, string breachName : m_aBreachNames)
				{
					if (!CanBreachNow(index) && !IsBreachArmed(index))
						continue;

					IEntity breach = GetGame().GetWorld().FindEntityByName(breachName);
					if (breach)
						m_Markers.Want(breach.GetOrigin(), "Breach point", BREACH_ICON, ARGB(255, 255, 160, 0));
				}
			}
		}

		m_Markers.Apply();
	}

	//===================================================================================
	// ACCESSORS
	//===================================================================================

	bool IsRoundRunning()
	{
		return m_iPhase == CRF_EObjectiveChainPhase.RUNNING;
	}

	string GetAttackerFactionKey()
	{
		return m_sAttackerFaction;
	}

	string GetDefenderFactionKey()
	{
		return m_sDefenderFaction;
	}

	int GetCurrentObjectiveIndex()
	{
		return m_iCurrent;
	}

	bool DidAttackersWin()
	{
		return m_bAttackersWon;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iPhase == CRF_EObjectiveChainPhase.WAITING)
		{
			lines.Insert("Objectives: start when the round goes live");
			return;
		}

		if (m_iPhase == CRF_EObjectiveChainPhase.ENDED)
		{
			string winner = CRF_SpecialsHelper.GetFactionName(m_sDefenderFaction);
			if (m_bAttackersWon)
				winner = CRF_SpecialsHelper.GetFactionName(m_sAttackerFaction);
			lines.Insert(string.Format("Objectives: round over, %1 won", winner));
			return;
		}

		if (!m_aObjectives || m_iCurrent >= m_aObjectives.Count())
			return;

		CRF_ObjectiveChainObjective objective = m_aObjectives[m_iCurrent];
		lines.Insert(string.Format("Objective %1 of %2: %3", m_iCurrent + 1, m_aObjectives.Count(), objective.m_sTitle));

		int required = BreachesRequired(m_iCurrent);
		if (required > 0 && CountBreached(m_iCurrent) < required)
			lines.Insert(string.Format("Breached: %1 of %2 needed", CountBreached(m_iCurrent), required));
		else
		{
			int percent = Math.Round(m_fProgress01 * 100);
			lines.Insert(string.Format("Control: %1%%", percent));
		}
	}
}
