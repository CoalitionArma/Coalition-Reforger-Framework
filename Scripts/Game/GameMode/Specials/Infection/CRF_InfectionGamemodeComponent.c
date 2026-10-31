//------------------------------------------------------------------------------------
// CRF_InfectionGamemodeComponent: "Infected among us" - a hidden-infection special
//
// The survivors' side starts with a few secret carriers. Each carrier is told privately that they are
// infected (and who the other carriers are), and turns into one of the infected some minutes later.
// Until then they are still a survivor to everyone else - and can sabotage. Zombies (AI such as Bacon
// Zombies, or players who have turned) can infect survivors with melee hits; a bitten survivor is told
// and has a few minutes to find an antidote before they turn too. Survivors who die while infected,
// or who are killed by the infected, rise as infected.
//
// Turning moves the player into a free slot on the infected faction and respawns them where they
// stood, so the Lobby gives them that faction's gearscript, group, radios and name tags.
//
// The survivors escape in vehicles placed in the mission (m_aEscapeVehicleNames), which start with
// empty tanks, destroyed engines and damaged rotors: they have to be refuelled (jerrycans, fuel trucks)
// and repaired (repair kits) with the game's own tools, then driven or flown m_fEscapeDistance away.
// Survivors aboard escape; the infected win if every escape vehicle is destroyed, if no survivor is
// left alive, or if only the infected are aboard. Other objective components can still end the
// mission too, and m_iSurvivalTimeLimit optionally gives the survivors the win for holding out.
// At the end everyone is told who the carriers were, and whether the infection got out.
//
// Setup for mission makers:
//   1. Add this component to the gamemode entity.
//   2. Survivors' faction (default BLUFOR): the normal playable slots.
//   3. Infected faction (default OPFOR): groups with at least as many slots as there are survivors,
//      locked so nobody picks them in slotting. Give the infected faction a melee gearscript (a
//      zombie head from the Plastics Halloween Pack works well). Turned players fill these slots.
//   4. Make the infected faction friendly with the zombie AI's faction and hostile to the survivors.
//   5. List the zombie AI's faction keys in m_aZombieFactions so their hits infect.
//   6. Place CRF_AntidoteCrate prefabs for the antidote doses (a shared pool, m_iAntidoteDoses).
//   7. Place and name the escape vehicles, list them in m_aEscapeVehicleNames, and put jerrycans
//      (Jerrycan_01_item) and repair kits (RepairKit_01_wrench) around the map to fix them with.
//------------------------------------------------------------------------------------

enum CRF_EInfectionStatus
{
	HEALTHY,
	INCUBATING,	//!< infected, not turned yet - still on the survivors' side
	TURNED		//!< moved to the infected faction
}

//------------------------------------------------------------------------------------------------
// Per-player infection state - server side only
class CRF_InfectionState
{
	CRF_EInfectionStatus m_eStatus = CRF_EInfectionStatus.HEALTHY;
	bool m_bCarrier;            //!< one of the hidden carriers picked at the start (can't be cured)
	int m_iTurnAtMs;            //!< server world time they turn, while incubating
	int m_iLastWarningSecs = -1;
	bool m_bRisePending;        //!< died while infected (or to the infected); rising shortly
}

//------------------------------------------------------------------------------------------------
// An escape vehicle and how far along it is - server side only
class CRF_InfectionEscapeVehicle
{
	IEntity m_Vehicle;
	string m_sLabel;
	vector m_vStartPos;
	bool m_bFuelled;
	bool m_bRepaired;
	bool m_bGone;               //!< destroyed, or left without survivors
	bool m_bWarnedNotReady;     //!< told the survivors it left the area before it was ready
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Hidden infection special: secret carriers among the survivors, bites infect, the infected turn and hunt the rest.")]
class CRF_InfectionGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_InfectionGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the survivors", category: "Infection - Factions")]
	protected string m_sSurvivorFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the infected. Needs free (locked) slots for every survivor who might turn.", category: "Infection - Factions")]
	protected string m_sInfectedFaction;

	[Attribute("", UIWidgets.Auto, "Faction keys of zombie AI (e.g. Bacon Zombies) whose hits can infect survivors. Hits from players on the infected faction always can.", category: "Infection - Factions")]
	protected ref array<string> m_aZombieFactions;

	[Attribute("10", UIWidgets.EditBox, "One hidden carrier per this many survivors (at least one)", params: "2 100 1", category: "Infection - Carriers")]
	protected int m_iSurvivorsPerCarrier;

	[Attribute("60", UIWidgets.EditBox, "Seconds after the round goes live before the carriers are picked", params: "0 1800 1", category: "Infection - Carriers")]
	protected float m_fCarrierPickDelay;

