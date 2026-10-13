//------------------------------------------------------------------------------------
// CRF_RadioDFGamemodeComponent: Radio Direction Finding ("hunt the transmitter")
//
// One side (the transmitters, e.g. partisans or VC) has to get a set amount of radio traffic on the air.
// Every second any of them spends transmitting on a radio counts toward their goal. The other side (the
// hunters) runs direction-finding teams: while a transmitter is on the air, every hunter carrying DF
// equipment within range gets a compass bearing to them every few seconds, with an error that grows with
// distance. Two teams crossing bearings on the map find the transmitter - no markers, just map and compass.
//
//   Transmitters win: their radio traffic reaches the goal.
//   Hunters win: the time limit runs out, or no transmitter-side player is left alive.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity.
// 2. Set the transmitter and hunter faction keys.
// 3. Give the transmitters radios, and the hunters' DF teams the DF equipment (by default a manpack:
//    a radio backpack plus a long-range radio - see CVON_Manpack).
// 4. Tune the goal, time limit, DF range and bearing error. The round starts when safestart ends.
//------------------------------------------------------------------------------------

enum CRF_EDFEquipment
{
	MANPACK,			// a radio backpack plus a long-range radio (CVON_Manpack)
	LONG_RANGE_RADIO,	// any long-range radio
	ANY_RADIO			// any radio
}

