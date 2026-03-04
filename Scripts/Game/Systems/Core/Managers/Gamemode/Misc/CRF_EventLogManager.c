/*
 * CRF_EventLogManager
 *
 * Server-side gamemode ScriptComponent that captures mission events (kills, unconscious,
 * gamemode state changes) and broadcasts formatted strings to all clients via
 * CRF_RplBroadcastManager so spectators can display a rolling event feed.
 *
 * Architecture:
 *   - Server-only event detection; all broadcast logic funnelled through CRF_RplBroadcastManager
 *   - Maintains a rolling buffer (MAX_HISTORY entries) so late-joining spectators can receive
 *     a catch-up dump via BroadcastEventLogHistory()
 *   - Event strings are fully formatted on the server before broadcast so clients need no
 *     additional data resolution
 *
 * Event types produced:
 *   "KILL"    – player or AI killed (killer | victim | weapon | range)
 *   "UNCON"   – player knocked unconscious (victim | attacker if known)
 *   "MISSION" – gamemode state changes and admin-injected annotations
 */
class CRF_EventLogManagerClass : ScriptComponentClass {}

class CRF_EventLogManager : ScriptComponent
{
	//=================================================================================================
	// CONSTANTS
	//=================================================================================================

	static const int MAX_HISTORY = 50; // Rolling event buffer size

	//=================================================================================================
	// RUNTIME STATE
	//=================================================================================================

	protected ref array<string> m_aEventHistory = {};   // Rolling buffer of formatted entries

	// Manager references cached at OnPostInit
	protected CRF_RplBroadcastManager m_BroadcastManager;
	protected PlayerManager            m_PlayerManager;
	protected FactionManager           m_FactionManager;

	// Singleton
	protected static CRF_EventLogManager m_sInstance;

	//=================================================================================================
	// LIFECYCLE
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	void CRF_EventLogManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_EventLogManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only functional on the server
		if (RplSession.Mode() == RplMode.Client)
			return;

