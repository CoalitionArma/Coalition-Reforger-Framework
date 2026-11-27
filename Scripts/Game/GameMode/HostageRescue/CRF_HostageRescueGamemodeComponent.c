//------------------------------------------------------------------------------------
// CRF_HostageRescueGamemodeComponent: an assault force has to get player hostages out alive
//
// Three sides: the attacking faction (the rescue force), the defending faction (the captors) and the
// hostage faction (player slots, normally CIV). Every living player of the hostage faction is a hostage.
//   - Safe start: the captors pick the hostage site. When the leader of a keeper group (squad callsign
//     starting with an m_aKeeperCallsigns entry) forward deploys, every hostage goes with them. Hostages
//     can't forward deploy on their own. Needs a COA_ForwardDeployBorder visible to the defending AND the
//     hostage faction, so a hostage moved into it is still inside a zone of their own.
//   - Round live: the hostages are restrained (ACE Captives). The rescue force frees them (ACE release)
//     or carries them out.
//   - A living hostage inside the extraction radius is rescued: they leave the round alive.
//   - Rescue force's result: m_fRescueShare of the hostages (counted at round live) rescued.
//   - Captors' result: the clock runs out, so many hostages died that the share can't be reached any
//     more, or (m_bWipeLoses) the rescue force has nobody left alive.
//   - A hostage killed by the defending faction (an execution) is handled by m_eExecutionOutcome: both
//     sides lose (default), the hostage is lost like any other death, or it counts as rescued. Any other
//     death loses that hostage.
//   - The mode never ends the round. The first result is final: it is announced to everyone and logged to
//     the admins, and play goes on (rescues and deaths are still annotated) until an admin ends it.
//   - Hostage events (moved, restrained, freed, recaptured, down, rescued, killed) are shown as hints to
//     the sides that would know about them.
//   - Optional intel: every few minutes the attackers are told which 1 km grid square the hostages are in.
// Only living players count; AI is ignored.
//
// Setup for mission makers (guide: !Docs/MissionMaking/Gamemodes/CRF_HOSTAGE_RESCUE_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the COA_Lobby gamemode entity.
//   2. Place a named entity at the extraction point (m_sExtractionName).
//   3. Give the hostage faction player slots and a safe start zone at the captors' start.
//   4. Place a forward deploy zone (COA_ForwardDeployBorder) with m_aVisibleForFactions = defending +
//      hostage faction over the area the captors may pick from.
//   5. Name the captors' keeper group so its callsign starts with an m_aKeeperCallsigns entry.
//------------------------------------------------------------------------------------

enum CRF_EHostageRescuePhase
{
	WAITING,	//!< the round isn't live yet
	LIVE,		//!< no result yet
	DECIDED		//!< a result was announced; the round goes on until an admin ends it
}

//! What a hostage killed by the captors means
enum CRF_EHostageExecution
{
	RESCUED,	//!< counts as rescued: killing hostages never helps the captors
	LOST,		//!< lost, like any other death
	BOTH_LOSE	//!< decides the round at once: both sides defeated
}

//! The round's result (replicated)
enum CRF_EHostageRescueOutcome
{
	CAPTORS_WON,
	RESCUERS_WON,
	BOTH_LOST
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Hostage Rescue: player hostages, captors who pick where to hold them, and a rescue force that has to get them out.")]
class CRF_HostageRescueGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_HostageRescueGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the rescue force", category: "Hostage Rescue - Factions")]
	protected string m_sAttackingFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the captors", category: "Hostage Rescue - Factions")]
	protected string m_sDefendingFaction;

	[Attribute("CIV", UIWidgets.EditBox, "Faction key of the hostages: every living player of this faction is a hostage", category: "Hostage Rescue - Factions")]
	protected string m_sHostageFaction;

	[Attribute("", UIWidgets.Auto, "Captor groups that hold the hostages: when the leader of a group whose squad callsign starts with one of these (not case-sensitive) forward deploys during safe start, every hostage goes with them", category: "Hostage Rescue - Hostages")]
	protected ref array<string> m_aKeeperCallsigns;

