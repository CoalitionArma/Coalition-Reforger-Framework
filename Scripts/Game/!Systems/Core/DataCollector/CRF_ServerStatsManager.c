/*
//! CRF Player Stats Manager
//!
//! Replacement for the vanilla SCR_DataCollectorComponent stats pipeline.
//! Tracks per-player stats independently using entity-level invokers and
//! SCR_BaseGameModeComponent lifecycle hooks.
//!
//!
//! Log line format (one line per player):
//!   player_stat,<guid>,<kills>,<ai_kills>,<deaths>,<shots>,<grenades_thrown>,
//!   <friendly_kills>,<friendly_ai_kills>,<distance_walked>,<distance_driven>,
//!   <roadkills>,<friendly_roadkills>,<ai_roadkills>,<friendly_ai_roadkills>,
//!   <distance_as_occupant>,<bandage_self>,<bandage_friendlies>,
//!   <tourniquet_self>,<tourniquet_friendlies>,<saline_self>,<saline_friendlies>,
//!   <morphine_self>,<morphine_friendlies>,<session_duration>,<level_experience>,
//!   <winner>
//!
//! Server-authority only.  No replication.
*/

//------------------------------------------------------------------------------------------------
//! Per-player stats data class.
class CRF_PlayerStats
{
	string guid;
	string playerName;
	string faction;

	// Kill / death counters
	int kills;
	int ai_kills;
	int deaths;
	int friendly_kills;
	int friendly_ai_kills;
	int roadkills;
	int friendly_roadkills;
	int ai_roadkills;
	int friendly_ai_roadkills;

	// Combat actions
	int shots;
	int grenades_thrown;

	// Medical actions
	int bandage_self;
	int bandage_friendlies;
	int tourniquet_self;
	int tourniquet_friendlies;
	int saline_self;
	int saline_friendlies;
	int morphine_self;
	int morphine_friendlies;

	// Distance tracking
	float distance_walked;       // metres, on foot
	float distance_driven;       // metres, as vehicle driver
	float distance_as_occupant;  // metres, as non-driver occupant

	// Session timing (world time in ms at spawn)
	float session_start;
	float session_duration; // seconds, calculated at flush

	// Calculated XP for this session (after winner bonus)
	int level_experience;

	// Whether this player was on the winning faction
	bool winner;

	// True once stats have been flushed to the log file — prevents double-writes
	bool flushed;
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "CRF Data Collection", description: "CRF-native player stats tracker. Replaces vanilla SCR_DataCollectorComponent for DB logging")]
class CRF_ServerStatsManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_ServerStatsManager : SCR_BaseGameModeComponent
{
	// ── XP weights (base, per unit of that stat) ────────────────────────────
	const float XP_KILL                    = 500.0;
	const float XP_AI_KILL                 = 100.0;
	const float XP_BANDAGE_SELF            = 20.0;
	const float XP_BANDAGE_ALLY            = 50.0;
	const float XP_TOURNIQUET_SELF         = 25.0;
	const float XP_TOURNIQUET_ALLY         = 60.0;
	const float XP_SALINE_SELF             = 30.0;
	const float XP_SALINE_ALLY             = 70.0;
	const float XP_MORPHINE_SELF           = 25.0;
	const float XP_MORPHINE_ALLY           = 60.0;
	const float XP_FRIENDLY_KILL_PENALTY   = -300.0;
	const float XP_FRIENDLY_AI_PENALTY     = -100.0;
	// Multiplier applied to winners' total XP (1.25 = 25 % bonus)
	const float XP_WINNER_MULT             = 1.25;

	// Max walking speed used to sanity-check per-frame distance increments (m/s)
	const float WALK_SPEED_CLAMP           = 10.0;
	// Update period for distance tracking (seconds)
	const float DISTANCE_UPDATE_PERIOD     = 10.0;
	const int HOOK_RETRY_DELAY_MS          = 250;
	const int HOOK_RETRY_MAX_ATTEMPTS      = 12;

	// ── Private state ────────────────────────────────────────────────────────
	private ref map<int, ref CRF_PlayerStats> m_mPlayerStats   = new map<int, ref CRF_PlayerStats>();

	// Players currently on foot → their controlled entity (for velocity-based distance_walked)
	private ref map<int, IEntity> m_mWalkingPlayers            = new map<int, IEntity>();
	// Players currently driving → reference to their vehicle entity
	private ref map<int, IEntity> m_mDriverVehicle             = new map<int, IEntity>();
	// Non-driver occupants → reference to their vehicle entity
	private ref map<int, IEntity> m_mOccupantVehicle           = new map<int, IEntity>();