[ComponentEditorProps(category: "Game Mode Component", description: "Radio Direction Finding: one side must get radio traffic on the air, the other hunts them down with direction-finding bearings.")]
class CRF_RadioDFGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_RadioDFGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("INDFOR", UIWidgets.EditBox, "Faction that has to get radio traffic on the air.", category: "Radio DF - Factions")]
	string m_sTransmitterFactionKey;

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction hunting the transmitters with direction finders.", category: "Radio DF - Factions")]
	string m_sHunterFactionKey;

	[Attribute("300", UIWidgets.Auto, "Seconds of radio transmission the transmitting side needs to win (every transmitter counts, so two at once score double).", category: "Radio DF - Victory")]
	int m_iTransmissionGoalSeconds;

	[Attribute("0", UIWidgets.CheckBox, "Only transmissions on long-range radios count toward the goal (and give bearings).", category: "Radio DF - Victory")]
	bool m_bOnlyLongRangeCounts;

	[Attribute("60", UIWidgets.Auto, "Minutes the hunters have to stop the transmitters. When it runs out the hunters win. 0 = no time limit.", category: "Radio DF - Victory")]
	int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Hunters win when no transmitter-side player is left alive.", category: "Radio DF - Victory")]
	bool m_bHuntersWinWhenTransmittersDead;

	[Attribute("1", UIWidgets.CheckBox, "Record the winning faction and advance the gamemode to the AAR when a side wins.", category: "Radio DF - Victory")]
	bool m_bEndMissionOnVictory;

	[Attribute("0", UIWidgets.ComboBox, "What a hunter needs to carry to take bearings.", "", ParamEnumArray.FromEnum(CRF_EDFEquipment), category: "Radio DF - Direction Finding")]
	CRF_EDFEquipment m_eDFEquipment;

	[Attribute("4000", UIWidgets.Auto, "Metres within which a DF set picks up a transmitter.", category: "Radio DF - Direction Finding")]
	float m_fDFRange;

	[Attribute("5", UIWidgets.Auto, "Seconds between bearings while a transmitter stays on the air.", category: "Radio DF - Direction Finding")]
	float m_fBearingInterval;

	[Attribute("8", UIWidgets.Auto, "Largest bearing error in degrees, at the edge of DF range (half that up close).", category: "Radio DF - Direction Finding")]
	float m_fBearingErrorDegrees;

	[Attribute("20", UIWidgets.Auto, "Announce the transmitters' progress to everyone every this many percent.", category: "Radio DF - HUD")]
	float m_fNotifyIncrementPercent;

	//===================================================================================
	// SERVER STATE
	//===================================================================================

	// Players currently on the air -> seconds since their last bearing
	protected ref map<int, float> m_mActiveTransmitters = new map<int, float>();
	protected ref map<int, string> m_mTransmitterFrequency = new map<int, string>();
	protected ref array<int> m_aToRemove = {};
	protected ref array<int> m_aOnAir = {};
	protected ref RandomGenerator m_Random = new RandomGenerator();

	protected bool m_bStarted;
	protected bool m_bEnded;
	protected float m_fScoreSeconds;
	protected float m_fElapsedSeconds;
	protected int m_iLastNotifiedBracket;
	protected int m_iNoTransmittersSeconds;

	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Seconds with nobody alive on the transmitting side before the hunters win (covers respawn gaps)
	protected static const int NO_TRANSMITTERS_GRACE = 10;

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

		if (!Replication.IsServer())
			return;

		m_Random.SetSeed(System.GetTickCount());
		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback for missions without safestart
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);

		CRF_RadioTransmissionEvents.GetOnTransmissionStarted().Insert(OnTransmissionStarted);
		CRF_RadioTransmissionEvents.GetOnTransmissionEnded().Insert(OnTransmissionEnded);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);

		CRF_RadioTransmissionEvents.GetOnTransmissionStarted().Remove(OnTransmissionStarted);
		CRF_RadioTransmissionEvents.GetOnTransmissionEnded().Remove(OnTransmissionEnded);

		super.OnDelete(owner);
	}

	//===================================================================================
	// ROUND START
	//===================================================================================

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void StartRound()
	{
		if (m_bStarted)
			return;

		m_bStarted = true;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			SendBriefing(playerId);
	}

	//! The rules for this player's side: at round start, and when they spawn later (late joiners, respawns)
	protected void SendBriefing(int playerId)
	{
		string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
		if (factionKey == m_sTransmitterFactionKey)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Get your traffic on the air",
				string.Format("%1 seconds of radio transmission wins. Every transmission gives enemy direction finders a bearing on you.", m_iTransmissionGoalSeconds), 10);
		}
		else if (factionKey == m_sHunterFactionKey)
		{
			string limit = "";
			if (m_iTimeLimitMinutes > 0)
				limit = string.Format(" Hold out %1 minutes or eliminate them.", m_iTimeLimitMinutes);
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Hunt the enemy transmitters",
				string.Format("DF operators (%1) get a bearing whenever they transmit. Cross bearings on the map to find them.%2", GetDFEquipmentName(), limit), 10);
		}
	}

	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		if (m_bStarted && !m_bEnded)
			SendBriefing(playerId);
	}

	//===================================================================================
	// TRANSMISSIONS
	//===================================================================================

	protected void OnTransmissionStarted(int playerId, string frequency, RplId radioId)
	{
		if (!m_bStarted || m_bEnded)
			return;

		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sTransmitterFactionKey)
			return;

		if (m_bOnlyLongRangeCounts && !IsLongRangeRadio(radioId))
			return;

		// First bearing a moment after keying up: a quick burst is harder to fix
		m_mActiveTransmitters.Set(playerId, m_fBearingInterval - 2);
		m_mTransmitterFrequency.Set(playerId, frequency);
	}

	protected void OnTransmissionEnded(int playerId)
	{
		m_mActiveTransmitters.Remove(playerId);
		m_mTransmitterFrequency.Remove(playerId);
	}

	protected bool IsLongRangeRadio(RplId radioId)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(radioId));
		if (!rpl || !rpl.GetEntity())
			return false;

		CVON_RadioComponent radio = CVON_RadioComponent.Cast(rpl.GetEntity().FindComponent(CVON_RadioComponent));
		if (!radio)
			return false;
		return radio.m_eRadioType == CVON_ERadioType.LONG;
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded)
			return;

		m_fElapsedSeconds += 1;

		// Score and bearings for everyone on the air (keys copied first: the map changes inside the loop)
		m_aOnAir.Clear();
		foreach (int activeId, float unused : m_mActiveTransmitters)
			m_aOnAir.Insert(activeId);

		m_aToRemove.Clear();
		foreach (int transmitterId : m_aOnAir)
		{
			IEntity transmitter = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(transmitterId);
			if (!transmitter)
			{
				m_aToRemove.Insert(transmitterId);
				continue;
			}

			m_fScoreSeconds += 1;
			float sinceBearing = m_mActiveTransmitters.Get(transmitterId) + 1;
			if (sinceBearing >= m_fBearingInterval)
			{
				SendBearings(transmitterId, transmitter);
				sinceBearing = 0;
			}
			m_mActiveTransmitters.Set(transmitterId, sinceBearing);
		}
		foreach (int goneId : m_aToRemove)
			OnTransmissionEnded(goneId);

		AnnounceProgress();

		if (m_iTransmissionGoalSeconds > 0 && m_fScoreSeconds >= m_iTransmissionGoalSeconds)
		{
			Victory(m_sTransmitterFactionKey, "The transmitters got their traffic through!");
			return;
		}

		if (m_iTimeLimitMinutes > 0 && m_fElapsedSeconds >= m_iTimeLimitMinutes * 60)
		{
			Victory(m_sHunterFactionKey, "Time is up - the transmitters were silenced.");
			return;
		}

		if (m_bHuntersWinWhenTransmittersDead)
		{
			if (CountAlivePlayers(m_sTransmitterFactionKey) == 0)
				m_iNoTransmittersSeconds++;
			else
				m_iNoTransmittersSeconds = 0;

			if (m_iNoTransmittersSeconds >= NO_TRANSMITTERS_GRACE)
				Victory(m_sHunterFactionKey, "The transmitters have been eliminated.");
		}
	}

	//! A bearing from every DF operator in range to this transmitter
	protected void SendBearings(int transmitterId, IEntity transmitter)
	{
		vector transmitterPos = transmitter.GetOrigin();
		string frequency = m_mTransmitterFrequency.Get(transmitterId);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != m_sHunterFactionKey)
				continue;

			IEntity dfOperator = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId);
			if (!dfOperator || !HasDFEquipment(dfOperator))
				continue;

			vector toTransmitter = transmitterPos - dfOperator.GetOrigin();
			float distance = toTransmitter.Length();
			if (distance > m_fDFRange)
				continue;

			// Compass bearing: 0 = north (+Z), 90 = east (+X); the error grows with distance
			float bearing = Math.Atan2(toTransmitter[0], toTransmitter[2]) * Math.RAD2DEG;
			float rangeFraction = Math.Clamp(distance / m_fDFRange, 0, 1);
			float maxError = m_fBearingErrorDegrees * (0.5 + 0.5 * rangeFraction);
			bearing += m_Random.RandFloatXY(-maxError, maxError);

			int bearingDeg = (int)Math.Round(bearing);
			while (bearingDeg < 0)
				bearingDeg += 360;
			while (bearingDeg >= 360)
				bearingDeg -= 360;

			string strength = "weak";
			if (rangeFraction < 0.33)
				strength = "strong";
			else if (rangeFraction < 0.66)
				strength = "medium";

			string bearingText = bearingDeg.ToString();
			while (bearingText.Length() < 3)
				bearingText = "0" + bearingText;

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "DF bearing " + bearingText + "°",
				string.Format("Signal %1, frequency %2", strength, frequency), 4);
		}
	}

	protected bool HasDFEquipment(IEntity character)
	{
		if (m_eDFEquipment == CRF_EDFEquipment.MANPACK)
		{
			if (CVON_Manpack.GetManpackRadio(character))
				return true;
			return false;
		}

		CVON_VONGameModeComponent cvon = CVON_VONGameModeComponent.GetInstance();
		if (!cvon)
			return false;

		array<IEntity> radios = cvon.GetRadioEntities(character);
		if (!radios || radios.IsEmpty())
			return false;

		if (m_eDFEquipment == CRF_EDFEquipment.ANY_RADIO)
			return true;

		foreach (IEntity radio : radios)
		{
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			if (radioComp && radioComp.m_eRadioType == CVON_ERadioType.LONG)
				return true;
		}
		return false;
	}

	protected string GetDFEquipmentName()
	{
		if (m_eDFEquipment == CRF_EDFEquipment.MANPACK)
			return "radio backpack and long-range radio";
		if (m_eDFEquipment == CRF_EDFEquipment.LONG_RANGE_RADIO)
			return "long-range radio";
		return "any radio";
	}

	protected int CountAlivePlayers(string factionKey)
	{
		int count = 0;
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) != factionKey)
				continue;
			if (CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId))
				count++;
		}
		return count;
	}

	protected void AnnounceProgress()
	{
		if (m_iTransmissionGoalSeconds <= 0 || m_fNotifyIncrementPercent <= 0)
			return;

		float percent = m_fScoreSeconds / m_iTransmissionGoalSeconds * 100.0;
		int bracket = (int)Math.Floor(percent / m_fNotifyIncrementPercent);
		if (bracket <= m_iLastNotifiedBracket)
			return;

		m_iLastNotifiedBracket = bracket;
		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("Enemy radio traffic: %1%%", (int)Math.Round(percent)),
			string.Format("%1 / %2 seconds on the air", (int)m_fScoreSeconds, m_iTransmissionGoalSeconds), 6);
	}

	//===================================================================================
	// VICTORY
	//===================================================================================

	protected void Victory(string factionKey, string reason)
	{
		if (m_bEnded)
			return;

		m_bEnded = true;
		GetGame().GetCallqueue().Remove(Tick);
		m_mActiveTransmitters.Clear();

		CRF_RadioTransmissionEvents.PopupToFaction("", factionKey + " wins", reason, 10);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(factionKey, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}
}