	[Attribute("1", UIWidgets.CheckBox, "Restrain every hostage (ACE Captives) when the round goes live", category: "Hostage Rescue - Hostages")]
	protected bool m_bRestrainAtRoundLive;

	[Attribute("2", UIWidgets.ComboBox, "A hostage killed by the captors (an execution). BOTH_LOSE: decides the round with both sides defeated. LOST: the hostage is lost like any other death. RESCUED: it counts as a rescue, so killing hostages never helps the captors.", enums: ParamEnumArray.FromEnum(CRF_EHostageExecution), category: "Hostage Rescue - Hostages")]
	protected CRF_EHostageExecution m_eExecutionOutcome;

	[Attribute("hostage_extract", UIWidgets.EditBox, "Name of an entity at the extraction point", category: "Hostage Rescue - Extraction")]
	protected string m_sExtractionName;

	[Attribute("60", UIWidgets.EditBox, "Metres from the extraction entity (on the map, height ignored) a living hostage has to reach to be rescued", params: "5 1000 1", category: "Hostage Rescue - Extraction")]
	protected float m_fExtractionRadius;

	[Attribute("0.6", UIWidgets.EditBox, "Share of the hostages slotted when the round goes live that has to be rescued for the rescue force to win, rounded up (at least one). 0.6 of 5 = 3.", params: "0.05 1 0.05", category: "Hostage Rescue - Extraction")]
	protected float m_fRescueShare;