	// Accumulator for the distance-update tick
	private float m_fDistanceTick = 0.0;

	// Scratch buffer for stale tracking entries found during a distance tick. Reused across ticks
	// so the hot path allocates nothing; collected during iteration and drained afterwards so the
	// maps are never mutated while being indexed.
	private ref array<int> m_aStaleTrackingIds = {};

	// ── AAR kill/death name tracking for the client-side kill panel ──────────
	// Populated in OnPlayerKilled; sent to each client via SendAARKillStats at game end.
	private ref map<int, ref array<string>> m_mSessionKills  = new map<int, ref array<string>>();
	private ref map<int, string>            m_mSessionDeaths = new map<int, string>();

	// Staggered AAR send queue (one entry per connected player).
	private ref array<int> m_aPendingAARSends = {};

	// Guard against double-flush if both CRF state transition and engine OnGameModeEnd fire.
	private bool m_bMissionEndTriggered = false;

	// Singleton
	private static CRF_ServerStatsManager s_Instance;

	//------------------------------------------------------------------------------------------------
	//! Singleton accessor (server-side only).
	static CRF_ServerStatsManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	void CRF_ServerStatsManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
		Print("[CRF_ServerStatsManager] Initialized", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_ServerStatsManager()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Drop every raw-entity-pointer reference held for this player.
	//! MUST be called from any path that ends this player's association with their current entity
	//! (death, disconnect, respawn), because these maps hold non-refcounted IEntity pointers that
	//! EOnFrame dereferences on a timer. See the comment in OnPlayerKilled for the failure mode.
	protected void StopDistanceTracking(int playerId)
	{
		m_mWalkingPlayers.Remove(playerId);
		m_mDriverVehicle.Remove(playerId);
		m_mOccupantVehicle.Remove(playerId);
	}

	//==============================================================================================
	// PUBLIC ENTRY POINTS
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called from COA_GamemodeManager.InitilizePlayer for every non-spectator player.
	//! On first spawn, creates a fresh stats record.
	//! On respawn (existing non-flushed record), preserves accumulated stats and re-wires
	//! entity-level hooks to the new character entity so distance and action tracking continues.
	void NotifyPlayerSpawned(int playerId, IEntity playerEntity)
	{
		if (!playerEntity || playerId <= 0)
			return;

		CRF_PlayerStats existingStats = m_mPlayerStats.Get(playerId);
		bool isRespawn = (existingStats && !existingStats.flushed);

		if (isRespawn)
		{
			Print(string.Format("[CRF_ServerStatsManager] Respawn — continuing stats for player %1 (ID: %2)",
				GetGame().GetPlayerManager().GetPlayerName(playerId), playerId), LogLevel.VERBOSE);

			// The old entity is dead/destroyed; its EHM hooks are auto-cleaned by the engine.
			// Clear the distance-tracking maps so EOnFrame stops referencing the dead entity and
			// we can re-register on the new one below. OnPlayerKilled already does this at the
			// moment of death; this is the belt-and-braces path for spawns that arrive without a
			// preceding kill event (e.g. admin-forced respawn, slot change).
			StopDistanceTracking(playerId);
		}
		else
		{
			Print(string.Format("[CRF_ServerStatsManager] Tracking stats for player %1 (ID: %2)",
				GetGame().GetPlayerManager().GetPlayerName(playerId), playerId), LogLevel.VERBOSE);

			// Initialise a fresh stats record (first spawn, or fresh session after flush).
			CRF_PlayerStats stats = new CRF_PlayerStats();
			stats.playerName    = GetGame().GetPlayerManager().GetPlayerName(playerId);
			stats.guid          = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			stats.session_start = GetGame().GetWorld().GetWorldTime(); // ms

			// Resolve faction from slotting manager (authoritative at spawn time).
			COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
			if (slottingManager)
			{
				Faction f = slottingManager.GetPlayerSlotFaction(playerId, true);
				if (f)
					stats.faction = f.GetFactionKey();
			}

			m_mPlayerStats.Set(playerId, stats);
		}

		// Start tracking on-foot distance with the new entity.
		m_mWalkingPlayers.Set(playerId, playerEntity);

		// Attach entity-level invokers for shots, grenades, healing, compartment.
		// Retry briefly in case character subcomponents are not ready on the same frame.
		RegisterEntityHooksWithRetry(playerId, playerEntity, 0);
	}

	//==============================================================================================
	// SCR_BaseGameModeComponent OVERRIDES
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	//! Enable frame updates - required for distance tracking (EOnFrame is otherwise never called).
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	//! Distance tracking - delayed frame updates
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Only run on server authority.
		if (RplSession.Mode() == RplMode.Client)
			return;

		m_fDistanceTick += timeSlice;
		if (m_fDistanceTick < DISTANCE_UPDATE_PERIOD)
			return;

		float dt = m_fDistanceTick;
		m_fDistanceTick = 0.0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// NOTE ON POINTER SAFETY
		// The three tracking maps hold raw IEntity pointers, which the engine does not clear when
		// the entity is deleted - a null check does NOT prove a stored pointer is still live. Every
		// loop below therefore re-resolves the entity from the authoritative PlayerManager and only
		// ever dereferences the freshly-resolved pointer. The stored value is used exclusively for
		// pointer-identity comparison (safe - no dereference) and self-healing removal, so a missed
		// cleanup path degrades to a dropped stat rather than a use-after-free.
		// m_aStaleTrackingIds is reused across ticks to avoid a per-tick allocation; entries are
		// collected during iteration and removed afterwards so we never mutate a map mid-loop.

		// ── On-foot distance ────────────────────────────────────────────────
		m_aStaleTrackingIds.Clear();
		for (int wi = 0; wi < m_mWalkingPlayers.Count(); wi++)
		{
			int playerId   = m_mWalkingPlayers.GetKey(wi);
			IEntity stored = m_mWalkingPlayers.GetElement(wi);

			// Pointer-identity check only. If the player has since died, disconnected or been
			// reassigned, the live entity differs (or is null) and the stale entry is dropped.
			IEntity live = playerManager.GetPlayerControlledEntity(playerId);
			if (!live || live != stored)
			{
				m_aStaleTrackingIds.Insert(playerId);
				continue;
			}

			// A player riding in a vehicle is credited by the driver/occupant loops below.
			if (CompartmentAccessComponent.GetVehicleIn(live))
				continue;

			Physics phys = live.GetPhysics();
			if (!phys)
				continue;

			float dist = phys.GetVelocity().Length() * dt;
			if (dist > WALK_SPEED_CLAMP * dt)
				dist = WALK_SPEED_CLAMP * dt;

			CRF_PlayerStats stats = m_mPlayerStats.Get(playerId);
			if (stats)
				stats.distance_walked += dist;
		}
		foreach (int staleWalkId : m_aStaleTrackingIds)
			m_mWalkingPlayers.Remove(staleWalkId);

		// ── Driver distance ──────────────────────────────────────────────────
		m_aStaleTrackingIds.Clear();
		for (int di = 0; di < m_mDriverVehicle.Count(); di++)
		{
			int playerId = m_mDriverVehicle.GetKey(di);

			float driven;
			if (!TryGetOccupiedVehicleSpeed(playerManager, playerId, driven))
			{
				m_aStaleTrackingIds.Insert(playerId);
				continue;
			}

			CRF_PlayerStats stats = m_mPlayerStats.Get(playerId);
			if (stats)
				stats.distance_driven += driven * dt;
		}
		foreach (int staleDriverId : m_aStaleTrackingIds)
			m_mDriverVehicle.Remove(staleDriverId);

		// ── Occupant distance ────────────────────────────────────────────────
		m_aStaleTrackingIds.Clear();
		for (int oi = 0; oi < m_mOccupantVehicle.Count(); oi++)
		{
			int playerId = m_mOccupantVehicle.GetKey(oi);

			float ridden;
			if (!TryGetOccupiedVehicleSpeed(playerManager, playerId, ridden))
			{
				m_aStaleTrackingIds.Insert(playerId);
				continue;
			}

			CRF_PlayerStats stats = m_mPlayerStats.Get(playerId);
			if (stats)
				stats.distance_as_occupant += ridden * dt;
		}
		foreach (int staleOccupantId : m_aStaleTrackingIds)
			m_mOccupantVehicle.Remove(staleOccupantId);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve the vehicle a player is currently riding in, straight from the engine, and report its
	//! speed. Returns false if the player is gone, dead, or no longer in a vehicle - in which case
	//! the caller should drop its tracking entry.
	//! Deliberately takes nothing from the tracking maps: the whole point is to avoid dereferencing
	//! a stored vehicle pointer that SCR_GarbageSystem may already have collected.
	protected bool TryGetOccupiedVehicleSpeed(notnull PlayerManager playerManager, int playerId, out float speed)
	{
		speed = 0.0;

		IEntity character = playerManager.GetPlayerControlledEntity(playerId);
		if (!character)
			return false;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
		if (!vehicle)
			return false;

		Physics phys = vehicle.GetPhysics();
		if (!phys)
			return false;

		speed = phys.GetVelocity().Length();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Called by the gamemode when a player or AI controllable is destroyed.
	override protected void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		int victimPlayerId = instigatorContextData.GetVictimPlayerID();
		IEntity victimEntity = instigatorContextData.GetVictimEntity();
		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		Instigator instigator = instigatorContextData.GetInstigator();

		// AI victim - route to AI-kill handler
		if (victimPlayerId <= 0)
		{
			HandleAIKill(victimEntity, killerEntity, instigator, instigatorContextData);
			return;
		}

		// Stop distance tracking immediately on death. These maps hold raw IEntity pointers, which
		// are NOT cleared when the engine deletes the entity - and `if (!entity)` in EOnFrame does
		// not detect a deleted entity, only one that was never set. The corpse is removed by
		// SCR_GarbageSystem on its own timer, independent of whether the player has respawned, so
		// waiting for NotifyPlayerSpawned() to clear these leaves EOnFrame dereferencing freed
		// memory every DISTANCE_UPDATE_PERIOD for any player who dies and does not respawn before
		// GC (wave respawn, spectating, AFK after death, elimination modes, end of mission).
		StopDistanceTracking(victimPlayerId);

		// Record death for the victim
		CRF_PlayerStats victimStats = m_mPlayerStats.Get(victimPlayerId);
		if (victimStats)
			victimStats.deaths++;

		// Record kill / friendly-kill for the killer.
		int killerPlayerId = 0;
		if (killerEntity)
			killerPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity);
		if (killerPlayerId <= 0 && instigator)
			killerPlayerId = instigator.GetInstigatorPlayerID();
		if (killerPlayerId <= 0)
			killerPlayerId = instigatorContextData.GetKillerPlayerID();

		if (killerPlayerId <= 0 || killerPlayerId == victimPlayerId)
			return;

		CRF_PlayerStats killerStats = m_mPlayerStats.Get(killerPlayerId);
		if (!killerStats)
			return;

		// Determine faction relationship via the context data flags.
		bool isFriendlyKill = instigatorContextData.HasAnyVictimKillerRelation(
			SCR_ECharacterDeathStatusRelations.KILLED_BY_FRIENDLY_PLAYER);

		if (isFriendlyKill)
			killerStats.friendly_kills++;
		else
			killerStats.kills++;

		// Track player names for the AAR kill panel 
		// Record victim name under the killer's kills list.
		string victimName = GetGame().GetPlayerManager().GetPlayerName(victimPlayerId);
		if (!m_mSessionKills.Contains(killerPlayerId))
			m_mSessionKills.Insert(killerPlayerId, new array<string>());
		m_mSessionKills.Get(killerPlayerId).Insert(victimName);

		// Record killer name as the last person to kill the victim.
		string killerName = GetGame().GetPlayerManager().GetPlayerName(killerPlayerId);
		if (m_mSessionDeaths.Contains(victimPlayerId))
			m_mSessionDeaths.Set(victimPlayerId, killerName);
		else
			m_mSessionDeaths.Insert(victimPlayerId, killerName);
	}

