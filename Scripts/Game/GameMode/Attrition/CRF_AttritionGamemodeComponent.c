//------------------------------------------------------------------------------------
// CRF_AttritionGamemodeComponent: Force-attrition gamemode for CRF
//
// Each side's total force (slotted players + their spawned vehicles) is tallied
// the moment safestart ends and locked in as that side's 100% pool. Every kill or
// vehicle destruction removes one unit from the victim's side's pool. Whenever a
// side's cumulative losses cross another notification increment (default 10%), an
// on-screen update is broadcast to all clients. A side wins once the opposing side's
// losses reach that side's configured victory threshold.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity.
// 2. Set m_sTeamAFactionKey / m_sTeamBFactionKey to the two factions fighting each other.
// 3. Set m_fTeamAVictoryThreshold / m_fTeamBVictoryThreshold to the destruction
//    percentage each side needs to inflict on the other to win.
// 4. Pools are finalized automatically when safestart ends (or, if the mission does
//    not use safestart, when the gamemode reaches the GAME state).
// 5. To pair Attrition with an objective-based respawn gamemode (Frontline, Rush, ...),
//    set Force Composition Mode to DYNAMIC so respawning players re-enter the pool
//    instead of only ever counting their first death.
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// Governs how the force pool responds to respawns during the match.
enum CRF_EAttritionForceMode
{
	STATIC,		// Pool is fixed at safestart end. Each tracked player/vehicle can only ever
				// count as destroyed once, even if a paired gamemode component respawns them.
	DYNAMIC		// Players (re-)enter the pool the moment they spawn if not already tracked —
				// covers both respawns after an earlier death and latecomers who slot in after
				// finalization — growing that side's pool total by one life each time, so the
				// destroyed/total ratio keeps meaning "cumulative losses so far" instead of
				// stalling after each original unit's first death.
}


//------------------------------------------------------------------------------------------------
// Per-side runtime pool state — server side only.
class CRF_AttritionTeamPool
{
	string m_sFactionKey;

	int m_iTotalUnits;
	int m_iDestroyedUnits;
	int m_iLastNotifiedBracket;

	ref array<int> m_aTrackedPlayerIds = {};
	ref array<ref CRF_AttritionVehicleWatcher> m_aVehicleWatchers = {};

	void CRF_AttritionTeamPool(string factionKey)
	{
		m_sFactionKey = factionKey;
	}
}


//------------------------------------------------------------------------------------------------
// Tracks a single vehicle counted into a pool. Destruction is detected by polling
// GetState() rather than subscribing to a damage-state-changed event — this matches
// the only proven-reliable technique for vehicle destruction elsewhere in CRF
// (see COA_RespawnManager's respawn-timer checks, which poll the same way).
class CRF_AttritionVehicleWatcher
{
	Vehicle m_Vehicle;
	SCR_DamageManagerComponent m_DamageManager;
	protected bool m_bCounted = false;

	// Returns true the first time the tracked vehicle is found destroyed; false otherwise
	// (including on every call after the first, so each vehicle is only ever counted once).
	bool CheckAndConsumeDestroyed()
	{
		if (m_bCounted || !m_DamageManager)
			return false;

		if (m_DamageManager.GetState() != EDamageState.DESTROYED)
			return false;

		m_bCounted = true;
		return true;
	}
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Force-attrition gamemode. Tallies each side's players and vehicles into a 100% pool at the end of safestart, then tracks destruction percentage toward configurable per-side victory thresholds.")]
class CRF_AttritionGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_AttritionGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key for side A.", category: "Attrition - Factions")]
	string m_sTeamAFactionKey;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key for side B.", category: "Attrition - Factions")]
	string m_sTeamBFactionKey;

	[Attribute("100", UIWidgets.Auto, desc: "Percentage of side B's force that must be destroyed for side A to win.", category: "Attrition - Victory")]
	float m_fTeamAVictoryThreshold;

	[Attribute("100", UIWidgets.Auto, desc: "Percentage of side A's force that must be destroyed for side B to win.", category: "Attrition - Victory")]
	float m_fTeamBVictoryThreshold;