	[Attribute("60", UIWidgets.EditBox, "Minutes from the round going live until the captors win", params: "1 600 1", category: "Hostage Rescue - Result")]
	protected int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "The captors win when the rescue force has nobody left alive", category: "Hostage Rescue - Result")]
	protected bool m_bWipeLoses;

	[Attribute("10", UIWidgets.EditBox, "Minutes after the round goes live of the first intel report to the rescue force: the 1 km grid square the hostages are in. 0: no intel.", params: "0 600 1", category: "Hostage Rescue - Intel")]
	protected int m_iIntelFirstMinutes;

	[Attribute("15", UIWidgets.EditBox, "Minutes between intel reports after the first. 0: only the first.", params: "0 600 1", category: "Hostage Rescue - Intel")]
	protected int m_iIntelIntervalMinutes;

	// Replicated: the pause-menu mission info
	[RplProp()]
	protected int m_iPhase;
	[RplProp()]
	protected CRF_EHostageRescueOutcome m_eOutcome;
	[RplProp()]
	protected int m_iHostages;
	[RplProp()]
	protected int m_iNeeded;
	[RplProp()]
	protected int m_iRescued;
	[RplProp()]
	protected int m_iLost;
	[RplProp()]
	protected int m_iSecondsLeft = -1;

	// Server
	protected COA_Gamemode m_Gamemode;
	protected bool m_bRoundLive;
	protected bool m_bStopped;				//!< the mission left the game state
	protected vector m_vExtraction;
	protected float m_fDeadline;			//!< world time (ms) the clock runs out
	protected float m_fNextIntel;			//!< world time (ms) of the next intel report, 0 = none
	protected int m_iTicks;
	protected ref set<int> m_aCounted = new set<int>();					//!< hostages already rescued or lost
	protected ref map<int, int> m_mHostageState = new map<int, int>();	//!< STATE_* flags last seen per hostage
	protected string m_sTaskRescue;
	protected string m_sTaskHold;

	protected static const int TICK_MS = 1000;
	protected static const int INFO_RESEND_SECONDS = 15;
	protected static const int QUIET_TICKS = 3;		//!< ticks after round live before captive changes are annotated
	protected static const int STATE_CAPTIVE = 1;
	protected static const int STATE_DOWN = 2;
	protected static const string LOG = "[CRF_HostageRescue]";
	protected static const string MODE = "HostageRescue";

	protected static CRF_HostageRescueGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_HostageRescueGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_HostageRescueGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	//! Every machine: the forward deploy override asks it about hostages
	static CRF_HostageRescueGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

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

	//! An admin ended the mission: stop watching
	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode || m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			return;

		m_bStopped = true;
		GetGame().GetCallqueue().Remove(Tick);
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		IEntity extraction = CRF_SpecialsHelper.FindNamedEntity(m_sExtractionName, LOG);
		if (!extraction)
		{
			Print(LOG + " The round went live without its extraction entity: the mode is not running.", LogLevel.ERROR);
			return;
		}

		array<int> hostages = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sHostageFaction, hostages);
		if (hostages.IsEmpty())
		{
			Print(LOG + " The round went live with no hostages slotted: the mode is not running.", LogLevel.WARNING);
			return;
		}

		m_bRoundLive = true;
		m_vExtraction = extraction.GetOrigin();
		m_iHostages = hostages.Count();
		m_iNeeded = Math.Max(1, Math.Ceil(m_iHostages * m_fRescueShare));
		m_iNeeded = Math.Min(m_iNeeded, m_iHostages);

		float now = GetGame().GetWorld().GetWorldTime();
		m_fDeadline = now + m_iTimeLimitMinutes * 60000;
		m_iSecondsLeft = m_iTimeLimitMinutes * 60;
		if (m_iIntelFirstMinutes > 0)
			m_fNextIntel = now + m_iIntelFirstMinutes * 60000;

		m_iPhase = CRF_EHostageRescuePhase.LIVE;
		Replication.BumpMe();

		if (m_bRestrainAtRoundLive)
		{
			foreach (int playerId : hostages)
				Restrain(playerId);

			Hint(m_sDefendingFaction, "Hostages restrained", string.Format("All %1 hostages are tied up.", m_iHostages));
			Hint(m_sHostageFaction, "Restrained", "You are tied up. Wait to be freed.");
		}

		CreateTasks();

		string captors = CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "RESCUE THE HOSTAGES",
			string.Format("%1 hold %2 hostages. Get at least %3 of them to the extraction point alive within %4 minutes.", captors, m_iHostages, m_iNeeded, m_iTimeLimitMinutes), 12);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefendingFaction, "HOLD THE HOSTAGES",
			string.Format("Keep them for %1 minutes. A rescue force is coming. %2", m_iTimeLimitMinutes, DescribeExecutionRule()), 12);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sHostageFaction, "YOU ARE A HOSTAGE",
			"Stay with your captors until you are freed. Then follow your rescuers to the extraction point.", 12);

		Print(string.Format("%1 Round live: %2 hostages, %3 needed, %4 min.", LOG, m_iHostages, m_iNeeded, m_iTimeLimitMinutes), LogLevel.NORMAL);
		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//! Server: tie the hostage up (ACE Captives); the rescue force can free them with the ACE release action
	protected void Restrain(int playerId)
	{
		SCR_CharacterControllerComponent controller = GetController(playerId);
		if (!controller)
			return;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(controller.GetOwner());
		CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
		if (compartmentAccess && compartmentAccess.IsInCompartment())
			return;

		if (!controller.ACE_Captives_IsCaptive())
			controller.ACE_Captives_SetCaptive(true);
	}

	protected SCR_CharacterControllerComponent GetController(int playerId)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId));
		if (!character)
			return null;

		return SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
	}

	//===================================================================================
	// SERVER: THE ROUND
	//===================================================================================

	protected void Tick()
	{
		if (m_bStopped)
			return;

		m_iTicks++;

		array<int> hostages = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sHostageFaction, hostages);
		foreach (int playerId : hostages)
		{
			if (m_aCounted.Contains(playerId))
				continue;

			IEntity entity = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId);
			if (!entity)
				continue;

			if (vector.DistanceXZ(entity.GetOrigin(), m_vExtraction) <= m_fExtractionRadius)
			{
				m_aCounted.Insert(playerId);
				m_mHostageState.Remove(playerId);
				CRF_SpecialsHelper.ExtractPlayer(playerId);
				CountRescue(string.Format("%1 reached the extraction point.", GetPlayerName(playerId)));
				continue;
			}

			WatchHostage(playerId);
		}

		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fNextIntel > 0 && now >= m_fNextIntel)
		{
			SendIntel(hostages);
			m_fNextIntel = 0;
			if (m_iIntelIntervalMinutes > 0)
				m_fNextIntel = now + m_iIntelIntervalMinutes * 60000;
		}

		if (m_iPhase != CRF_EHostageRescuePhase.LIVE)
			return;

		if (m_bWipeLoses)
		{
			array<int> attackers = {};
			CRF_SpecialsHelper.GetAlivePlayers(m_sAttackingFaction, attackers);
			if (attackers.IsEmpty())
			{
				Decide(CRF_EHostageRescueOutcome.CAPTORS_WON, string.Format("%1 has nobody left alive.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction)));
				return;
			}
		}

		TickClock(now);
	}

	//! Hints when a hostage is freed, tied up again, goes down or comes round. The first QUIET_TICKS only
	//! record the state: the restraint at round live takes a moment to register.
	protected void WatchHostage(int playerId)
	{
		SCR_CharacterControllerComponent controller = GetController(playerId);
		if (!controller)
			return;

		int state = 0;
		if (controller.ACE_Captives_IsCaptive())
			state |= STATE_CAPTIVE;
		if (controller.GetLifeState() == ECharacterLifeState.INCAPACITATED)
			state |= STATE_DOWN;

		int previous;
		bool known = m_mHostageState.Find(playerId, previous);
		m_mHostageState.Set(playerId, state);
		if (!known || m_iTicks <= QUIET_TICKS || state == previous)
			return;

		bool wasCaptive = (previous & STATE_CAPTIVE) != 0;
		bool isCaptive = (state & STATE_CAPTIVE) != 0;
		bool wasDown = (previous & STATE_DOWN) != 0;
		bool isDown = (state & STATE_DOWN) != 0;

		string name = GetPlayerName(playerId);
		if (wasCaptive && !isCaptive)
		{
			Hint(m_sAttackingFaction, "Hostage freed", string.Format("%1 is free. Get them to the extraction point.", name));
			Hint(m_sHostageFaction, "Hostage freed", string.Format("%1 is free.", name));
		}
		else if (!wasCaptive && isCaptive)
		{
			Hint(m_sDefendingFaction, "Hostage recaptured", string.Format("%1 is tied up again.", name));
			Hint(m_sHostageFaction, "Hostage recaptured", string.Format("%1 is tied up again.", name));
		}

		if (!wasDown && isDown)
			HintAll("Hostage down", string.Format("%1 is wounded and unconscious.", name));
		else if (wasDown && !isDown)
			HintAll("Hostage up", string.Format("%1 is conscious again.", name));
	}

	protected void TickClock(float now)
	{
		int secondsLeft = Math.Max(0, Math.Ceil((m_fDeadline - now) / 1000));
		if (secondsLeft <= 0)
		{
			m_iSecondsLeft = 0;
			Replication.BumpMe();
			Decide(CRF_EHostageRescueOutcome.CAPTORS_WON, string.Format("%1 held the hostages for %2 minutes.", CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction), m_iTimeLimitMinutes));
			return;
		}

		if (m_iSecondsLeft - secondsLeft >= INFO_RESEND_SECONDS || secondsLeft <= 60)
		{
			m_iSecondsLeft = secondsLeft;
			Replication.BumpMe();
		}
	}

	//! A hostage got out (or an execution counted as a rescue). Decides the round when enough are out.
	protected void CountRescue(string what)
	{
		m_iRescued++;
		Replication.BumpMe();
		UpdateTasks();

		HintAll("Hostage rescued", string.Format("%1 Rescued: %2 of %3 needed.", what, m_iRescued, m_iNeeded));
		Print(string.Format("%1 Rescued %2 of %3: %4", LOG, m_iRescued, m_iNeeded, what), LogLevel.NORMAL);

		if (m_iPhase == CRF_EHostageRescuePhase.LIVE && m_iRescued >= m_iNeeded)
			Decide(CRF_EHostageRescueOutcome.RESCUERS_WON, string.Format("%1 rescued %2 of %3 hostages.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction), m_iRescued, m_iHostages));
	}

	//! A hostage died and can't be rescued any more. Decides the round when the share can't be reached.
	protected void CountLoss(string what)
	{
		m_iLost++;
		Replication.BumpMe();
		UpdateTasks();

		HintAll("Hostage killed", string.Format("%1 Lost: %2. Rescued: %3 of %4 needed.", what, m_iLost, m_iRescued, m_iNeeded));
		Print(string.Format("%1 Lost %2: %3", LOG, m_iLost, what), LogLevel.NORMAL);

		if (m_iPhase == CRF_EHostageRescuePhase.LIVE && m_iHostages - m_iLost < m_iNeeded)
			Decide(CRF_EHostageRescueOutcome.CAPTORS_WON, string.Format("Too many hostages died: %1 of %2. %3 needed %4.", m_iLost, m_iHostages, CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction), m_iNeeded));
	}

	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || !m_bRoundLive || m_bStopped)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0 || m_aCounted.Contains(victimId))
			return;

		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(victimId) != m_sHostageFaction)
			return;

		m_aCounted.Insert(victimId);
		m_mHostageState.Remove(victimId);

		int killerId = instigatorContextData.GetKillerPlayerID();
		string killerFaction;
		if (killerId > 0 && killerId != victimId)
			killerFaction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(killerId);

		string name = GetPlayerName(victimId);
		if (killerFaction != m_sDefendingFaction)
		{
			string by = "";
			if (killerFaction == m_sAttackingFaction)
				by = string.Format(" by %1", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction));
			CountLoss(string.Format("%1 was killed%2.", name, by));
			return;
		}

		Print(string.Format("%1 Hostage %2 executed by %3 (rule %4).", LOG, name, GetPlayerName(killerId), typename.EnumToString(CRF_EHostageExecution, m_eExecutionOutcome)), LogLevel.NORMAL);
		if (m_eExecutionOutcome == CRF_EHostageExecution.RESCUED)
		{
			CountRescue(string.Format("%1 was executed by the captors: it counts as a rescue.", name));
			return;
		}

		if (m_eExecutionOutcome == CRF_EHostageExecution.LOST)
		{
			CountLoss(string.Format("%1 was executed by the captors.", name));
			return;
		}

		// BOTH_LOSE: the hostage is lost and, if nothing was decided yet, both sides have failed
		m_iLost++;
		Replication.BumpMe();
		UpdateTasks();
		HintAll("Hostage executed", string.Format("%1 was executed by the captors.", name));
		if (m_iPhase == CRF_EHostageRescuePhase.LIVE)
			Decide(CRF_EHostageRescueOutcome.BOTH_LOST, string.Format("%1 was executed by the captors. Both sides have failed.", name));
	}

	//! Attackers only: the 1 km square around the hostages still held (their mean position)
	protected void SendIntel(notnull array<int> hostages)
	{
		vector sum;
		int count;
		foreach (int playerId : hostages)
		{
			if (m_aCounted.Contains(playerId))
				continue;

			IEntity entity = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId);
			if (!entity)
				continue;

			sum = sum + entity.GetOrigin();
			count++;
		}

		if (count == 0)
			return;

		vector mean = sum * (1.0 / count);
		int east = Math.Floor(mean[0] / 1000) * 10;
		int north = Math.Floor(mean[2] / 1000) * 10;
		string text = string.Format("The hostages are in the 1 km grid square %1-%2 east, %3-%4 north.", FormatGrid(east), FormatGrid(east + 10), FormatGrid(north), FormatGrid(north + 10));
		Hint(m_sAttackingFaction, "Intel", text);
		Print(string.Format("%1 Intel to %2: %3", LOG, m_sAttackingFaction, text), LogLevel.NORMAL);
	}

	//! Map grid label in 100 m units, three digits ("085")
	protected string FormatGrid(int hundreds)
	{
		return hundreds.ToString(3);
	}

	protected string GetPlayerName(int playerId)
	{
		if (playerId <= 0)
			return "A hostage";

		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return "A hostage";

		return name;
	}

	//===================================================================================
	// SERVER: HINTS
	//===================================================================================

	//! A hint card ("[title] text") for one faction's players
	protected void Hint(string factionKey, string title, string text)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager && !factionKey.IsEmpty())
			broadcastManager.SendHint(string.Format("[%1] %2", title, text), -1, factionKey);
	}

	//! The same hint for the rescue force, the captors and the hostages
	protected void HintAll(string title, string text)
	{
		Hint(m_sAttackingFaction, title, text);
		Hint(m_sDefendingFaction, title, text);
		Hint(m_sHostageFaction, title, text);
	}

	//===================================================================================
	// SERVER: FORWARD DEPLOY (called by the modded COA_ForwardDeployManager)
	//===================================================================================

	//! Every machine: hostages don't get the forward deploy option and can't request it
	string GetHostageFaction()
	{
		return m_sHostageFaction;
	}

	bool IsHostage(int playerId)
	{
		return CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) == m_sHostageFaction;
	}

	//! Server: playerId's group is forward deploying. If they lead a keeper group, outHostages gets every
	//! living hostage, to be moved to the same spot.
	bool GetHostagesToMove(int playerId, notnull array<int> outHostages)
	{
		outHostages.Clear();
		if (m_bRoundLive || !m_aKeeperCallsigns || m_aKeeperCallsigns.IsEmpty())
			return false;

		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sDefendingFaction)
			return false;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return false;

		SCR_AIGroup group = groupsManager.GetPlayerGroup(playerId);
		if (!group || !IsKeeperGroup(group) || !COA_ForwardDeployManager.CanLeadForwardDeploy(playerId, group))
			return false;

		CRF_SpecialsHelper.GetAlivePlayers(m_sHostageFaction, outHostages);
		if (outHostages.IsEmpty())
			return false;

		Print(string.Format("%1 %2 moved the hostage site: %3 hostages follow.", LOG, GetPlayerName(playerId), outHostages.Count()), LogLevel.NORMAL);
		Hint(m_sDefendingFaction, "Hostages moved", string.Format("%1 moved the hostages to the new site.", GetPlayerName(playerId)));
		Hint(m_sHostageFaction, "Moved", "Your captors have moved you to their chosen site.");
		return true;
	}

	protected bool IsKeeperGroup(notnull SCR_AIGroup group)
	{
		string company, platoon, squad, character, format;
		group.GetCallsigns(company, platoon, squad, character, format);
		squad.ToLower();
		foreach (string callsign : m_aKeeperCallsigns)
		{
			string prefix = callsign;
			prefix.ToLower();
			if (!prefix.IsEmpty() && squad.StartsWith(prefix))
				return true;
		}

		return false;
	}

	//===================================================================================
	// SERVER: TASKS AND THE RESULT
	//===================================================================================

	//! No positions: the hostage site is a secret and tasks reach every client
	protected void CreateTasks()
	{
		m_sTaskRescue = CRF_ObjectiveTasks.MakeId(MODE, "Rescue", m_sAttackingFaction);
		CRF_ObjectiveTasks.Create(m_sTaskRescue, m_sAttackingFaction, "Rescue the hostages", DescribeRescueTask(),
			CRF_ObjectiveTasks.ICON_PROTECT, SCR_ETaskState.FAILED);

		m_sTaskHold = CRF_ObjectiveTasks.MakeId(MODE, "Hold", m_sDefendingFaction);
		CRF_ObjectiveTasks.Create(m_sTaskHold, m_sDefendingFaction, "Hold the hostages", DescribeHoldTask(),
			CRF_ObjectiveTasks.ICON_DEFEND, SCR_ETaskState.COMPLETED);
	}

	protected string DescribeRescueTask()
	{
		return string.Format("Get %1 of the %2 hostages to the extraction point alive. Rescued: %3. Killed: %4.", m_iNeeded, m_iHostages, m_iRescued, m_iLost);
	}

	protected string DescribeHoldTask()
	{
		return string.Format("Keep the hostages until the clock runs out. Rescued: %1 of %2 needed. %3", m_iRescued, m_iNeeded, DescribeExecutionRule());
	}

	protected void UpdateTasks()
	{
		if (m_iPhase != CRF_EHostageRescuePhase.LIVE)
			return;

		CRF_ObjectiveTasks.SetText(m_sTaskRescue, "Rescue the hostages", DescribeRescueTask());
		CRF_ObjectiveTasks.SetProgress(m_sTaskRescue, m_iRescued / Math.Max(m_iNeeded, 1.0));
		CRF_ObjectiveTasks.SetText(m_sTaskHold, "Hold the hostages", DescribeHoldTask());
	}

	//! The captors' side of the execution rule, for their popup and task
	protected string DescribeExecutionRule()
	{
		if (m_eExecutionOutcome == CRF_EHostageExecution.BOTH_LOSE)
			return "Executing a hostage means both sides lose.";

		if (m_eExecutionOutcome == CRF_EHostageExecution.RESCUED)
			return "A hostage you kill counts as rescued.";

		return "A hostage you kill is lost.";
	}

	//! The first result is final. It is announced and logged to the admins; the round goes on until an
	//! admin ends the mission (the mode never ends it).
	protected void Decide(CRF_EHostageRescueOutcome outcome, string reason)
	{
		if (m_iPhase != CRF_EHostageRescuePhase.LIVE)
			return;

		m_iPhase = CRF_EHostageRescuePhase.DECIDED;
		m_eOutcome = outcome;
		Replication.BumpMe();

		string winner;	// empty: nobody won
		if (outcome == CRF_EHostageRescueOutcome.CAPTORS_WON)
			winner = m_sDefendingFaction;
		else if (outcome == CRF_EHostageRescueOutcome.RESCUERS_WON)
			winner = m_sAttackingFaction;

		map<string, string> taskIds = new map<string, string>();
		taskIds.Set(m_sAttackingFaction, m_sTaskRescue);
		taskIds.Set(m_sDefendingFaction, m_sTaskHold);
		CRF_ObjectiveTasks.Resolve(taskIds, winner);

		AnnounceResult(winner, reason);

		string result = "both sides lost";
		if (!winner.IsEmpty())
			result = string.Format("%1 wins", CRF_SpecialsHelper.GetFactionName(winner));
		string message = string.Format("Hostage Rescue decided: %1. %2 End the mission from the admin menu.", result, reason);
		Print(string.Format("%1 %2", LOG, message), LogLevel.NORMAL);

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.LogAdminAction(message, -1, false, COA_EAdminLogLevel.High);
	}

	//! winner empty: both sides lost
	protected void AnnounceResult(string winner, string reason)
	{
		string neutralTitle = "MISSION FAILED";
		if (!winner.IsEmpty())
			neutralTitle = string.Format("Winner: %1", CRF_SpecialsHelper.GetFactionName(winner));

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string faction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string title = neutralTitle;
			if (!winner.IsEmpty() && faction == winner)
				title = "VICTORY";
			else if (faction == m_sDefendingFaction || faction == m_sAttackingFaction)
				title = "DEFEAT";

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, title, reason + " The mission goes on until an admin ends it.", 12);
		}
	}

	//===================================================================================
	// EVERY MACHINE
	//===================================================================================

	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iPhase == CRF_EHostageRescuePhase.WAITING)
		{
			lines.Insert("Hostages: the rescue starts when the round goes live");
			return;
		}

		lines.Insert(string.Format("Hostages: %1 of %2 needed rescued, %3 killed (of %4)", m_iRescued, m_iNeeded, m_iLost, m_iHostages));

		if (m_iPhase == CRF_EHostageRescuePhase.DECIDED)
		{
			if (m_eOutcome == CRF_EHostageRescueOutcome.BOTH_LOST)
			{
				lines.Insert("Hostages: decided, both sides lost (a hostage was executed)");
				return;
			}

			string winner = CRF_SpecialsHelper.GetFactionName(m_sDefendingFaction);
			if (m_eOutcome == CRF_EHostageRescueOutcome.RESCUERS_WON)
				winner = CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction);
			lines.Insert(string.Format("Hostages: decided, %1 won", winner));
			return;
		}

		if (m_iSecondsLeft >= 0)
			lines.Insert(string.Format("Hostages: %1 left", CRF_SpecialsHelper.FormatSeconds(m_iSecondsLeft)));
	}
}
