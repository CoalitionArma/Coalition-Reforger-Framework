//------------------------------------------------------------------------------------
// CRF_CasualtyCounterGamemodeComponent: counts one side's casualties against a limit
//
// The mission maker picks the monitored side and how many casualties it can take, as a percentage of
// its players. The limit itself is worked out when the round goes live (safe start ends): that many
// percent of the side's slotted players at that moment, rounded up. Every death of a monitored-side
// player from then on is a casualty - respawned players count again each time they die. When the count
// reaches the limit, the opposite side wins.
//
// It never ends the mission: the result is announced to everyone and written to the admin log, and an
// admin ends the mission. It adds no tasks, so it sits next to any other mode (an objective chain,
// Rush...) without fighting over the task HUD.
//
// The server counts. Clients get the limit, the count and the winner, for the pause menu's mission
// info and the announcements.
//
// Setup for mission makers (full guide: !Docs/MissionMaking/Gamemodes/CRF_CASUALTY_COUNTER_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the COA_Lobby gamemode entity.
//   2. Set the monitored side's faction key and the casualty limit (% of its players).
//   3. Optionally name the side that wins (default: the hostile side with players).
//------------------------------------------------------------------------------------

enum CRF_ECasualtyCounterPhase
{
	WAITING,	//!< the round isn't live yet, no limit
	COUNTING,
	REACHED,	//!< the limit was reached - the winner is decided, casualties are still counted
	DISABLED	//!< misconfigured (no players on the monitored side when the round went live)
}