		m_BroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_PlayerManager    = GetGame().GetPlayerManager();
		m_FactionManager   = GetGame().GetFactionManager();
	}

	//=================================================================================================
	// SERVER EVENT HANDLERS — called directly from CRF_SCR_DataCollectorComponent
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	/**
	 * Called by CRF_SCR_DataCollectorComponent.OnPlayerKilled (server-only).
	 * Generates a KILL event entry and broadcasts it.
	 */
	void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		if (!m_PlayerManager || !m_BroadcastManager)
			return;

		IEntity entity       = instigatorContextData.GetVictimEntity();
		IEntity killerEntity = instigatorContextData.GetKillerEntity();

		int victimPlayerId = instigatorContextData.GetVictimPlayerID();
		if (victimPlayerId <= 0)
			victimPlayerId = m_PlayerManager.GetPlayerIdFromControlledEntity(entity);

		string victimName    = ResolveEntityName(entity, victimPlayerId);
		string victimFaction = ResolveEntityFaction(entity);
		string killerName    = "";
		string killerFaction = "";
		string weapon        = "Unknown";
		string rangeStr      = "";

		// Resolve killer
		if (killerEntity)
		{
			int killerId     = instigatorContextData.GetKillerPlayerID();
			if (killerId <= 0)
				killerId = m_PlayerManager.GetPlayerIdFromControlledEntity(killerEntity);

			killerName    = ResolveEntityName(killerEntity, killerId);
			killerFaction = ResolveEntityFaction(killerEntity);

			float range = vector.Distance(entity.GetOrigin(), killerEntity.GetOrigin());
			rangeStr = Math.Round(range).ToString() + "m";
		}

		// Weapon — LogPlayerKill already ran so we use our lightweight fallback
		weapon = ResolveWeaponFromKiller(killerEntity);

		// Build display string  e.g.  "[KILL]  Smith (BLUFOR) → Jones (OPFOR) | M4A1 | 247m"
		string entry = string.Format("[KILL]  %1 (%2)  ->  %3 (%4)  |  %5  |  %6",
			killerName, killerFaction,
			victimName, victimFaction,
			weapon, rangeStr);

		BroadcastEntry(entry);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Called by CRF_SCR_CharacterDamageManagerComponent when a player's damage state
	 * changes to INCAPACITATED (unconscious / downed).
	 * This is a PUBLIC server-side API — the damage manager calls it directly.
	 *
	 * @param victim           The incapacitated character entity
	 * @param instigatorEntity The entity that caused the incapacitation (may be null)
	 */
	void OnPlayerIncapacitated(IEntity victim, IEntity instigatorEntity)
	{
		if (RplSession.Mode() == RplMode.Client || !m_PlayerManager || !m_BroadcastManager)
			return;

		int victimPlayerId = m_PlayerManager.GetPlayerIdFromControlledEntity(victim);
		if (victimPlayerId <= 0)
			return; // Only track player unconscious events

		string victimName    = ResolveEntityName(victim, victimPlayerId);
		string victimFaction = ResolveEntityFaction(victim);

		string entry;
		if (instigatorEntity)
		{
			int instigatorId         = m_PlayerManager.GetPlayerIdFromControlledEntity(instigatorEntity);
			string instigatorName    = ResolveEntityName(instigatorEntity, instigatorId);
			string instigatorFaction = ResolveEntityFaction(instigatorEntity);

			entry = string.Format("[UNCON]  %1 (%2) knocked out by %3 (%4)",
				victimName, victimFaction, instigatorName, instigatorFaction);
		}
		else
		{
			entry = string.Format("[UNCON]  %1 (%2) is unconscious", victimName, victimFaction);
		}

		BroadcastEntry(entry);
	}

	//=================================================================================================
	// PUBLIC API — SERVER ONLY
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	/**
	 * Manually inject a mission-important event string (e.g. objective captured, gamemode change).
	 * Call from other server-side managers to annotate the event feed.
	 * @param description  Human-readable event description (already fully formatted)
	 */
	void LogMissionEvent(string description)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		string entry = "[MISSION]  " + description;
		BroadcastEntry(entry);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Called by a newly-opened SpectatorMenu (via RPC) to receive the current history buffer.
	 * Triggers BroadcastEventLogHistory targeted at the requesting player.
	 */
	void SendHistoryToPlayer(int playerId)
	{
		if (RplSession.Mode() == RplMode.Client || !m_BroadcastManager)
			return;

		m_BroadcastManager.BroadcastEventLogHistory(playerId, m_aEventHistory);
	}

	//=================================================================================================
	// INTERNAL HELPERS
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	/** Add a formatted entry to the rolling history buffer and broadcast to all clients. */
	protected void BroadcastEntry(string entry)
	{
		// Prepend world-time timestamp  HH:MM
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		string timestamp   = "";
		if (world)
		{
			float worldMs  = world.GetWorldTime();
			int totalSec   = (worldMs / 1000);
			timestamp      = SCR_FormatHelper.FormatTime(totalSec) + "  ";
		}

		string fullEntry = timestamp + entry;

		// Maintain rolling buffer
		m_aEventHistory.Insert(fullEntry);
		while (m_aEventHistory.Count() > MAX_HISTORY)
			m_aEventHistory.Remove(0);

		// Broadcast to all clients
		if (m_BroadcastManager)
			m_BroadcastManager.BroadcastEventLogEntry(fullEntry);
	}

	//------------------------------------------------------------------------------------------------
	protected string ResolveEntityName(IEntity entity, int playerId)
	{
		if (playerId > 0 && m_PlayerManager)
		{
			string name = m_PlayerManager.GetPlayerName(playerId);
			if (!name.IsEmpty())
				return name;
		}
		return "AI";
	}

	//------------------------------------------------------------------------------------------------
	protected string ResolveEntityFaction(IEntity entity)
	{
		if (!entity)
			return "?";

		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(entity);
		if (ch)
		{
			FactionKey fk = ch.GetFactionKey();
			if (!fk.IsEmpty())
				return fk;
		}
		return "?";
	}

	//------------------------------------------------------------------------------------------------
	/** Lightweight weapon resolver: reads the killer's currently held weapon. */
	protected string ResolveWeaponFromKiller(IEntity killer)
	{
		if (!killer)
			return "Unknown";

		SCR_CharacterInventoryStorageComponent inv = SCR_CharacterInventoryStorageComponent.Cast(
			killer.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!inv)
			return "Unknown";

		BaseWeaponComponent bwc = inv.GetCurrentCharacterWeapon();
		if (!bwc)
			return "Unknown";

		UIInfo info = bwc.GetUIInfo();
		if (!info)
			return "Unknown";

		string name = info.GetName();
		if (name.IsEmpty())
			return "Unknown";
		return name;
	}
}