	[Attribute("true", UIWidgets.CheckBox, desc: "Automatically records the winning faction and advances the gamemode to AAR once a victory threshold is reached.", category: "Attrition - Victory")]
	bool m_bEndMissionOnVictory;

	[Attribute("10", UIWidgets.Auto, desc: "Minimum percentage increase in a side's losses before an on-screen update is broadcast to all players.", category: "Attrition - HUD")]
	float m_fNotifyIncrementPercent;

	[Attribute("0", UIWidgets.ComboBox, "STATIC: pool is fixed at safestart end; each tracked player/vehicle is destroyed at most once. DYNAMIC: players re-enter the pool on every spawn (respawns and latecomers alike) and grow the pool total, so repeated deaths keep counting — use this when pairing Attrition with an objective-based respawn gamemode (Frontline, Rush).", "", ParamEnumArray.FromEnum(CRF_EAttritionForceMode), category: "Attrition - Pool")]
	CRF_EAttritionForceMode m_eForceCompositionMode;

	[Attribute("true", UIWidgets.CheckBox, desc: "Count slotted players toward each side's force pool.", category: "Attrition - Pool")]
	bool m_bCountPlayers;

	[Attribute("true", UIWidgets.CheckBox, desc: "Count spawned vehicles toward each side's force pool.", category: "Attrition - Pool")]
	bool m_bCountVehicles;

	[Attribute("30", UIWidgets.Auto, desc: "After safestart ends, keep scanning for newly-appeared vehicles for this many seconds and add any found to the appropriate side's pool (their total increases accordingly). Covers vehicles that spawn in around the safestart/game-start transition. Set to 0 to only count vehicles that already exist the instant safestart ends.", category: "Attrition - Pool")]
	float m_fVehicleGracePeriodSeconds;

	//===================================================================================
	// SERVER-SIDE RUNTIME STATE
	//===================================================================================

	protected ref CRF_AttritionTeamPool m_TeamA;
	protected ref CRF_AttritionTeamPool m_TeamB;

	protected bool m_bPoolsFinalized = false;
	protected bool m_bVictoryTriggered = false;
	protected float m_fVehicleGraceRemaining = 0;

	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	//===================================================================================
	// CLIENT-SIDE HUD STATE
	//===================================================================================

	protected Widget m_wCurrentAlert;
	protected int m_iWidgetGeneration = 0; // used to cancel stale fade callbacks

	//===================================================================================
	// STATIC ACCESSOR
	//===================================================================================

	protected static CRF_AttritionGamemodeComponent m_sInstance;

	void CRF_AttritionGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	void ~CRF_AttritionGamemodeComponent()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	static CRF_AttritionGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif

		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif

		m_TeamA = new CRF_AttritionTeamPool(m_sTeamAFactionKey);
		m_TeamB = new CRF_AttritionTeamPool(m_sTeamBFactionKey);

		if (!Replication.IsServer())
			return;

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		// Primary trigger: finalize the moment safestart ends.
		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback trigger for missions that never enable safestart at all.
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		// DYNAMIC mode only — re-tracks respawning players (see OnPlayerRespawned).
		if (m_eForceCompositionMode == CRF_EAttritionForceMode.DYNAMIC && m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Insert(OnPlayerRespawned);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(FadeAlert);
			GetGame().GetCallqueue().Remove(DeleteAlert);
			GetGame().GetCallqueue().Remove(PollVehicleDestruction);
		}

		ClearAlert();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_eForceCompositionMode == CRF_EAttritionForceMode.DYNAMIC && m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Remove(OnPlayerRespawned);

		if (m_sInstance == this)
			m_sInstance = null;