class CRF_CasualtyCounterGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_CasualtyCounterGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the side whose casualties are counted", category: "Casualty Counter")]
	protected string m_sMonitoredFaction;

	[Attribute("100", UIWidgets.EditBox, "Casualties the monitored side can take before the other side wins, as a percentage of its players when the round goes live (safe start ends), rounded up. 100 = as many casualties as it had players; with respawns, more than 100 makes sense.", params: "1 1000 1", category: "Casualty Counter")]
	protected float m_fCasualtyLimitPercent;

	[Attribute("", UIWidgets.EditBox, "Faction key of the side that wins when the limit is reached. Empty: the side hostile to the monitored one with the most players when the round goes live.", category: "Casualty Counter")]
	protected string m_sWinningFaction;

	[Attribute("1", UIWidgets.CheckBox, "Show the count to everyone: in the pause menu's mission info, and in announcements as it climbs. Off: players only learn about it when the limit is reached (admins still see it in the admin log).", category: "Casualty Counter")]
	protected bool m_bShowCounter;

	[Attribute("25", UIWidgets.EditBox, "Announce the count to everyone each time it passes another this-many percent of the limit (needs Show Counter). 0: only when the limit is reached.", params: "0 100 1", category: "Casualty Counter")]
	protected float m_fAnnounceEveryPercent;

	// Replicated
	[RplProp()]
	protected int m_iPhase;
	[RplProp()]
	protected int m_iCasualties;
	[RplProp()]
	protected int m_iLimit;
	[RplProp()]
	protected int m_iStartingPlayers;
	[RplProp()]
	protected string m_sWinner;			//!< resolved winning faction key, set when counting starts

	// Server
	protected COA_Gamemode m_Gamemode;
	protected bool m_bStopped;			//!< the mission left the game state - stop counting
	protected int m_iLastAnnouncedStep;
	protected ref map<int, IEntity> m_mLastCountedVictim = new map<int, IEntity>();

	protected static const string LOG = "[CRF_CasualtyCounter]";

	protected static CRF_CasualtyCounterGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_CasualtyCounterGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_CasualtyCounterGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_CasualtyCounterGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		if (m_sMonitoredFaction.IsEmpty())
		{
			Print(LOG + " No monitored faction set: the casualty counter is not running.", LogLevel.ERROR);
			return;
		}

		// Polled: the Lobby managers may not exist yet this early, and a mission without safestart
		// goes live on the gamemode state instead
		GetGame().GetCallqueue().CallLater(WaitForRoundLive, 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(WaitForRoundLive);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		super.OnDelete(owner);
	}

	//===================================================================================
	// SERVER: STARTING THE COUNT
	//===================================================================================

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
		StartCounting();
	}

	//! The round is live: fix the limit from the monitored side's players right now
	protected void StartCounting()
	{
		map<string, int> playersByFaction = new map<string, int>();
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			if (factionKey.IsEmpty())
				continue;

			playersByFaction.Set(factionKey, playersByFaction.Get(factionKey) + 1);
		}

		m_iStartingPlayers = playersByFaction.Get(m_sMonitoredFaction);
		m_sWinner = ResolveWinner(playersByFaction);

		if (m_iStartingPlayers <= 0)
		{
			m_iPhase = CRF_ECasualtyCounterPhase.DISABLED;
			Replication.BumpMe();
			Print(string.Format("%1 %2 has no players as the round goes live: the casualty counter is not running. Check the faction key.", LOG, m_sMonitoredFaction), LogLevel.ERROR);
			return;
		}

		m_iLimit = (int)Math.Max(1, Math.Ceil(m_iStartingPlayers * m_fCasualtyLimitPercent / 100));
		m_iCasualties = 0;
		m_iPhase = CRF_ECasualtyCounterPhase.COUNTING;
		Replication.BumpMe();

		string message = string.Format("%1 casualty limit: %2 (%3%% of %4 players). %5 wins if it's reached.", MonitoredName(), m_iLimit, Math.Round(m_fCasualtyLimitPercent), m_iStartingPlayers, WinnerName());
		Print(string.Format("%1 %2", LOG, message), LogLevel.NORMAL);
		LogToAdmins(message, COA_EAdminLogLevel.Low);

		if (m_sWinner.IsEmpty())
			Print(string.Format("%1 No side hostile to %2 has players: the limit has no winner to announce. Set the winning faction.", LOG, m_sMonitoredFaction), LogLevel.WARNING);
	}

	//! The configured winner, or the hostile side with the most players
	protected string ResolveWinner(notnull map<string, int> playersByFaction)
	{
		if (!m_sWinningFaction.IsEmpty())
			return m_sWinningFaction;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return string.Empty;

		Faction monitored = factionManager.GetFactionByKey(m_sMonitoredFaction);
		if (!monitored)
			return string.Empty;

		string best;
		int bestPlayers;
		foreach (string factionKey, int players : playersByFaction)
		{
			if (factionKey == m_sMonitoredFaction || players <= bestPlayers)
				continue;

			Faction other = factionManager.GetFactionByKey(factionKey);
			if (!other || !monitored.IsFactionEnemy(other))
				continue;

			best = factionKey;
			bestPlayers = players;
		}

		return best;
	}

	//! The mission left the game state (an admin ended it): stop counting
	protected void OnGamemodeStateChanged()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			m_bStopped = true;
	}

	//===================================================================================
	// SERVER: COUNTING
	//===================================================================================

	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || m_bStopped)
			return;

		if (m_iPhase != CRF_ECasualtyCounterPhase.COUNTING && m_iPhase != CRF_ECasualtyCounterPhase.REACHED)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0 || CRF_RadioTransmissionEvents.GetPlayerFactionKey(victimId) != m_sMonitoredFaction)
			return;

		// One casualty per body, should the kill be reported twice
		IEntity victim = instigatorContextData.GetVictimEntity();
		if (victim && m_mLastCountedVictim.Get(victimId) == victim)
			return;

		m_mLastCountedVictim.Set(victimId, victim);
		m_iCasualties++;
		Replication.BumpMe();

		if (m_iPhase == CRF_ECasualtyCounterPhase.COUNTING && m_iCasualties >= m_iLimit)
		{
			ReachLimit();
			return;
		}

		AnnounceProgress();
	}

	//! Each time the count passes another m_fAnnounceEveryPercent of the limit
	protected void AnnounceProgress()
	{
		if (m_iPhase != CRF_ECasualtyCounterPhase.COUNTING || !m_bShowCounter || m_fAnnounceEveryPercent <= 0)
			return;

		int step = (int)Math.Floor(m_iCasualties * 100.0 / (m_iLimit * m_fAnnounceEveryPercent));
		if (step <= m_iLastAnnouncedStep)
			return;

		m_iLastAnnouncedStep = step;

		string title = string.Format("%1 casualties: %2 of %3", MonitoredName(), m_iCasualties, m_iLimit);
		string text = string.Format("%1 wins at %2.", WinnerName(), m_iLimit);
		PopupToEveryone(title, title, title, text, 6);
	}

	protected void ReachLimit()
	{
		m_iPhase = CRF_ECasualtyCounterPhase.REACHED;
		Replication.BumpMe();

		string reason = string.Format("%1 has taken %2 casualties, its limit. The mission goes on until an admin ends it.", MonitoredName(), m_iCasualties);
		PopupToEveryone("CASUALTY LIMIT REACHED", "VICTORY", string.Format("Winner: %1", WinnerName()), reason, 12);

		string message = string.Format("Casualty limit reached: %1 took %2 casualties (limit %3) - %4 wins. End the mission from the admin menu.", MonitoredName(), m_iCasualties, m_iLimit, WinnerName());
		Print(string.Format("%1 %2", LOG, message), LogLevel.NORMAL);
		LogToAdmins(message, COA_EAdminLogLevel.High);
	}

	//! A popup for each player: one title for the monitored side, one for the winner, one for anyone else
	protected void PopupToEveryone(string monitoredTitle, string winnerTitle, string neutralTitle, string text, float duration)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string faction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string title = neutralTitle;
			if (faction == m_sMonitoredFaction)
				title = monitoredTitle;
			else if (!m_sWinner.IsEmpty() && faction == m_sWinner)
				title = winnerTitle;

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, title, text, duration);
		}
	}

	protected void LogToAdmins(string message, COA_EAdminLogLevel level)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.LogAdminAction(message, -1, false, level);
	}

	//===================================================================================
	// ACCESSORS (every machine, from replicated values)
	//===================================================================================

	string GetMonitoredFactionKey()
	{
		return m_sMonitoredFaction;
	}

	//! The winning faction key, empty until the round goes live (or if none could be found)
	string GetWinningFactionKey()
	{
		return m_sWinner;
	}

	int GetCasualties()
	{
		return m_iCasualties;
	}

	//! 0 until the round goes live
	int GetLimit()
	{
		return m_iLimit;
	}

	bool IsLimitReached()
	{
		return m_iPhase == CRF_ECasualtyCounterPhase.REACHED;
	}

	protected string MonitoredName()
	{
		return CRF_SpecialsHelper.GetFactionName(m_sMonitoredFaction);
	}

	protected string WinnerName()
	{
		if (m_sWinner.IsEmpty())
			return "The other side";

		return CRF_SpecialsHelper.GetFactionName(m_sWinner);
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook)
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iPhase == CRF_ECasualtyCounterPhase.REACHED)
		{
			lines.Insert(string.Format("%1 casualty limit reached (%2 of %3): %4 wins", MonitoredName(), m_iCasualties, m_iLimit, WinnerName()));
			return;
		}

		if (!m_bShowCounter)
			return;

		if (m_iPhase == CRF_ECasualtyCounterPhase.WAITING)
			lines.Insert(string.Format("%1 casualty limit: %2%% of their players, set when the round goes live", MonitoredName(), Math.Round(m_fCasualtyLimitPercent)));
		else if (m_iPhase == CRF_ECasualtyCounterPhase.COUNTING)
			lines.Insert(string.Format("%1 casualties: %2 of %3 (%4 wins at %3)", MonitoredName(), m_iCasualties, m_iLimit, WinnerName()));
	}
}