	[Attribute("600", UIWidgets.EditBox, "Shortest time (seconds) a carrier takes to turn", params: "30 7200 1", category: "Infection - Carriers")]
	protected float m_fCarrierIncubationMin;

	[Attribute("1200", UIWidgets.EditBox, "Longest time (seconds) a carrier takes to turn", params: "30 7200 1", category: "Infection - Carriers")]
	protected float m_fCarrierIncubationMax;

	[Attribute("true", UIWidgets.CheckBox, "Carriers are told who the other carriers are", category: "Infection - Carriers")]
	protected bool m_bCarriersKnowEachOther;

	[Attribute("0.35", UIWidgets.Slider, "Chance (0-1) that a hit from a zombie infects the survivor", params: "0 1 0.05", category: "Infection - Bites")]
	protected float m_fBiteChance;

	[Attribute("false", UIWidgets.CheckBox, "Any damage from a zombie can infect, not only melee (for zombie AI that attacks some other way)", category: "Infection - Bites")]
	protected bool m_bAnyDamageInfects;

	[Attribute("120", UIWidgets.EditBox, "Shortest time (seconds) a bitten survivor takes to turn", params: "15 3600 1", category: "Infection - Bites")]
	protected float m_fBiteIncubationMin;

	[Attribute("240", UIWidgets.EditBox, "Longest time (seconds) a bitten survivor takes to turn", params: "15 3600 1", category: "Infection - Bites")]
	protected float m_fBiteIncubationMax;

	[Attribute("true", UIWidgets.CheckBox, "Survivors killed by the infected rise as infected (those who die while infected always do)", category: "Infection - Bites")]
	protected bool m_bKilledByInfectedRise;

	[Attribute("10", UIWidgets.EditBox, "Seconds between dying and rising as infected", params: "1 120 1", category: "Infection - Bites")]
	protected float m_fRiseDelay;

	[Attribute("false", UIWidgets.CheckBox, "Every survivor who dies rises as infected, however they died - nobody ends up spectating", category: "Infection - Bites")]
	protected bool m_bAllDeadSurvivorsRise;

	[Attribute("true", UIWidgets.CheckBox, "The infected never tire: running and sprinting don't drain their stamina (CRF_SCR_CharacterStaminaComponent)", category: "Infection - Infected")]
	protected bool m_bInfectedTireless;

	[Attribute("5", UIWidgets.EditBox, "Seconds after an infected dies before the body crumbles away, so survivors can't loot their gear (night vision). 0 = bodies stay.", params: "0 600 1", category: "Infection - Infected")]
	protected float m_fInfectedBodyRemoveDelay;

	[Attribute("", UIWidgets.Auto, "Names of the escape vehicles placed in the mission (helicopters, trucks...). They start with empty tanks and destroyed engines (rotors badly damaged): survivors refuel them (jerrycans, fuel trucks) and repair them (repair kits). Whoever is aboard when one gets m_fEscapeDistance away escapes.", category: "Infection - Escape")]
	protected ref array<string> m_aEscapeVehicleNames;

	[Attribute("", UIWidgets.Auto, "What players call each escape vehicle, in the same order as the names (e.g. \"Huey\")", category: "Infection - Escape")]
	protected ref array<string> m_aEscapeVehicleLabels;

	[Attribute("60", UIWidgets.EditBox, "Litres of fuel an escape vehicle needs to count as fuelled (a jerrycan holds 20)", params: "1 2000 1", category: "Infection - Escape")]
	protected float m_fEscapeFuelLitres;

	[Attribute("0.45", UIWidgets.Slider, "Health (0-1) every engine and rotor needs to count as repaired. A repair kit repairs up to 0.5.", params: "0.05 1 0.05", category: "Infection - Escape")]
	protected float m_fEscapeRepairedHealth;

	[Attribute("0.2", UIWidgets.Slider, "Health (0-1) the rotors start at (engines start destroyed)", params: "0 1 0.05", category: "Infection - Escape")]
	protected float m_fEscapeRotorStartHealth;

	[Attribute("2000", UIWidgets.EditBox, "Metres an escape vehicle has to get from where it started for the people aboard to escape", params: "100 20000 10", category: "Infection - Escape")]
	protected float m_fEscapeDistance;

	[Attribute("true", UIWidgets.CheckBox, "The infected win when every escape vehicle has been destroyed", category: "Infection - Escape")]
	protected bool m_bInfectedWinWhenNoEscape;

	[Attribute("4", UIWidgets.EditBox, "Antidote doses for the whole mission, shared by every CRF_AntidoteCrate", params: "0 100 1", category: "Infection - Antidote")]
	protected int m_iStartingAntidoteDoses;

	// Doses left (crates show it in their action name)
	[RplProp()]
	protected int m_iAntidoteDoses;

	[Attribute("true", UIWidgets.CheckBox, "End the mission as an infected win when no survivor is left alive", category: "Infection - Ending")]
	protected bool m_bInfectedWinWhenSurvivorsGone;

