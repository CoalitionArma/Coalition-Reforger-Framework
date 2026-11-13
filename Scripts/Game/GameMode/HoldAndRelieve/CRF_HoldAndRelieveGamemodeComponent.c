//------------------------------------------------------------------------------------
// CRF_HoldAndRelieveGamemodeComponent: a cut-off force holds a position until relieved
//
// One side (the defending faction) has an element holding a zone and a relief force fighting its way to
// it; the relief force is the defending faction's groups whose squad callsign starts with one of
// m_aReliefCallsigns. The other side (the attacking faction) sits between them.
//   - Defenders win when enough relief players reach the zone and hold it (link-up), or when the hold
//     time runs out without the zone falling.
//   - Attackers win when they hold the zone with no living defending-faction player inside (seized).
// Only living players count; AI is ignored.
//
// Setup for mission makers (guide: !Docs/MissionMaking/Gamemodes/CRF_HOLD_AND_RELIEVE_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the COA_Lobby gamemode entity.
//   2. Place a named entity at the centre of the zone (m_sZoneAnchorName).
//   3. Name the relief force's groups so their callsigns start with an m_aReliefCallsigns entry.
//------------------------------------------------------------------------------------

enum CRF_EHoldAndRelievePhase
{
	WAITING,	//!< the round isn't live yet
	LIVE,
	ENDED
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Hold and Relieve: hold a zone until a relief force links up, or until the clock runs out. The other side wins by seizing it.")]
class CRF_HoldAndRelieveGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_HoldAndRelieveGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the side holding the zone (the cut-off element and the relief force)", category: "Hold and Relieve")]
	protected string m_sDefendingFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the side trying to seize the zone", category: "Hold and Relieve")]
	protected string m_sAttackingFaction;

	[Attribute("", UIWidgets.Auto, "Squad callsigns of the relief force: a defending-faction group whose callsign starts with one of these (not case-sensitive) is relief. Everyone else on that side is the cut-off element.", category: "Hold and Relieve")]
	protected ref array<string> m_aReliefCallsigns;

	[Attribute("holdrelieve_zone", UIWidgets.EditBox, "Name of an entity at the centre of the zone", category: "Hold and Relieve")]
	protected string m_sZoneAnchorName;

	[Attribute("The Crossroads", UIWidgets.EditBox, "Name of the zone in tasks and announcements", category: "Hold and Relieve")]
	protected string m_sZoneLabel;

	[Attribute("150", UIWidgets.EditBox, "Metres from the zone entity that count as the zone", params: "20 2000 1", category: "Hold and Relieve")]
	protected float m_fZoneRadius;

	[Attribute("4", UIWidgets.EditBox, "Living relief players needed inside the zone to link up", params: "1 100 1", category: "Hold and Relieve")]
	protected int m_iLinkUpPlayers;

	[Attribute("2", UIWidgets.EditBox, "Link-up only counts while at most this many living attackers are inside the zone", params: "0 100 1", category: "Hold and Relieve")]
	protected int m_iLinkUpMaxAttackers;

	[Attribute("30", UIWidgets.EditBox, "Seconds the link-up condition must hold", params: "0 600 1", category: "Hold and Relieve")]
	protected int m_iLinkUpSeconds;

	[Attribute("3", UIWidgets.EditBox, "Living attackers needed inside the zone to seize it (and no living defending-faction player there)", params: "1 100 1", category: "Hold and Relieve")]
	protected int m_iSeizePlayers;

	[Attribute("60", UIWidgets.EditBox, "Seconds the seize condition must hold", params: "0 600 1", category: "Hold and Relieve")]
	protected int m_iSeizeSeconds;

	[Attribute("60", UIWidgets.EditBox, "Minutes from the round going live until the defending side wins by holding out. 0: no limit (only link-up or seizure decide it).", params: "0 600 1", category: "Hold and Relieve")]
	protected int m_iHoldMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Hold and Relieve")]
	protected bool m_bEndMissionOnVictory;

	// Replicated: the pause-menu mission info
	[RplProp()]
	protected int m_iPhase;
	[RplProp()]
	protected bool m_bDefendersWon;
	[RplProp()]
	protected int m_iSecondsLeft = -1;		//!< -1: no hold limit
	[RplProp()]
	protected int m_iLinkUpHeld;
	[RplProp()]
	protected int m_iSeizeHeld;