	//------------------------------------------------------------------------------------------------
	//! Called at the end of the game mode - flush all unflushed players.
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		NotifyMissionEnded();
	}

	//------------------------------------------------------------------------------------------------
	//! Called from CRF_COA_Gamemode when a new round begins (COA_EGamemodeState.SLOTTING).
	//! This component is a persistent singleton that outlives many SLOTTING/GAME/AAR cycles
	//! within the same mission load, so the one-shot m_bMissionEndTriggered guard and the
	//! session-scoped kill/death tracking MUST be cleared here - otherwise NotifyMissionEnded()
	//! silently no-ops for every round after the first, and stale kill/death names from the
	//! previous round bleed into this round's AAR kill panel.
	void ResetForNewRound()
	{
		m_bMissionEndTriggered = false;
		m_mSessionKills.Clear();
		m_mSessionDeaths.Clear();
		m_aPendingAARSends.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Flush all remaining player stats to the log and queue in-game AAR data sends.
	//! Called either from COA_Gamemode when transitioning to AAR state, or from OnGameModeEnd as
	//! a fallback. The one-shot flag prevents double-work if both paths fire.
	void NotifyMissionEnded()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;

		if (m_bMissionEndTriggered)
			return;
		m_bMissionEndTriggered = true;

		Print(string.Format("[CRF_ServerStatsManager] Mission ended — flushing stats for %1 players", m_mPlayerStats.Count()), LogLevel.NORMAL);

		// Determine the winning faction (sourced from CRF_LoggingManager).
		FactionKey winningFaction = "";
		CRF_LoggingManager lm = CRF_LoggingManager.GetInstance();
		if (lm)
			winningFaction = lm.GetWinningFaction();

		// Update faction on each player record (faction may have changed during mission,
		// e.g. player re-slotted), then flush.
		for (int i = 0; i < m_mPlayerStats.Count(); i++)
		{
			int playerId    = m_mPlayerStats.GetKey(i);
			CRF_PlayerStats stats = m_mPlayerStats.GetElement(i);
			if (!stats || stats.flushed)
				continue;

			// Refresh faction from slotting manager.
			UpdatePlayerFaction(playerId, stats);

			// Mark winners.
			stats.winner = (winningFaction != "" && stats.faction == winningFaction);

			FlushPlayerStats(playerId, stats);
		}

		// Build the staggered AAR send queue for connected players.
		// SendAARStats and SendAARKillStats are dispatched 500 ms apart per player.
		m_aPendingAARSends.Clear();
		PlayerManager pm = GetGame().GetPlayerManager();
		array<int> connectedPlayers = {};
		pm.GetAllPlayers(connectedPlayers);
		foreach (int pid : connectedPlayers)
		{
			if (!pm.IsPlayerConnected(pid))
				continue;
			PlayerController pc = pm.GetPlayerController(pid);
			if (!pc)
				continue;
			COA_PlayerRplToOwnerManager rplManager = COA_PlayerRplToOwnerManager.Cast(pc.FindComponent(COA_PlayerRplToOwnerManager));
			if (!rplManager)
				continue;
			m_aPendingAARSends.Insert(pid);
		}

		GetGame().GetCallqueue().CallLater(SendNextAARData, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Processes one pending AAR data send per call (500 ms apart) until the queue is empty.
	//! Dispatches both SendAARKillStats (kill/death name lists) and SendAARStats (numeric totals).
	protected void SendNextAARData()
	{
		if (m_aPendingAARSends.IsEmpty())
			return;

		int playerId = m_aPendingAARSends[0];
		m_aPendingAARSends.RemoveOrdered(0);

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (pc)
		{
			COA_PlayerRplToOwnerManager rplManager = COA_PlayerRplToOwnerManager.Cast(pc.FindComponent(COA_PlayerRplToOwnerManager));
			if (rplManager)
			{
				// --- Kill / death name lists ---
				array<string> kills = m_mSessionKills.Get(playerId);
				if (!kills)
					kills = new array<string>();

				string killedBy = "";
				if (m_mSessionDeaths.Contains(playerId))
					killedBy = m_mSessionDeaths.Get(playerId);

				rplManager.SendAARKillStats(kills, killedBy);

				// --- Numeric session stats ---
				CRF_PlayerStats stats = m_mPlayerStats.Get(playerId);
				if (stats)
				{
					int bandages = stats.bandage_self + stats.bandage_friendlies
					             + stats.tourniquet_self + stats.tourniquet_friendlies
					             + stats.saline_self + stats.saline_friendlies
					             + stats.morphine_self + stats.morphine_friendlies;
					int distKm = ((int)(stats.distance_walked + stats.distance_driven)) / 1000;
					rplManager.SendAARStats(
						stats.kills,
						stats.deaths,
						stats.shots,
						stats.grenades_thrown,
						bandages,
						distKm,
						stats.friendly_kills,
						stats.level_experience
					);
				}
			}
		}

		if (!m_aPendingAARSends.IsEmpty())
			GetGame().GetCallqueue().CallLater(SendNextAARData, 500, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a player disconnects — flush their stats immediately.
	override protected void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;

		CRF_PlayerStats stats = m_mPlayerStats.Get(playerId);
		if (!stats || stats.flushed)
		{
			CleanupEntityHooks(playerId);
			return;
		}

		Print(string.Format("[CRF_ServerStatsManager] Player %1 (ID: %2) disconnected — flushing mid-session stats",
			stats.playerName, playerId), LogLevel.NORMAL);

		UpdatePlayerFaction(playerId, stats);
		// Winner can't be known at disconnect time — treat as no winner (false by default).
		FlushPlayerStats(playerId, stats);
		CleanupEntityHooks(playerId);
	}

	//==============================================================================================
	// ENTITY-LEVEL HOOK MANAGEMENT
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	protected bool RegisterEntityHooks(int playerId, IEntity playerEntity)
	{
		if (!playerEntity)
			return false;

		bool hasAnyHook = false;

		// ── Shots & grenades — EventHandlerManagerComponent ─────────────────
		EventHandlerManagerComponent ehm = EventHandlerManagerComponent.Cast(
			playerEntity.FindComponent(EventHandlerManagerComponent));
		if (ehm)
		{
			ehm.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired_Wrapper);
			ehm.RemoveScriptHandler("OnGrenadeThrown",  this, OnGrenadeThrown_Wrapper);
			ehm.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired_Wrapper);
			ehm.RegisterScriptHandler("OnGrenadeThrown",  this, OnGrenadeThrown_Wrapper);
			hasAnyHook = true;
		}

		// ── Healing items — SCR_CharacterControllerComponent ────────────────
		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
			playerEntity.FindComponent(SCR_CharacterControllerComponent));
		if (charCtrl)
		{
			charCtrl.m_OnItemUseEndedInvoker.Remove(OnHealingItemUsed);
			charCtrl.m_OnItemUseEndedInvoker.Insert(OnHealingItemUsed);
			hasAnyHook = true;
		}

		// ── Compartment access — for driving / occupant tracking ─────────────
		SCR_CompartmentAccessComponent compartmentAccess = SCR_CompartmentAccessComponent.Cast(
			playerEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (compartmentAccess)
		{
			compartmentAccess.GetOnCompartmentEntered().Remove(OnCompartmentEntered);
			compartmentAccess.GetOnCompartmentLeft().Remove(OnCompartmentLeft);
			compartmentAccess.GetOnCompartmentEntered().Insert(OnCompartmentEntered);
			compartmentAccess.GetOnCompartmentLeft().Insert(OnCompartmentLeft);
			hasAnyHook = true;
		}

		return hasAnyHook;
	}

	//------------------------------------------------------------------------------------------------
	protected void RegisterEntityHooksWithRetry(int playerId, IEntity playerEntity, int attempt)
	{
		if (!playerEntity || playerId <= 0)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		if (playerManager.GetPlayerControlledEntity(playerId) != playerEntity)
			return;

		if (RegisterEntityHooks(playerId, playerEntity))
			return;

		if (attempt + 1 >= HOOK_RETRY_MAX_ATTEMPTS)
		{
			Print(string.Format("[CRF_ServerStatsManager] WARNING: Failed to attach entity hooks for player %1 after %2 attempts", playerId, HOOK_RETRY_MAX_ATTEMPTS), LogLevel.WARNING);
			return;
		}

		GetGame().GetCallqueue().CallLater(RegisterEntityHooksWithRetry, HOOK_RETRY_DELAY_MS, false, playerId, playerEntity, attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void CleanupEntityHooks(int playerId)
	{
		// Remove the player from all distance-tracking maps.
		StopDistanceTracking(playerId);

		// Entity hooks are removed automatically when the entity is destroyed.
		// If the entity is still alive (disconnect before death), attempt removal.
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		EventHandlerManagerComponent ehm = EventHandlerManagerComponent.Cast(
			playerEntity.FindComponent(EventHandlerManagerComponent));
		if (ehm)
		{
			ehm.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired_Wrapper);
			ehm.RemoveScriptHandler("OnGrenadeThrown",  this, OnGrenadeThrown_Wrapper);
		}

		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
			playerEntity.FindComponent(SCR_CharacterControllerComponent));
		if (charCtrl)
			charCtrl.m_OnItemUseEndedInvoker.Remove(OnHealingItemUsed);

		SCR_CompartmentAccessComponent compartmentAccess = SCR_CompartmentAccessComponent.Cast(
			playerEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (compartmentAccess)
		{
			compartmentAccess.GetOnCompartmentEntered().Remove(OnCompartmentEntered);
			compartmentAccess.GetOnCompartmentLeft().Remove(OnCompartmentLeft);
		}
	}

	//==============================================================================================
	// ENTITY-LEVEL INVOKER CALLBACKS
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired_Wrapper(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		CRF_PlayerStats stats = m_mPlayerStats.Get(playerID);
		if (stats)
			stats.shots++;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown_Wrapper(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;
		// Skip smoke grenades (deal no damage — matches vanilla behaviour).
		if (weapon.GetWeaponType() == EWeaponType.WT_SMOKEGRENADE)
			return;

		CRF_PlayerStats stats = m_mPlayerStats.Get(playerID);
		if (stats)
			stats.grenades_thrown++;
	}

	//------------------------------------------------------------------------------------------------
	//! Fired when a character completes use of a consumable item (bandage, tourniquet, etc.).
	protected void OnHealingItemUsed(IEntity item, bool actionCompleted, ItemUseParameters animParams)
	{
		if (!item || !actionCompleted)
			return;

		SCR_ConsumableItemComponent consumable = SCR_ConsumableItemComponent.Cast(
			item.FindComponent(SCR_ConsumableItemComponent));
		if (!consumable)
			return;

		IEntity user = consumable.GetCharacterOwner();
		if (!user)
			return;

		int userId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user);
		if (userId <= 0)
			return;

		CRF_PlayerStats stats = m_mPlayerStats.Get(userId);
		if (!stats)
			return;

		IEntity target = consumable.GetTargetCharacter();
		bool isSelf = (!target || target == user);

		SCR_EConsumableType typeId = consumable.GetConsumableType();

		switch (typeId)
		{
			case SCR_EConsumableType.BANDAGE:
				if (isSelf) stats.bandage_self++;
				else        stats.bandage_friendlies++;
				break;
			case SCR_EConsumableType.TOURNIQUET:
				if (isSelf) stats.tourniquet_self++;
				else        stats.tourniquet_friendlies++;
				break;
			case SCR_EConsumableType.SALINE:
				if (isSelf) stats.saline_self++;
				else        stats.saline_friendlies++;
				break;
			case SCR_EConsumableType.MORPHINE:
				if (isSelf) stats.morphine_self++;
				else        stats.morphine_friendlies++;
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Player entered a vehicle compartment — stop walking tracking, start driving/occupant.
	protected void OnCompartmentEntered(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		if (!manager)
			return;

		BaseCompartmentSlot slot = manager.FindCompartment(slotID, mgrID);
		if (!slot)
			return;

		IEntity occupant = slot.GetOccupant();
		if (!occupant)
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(occupant);
		if (playerId <= 0)
			return;

		// Stop on-foot tracking for this player.
		m_mWalkingPlayers.Remove(playerId);

		IEntity vehicle = targetEntity;

		// Determine driver vs. passenger by checking if this is the driver seat.
		// The driver compartment type is ECompartmentType.PILOT for vehicles.
		if (slot.GetType() == ECompartmentType.PILOT)
			m_mDriverVehicle.Set(playerId, vehicle);
		else
			m_mOccupantVehicle.Set(playerId, vehicle);
	}

	//------------------------------------------------------------------------------------------------
	//! Player left a vehicle compartment — resume walking tracking, stop driving/occupant.
	protected void OnCompartmentLeft(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		// targetEntity is the entity that just left the compartment.
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(targetEntity);
		if (playerId <= 0)
			return;

		m_mDriverVehicle.Remove(playerId);
		m_mOccupantVehicle.Remove(playerId);

		// Resume on-foot tracking.
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (playerEntity)
			m_mWalkingPlayers.Set(playerId, playerEntity);
	}

	//==============================================================================================
	// AI KILL TRACKING
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	//! Routes an AI-entity death to the appropriate killer's stat counters.
	protected void HandleAIKill(IEntity aiEntity, IEntity killerEntity, Instigator instigator, notnull SCR_InstigatorContextData instigatorContextData)
	{
		// instigator can be null for AI-kill events (e.g. no direct weapon attribution) —
		// fall back to entity lookup, then to the context data, same as CRF_LoggingManager.
		int killerPlayerId = 0;
		if (killerEntity)
			killerPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity);
		if (killerPlayerId <= 0 && instigator)
			killerPlayerId = instigator.GetInstigatorPlayerID();
		if (killerPlayerId <= 0)
			killerPlayerId = instigatorContextData.GetKillerPlayerID();
		if (killerPlayerId <= 0)
			return;

		CRF_PlayerStats killerStats = m_mPlayerStats.Get(killerPlayerId);
		if (!killerStats)
			return;

		// Detect roadkill: killer is currently tracked as a vehicle driver.
		bool isVehicleKill = m_mDriverVehicle.Contains(killerPlayerId);

		// Determine friendly fire: killer and victim in same faction.
		bool isFriendly = false;
		if (killerEntity && aiEntity)
		{
			SCR_ChimeraCharacter killerChar = SCR_ChimeraCharacter.Cast(killerEntity);
			SCR_ChimeraCharacter victimChar = SCR_ChimeraCharacter.Cast(aiEntity);
			if (killerChar && victimChar)
			{
				Faction killerFac = killerChar.GetFaction();
				Faction victimFac = victimChar.GetFaction();
				if (killerFac && victimFac && killerFac == victimFac)
					isFriendly = true;
			}
		}

		if (isVehicleKill)
		{
			if (isFriendly)
				killerStats.friendly_ai_roadkills++;
			else
				killerStats.ai_roadkills++;
		}
		else
		{
			if (isFriendly)
				killerStats.friendly_ai_kills++;
			else
				killerStats.ai_kills++;
		}
	}

	//==============================================================================================
	// STAT FLUSH (LOG FILE)
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	//! Calculates session duration and XP, then writes one "player_stat" CSV line.
	protected void FlushPlayerStats(int playerId, CRF_PlayerStats stats)
	{
		if (!stats || stats.flushed)
			return;

		stats.flushed = true;

		Print(string.Format("[CRF_ServerStatsManager] Flushing — player: %1 | kills: %2 | deaths: %3 | winner: %4",
			stats.playerName, stats.kills, stats.deaths, stats.winner), LogLevel.NORMAL);

		// ── Session duration ─────────────────────────────────────────────────
		float nowMs            = GetGame().GetWorld().GetWorldTime();
		stats.session_duration = (nowMs - stats.session_start) / 1000.0; // convert ms → s

		// ── XP calculation ───────────────────────────────────────────────────
		float xp =
			stats.kills              * XP_KILL               +
			stats.ai_kills           * XP_AI_KILL             +
			stats.bandage_self       * XP_BANDAGE_SELF        +
			stats.bandage_friendlies * XP_BANDAGE_ALLY        +
			stats.tourniquet_self    * XP_TOURNIQUET_SELF     +
			stats.tourniquet_friendlies * XP_TOURNIQUET_ALLY  +
			stats.saline_self        * XP_SALINE_SELF         +
			stats.saline_friendlies  * XP_SALINE_ALLY         +
			stats.morphine_self      * XP_MORPHINE_SELF       +
			stats.morphine_friendlies * XP_MORPHINE_ALLY      +
			stats.friendly_kills     * XP_FRIENDLY_KILL_PENALTY +
			stats.friendly_ai_kills  * XP_FRIENDLY_AI_PENALTY;

		if (xp < 0) xp = 0;

		if (stats.winner)
			xp = xp * XP_WINNER_MULT;

		stats.level_experience = xp.ToString(0).ToInt();

		// ── Write log line ───────────────────────────────────────────────────
		CRF_LoggingManager lm = CRF_LoggingManager.GetInstance();
		if (!lm)
			return;

		FileHandle fh = lm.GetLogFileHandle();
		if (!fh)
			return;

		string linePart1 = stats.guid
			+ "," + stats.kills
			+ "," + stats.ai_kills
			+ "," + stats.deaths
			+ "," + stats.shots
			+ "," + stats.grenades_thrown
			+ "," + stats.friendly_kills
			+ "," + stats.friendly_ai_kills
			+ "," + stats.distance_walked.ToString(2);

		string linePart2 = stats.distance_driven.ToString(2)
			+ "," + stats.roadkills
			+ "," + stats.friendly_roadkills
			+ "," + stats.ai_roadkills
			+ "," + stats.friendly_ai_roadkills
			+ "," + stats.distance_as_occupant.ToString(2)
			+ "," + stats.bandage_self
			+ "," + stats.bandage_friendlies
			+ "," + stats.tourniquet_self;

		string winnerStr = "0";
		if (stats.winner)
			winnerStr = "1";

		string linePart3 = stats.tourniquet_friendlies.ToString()
			+ "," + stats.saline_self
			+ "," + stats.saline_friendlies
			+ "," + stats.morphine_self
			+ "," + stats.morphine_friendlies
			+ "," + stats.session_duration.ToString(0)
			+ "," + stats.level_experience
			+ "," + winnerStr;

		string line = "player_stat," + linePart1 + "," + linePart2 + "," + linePart3;

		fh.WriteLine(line);
	}

	//==============================================================================================
	// HELPERS
	//==============================================================================================

	//------------------------------------------------------------------------------------------------
	//! Refresh stats.faction from slotting manager (use slot faction, fall back to entity faction).
	protected void UpdatePlayerFaction(int playerId, CRF_PlayerStats stats)
	{
		Faction f = null;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
			f = slottingManager.GetPlayerSlotFaction(playerId, true);

		if (!f)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(entity);
			if (ch)
				f = ch.GetFaction();
		}

		if (f)
			stats.faction = f.GetFactionKey();
	}
}