	[Attribute("0", UIWidgets.EditBox, "Survivors win if any are still alive after this many seconds of the round (0 = off, the mission's objectives decide)", params: "0 14400 1", category: "Infection - Ending")]
	protected int m_iSurvivalTimeLimit;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Infection - Ending")]
	protected bool m_bEndMissionOnVictory;

	// Seconds with no survivor alive before the infected win (covers respawn gaps)
	protected static const int NO_SURVIVOR_GRACE = 15;

	protected static CRF_InfectionGamemodeComponent s_Instance;

	// Server state
	protected ref map<int, ref CRF_InfectionState> m_mStates = new map<int, ref CRF_InfectionState>();
	protected ref array<string> m_aCarrierNames = {};
	protected ref array<string> m_aTurnedNames = {};
	protected ref array<ref CRF_InfectionEscapeVehicle> m_aEscapeVehicles = {};
	protected bool m_bSomeoneEscaped;
	protected ref array<string> m_aEscapedInfectedNames = {};	// still infected when they escaped
	protected bool m_bRoundLive;
	protected bool m_bCarriersPicked;
	protected bool m_bEnded;
	protected bool m_bEndRequested;	// this component asked for the mission to end (once)
	protected int m_iRoundStartMs;
	protected int m_iNoSurvivorSeconds;
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_Safestart;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_InfectionGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_InfectionGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_InfectionGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		m_iAntidoteDoses = m_iStartingAntidoteDoses;
		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(HookGamemode, 1000, false);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(PickCarriers);
			GetGame().GetCallqueue().Remove(HookGamemode);
			GetGame().GetCallqueue().Remove(RisePlayer);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Remove(OnSafestartChanged);