	// Server
	protected COA_Gamemode m_Gamemode;
	protected bool m_bRoundLive;
	protected bool m_bEndRequested;
	protected vector m_vZonePosition;
	protected float m_fDeadline;			//!< world time (ms) the hold time runs out, 0 = none
	protected bool m_bLinkUpAnnounced;
	protected bool m_bSeizeAnnounced;
	protected string m_sTaskHold;
	protected string m_sTaskSeize;

	protected static const int TICK_MS = 1000;
	protected static const int INFO_RESEND_SECONDS = 15;	//!< m_iSecondsLeft is replicated this often
	protected static const string LOG = "[CRF_HoldAndRelieve]";
	protected static const string MODE = "HoldAndRelieve";

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		// Polled: the Lobby managers may not exist yet this early
		GetGame().GetCallqueue().CallLater(WaitForRoundLive, 1000, true);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(WaitForRoundLive);
			GetGame().GetCallqueue().Remove(Tick);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		super.OnDelete(owner);
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

		if (m_bRoundLive && m_iPhase != CRF_EHoldAndRelievePhase.ENDED)
		{
			m_bEndRequested = true;
			GetGame().GetCallqueue().Remove(Tick);
			m_iPhase = CRF_EHoldAndRelievePhase.ENDED;
			Replication.BumpMe();
		}
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		IEntity zone = CRF_SpecialsHelper.FindNamedEntity(m_sZoneAnchorName, LOG);
		if (!zone)
		{
			Print(LOG + " The round went live without its zone entity: the mode is not running.", LogLevel.ERROR);
			return;
		}

		if (!m_aReliefCallsigns || m_aReliefCallsigns.IsEmpty())
			Print(LOG + " No relief callsigns set: nobody can link up, so only the hold time or a seizure decides the round.", LogLevel.WARNING);

		m_bRoundLive = true;
		m_vZonePosition = zone.GetOrigin();
		if (m_iHoldMinutes > 0)
		{
			m_fDeadline = GetGame().GetWorld().GetWorldTime() + m_iHoldMinutes * 60000;
			m_iSecondsLeft = m_iHoldMinutes * 60;
		}

		m_iPhase = CRF_EHoldAndRelievePhase.LIVE;
		Replication.BumpMe();

		CreateTasks();