		super.OnDelete(owner);
	}

	//===================================================================================
	// POOL FINALIZATION
	//===================================================================================

	protected void OnSafestartChanged(bool safestartActive)
	{
		// Only care about the transition into "safestart has ended".
		if (safestartActive || m_bPoolsFinalized)
			return;

		FinalizePools();
	}

	protected void OnGamemodeStateChanged()
	{
		if (m_bPoolsFinalized || !m_Gamemode)
			return;

		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			FinalizePools();
	}

	protected void FinalizePools()
	{
		m_bPoolsFinalized = true;

		PopulateTeamPlayers(m_TeamA);
		PopulateTeamPlayers(m_TeamB);

		if (m_bCountVehicles)
		{
			m_fVehicleGraceRemaining = m_fVehicleGracePeriodSeconds;
			ScanForVehicles(true);
		}

		Print(string.Format("[CRF_Attrition] Pools finalized — %1: %2 units (%3 players, %4 vehicles), %5: %6 units (%7 players, %8 vehicles)",
			m_TeamA.m_sFactionKey, m_TeamA.m_iTotalUnits, m_TeamA.m_aTrackedPlayerIds.Count(), m_TeamA.m_aVehicleWatchers.Count(),
			m_TeamB.m_sFactionKey, m_TeamB.m_iTotalUnits, m_TeamB.m_aTrackedPlayerIds.Count(), m_TeamB.m_aVehicleWatchers.Count()), LogLevel.NORMAL);

		if (m_TeamA.m_iTotalUnits == 0)
			Print(string.Format("[CRF_Attrition] WARNING: %1 has 0 tracked units — check the faction key and that players/vehicles exist for this side.", m_TeamA.m_sFactionKey), LogLevel.WARNING);

		if (m_TeamB.m_iTotalUnits == 0)
			Print(string.Format("[CRF_Attrition] WARNING: %1 has 0 tracked units — check the faction key and that players/vehicles exist for this side.", m_TeamB.m_sFactionKey), LogLevel.WARNING);

		if (m_bCountVehicles)
			GetGame().GetCallqueue().CallLater(PollVehicleDestruction, 1000, true);
	}

	// Polls every tracked vehicle once a second for its destroyed state, and — for a
	// bounded grace period after finalization — also rescans for newly-appeared vehicles
	// (see ScanForVehicles). See CRF_AttritionVehicleWatcher for why destruction is
	// polled rather than event-driven.
	protected void PollVehicleDestruction()
	{
		if (m_bVictoryTriggered)
		{
			GetGame().GetCallqueue().Remove(PollVehicleDestruction);
			return;
		}

		PollPoolVehicles(m_TeamA);
		PollPoolVehicles(m_TeamB);

		if (m_fVehicleGraceRemaining > 0)
		{
			m_fVehicleGraceRemaining -= 1.0;
			ScanForVehicles(false);
		}
	}

	protected void PollPoolVehicles(notnull CRF_AttritionTeamPool pool)
	{
		foreach (CRF_AttritionVehicleWatcher watcher : pool.m_aVehicleWatchers)
		{
			if (watcher.CheckAndConsumeDestroyed())
				RegisterDestruction(pool);
		}
	}

	protected void PopulateTeamPlayers(notnull CRF_AttritionTeamPool pool)
	{
		pool.m_aTrackedPlayerIds.Clear();
		pool.m_aVehicleWatchers.Clear();
		pool.m_iDestroyedUnits = 0;
		pool.m_iLastNotifiedBracket = 0;
		pool.m_iTotalUnits = 0;

		if (m_bCountPlayers)
		{
			COA_SlottingManager slotMgr = COA_SlottingManager.GetInstance();
			if (slotMgr)
			{
				map<int, ref COA_SlotData> slots = slotMgr.GetSlotMap();
				foreach (int slotId, COA_SlotData slotData : slots)
				{
					if (!slotData || slotData.GetSlotCurrentPlayerId() <= 0)
						continue;

					if (slotData.GetSlotFactionKey() != pool.m_sFactionKey)
						continue;

					pool.m_aTrackedPlayerIds.Insert(slotData.GetSlotCurrentPlayerId());
				}
			}
		}

		pool.m_iTotalUnits = pool.m_aTrackedPlayerIds.Count();
	}

	// DYNAMIC mode only (see CRF_EAttritionForceMode) — subscribed in EOnInit to the
	// gamemode's own player-spawned event, so no cooperation from whatever respawn/objective
	// component is granting the spawn (Frontline, Rush, ...) is required. Fires on every
	// spawn, including each player's first, but re-tracking is a no-op if already tracked —
	// so this only actually does something the moment a player spawns without being counted:
	// either a respawn after an earlier death, or a latecomer slotting in post-finalization.
	// Either way they become destroyable again and the pool's total grows by one life, which
	// is what keeps destroyedUnits/totalUnits meaningful instead of stalling at each unit's
	// first death.
	protected void OnPlayerRespawned(int playerId, IEntity controlledEntity)
	{
		if (!Replication.IsServer() || !m_bPoolsFinalized || m_bVictoryTriggered || !m_bCountPlayers)
			return;

		CRF_AttritionTeamPool pool = ResolvePlayerFactionPool(playerId);
		if (!pool || pool.m_aTrackedPlayerIds.Contains(playerId))
			return;

		pool.m_aTrackedPlayerIds.Insert(playerId);
		pool.m_iTotalUnits++;
	}

	protected CRF_AttritionTeamPool ResolvePlayerFactionPool(int playerId)
	{
		COA_SlottingManager slotMgr = COA_SlottingManager.GetInstance();
		if (!slotMgr)
			return null;

		COA_SlotData slotData = slotMgr.GetPlayerSlotData(playerId);
		if (!slotData)
			return null;

		string factionKey = slotData.GetSlotFactionKey();
		if (factionKey == m_TeamA.m_sFactionKey)
			return m_TeamA;
		if (factionKey == m_TeamB.m_sFactionKey)
			return m_TeamB;

		return null;
	}

	// Finds every Vehicle-typed entity in the world directly via a (very large) sphere
	// query, rather than trusting CRF_VehicleGearscriptManager's spawned-vehicle array.
	// Vehicles placed directly in the world only self-register into that array from
	// their own constructor, which fires the instant the entity is created — if that
	// happens before CRF_VehicleGearscriptManager itself has finished initializing
	// (a real race for world-placed entities, whose construction order relative to the
	// gamemode entity isn't guaranteed), the vehicle is silently never added.
	//
	// Called once at finalization and then every second for m_fVehicleGracePeriodSeconds
	// afterward, to also catch vehicles that spawn in around the safestart/game-start
	// transition. Already-tracked vehicles (checked via IsVehicleTracked) are skipped, so
	// repeated calls only ever add newly-appeared vehicles — each addition increases the
	// owning pool's total, since it represents force that genuinely wasn't there before.
	protected ref array<IEntity> m_aQueriedEntities;

	protected void ScanForVehicles(bool verbose)
	{
		m_aQueriedEntities = {};
		GetGame().GetWorld().QueryEntitiesBySphere(vector.Zero, 100000, CollectVehicleCallback, null, EQueryEntitiesFlags.ALL);

		if (verbose)
			Print(string.Format("[CRF_Attrition] World scan found %1 candidate entity/entities.", m_aQueriedEntities.Count()), LogLevel.NORMAL);

		foreach (IEntity ent : m_aQueriedEntities)
		{
			Vehicle vehicle = Vehicle.Cast(ent);
			if (!vehicle)
				continue;

			string factionKey = ResolveVehicleFactionKey(vehicle);

			CRF_AttritionTeamPool pool;
			if (factionKey == m_TeamA.m_sFactionKey)
				pool = m_TeamA;
			else if (factionKey == m_TeamB.m_sFactionKey)
				pool = m_TeamB;
			else
			{
				if (verbose)
					Print(string.Format("[CRF_Attrition] Skipping vehicle %1 — no explicit FactionAffiliationComponent match for either side (%2 / %3). Add one set to the correct faction in the editor to have it counted.",
						vehicle.GetPrefabData().GetPrefabName(), m_TeamA.m_sFactionKey, m_TeamB.m_sFactionKey), LogLevel.NORMAL);
				continue;
			}

			if (IsVehicleTracked(pool, vehicle))
				continue;

			SCR_DamageManagerComponent dmgMgr = FindVehicleDamageManager(vehicle);
			if (!dmgMgr)
			{
				Print(string.Format("[CRF_Attrition] WARNING: Vehicle %1 (faction %2) has no damage manager component and will not be tracked.",
					vehicle.GetPrefabData().GetPrefabName(), factionKey), LogLevel.WARNING);
				continue;
			}

			CRF_AttritionVehicleWatcher watcher = new CRF_AttritionVehicleWatcher();
			watcher.m_Vehicle = vehicle;
			watcher.m_DamageManager = dmgMgr;
			pool.m_aVehicleWatchers.Insert(watcher);
			pool.m_iTotalUnits++;

			Print(string.Format("[CRF_Attrition] Added vehicle %1 to the %2 pool — new total %3 units.",
				vehicle.GetPrefabData().GetPrefabName(), pool.m_sFactionKey, pool.m_iTotalUnits), LogLevel.NORMAL);
		}

		m_aQueriedEntities = null;
	}

	protected bool CollectVehicleCallback(IEntity ent)
	{
		if (Vehicle.Cast(ent))
			m_aQueriedEntities.Insert(ent);

		return true;
	}

	protected bool IsVehicleTracked(notnull CRF_AttritionTeamPool pool, Vehicle vehicle)
	{
		foreach (CRF_AttritionVehicleWatcher watcher : pool.m_aVehicleWatchers)
		{
			if (watcher.m_Vehicle == vehicle)
				return true;
		}

		return false;
	}

	// Trusted, in priority order:
	//  1. An explicit FactionAffiliationComponent set directly on the vehicle in the
	//     editor — the standard, deliberate way mission makers assign entity ownership.
	//  2. m_sFactionKey when the vehicle was spawned via COA_VehicleSpawner. That path
	//     (CRF_VehicleGearscriptManager.SetVehicle()) copies the spawner's own explicit
	//     faction attribute directly onto the vehicle — a deliberate assignment, not a
	//     guess. m_iVehicleSpawnerIndex is only ever set (>= 0) by that same path, so it
	//     doubles as a reliable "this came from a spawner" marker.
	// NOT trusted: CRF's proximity-based m_sFactionKey heuristic for world-placed
	// vehicles (nearest player within 200m, via FindFactionByClosestPlayer) — it produced
	// wrong-side attributions right at the safestart-end instant, when player positions
	// don't yet reliably reflect ownership. Silently excluding an untagged, non-spawner
	// vehicle is far safer than guessing and corrupting the score.
	protected string ResolveVehicleFactionKey(notnull Vehicle vehicle)
	{
		FactionKey affiliatedKey = COA_EntityHelper.DetermineFactionKey(vehicle);
		if (!affiliatedKey.IsEmpty() && affiliatedKey != "CIV")
			return affiliatedKey;

		if (vehicle.m_iVehicleSpawnerIndex >= 0 && !vehicle.m_sFactionKey.IsEmpty())
			return vehicle.m_sFactionKey;

		return affiliatedKey; // "CIV" or empty — no explicit affiliation and not spawner-assigned
	}

	// Vehicles use SCR_VehicleDamageManagerComponent (see COA_RespawnManager's respawn-timer
	// checks), a subclass of SCR_DamageManagerComponent, rather than the generic component
	// found on plain destructible props. FindComponent() only matches the exact concrete type
	// attached to the entity, so the vehicle-specific type has to be searched for directly —
	// searching for the generic base type finds nothing on any vehicle.
	protected SCR_DamageManagerComponent FindVehicleDamageManager(notnull Vehicle vehicle)
	{
		SCR_DamageManagerComponent dmgMgr = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (dmgMgr)
			return dmgMgr;

		// Fallback for any non-standard vehicle prefab using the generic component directly.
		return SCR_DamageManagerComponent.Cast(vehicle.FindComponent(SCR_DamageManagerComponent));
	}

	//===================================================================================
	// DESTRUCTION TRACKING
	//===================================================================================

	// Called once per confirmed kill by a player originally counted into a pool.
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || !m_bPoolsFinalized || m_bVictoryTriggered || !m_bCountPlayers)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0)
			return;

		if (m_TeamA.m_aTrackedPlayerIds.Contains(victimId))
		{
			m_TeamA.m_aTrackedPlayerIds.RemoveItem(victimId);
			RegisterDestruction(m_TeamA);
		}
		else if (m_TeamB.m_aTrackedPlayerIds.Contains(victimId))
		{
			m_TeamB.m_aTrackedPlayerIds.RemoveItem(victimId);
			RegisterDestruction(m_TeamB);
		}
	}

	// Called by a CRF_AttritionVehicleWatcher when its tracked vehicle is destroyed,
	// and directly above for player kills. 'pool' is the side that just lost a unit.
	void RegisterDestruction(CRF_AttritionTeamPool pool)
	{
		if (!pool || !m_bPoolsFinalized || m_bVictoryTriggered)
			return;

		if (pool.m_iTotalUnits <= 0)
			return;

		// Clamped at the source rather than just at display time, so m_iDestroyedUnits itself
		// can never exceed m_iTotalUnits — e.g. if a kill and a vehicle-destruction event both
		// land for the same unit, or DYNAMIC-mode re-tracking races a destruction on the same
		// tick. Every downstream percent computation inherits the <=100% guarantee for free.
		pool.m_iDestroyedUnits = Math.Min(pool.m_iDestroyedUnits + 1, pool.m_iTotalUnits);

		float destroyedPercent = (float)pool.m_iDestroyedUnits / (float)pool.m_iTotalUnits * 100.0;

		int bracket = 0;
		if (m_fNotifyIncrementPercent > 0)
			bracket = Math.Floor(destroyedPercent / m_fNotifyIncrementPercent);

		if (bracket > pool.m_iLastNotifiedBracket)
		{
			pool.m_iLastNotifiedBracket = bracket;
			BroadcastUpdate();
		}

		CheckVictory(pool, destroyedPercent);
	}

	protected void CheckVictory(notnull CRF_AttritionTeamPool losingPool, float destroyedPercent)
	{
		CRF_AttritionTeamPool winningPool;
		float threshold;

		if (losingPool == m_TeamB)
		{
			winningPool = m_TeamA;
			threshold = m_fTeamAVictoryThreshold;
		}
		else
		{
			winningPool = m_TeamB;
			threshold = m_fTeamBVictoryThreshold;
		}

		if (threshold <= 0 || destroyedPercent < threshold)
			return;

		m_bVictoryTriggered = true;

		Rpc(RpcDo_BroadcastMessage, string.Format("%1 forces have been reduced by %2%%! %3 achieves victory!",
			losingPool.m_sFactionKey, Math.Round(destroyedPercent).ToString(), winningPool.m_sFactionKey));

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winningPool.m_sFactionKey, "automatic");

		if (m_bEndMissionOnVictory && m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	protected void BroadcastUpdate()
	{
		float percentA = 0;
		if (m_TeamA.m_iTotalUnits > 0)
			percentA = (float)m_TeamA.m_iDestroyedUnits / (float)m_TeamA.m_iTotalUnits * 100.0;

		float percentB = 0;
		if (m_TeamB.m_iTotalUnits > 0)
			percentB = (float)m_TeamB.m_iDestroyedUnits / (float)m_TeamB.m_iTotalUnits * 100.0;

		Rpc(RpcDo_ShowAttritionUpdate, percentA, percentB);
	}

	//===================================================================================
	// RPCs — sent by server, received by all clients
	//===================================================================================

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_BroadcastMessage(string message)
	{
		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (notification)
			notification.PopupMsg(message);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowAttritionUpdate(float percentADestroyed, float percentBDestroyed)
	{
		if (m_wCurrentAlert)
			ClearAlert();

		m_iWidgetGeneration++;
		int thisGeneration = m_iWidgetGeneration;

		if (!GetGame() || !GetGame().GetWorkspace())
			return;

		m_wCurrentAlert = GetGame().GetWorkspace().CreateWidgets("{9B3F2A1C7D8E4B56}UI/layouts/HUD/Attrition/CRF_AttritionPopUp.layout");
		if (!m_wCurrentAlert)
			return;

		m_wCurrentAlert.SetOpacity(0);
		AnimateWidget.Opacity(m_wCurrentAlert, 1, 0.5);

		UpdateTeamRow(m_wCurrentAlert, "TeamALabel", "TeamAPercent", "TeamAProgress",
			m_sTeamAFactionKey, percentADestroyed, m_fTeamBVictoryThreshold);

		UpdateTeamRow(m_wCurrentAlert, "TeamBLabel", "TeamBPercent", "TeamBProgress",
			m_sTeamBFactionKey, percentBDestroyed, m_fTeamAVictoryThreshold);

		// Fade out after 5 seconds, but only if no newer alert has appeared
		GetGame().GetCallqueue().CallLater(FadeAlert, 5000, false, thisGeneration);
	}

	// Fills in one team's row of the popup. 'destroyedPercent' is how much of THIS team
	// has been destroyed; 'opposingVictoryThreshold' is the destruction % of THIS team
	// that grants the OTHER side victory, used to make the bar threshold-relative.
	protected void UpdateTeamRow(notnull Widget root, string labelWidgetName, string percentWidgetName, string progressWidgetName,
		string factionKey, float destroyedPercent, float opposingVictoryThreshold)
	{
		TextWidget labelWidget = TextWidget.Cast(root.FindWidget(labelWidgetName));
		if (labelWidget)
			labelWidget.SetText(factionKey);

		TextWidget percentWidget = TextWidget.Cast(root.FindWidget(percentWidgetName));
		if (percentWidget)
			percentWidget.SetText(string.Format("%1%% destroyed", Math.Round(destroyedPercent).ToString()));

		ProgressBarWidget progressWidget = ProgressBarWidget.Cast(root.FindWidget(progressWidgetName));
		if (progressWidget)
		{
			float barPercent = 0;
			if (opposingVictoryThreshold > 0)
				barPercent = destroyedPercent / opposingVictoryThreshold * 100.0;

			progressWidget.SetCurrent(Math.Clamp(barPercent, 0, 100));
			progressWidget.SetColorInt(GetColorForFactionKey(factionKey));
		}
	}

	// Standard CRF faction color convention (matches COA_SlottingMenu.GetFactionColor):
	// BLUFOR = blue, OPFOR = red, INDFOR = green, CIV = purple.
	protected int GetColorForFactionKey(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return ARGB(255, 0, 20, 255);
			case "OPFOR":  return ARGB(255, 188, 0, 0);
			case "INDFOR": return ARGB(255, 0, 145, 43);
			case "CIV":    return ARGB(255, 137, 0, 188);
		}

		return ARGB(255, 130, 130, 130); // fallback grey for unrecognized/custom faction keys
	}

	//===================================================================================
	// CLIENT — HUD ALERT HELPERS
	//===================================================================================

	protected void FadeAlert(int generation)
	{
		// A newer alert has since been shown — let that one manage its own fade
		if (generation != m_iWidgetGeneration || !m_wCurrentAlert)
			return;

		AnimateWidget.Opacity(m_wCurrentAlert, 0, 1.0);
		GetGame().GetCallqueue().CallLater(DeleteAlert, 1100, false, generation);
	}

	protected void DeleteAlert(int generation)
	{
		if (generation != m_iWidgetGeneration || !m_wCurrentAlert)
			return;

		ClearAlert();
	}

	protected void ClearAlert()
	{
		Widget alert = m_wCurrentAlert;
		m_wCurrentAlert = null;
		m_iWidgetGeneration++;

		if (!alert)
			return;

		AnimateWidget.StopAllAnimations(alert);
		alert.RemoveFromHierarchy();
	}
}