		super.OnDelete(owner);
	}

	protected void HookGamemode()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		if (!m_Gamemode)
		{
			Print("[CRF_Infection] No COA_Gamemode - the infection special is not running.", LogLevel.WARNING);
			return;
		}

		m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		// Empty and broken from the start, so nobody drives off during the briefing
		PrepareEscapeVehicles();

		// The round (and every incubation clock) starts when safestart ends, not when it begins
		m_Safestart = COA_SafestartManager.GetInstance();
		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Insert(OnSafestartChanged);

		OnGamemodeStateChanged();
	}

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive && m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		m_bRoundLive = true;
		m_iRoundStartMs = GetGame().GetWorld().GetWorldTime();
		GetGame().GetCallqueue().CallLater(PickCarriers, m_fCarrierPickDelay * 1000, false);
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
	}

	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode)
			return;

		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME && !m_bRoundLive)
		{
			// Missions with safestart start on OnSafestartChanged
			if (!m_Safestart || !m_Safestart.GetSafestartStatus())
				StartRound();
			return;
		}

		// The mission ended (whichever side won): say who was infected
		if (m_bRoundLive && m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME && !m_bEnded)
		{
			m_bEnded = true;
			GetGame().GetCallqueue().Remove(Tick);
			Reveal();
		}
	}

	//===================================================================================
	// CARRIERS
	//===================================================================================

	protected void PickCarriers()
	{
		if (m_bCarriersPicked || m_bEnded)
			return;

		m_bCarriersPicked = true;

		array<int> survivors = {};
		GetLivingSurvivors(survivors);
		if (survivors.Count() < 2)
		{
			Print("[CRF_Infection] Fewer than two survivors alive - no carriers picked.", LogLevel.WARNING);
			return;
		}

		int carrierCount = Math.Max(1, survivors.Count() / m_iSurvivorsPerCarrier);
		carrierCount = Math.Min(carrierCount, survivors.Count() - 1);

		// Random picks
		array<int> carriers = {};
		for (int i = 0; i < carrierCount; i++)
		{
			int pick = Math.RandomInt(0, survivors.Count());
			carriers.Insert(survivors[pick]);
			survivors.Remove(pick);
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		array<string> names = {};
		foreach (int carrierId : carriers)
		{
			names.Insert(playerManager.GetPlayerName(carrierId));
		}

		foreach (int carrierId : carriers)
		{
			CRF_InfectionState state = GetState(carrierId);
			state.m_bCarrier = true;
			Infect(carrierId, Math.RandomFloat(m_fCarrierIncubationMin, m_fCarrierIncubationMax), false);
			m_aCarrierNames.Insert(playerManager.GetPlayerName(carrierId));

			string others = "You're on your own.";
			if (m_bCarriersKnowEachOther && carriers.Count() > 1)
			{
				others = "The others: ";
				bool first = true;
				foreach (int otherId : carriers)
				{
					if (otherId == carrierId)
						continue;

					if (!first)
						others += ", ";
					others += playerManager.GetPlayerName(otherId);
					first = false;
				}
			}

			int minutes = Math.Max(1, Math.Round(GetSecondsToTurn(carrierId) / 60.0));
			CRF_RadioTransmissionEvents.PopupToPlayer(carrierId, "YOU ARE INFECTED",
				string.Format("Nobody knows. You turn in about %1 minutes - until then, help the infection. %2", minutes, others), 15);
		}

		CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, "SOMEONE IS INFECTED",
			string.Format("%1 of you carry the infection and don't show it yet. Trust nobody.", carrierCount), 10);
	}

	//===================================================================================
	// INFECTION
	//===================================================================================

	protected CRF_InfectionState GetState(int playerId)
	{
		CRF_InfectionState state = m_mStates.Get(playerId);
		if (!state)
		{
			state = new CRF_InfectionState();
			m_mStates.Set(playerId, state);
		}

		return state;
	}

	//! Infect a survivor: they turn after incubationSecs
	protected void Infect(int playerId, float incubationSecs, bool bitten)
	{
		CRF_InfectionState state = GetState(playerId);
		if (state.m_eStatus != CRF_EInfectionStatus.HEALTHY)
			return;

		state.m_eStatus = CRF_EInfectionStatus.INCUBATING;
		state.m_iTurnAtMs = GetGame().GetWorld().GetWorldTime() + incubationSecs * 1000;
		state.m_iLastWarningSecs = -1;

		if (bitten)
		{
			int minutes = Math.Max(1, Math.Round(incubationSecs / 60));
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "YOU'VE BEEN BITTEN",
				string.Format("You'll turn in about %1 minutes unless you find an antidote.", minutes), 10);
		}
	}

	protected int GetSecondsToTurn(int playerId)
	{
		CRF_InfectionState state = m_mStates.Get(playerId);
		if (!state || state.m_eStatus != CRF_EInfectionStatus.INCUBATING)
			return -1;

		return Math.Max(0, (state.m_iTurnAtMs - GetGame().GetWorld().GetWorldTime()) / 1000);
	}

	//! Server, from the damage hook (CRF_Infection_SCR_DamageManagerComponent): a zombie's hit may infect
	void OnCharacterDamaged(IEntity victim, notnull BaseDamageContext damageContext)
	{
		if (!m_bRoundLive || m_bEnded || !victim || damageContext.damageValue <= 0)
			return;

		if (!m_bAnyDamageInfects && damageContext.damageType != EDamageType.MELEE)
			return;

		int victimId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(victim);
		if (victimId <= 0 || !IsSurvivor(victimId))
			return;

		CRF_InfectionState state = m_mStates.Get(victimId);
		if (state && state.m_eStatus != CRF_EInfectionStatus.HEALTHY)
			return;

		IEntity attacker;
		if (damageContext.instigator)
			attacker = damageContext.instigator.GetInstigatorEntity();

		if (!IsInfectious(attacker) || Math.RandomFloat01() > m_fBiteChance)
			return;

		Infect(victimId, Math.RandomFloat(m_fBiteIncubationMin, m_fBiteIncubationMax), true);
	}

	//! A zombie: zombie AI (m_aZombieFactions) or a player on the infected faction
	protected bool IsInfectious(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		Faction faction = character.GetFaction();
		if (!faction)
			return false;

		string key = faction.GetFactionKey();
		return key == m_sInfectedFaction || (m_aZombieFactions && m_aZombieFactions.Contains(key));
	}

	//! On the survivors' slots (whether or not they're secretly infected)
	protected bool IsSurvivor(int playerId)
	{
		Faction faction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId, true);
		return faction && faction.GetFactionKey() == m_sSurvivorFaction;
	}

	//! Survivors with a living body
	protected void GetLivingSurvivors(notnull array<int> outPlayers)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (!IsSurvivor(playerId))
				continue;

			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || COA_EntityHelper.IsSpectator(entity))
				continue;

			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character || character.GetCharacterController().IsDead())
				continue;

			outPlayers.Insert(playerId);
		}
	}

	//===================================================================================
	// TICK (server, once a second while the round is live)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded || m_bEndRequested)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		array<int> warnings = {120, 30, 10};

		// Incubation: private warnings, then turning. Collected first - turning changes slots and
		// shouldn't happen while the state map is being walked.
		array<int> turning = {};
		foreach (int playerId, CRF_InfectionState state : m_mStates)
		{
			if (state.m_eStatus != CRF_EInfectionStatus.INCUBATING || state.m_bRisePending)
				continue;

			int secsLeft = (state.m_iTurnAtMs - now) / 1000;
			if (secsLeft <= 0)
			{
				turning.Insert(playerId);
				continue;
			}

			// Each threshold once, in order: 120 s, 30 s, 10 s before turning
			foreach (int warning : warnings)
			{
				if (secsLeft > warning || (state.m_iLastWarningSecs >= 0 && state.m_iLastWarningSecs <= warning))
					continue;

				state.m_iLastWarningSecs = warning;
				string text = "The fever is rising.";
				if (warning <= 10)
					text = "You can't hold it back any longer...";
				else if (warning <= 30)
					text = "Your vision blurs. Not long now.";

				CRF_RadioTransmissionEvents.PopupToPlayer(playerId, text, "", 5);
				break;
			}
		}

		foreach (int turningId : turning)
		{
			TurnPlayer(turningId);
		}

		UpdateEscapeVehicles();
		CheckEnding(now);
	}

	protected void CheckEnding(int now)
	{
		if (!m_bCarriersPicked || m_bEndRequested)
			return;

		array<int> survivors = {};
		GetLivingSurvivors(survivors);

		// Survivors held out
		if (m_iSurvivalTimeLimit > 0 && now - m_iRoundStartMs >= m_iSurvivalTimeLimit * 1000 && !survivors.IsEmpty())
		{
			EndMission(m_sSurvivorFaction, "THE SURVIVORS HELD OUT", "Help arrived in time.");
			return;
		}

		if (!m_bInfectedWinWhenSurvivorsGone)
			return;

		// Nobody healthy enough to fight it: anyone still incubating will turn too
		bool anyHealthy;
		foreach (int survivorId : survivors)
		{
			CRF_InfectionState state = m_mStates.Get(survivorId);
			if (!state || state.m_eStatus == CRF_EInfectionStatus.HEALTHY)
			{
				anyHealthy = true;
				break;
			}
		}

		if (anyHealthy)
		{
			m_iNoSurvivorSeconds = 0;
			return;
		}

		m_iNoSurvivorSeconds++;
		if (m_iNoSurvivorSeconds >= NO_SURVIVOR_GRACE)
			EndMission(m_sInfectedFaction, "THE INFECTION WINS", "No survivor is left standing.");
	}

	protected void EndMission(string winnerFaction, string title, string text)
	{
		if (m_bEnded || m_bEndRequested)
			return;

		m_bEndRequested = true;

		CRF_RadioTransmissionEvents.PopupToFaction("", title, text, 10);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winnerFaction, "automatic");

		// Advancing the state calls OnGamemodeStateChanged, which reveals the carriers
		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//! End of the mission: who carried it, how many turned, and whether it got out
	protected void Reveal()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();

		// Who carried it out: the infected aboard an escape vehicle, or (no escape) everyone still
		// secretly infected when the mission ended
		array<string> carriedOut = {};
		if (m_bSomeoneEscaped)
		{
			carriedOut.Copy(m_aEscapedInfectedNames);
		}
		else
		{
			array<int> survivors = {};
			GetLivingSurvivors(survivors);
			foreach (int survivorId : survivors)
			{
				CRF_InfectionState state = m_mStates.Get(survivorId);
				if (state && state.m_eStatus == CRF_EInfectionStatus.INCUBATING)
					carriedOut.Insert(playerManager.GetPlayerName(survivorId));
			}
		}

		string carriers = "nobody";
		if (!m_aCarrierNames.IsEmpty())
			carriers = JoinNames(m_aCarrierNames);

		string text = string.Format("Patient zero: %1. %2 turned.", carriers, m_aTurnedNames.Count());
		if (!carriedOut.IsEmpty())
			text += string.Format(" Still infected: %1 - the infection got out.", JoinNames(carriedOut));

		CRF_RadioTransmissionEvents.PopupToFaction("", "THE INFECTED WERE...", text, 15);
		Print("[CRF_Infection] " + text, LogLevel.NORMAL);
	}

	protected static string JoinNames(array<string> names)
	{
		string joined;
		foreach (int i, string name : names)
		{
			if (i > 0)
				joined += ", ";
			joined += name;
		}

		return joined;
	}

	//===================================================================================
	// ESCAPE VEHICLES
	//===================================================================================

	//! Empties every escape vehicle's tank and wrecks its engines (as the vanilla vehicle maintenance
	//! tutorial does) and damages its rotors, so survivors have to refuel and repair it
	protected void PrepareEscapeVehicles()
	{
		m_aEscapeVehicles.Clear();
		if (!m_aEscapeVehicleNames)
			return;

		foreach (int i, string vehicleName : m_aEscapeVehicleNames)
		{
			IEntity vehicle = GetGame().GetWorld().FindEntityByName(vehicleName);
			if (!vehicle)
			{
				Print(string.Format("[CRF_Infection] Escape vehicle '%1' isn't in the mission.", vehicleName), LogLevel.WARNING);
				continue;
			}

			CRF_InfectionEscapeVehicle escape = new CRF_InfectionEscapeVehicle();
			escape.m_Vehicle = vehicle;
			escape.m_vStartPos = vehicle.GetOrigin();
			escape.m_sLabel = "escape vehicle";
			if (m_aEscapeVehicleLabels && m_aEscapeVehicleLabels.IsIndexValid(i) && !m_aEscapeVehicleLabels[i].IsEmpty())
				escape.m_sLabel = m_aEscapeVehicleLabels[i];

			m_aEscapeVehicles.Insert(escape);

			SCR_FuelManagerComponent fuelManager = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
			if (fuelManager)
				fuelManager.SetTotalFuelPercentage(0);

			array<HitZone> hitZones = {};
			GetEngineAndRotorHitZones(vehicle, hitZones);
			foreach (HitZone hitZone : hitZones)
			{
				if (SCR_EngineHitZone.Cast(hitZone))
					hitZone.HandleDamage(hitZone.GetMaxHealth(), EDamageType.TRUE, null);
				else if (hitZone.GetHealthScaled() > m_fEscapeRotorStartHealth)
					hitZone.SetHealthScaled(m_fEscapeRotorStartHealth);
			}
		}
	}

	//! The parts that keep a vehicle from leaving: engines, and rotors on a helicopter
	protected void GetEngineAndRotorHitZones(IEntity vehicle, notnull array<HitZone> outHitZones)
	{
		HitZoneContainerComponent container = HitZoneContainerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!container)
			return;

		array<HitZone> hitZones = {};
		container.GetAllHitZonesInHierarchy(hitZones);
		foreach (HitZone hitZone : hitZones)
		{
			if (SCR_EngineHitZone.Cast(hitZone) || SCR_RotorHitZone.Cast(hitZone))
				outHitZones.Insert(hitZone);
		}
	}

	protected bool IsFuelled(IEntity vehicle)
	{
		SCR_FuelManagerComponent fuelManager = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
		if (!fuelManager)
			return true;

		// Small tanks only need to be (nearly) full
		return fuelManager.GetTotalFuel() >= Math.Min(m_fEscapeFuelLitres, fuelManager.GetTotalMaxFuel() * 0.95);
	}

	protected bool IsRepaired(IEntity vehicle)
	{
		array<HitZone> hitZones = {};
		GetEngineAndRotorHitZones(vehicle, hitZones);
		foreach (HitZone hitZone : hitZones)
		{
			if (hitZone.GetDamageState() == EDamageState.DESTROYED || hitZone.GetHealthScaled() < m_fEscapeRepairedHealth - 0.001)
				return false;
		}

		return true;
	}

	//! Once a second: progress announcements, destroyed vehicles, and escapes
	protected void UpdateEscapeVehicles()
	{
		if (m_aEscapeVehicles.IsEmpty() || m_bEndRequested)
			return;

		int usable;
		foreach (CRF_InfectionEscapeVehicle escape : m_aEscapeVehicles)
		{
			if (escape.m_bGone)
				continue;

			string label = escape.m_sLabel;
			label.ToUpper();

			IEntity vehicle = escape.m_Vehicle;
			DamageManagerComponent damageManager;
			if (vehicle)
				damageManager = DamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));

			if (!vehicle || (damageManager && damageManager.IsDestroyed()))
			{
				escape.m_bGone = true;
				CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("THE %1 IS DESTROYED", label), "", 6);
				continue;
			}

			usable++;

			bool fuelled = IsFuelled(vehicle);
			bool repaired = IsRepaired(vehicle);
			if (fuelled && repaired && (!escape.m_bFuelled || !escape.m_bRepaired))
			{
				float distanceKm = Math.Round(m_fEscapeDistance / 100) / 10; // one decimal
				CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, string.Format("THE %1 IS READY", label),
					string.Format("Get everyone aboard and get %1 km away.", distanceKm), 8);
			}
			else if (fuelled && !escape.m_bFuelled)
			{
				CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, string.Format("THE %1 IS FUELLED", label), "It still needs repairs.", 6);
			}
			else if (repaired && !escape.m_bRepaired)
			{
				CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, string.Format("THE %1 IS REPAIRED", label), "It still needs fuel.", 6);
			}

			escape.m_bFuelled = fuelled;
			escape.m_bRepaired = repaired;

			if (vector.Distance(vehicle.GetOrigin(), escape.m_vStartPos) < m_fEscapeDistance)
			{
				escape.m_bWarnedNotReady = false;
				continue;
			}

			if (!fuelled || !repaired)
			{
				if (!escape.m_bWarnedNotReady)
				{
					escape.m_bWarnedNotReady = true;
					CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, string.Format("THE %1 WON'T MAKE IT", label), "Bring it back and finish the repairs and refuelling.", 6);
				}
				continue;
			}

			OnVehicleEscaped(escape);
			if (m_bEndRequested)
				return;
		}

		if (usable == 0 && m_bInfectedWinWhenNoEscape)
			EndMission(m_sInfectedFaction, "NO WAY OUT", "Every way out has been destroyed.");
	}

	//! A ready escape vehicle got far enough away: whoever is aboard escaped
	protected void OnVehicleEscaped(notnull CRF_InfectionEscapeVehicle escape)
	{
		escape.m_bGone = true;
		PlayerManager playerManager = GetGame().GetPlayerManager();

		array<IEntity> occupants = {};
		SCR_BaseCompartmentManagerComponent compartmentManager = SCR_BaseCompartmentManagerComponent.Cast(escape.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compartmentManager)
			compartmentManager.GetOccupants(occupants);

		array<string> aboard = {};
		array<string> infectedAboard = {};
		int healthy;
		foreach (IEntity occupant : occupants)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(occupant);
			if (playerId <= 0 || !IsSurvivor(playerId))
				continue;

			string name = playerManager.GetPlayerName(playerId);
			aboard.Insert(name);

			CRF_InfectionState state = m_mStates.Get(playerId);
			if (state && state.m_eStatus == CRF_EInfectionStatus.INCUBATING)
				infectedAboard.Insert(name);
			else
				healthy++;
		}

		string label = escape.m_sLabel;
		if (aboard.IsEmpty())
		{
			label.ToUpper();
			CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, string.Format("THE %1 LEFT WITHOUT YOU", label), "", 6);
			return;
		}

		m_bSomeoneEscaped = true;
		foreach (string infectedName : infectedAboard)
		{
			m_aEscapedInfectedNames.Insert(infectedName);
		}

		if (healthy == 0)
		{
			EndMission(m_sInfectedFaction, "THE INFECTION ESCAPED", string.Format("Everyone aboard the %1 was infected.", label));
			return;
		}

		EndMission(m_sSurvivorFaction, "THEY GOT OUT", string.Format("%1 escaped in the %2.", JoinNames(aboard), label));
	}

	//===================================================================================
	// DEATH AND TURNING
	//===================================================================================

	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || !m_bRoundLive || m_bEnded)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();

		// A dead infected crumbles away, taking its gear (night vision) with it
		IEntity victimEntity = instigatorContextData.GetVictimEntity();
		if (m_fInfectedBodyRemoveDelay > 0 && IsInfectedCharacter(victimEntity))
		{
			RplComponent bodyRpl = RplComponent.Cast(victimEntity.FindComponent(RplComponent));
			if (bodyRpl)
				GetGame().GetCallqueue().CallLater(RemoveInfectedBody, m_fInfectedBodyRemoveDelay * 1000, false, bodyRpl.Id());
			return;
		}

		if (victimId <= 0 || !IsSurvivor(victimId))
			return;

		CRF_InfectionState state = GetState(victimId);
		if (state.m_eStatus == CRF_EInfectionStatus.TURNED)
			return;

		bool rise = m_bAllDeadSurvivorsRise || state.m_eStatus == CRF_EInfectionStatus.INCUBATING;
		if (!rise && m_bKilledByInfectedRise)
			rise = IsInfectious(instigatorContextData.GetKillerEntity());

		if (!rise)
			return;

		state.m_bRisePending = true;
		if (state.m_eStatus == CRF_EInfectionStatus.HEALTHY)
			state.m_eStatus = CRF_EInfectionStatus.INCUBATING;

		GetGame().GetCallqueue().CallLater(RisePlayer, m_fRiseDelay * 1000, false, victimId);
	}

	protected void RemoveInfectedBody(RplId bodyId)
	{
		IEntity body = COA_EntityHelper.GetEntityFromRplId(bodyId);
		if (body)
			SCR_EntityHelper.DeleteEntityAndChildren(body);
	}

	//! A character on the infected faction (a player who turned, or a Ghoul)
	protected bool IsInfectedCharacter(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		Faction faction = character.GetFaction();
		return faction && faction.GetFactionKey() == m_sInfectedFaction;
	}

	//! Everywhere (the stamina hook runs on the character's owner): an infected who never tires
	bool IsTirelessInfected(IEntity character)
	{
		return m_bInfectedTireless && IsInfectedCharacter(character);
	}

	protected void RisePlayer(int playerId)
	{
		CRF_InfectionState state = m_mStates.Get(playerId);
		if (!state || m_bEnded)
			return;

		state.m_bRisePending = false;
		TurnPlayer(playerId);
	}

	//! Moves the player to a free infected slot and respawns them where their body is, then removes the
	//! old body. The Lobby applies the infected faction's gear, group, radios and name tags.
	protected void TurnPlayer(int playerId)
	{
		CRF_InfectionState state = GetState(playerId);
		if (state.m_eStatus == CRF_EInfectionStatus.TURNED)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager.IsPlayerConnected(playerId))
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_GamemodeManager gamemodeManager = COA_GamemodeManager.GetInstance();
		if (!slottingManager || !gamemodeManager)
			return;

		int oldSlotId = slottingManager.GetPlayerSlotID(playerId);
		int newSlotId = FindFreeInfectedSlot();
		if (oldSlotId < 0 || newSlotId < 0)
		{
			Print(string.Format("[CRF_Infection] Can't turn %1: no free slot on the infected faction (%2). Add more infected slots.", playerManager.GetPlayerName(playerId), m_sInfectedFaction), LogLevel.ERROR);
			state.m_iTurnAtMs = GetGame().GetWorld().GetWorldTime() + 30000; // try again shortly
			return;
		}

		// Where they rise: their body, alive or dead - or wherever their camera is
		COA_SlotData oldSlot = slottingManager.GetSlotData(oldSlotId);
		RplId anchorId = oldSlot.GetSlotCurrentCharacter();
		IEntity oldBody = COA_EntityHelper.GetEntityFromRplId(anchorId);
		if (!oldBody)
		{
			IEntity controlled = playerManager.GetPlayerControlledEntity(playerId);
			RplComponent controlledRpl;
			if (controlled)
				controlledRpl = RplComponent.Cast(controlled.FindComponent(RplComponent));
			if (!controlledRpl)
			{
				Print(string.Format("[CRF_Infection] Can't turn %1: nowhere to spawn them.", playerManager.GetPlayerName(playerId)), LogLevel.ERROR);
				return;
			}
			anchorId = controlledRpl.Id();
		}

		// Vacating a slot deletes its character, so detach the old body first - the new one spawns on it
		slottingManager.UpdateSlotCharacter(oldSlotId, RplId.Invalid());
		slottingManager.UpdateSlotPlayerID(oldSlotId, 0);
		slottingManager.ForceUpdateSlotPlayerID(newSlotId, playerId);
		slottingManager.UpdateSlotDeathState(newSlotId, false);

		state.m_eStatus = CRF_EInfectionStatus.TURNED;
		m_aTurnedNames.Insert(playerManager.GetPlayerName(playerId));

		if (!gamemodeManager.InitilizePlayer(playerId, -1, anchorId))
			Print(string.Format("[CRF_Infection] Spawning %1 as infected failed.", playerManager.GetPlayerName(playerId)), LogLevel.ERROR);

		if (oldBody)
			SCR_EntityHelper.DeleteEntityAndChildren(oldBody);

		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "YOU HAVE TURNED", "Hunt the survivors. Every one you bring down joins you.", 8);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, "SOMEONE HAS TURNED", "", 5);
	}

	//! The lowest free slot on the infected faction, or -1
	protected int FindFreeInfectedSlot()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		int best = -1;
		foreach (int slotId, COA_SlotData slotData : slottingManager.GetSlotMap())
		{
			if (!slotData || slotData.GetSlotFactionKey() != m_sInfectedFaction || slotData.GetSlotCurrentPlayerId() > 0)
				continue;

			if (best < 0 || slotId < best)
				best = slotId;
		}

		return best;
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (Replication.IsServer())
			m_mStates.Remove(playerId);
	}

	//===================================================================================
	// ANTIDOTE
	//===================================================================================

	int GetAntidoteDoses()
	{
		return m_iAntidoteDoses;
	}

	string GetSurvivorFaction()
	{
		return m_sSurvivorFaction;
	}

	//! Server, from CRF_InfectionAntidoteAction via COA_PlayerRplToAuthorityManager: a survivor injects
	//! a dose at the crate at cratePos. Bitten survivors are cured; carriers and the healthy waste it.
	void UseAntidote(int playerId, vector cratePos)
	{
		if (!m_bRoundLive || m_bEnded || !IsSurvivor(playerId))
			return;

		IEntity user = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!user || vector.Distance(user.GetOrigin(), cratePos) > 5 || !CRF_InfectionAntidoteStation.IsStationAt(cratePos))
			return;

		if (m_iAntidoteDoses <= 0)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "THE CRATE IS EMPTY", "", 4);
			return;
		}

		m_iAntidoteDoses--;
		Replication.BumpMe();

		CRF_InfectionState state = m_mStates.Get(playerId);
		if (!state || state.m_eStatus == CRF_EInfectionStatus.HEALTHY)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "NOTHING HAPPENS", "You weren't infected - the dose is gone.", 5);
			return;
		}

		if (state.m_bCarrier)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "IT DOESN'T WORK", "The infection is too deep in you.", 5);
			return;
		}

		state.m_eStatus = CRF_EInfectionStatus.HEALTHY;
		state.m_iLastWarningSecs = -1;
		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "THE FEVER BREAKS", "You're cured - for now.", 6);
	}
}