		string defenders = CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefendingFaction, "HOLD " + GetUpperLabel(), "The cut-off element holds until the relief force links up." + DescribeHoldTime(), 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "SEIZE " + GetUpperLabel(), string.Format("%1 is cut off there and a relief force is coming. Take it before they link up.", defenders), 10);

		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	protected string DescribeHoldTime()
	{
		if (m_iHoldMinutes <= 0)
			return "";

		return string.Format(" Or hold out for %1 minutes.", m_iHoldMinutes);
	}

	//! string.ToUpper() changes the string in place and returns its length, so work on a copy
	protected string GetUpperLabel()
	{
		string label = m_sZoneLabel;
		label.ToUpper();
		return label;
	}

	//===================================================================================
	// SERVER: THE ROUND
	//===================================================================================

	protected void Tick()
	{
		if (m_bEndRequested)
			return;

		array<int> defenders = {};
		array<int> attackers = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefendingFaction, defenders);
		CRF_SpecialsHelper.GetAlivePlayers(m_sAttackingFaction, attackers);

		array<int> relief = {};
		foreach (int playerId : defenders)
		{
			if (IsReliefPlayer(playerId))
				relief.Insert(playerId);
		}

		int defendersInZone = CRF_SpecialsHelper.CountPlayersWithin(defenders, m_vZonePosition, m_fZoneRadius);
		int reliefInZone = CRF_SpecialsHelper.CountPlayersWithin(relief, m_vZonePosition, m_fZoneRadius);
		int attackersInZone = CRF_SpecialsHelper.CountPlayersWithin(attackers, m_vZonePosition, m_fZoneRadius);

		TickLinkUp(reliefInZone, attackersInZone);
		if (m_bEndRequested)
			return;

		TickSeize(attackersInZone, defendersInZone);
		if (m_bEndRequested)
			return;

		TickClock();
	}

	protected void TickLinkUp(int reliefInZone, int attackersInZone)
	{
		bool linking = reliefInZone >= m_iLinkUpPlayers && attackersInZone <= m_iLinkUpMaxAttackers;
		int held = 0;
		if (linking)
			held = m_iLinkUpHeld + 1;

		SetLinkUpHeld(held);

		if (linking && !m_bLinkUpAnnounced)
		{
			m_bLinkUpAnnounced = true;
			CRF_RadioTransmissionEvents.PopupToFaction(m_sDefendingFaction, "RELIEF AT " + GetUpperLabel(), string.Format("Hold it together for %1 seconds to link up.", m_iLinkUpSeconds), 8);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "ENEMY RELIEF AT " + GetUpperLabel(), "Push them out before they link up.", 8);
		}
		else if (!linking)
		{
			m_bLinkUpAnnounced = false;
		}

		if (linking && held >= m_iLinkUpSeconds)
			EndRound(true, string.Format("The relief force linked up at %1.", m_sZoneLabel));
	}

	protected void TickSeize(int attackersInZone, int defendersInZone)
	{
		bool seizing = attackersInZone >= m_iSeizePlayers && defendersInZone == 0;
		int held = 0;
		if (seizing)
			held = m_iSeizeHeld + 1;

		SetSeizeHeld(held);

		if (seizing && !m_bSeizeAnnounced)
		{
			m_bSeizeAnnounced = true;
			CRF_RadioTransmissionEvents.PopupToFaction(m_sDefendingFaction, GetUpperLabel() + " IS FALLING", string.Format("Nobody of ours is left inside. Get back in within %1 seconds.", m_iSeizeSeconds), 8);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, GetUpperLabel() + " IS CLEAR", string.Format("Hold it for %1 seconds to seize it.", m_iSeizeSeconds), 8);
		}
		else if (!seizing)
		{
			m_bSeizeAnnounced = false;
		}

		if (seizing && held >= m_iSeizeSeconds)
			EndRound(false, string.Format("%1 seized %2.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction), m_sZoneLabel));
	}

	protected void TickClock()
	{
		if (m_fDeadline <= 0)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		int secondsLeft = Math.Max(0, Math.Ceil((m_fDeadline - now) / 1000));
		if (secondsLeft <= 0)
		{
			EndRound(true, string.Format("%1 held %2 for %3 minutes.", CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction), m_sZoneLabel, m_iHoldMinutes));
			return;
		}

		if (m_iSecondsLeft - secondsLeft >= INFO_RESEND_SECONDS || secondsLeft <= 60)
		{
			m_iSecondsLeft = secondsLeft;
			Replication.BumpMe();
			UpdateTaskText();
		}
	}

	protected void SetLinkUpHeld(int held)
	{
		if (held == m_iLinkUpHeld)
			return;

		m_iLinkUpHeld = held;
		Replication.BumpMe();
		CRF_ObjectiveTasks.SetProgress(m_sTaskHold, held / Math.Max(m_iLinkUpSeconds, 1.0));
	}

	protected void SetSeizeHeld(int held)
	{
		if (held == m_iSeizeHeld)
			return;

		m_iSeizeHeld = held;
		Replication.BumpMe();
		CRF_ObjectiveTasks.SetProgress(m_sTaskSeize, held / Math.Max(m_iSeizeSeconds, 1.0));
	}

	//! Relief: the player's group callsign starts with one of m_aReliefCallsigns
	protected bool IsReliefPlayer(int playerId)
	{
		if (!m_aReliefCallsigns || m_aReliefCallsigns.IsEmpty())
			return false;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return false;

		SCR_AIGroup group = groupsManager.GetPlayerGroup(playerId);
		if (!group)
			return false;

		string company, platoon, squad, character, format;
		group.GetCallsigns(company, platoon, squad, character, format);
		squad.ToLower();
		foreach (string callsign : m_aReliefCallsigns)
		{
			string prefix = callsign;
			prefix.ToLower();
			if (!prefix.IsEmpty() && squad.StartsWith(prefix))
				return true;
		}

		return false;
	}

	//===================================================================================
	// SERVER: TASKS AND THE END
	//===================================================================================

	protected void CreateTasks()
	{
		m_sTaskHold = CRF_ObjectiveTasks.MakeId(MODE, "Hold", m_sDefendingFaction);
		CRF_ObjectiveTasks.Create(m_sTaskHold, m_sDefendingFaction, "Hold " + m_sZoneLabel, DescribeHoldTask(),
			CRF_ObjectiveTasks.ICON_DEFEND, SCR_ETaskState.COMPLETED, m_vZonePosition);

		m_sTaskSeize = CRF_ObjectiveTasks.MakeId(MODE, "Seize", m_sAttackingFaction);
		CRF_ObjectiveTasks.Create(m_sTaskSeize, m_sAttackingFaction, "Seize " + m_sZoneLabel,
			string.Format("Hold %1 (%2 m) with at least %3 of you and nobody of theirs alive inside for %4 seconds. Stop the relief force getting there.", m_sZoneLabel, Math.Round(m_fZoneRadius), m_iSeizePlayers, m_iSeizeSeconds),
			CRF_ObjectiveTasks.ICON_SEIZE, SCR_ETaskState.FAILED, m_vZonePosition);
	}

	protected string DescribeHoldTask()
	{
		string text = string.Format("Keep someone alive inside %1 (%2 m). The relief force links up when %3 of them hold it for %4 seconds.", m_sZoneLabel, Math.Round(m_fZoneRadius), m_iLinkUpPlayers, m_iLinkUpSeconds);
		if (m_iSecondsLeft >= 0)
			text += string.Format(" Otherwise hold out: %1 left.", CRF_SpecialsHelper.FormatSeconds(m_iSecondsLeft));

		return text;
	}

	protected void UpdateTaskText()
	{
		CRF_ObjectiveTasks.SetText(m_sTaskHold, "Hold " + m_sZoneLabel, DescribeHoldTask());
	}

	protected void EndRound(bool defendersWon, string reason)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		GetGame().GetCallqueue().Remove(Tick);

		m_iPhase = CRF_EHoldAndRelievePhase.ENDED;
		m_bDefendersWon = defendersWon;
		Replication.BumpMe();

		string winner = m_sAttackingFaction;
		if (defendersWon)
			winner = m_sDefendingFaction;

		map<string, string> taskIds = new map<string, string>();
		taskIds.Set(m_sDefendingFaction, m_sTaskHold);
		taskIds.Set(m_sAttackingFaction, m_sTaskSeize);
		CRF_ObjectiveTasks.Resolve(taskIds, winner);

		AnnounceResult(winner, reason);
		Print(string.Format("%1 Round over, %2 wins: %3", LOG, winner, reason), LogLevel.NORMAL);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	protected void AnnounceResult(string winner, string reason)
	{
		string neutralTitle = string.Format("Winner: %1", CRF_SpecialsHelper.GetFactionName(winner));

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string faction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string title = neutralTitle;
			if (faction == winner)
				title = "VICTORY";
			else if (faction == m_sDefendingFaction || faction == m_sAttackingFaction)
				title = "DEFEAT";

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, title, reason, 12);
		}
	}

	//===================================================================================
	// EVERY MACHINE
	//===================================================================================

	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iPhase == CRF_EHoldAndRelievePhase.WAITING)
		{
			lines.Insert(string.Format("%1: starts when the round goes live", m_sZoneLabel));
			return;
		}

		if (m_iPhase == CRF_EHoldAndRelievePhase.ENDED)
		{
			string winner = CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction);
			if (m_bDefendersWon)
				winner = CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction);
			lines.Insert(string.Format("%1: round over, %2 won", m_sZoneLabel, winner));
			return;
		}

		if (m_iSecondsLeft >= 0)
			lines.Insert(string.Format("%1: %2 left to hold", m_sZoneLabel, CRF_SpecialsHelper.FormatSeconds(m_iSecondsLeft)));

		if (m_iLinkUpHeld > 0)
			lines.Insert(string.Format("%1: relief linking up (%2/%3 s)", m_sZoneLabel, m_iLinkUpHeld, m_iLinkUpSeconds));

		if (m_iSeizeHeld > 0)
			lines.Insert(string.Format("%1: being seized (%2/%3 s)", m_sZoneLabel, m_iSeizeHeld, m_iSeizeSeconds));
	}
}
