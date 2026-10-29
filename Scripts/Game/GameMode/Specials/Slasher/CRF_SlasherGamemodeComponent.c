//------------------------------------------------------------------------------------
// CRF_SlasherGamemodeComponent: "Slasher" - an asymmetric Halloween hunt
//
// A handful of killers (masked, night vision, a blade, never tire, hard to put down) hunt a large group
// of survivors in the dark. The survivors have to get the generators running, use the powered radio to
// call for extraction, then hold out until it arrives and reach the landing zone.
//
//   1. Generators: a survivor standing at a generator works on it; a killer nearby stops the work.
//      Killers are told roughly where a generator has started up.
//   2. Radio: once enough generators run, a survivor stands at the radio to call extraction.
//   3. Extraction: it arrives after a delay and waits a short time. Survivors who reach the LZ escape
//      (they're taken out of the round and spectate).
//
//   Survivors win: at least m_iSurvivorsToEscape escape.
//   Killers win: too few escape - every survivor is dead, the extraction leaves, or time runs out.
//
// Everything is an empty named entity in the world; no special prefabs are needed. Only their position on
// the map matters (distances ignore height), and the mode spawns a generator and a radio prop on the ground
// at them, so their height in the editor doesn't have to be exact:
//   - generators: names in m_aGeneratorNames
//   - the radio: m_sRadioName - leave empty to call extraction as soon as the generators run
//   - the landing zone: m_sExtractionName
//
// Setup for mission makers
// 1. Add this component to the gamemode entity; set the survivor and killer factions.
// 2. Place and name the generators, the radio and the LZ, and list the names here.
// 3. Killers: a few slots with Configs/Gearscripts/Standard/Specials/Slasher/COA_GS_Slasher_Killer.conf.
//    Survivors: flashlights and very little ammo. Night, fog, no respawns for survivors; killers can
//    respawn through the Lobby's normal respawn settings.
//------------------------------------------------------------------------------------

enum CRF_ESlasherPhase
{
	GENERATORS,
	RADIO,
	INBOUND,	//!< extraction called, on its way
	BOARDING,	//!< extraction at the LZ
	ENDED
}

[ComponentEditorProps(category: "Game Mode Component", description: "Slasher: a few unstoppable killers hunt the survivors, who must start generators, call extraction on the radio and escape.")]
class CRF_SlasherGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_SlasherGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction of the survivors.", category: "Slasher - Factions")]
	protected string m_sSurvivorFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction of the killers.", category: "Slasher - Factions")]
	protected string m_sKillerFaction;

	[Attribute("", UIWidgets.Auto, "Names of the generator entities placed in the world.", category: "Slasher - Generators")]
	protected ref array<string> m_aGeneratorNames;

	[Attribute("0", UIWidgets.Auto, "Generators that have to run before the radio works. 0 = all of them.", category: "Slasher - Generators")]
	protected int m_iGeneratorsRequired;

	[Attribute("150", UIWidgets.Auto, "Seconds of work a generator needs (one survivor; two work faster, see Extra Worker Speed).", category: "Slasher - Generators")]
	protected int m_iGeneratorSeconds;

	[Attribute("0.5", UIWidgets.Slider, "How much each survivor beyond the first speeds up the work (0.5 = two survivors work 1.5x as fast). At most three count.", params: "0 1 0.05", category: "Slasher - Generators")]
	protected float m_fExtraWorkerSpeed;

	[Attribute("8", UIWidgets.Auto, "Metres (on the map, ignoring height) from a generator or the radio a survivor has to be to work on it.", category: "Slasher - Generators")]
	protected float m_fWorkRadius;

	[Attribute("{6597847D2AC253F5}Prefabs/Props/Military/Generators/GeneratorMilitary_USSR_01/GeneratorMilitary_USSR_01.et", UIWidgets.ResourceNamePicker, "Prop spawned on the ground at each generator entity. Empty = none (place your own).", params: "et", category: "Slasher - Generators")]
	protected ResourceName m_sGeneratorPrefab;

	[Attribute("{ABA12EC2F6D0F778}Prefabs/Props/Military/Compositions/USSR/RadioStation_USSR.et", UIWidgets.ResourceNamePicker, "Prop spawned on the ground at the radio entity. Empty = none (place your own).", params: "et", category: "Slasher - Extraction")]
	protected ResourceName m_sRadioPrefab;

	[Attribute("1", UIWidgets.CheckBox, "A killer within the work radius stops the work.", category: "Slasher - Generators")]
	protected bool m_bKillerStopsWork;

	[Attribute("1", UIWidgets.CheckBox, "Killers are told the grid of a generator when survivors start working on it.", category: "Slasher - Generators")]
	protected bool m_bRevealWork;

	[Attribute("", UIWidgets.EditBox, "Name of the radio entity. Empty = extraction is called as soon as the generators run.", category: "Slasher - Extraction")]
	protected string m_sRadioName;

	[Attribute("20", UIWidgets.Auto, "Seconds a survivor has to stay at the radio to call extraction.", category: "Slasher - Extraction")]
	protected int m_iRadioSeconds;

	[Attribute("", UIWidgets.EditBox, "Name of the landing zone entity.", category: "Slasher - Extraction")]
	protected string m_sExtractionName;

	[Attribute("150", UIWidgets.Auto, "Seconds from the call until extraction arrives.", category: "Slasher - Extraction")]
	protected int m_iInboundSeconds;

	[Attribute("60", UIWidgets.Auto, "Seconds extraction waits at the LZ.", category: "Slasher - Extraction")]
	protected int m_iBoardingSeconds;

	[Attribute("20", UIWidgets.Auto, "Metres from the LZ a survivor has to reach while extraction waits.", category: "Slasher - Extraction")]
	protected float m_fExtractionRadius;

	[Attribute("1", UIWidgets.Auto, "Survivors who have to escape for the survivors to win.", category: "Slasher - Victory")]
	protected int m_iSurvivorsToEscape;

	[Attribute("60", UIWidgets.Auto, "Minutes before the killers win anyway. 0 = no time limit.", category: "Slasher - Victory")]
	protected int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round ends.", category: "Slasher - Victory")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("1", UIWidgets.CheckBox, "Killers never tire: running and sprinting don't drain their stamina.", category: "Slasher - Killers")]
	protected bool m_bKillersTireless;

	[Attribute("0.35", UIWidgets.Slider, "Share of incoming damage killers take (0.35 = about three times as hard to kill).", params: "0.05 1 0.05", category: "Slasher - Killers")]
	protected float m_fKillerDamageTaken;

	[Attribute("0.9", UIWidgets.Slider, "Survivors' top speed as a share of normal, so a killer can run them down. 1 = normal.", params: "0.5 1 0.05", category: "Slasher - Killers")]
	protected float m_fSurvivorMaxSpeed;

	[Attribute("30", UIWidgets.Auto, "A survivor within this many metres of a killer gets a warning that something is close. 0 = off.", category: "Slasher - Killers")]
	protected float m_fTerrorRadius;

	[Attribute("5", UIWidgets.Auto, "Seconds after a killer dies before the body vanishes, so survivors can't take the mask (night vision) or the blade. 0 = bodies stay.", category: "Slasher - Killers")]
	protected float m_fKillerBodyRemoveDelay;

	[Attribute("{FCED797E251D348F}Sounds/Cultits.acp", UIWidgets.ResourceNamePicker, "Sound project played on killers who are on the move, so survivors hear them coming (BallienCreatures' cultist voices by default). Empty = silent killers.", params: "acp", category: "Slasher - Killer Sounds")]
	protected ResourceName m_sKillerSoundProject;

	[Attribute("SOUND_IDLE", UIWidgets.EditBox, "Event played while a killer walks or runs.", category: "Slasher - Killer Sounds")]
	protected string m_sKillerMoveEvent;

	[Attribute("SOUND_COMBAT", UIWidgets.EditBox, "Event played while a killer sprints.", category: "Slasher - Killer Sounds")]
	protected string m_sKillerSprintEvent;

	[Attribute("9", UIWidgets.Auto, "Average seconds between sounds while a killer walks or runs (varied by 30% either way).", category: "Slasher - Killer Sounds")]
	protected float m_fKillerMoveInterval;

	[Attribute("4", UIWidgets.Auto, "Average seconds between sounds while a killer sprints.", category: "Slasher - Killer Sounds")]
	protected float m_fKillerSprintInterval;

	[Attribute("150", UIWidgets.Auto, "Killers further than this from you (metres) aren't played at all. How far a sound really carries is set in the sound project.", category: "Slasher - Killer Sounds")]
	protected float m_fKillerSoundRange;

	[Attribute("{7C1AE1F2A3B4C800}Sounds/Halloween/CRF_DistantSiren.wav", UIWidgets.ResourceNamePicker, "Sound file everyone hears now and then - a far-off civil defence siren by default. Empty = none.", params: "wav", category: "Slasher - Atmosphere")]
	protected ResourceName m_sSirenSound;

	[Attribute("1", UIWidgets.Auto, "Minutes into the round the siren first sounds.", category: "Slasher - Atmosphere")]
	protected float m_fSirenFirstMinutes;

	[Attribute("12", UIWidgets.Auto, "Average minutes between sirens after that (varied by 25% either way). 0 = only once.", category: "Slasher - Atmosphere")]
	protected float m_fSirenIntervalMinutes;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker icon for generators, the radio and the LZ.", params: "edds", category: "Slasher - Map")]
	protected ResourceName m_sMarkerIcon;

	//===================================================================================
	// STATE
	//===================================================================================

	[RplProp()]
	protected int m_iPhase;

	[RplProp()]
	protected bool m_bRoundLive;

	//! Per generator: progress 0-100
	[RplProp()]
	protected ref array<int> m_aGeneratorPercent = {};

	//! Per generator: survivors are working on it right now
	[RplProp()]
	protected ref array<bool> m_aGeneratorWorked = {};

	[RplProp()]
	protected int m_iRadioPercent;

	[RplProp()]
	protected bool m_bRadioWorked;

	//! Seconds left while extraction is inbound or boarding
	[RplProp()]
	protected int m_iPhaseSecondsLeft;

	[RplProp()]
	protected int m_iEscaped;

	// Server
	protected bool m_bEnded;
	protected float m_fElapsedSeconds;
	protected ref array<IEntity> m_aGenerators = {};
	protected ref array<float> m_aGeneratorWork = {};
	protected ref array<float> m_aLastReveal = {};
	protected float m_fRadioWork;
	protected IEntity m_Radio;
	protected IEntity m_Extraction;
	protected int m_iNoSurvivorsSeconds;
	protected float m_fNextSirenAt = -1;
	protected ref map<int, float> m_mLastTerror = new map<int, float>();
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Client
	protected ref CRF_SpecialsMarkerSet m_Markers = new CRF_SpecialsMarkerSet();
	protected IEntity m_SpeedLimitedCharacter;
	protected ref map<IEntity, vector> m_mKillerLastPosition = new map<IEntity, vector>();
	protected ref map<IEntity, float> m_mKillerNextSound = new map<IEntity, float>();
	protected ref array<IEntity> m_aNearbyKillers = {};

	protected static CRF_SlasherGamemodeComponent s_Instance;

	protected static const string LOG = "[CRF_Slasher]";
	protected static const int NO_SURVIVORS_GRACE = 10;
	protected static const float REVEAL_COOLDOWN = 45;
	protected static const float TERROR_COOLDOWN = 20;

	// Client update period, and the speeds (m/s, on the map) that count as moving and sprinting
	protected static const float CLIENT_UPDATE_SECONDS = 1;
	protected static const float KILLER_MOVING_SPEED = 0.8;
	protected static const float KILLER_SPRINT_SPEED = 4.5;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_SlasherGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_SlasherGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (GetGame().InPlayMode())
			SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Markers and the survivors' speed limit on every machine with a player
		if (!System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(ClientUpdate, CLIENT_UPDATE_SECONDS * 1000, true);

		if (!Replication.IsServer())
			return;

		if (m_aGeneratorNames)
		{
			foreach (string generatorName : m_aGeneratorNames)
			{
				m_aGenerators.Insert(CRF_SpecialsHelper.FindNamedEntity(generatorName, LOG));
				m_aGeneratorWork.Insert(0);
				m_aLastReveal.Insert(-REVEAL_COOLDOWN);
				m_aGeneratorPercent.Insert(0);
				m_aGeneratorWorked.Insert(false);
			}
		}

		m_Radio = CRF_SpecialsHelper.FindNamedEntity(m_sRadioName, LOG);
		m_Extraction = CRF_SpecialsHelper.FindNamedEntity(m_sExtractionName, LOG);
		if (!m_Extraction)
			Print(LOG + " No landing zone - set Extraction Name, or the survivors can never escape.", LogLevel.ERROR);

		m_iPhase = CRF_ESlasherPhase.GENERATORS;
		Replication.BumpMe();

		// The props go on the ground wherever the anchors are, once the world has settled
		GetGame().GetCallqueue().CallLater(SpawnProps, 2000, false);

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback for missions without safestart
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(ClientUpdate);
			GetGame().GetCallqueue().Remove(SpawnProps);
			GetGame().GetCallqueue().Remove(RemoveKillerBody);
		}

		m_Markers.Clear();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a generator prop at every generator entity, and the radio prop
	protected void SpawnProps()
	{
		foreach (IEntity generator : m_aGenerators)
		{
			if (generator)
				CRF_SpecialsHelper.SpawnOnGround(m_sGeneratorPrefab, generator.GetOrigin(), generator.GetYawPitchRoll()[0]);
		}

		if (m_Radio)
			CRF_SpecialsHelper.SpawnOnGround(m_sRadioPrefab, m_Radio.GetOrigin(), m_Radio.GetYawPitchRoll()[0]);
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
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME && (!m_SafestartManager || !m_SafestartManager.GetSafestartStatus()))
			StartRound();
	}

	protected void StartRound()
	{
		if (m_bRoundLive || m_bEnded)
			return;

		m_bRoundLive = true;
		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		if (!m_sSirenSound.IsEmpty())
			m_fNextSirenAt = m_fSirenFirstMinutes * 60;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			SendBriefing(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a far-off siren for everyone, then the next one scheduled
	protected void UpdateSiren()
	{
		if (m_fNextSirenAt < 0 || m_fElapsedSeconds < m_fNextSirenAt)
			return;

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.PlaySoundForEveryone(m_sSirenSound);

		if (m_fSirenIntervalMinutes > 0)
			m_fNextSirenAt = m_fElapsedSeconds + m_fSirenIntervalMinutes * 60 * Math.RandomFloat(0.75, 1.25);
		else
			m_fNextSirenAt = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected void SendBriefing(int playerId)
	{
		string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
		if (factionKey == m_sSurvivorFaction)
		{
			string radio = "call extraction on the radio";
			if (!m_Radio)
				radio = "extraction is called";
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Survive the night",
				string.Format("Get %1 of the %2 generators running (stand at one to work on it), then %3 and reach the landing zone. They're out there. Stay together.", GetGeneratorsRequired(), m_aGenerators.Count(), radio), 12);
		}
		else if (factionKey == m_sKillerFaction)
		{
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Hunt them",
				"Nobody leaves. You never tire and you're hard to kill. Stop them working on the generators - you'll hear when one starts up.", 12);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		if (m_bRoundLive && !m_bEnded)
			SendBriefing(playerId);
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded)
			return;

		m_fElapsedSeconds += 1;
		UpdateSiren();

		array<int> survivors = {};
		array<int> killers = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sSurvivorFaction, survivors);
		CRF_SpecialsHelper.GetAlivePlayers(m_sKillerFaction, killers);

		switch (m_iPhase)
		{
			case CRF_ESlasherPhase.GENERATORS:
				UpdateGenerators(survivors, killers);
				break;

			case CRF_ESlasherPhase.RADIO:
				UpdateRadio(survivors, killers);
				break;

			case CRF_ESlasherPhase.INBOUND:
			{
				m_iPhaseSecondsLeft--;
				if (m_iPhaseSecondsLeft == 30)
					CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, "Extraction 30 seconds out", "Get to the landing zone.", 6);

				if (m_iPhaseSecondsLeft <= 0)
				{
					m_iPhase = CRF_ESlasherPhase.BOARDING;
					m_iPhaseSecondsLeft = m_iBoardingSeconds;
					CRF_RadioTransmissionEvents.PopupToFaction("", "Extraction is at the landing zone",
						string.Format("It leaves in %1.", CRF_SpecialsHelper.FormatSeconds(m_iBoardingSeconds)), 8);
				}
				Replication.BumpMe();
				break;
			}

			case CRF_ESlasherPhase.BOARDING:
			{
				Board(survivors);
				m_iPhaseSecondsLeft--;
				Replication.BumpMe();
				if (m_iPhaseSecondsLeft <= 0)
				{
					EndByEscapes("The extraction has left.");
					return;
				}
				break;
			}
		}

		Terrorize(survivors, killers);

		// Out of survivors: whoever escaped decides it
		CRF_SpecialsHelper.GetAlivePlayers(m_sSurvivorFaction, survivors);
		if (survivors.IsEmpty())
		{
			m_iNoSurvivorsSeconds++;
			if (m_iNoSurvivorsSeconds >= NO_SURVIVORS_GRACE || m_iEscaped >= m_iSurvivorsToEscape)
			{
				EndByEscapes("No survivors are left.");
				return;
			}
		}
		else
		{
			m_iNoSurvivorsSeconds = 0;
		}

		if (m_iTimeLimitMinutes > 0 && m_fElapsedSeconds >= m_iTimeLimitMinutes * 60)
			EndByEscapes("Dawn never came - time is up.");
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateGenerators(array<int> survivors, array<int> killers)
	{
		bool changed;
		int running = 0;

		foreach (int i, IEntity generator : m_aGenerators)
		{
			if (m_aGeneratorPercent[i] >= 100)
			{
				running++;
				continue;
			}

			bool worked = false;
			if (generator)
			{
				int workers = CountNear(generator.GetOrigin(), m_fWorkRadius, survivors);
				int blockers = CountNear(generator.GetOrigin(), m_fWorkRadius, killers);
				worked = workers > 0 && (!m_bKillerStopsWork || blockers == 0);
				if (worked)
				{
					float speed = 1 + m_fExtraWorkerSpeed * (Math.Min(workers, 3) - 1);
					m_aGeneratorWork[i] = m_aGeneratorWork[i] + speed;

					// Killers hear it start up (once in a while, not every second)
					if (!m_aGeneratorWorked[i] && m_bRevealWork && m_fElapsedSeconds - m_aLastReveal[i] >= REVEAL_COOLDOWN)
					{
						m_aLastReveal[i] = m_fElapsedSeconds;
						CRF_RadioTransmissionEvents.PopupToFaction(m_sKillerFaction, "A generator is starting up",
							string.Format("%1, grid %2.", GetGeneratorLabel(i), CRF_RadioTransmissionEvents.GetGrid100(generator.GetOrigin())), 8);
					}
				}
			}

			int percent = Math.Clamp((int)(m_aGeneratorWork[i] * 100 / Math.Max(1, m_iGeneratorSeconds)), 0, 100);
			if (percent != m_aGeneratorPercent[i] || worked != m_aGeneratorWorked[i])
			{
				m_aGeneratorPercent[i] = percent;
				m_aGeneratorWorked[i] = worked;
				changed = true;
			}

			if (percent >= 100)
			{
				running++;
				m_aGeneratorWorked[i] = false;
				CRF_RadioTransmissionEvents.PopupToFaction("", GetGeneratorLabel(i) + " is running",
					string.Format("%1 of %2 generators needed.", running, GetGeneratorsRequired()), 6);
			}
		}

		if (changed)
			Replication.BumpMe();

		if (running < GetGeneratorsRequired())
			return;

		if (m_Radio)
		{
			m_iPhase = CRF_ESlasherPhase.RADIO;
			CRF_RadioTransmissionEvents.PopupToFaction(m_sSurvivorFaction, "The radio has power",
				"Get to the radio and call for extraction.", 8);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sKillerFaction, "The radio has power",
				string.Format("They'll try to call for help. Radio at grid %1.", CRF_RadioTransmissionEvents.GetGrid100(m_Radio.GetOrigin())), 8);
		}
		else
		{
			CallExtraction();
		}

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRadio(array<int> survivors, array<int> killers)
	{
		int workers = CountNear(m_Radio.GetOrigin(), m_fWorkRadius, survivors);
		int blockers = CountNear(m_Radio.GetOrigin(), m_fWorkRadius, killers);
		bool worked = workers > 0 && (!m_bKillerStopsWork || blockers == 0);
		if (worked)
			m_fRadioWork += 1;

		int percent = Math.Clamp((int)(m_fRadioWork * 100 / Math.Max(1, m_iRadioSeconds)), 0, 100);
		if (percent != m_iRadioPercent || worked != m_bRadioWorked)
		{
			m_iRadioPercent = percent;
			m_bRadioWorked = worked;
			Replication.BumpMe();
		}

		if (percent >= 100)
			CallExtraction();
	}

	//------------------------------------------------------------------------------------------------
	protected void CallExtraction()
	{
		m_iPhase = CRF_ESlasherPhase.INBOUND;
		m_iPhaseSecondsLeft = m_iInboundSeconds;
		m_bRadioWorked = false;
		Replication.BumpMe();

		string grid = "";
		if (m_Extraction)
			grid = " Landing zone: grid " + CRF_RadioTransmissionEvents.GetGrid100(m_Extraction.GetOrigin()) + ".";

		CRF_RadioTransmissionEvents.PopupToFaction("", "Extraction is on its way",
			string.Format("It lands in %1 and won't wait long.%2", CRF_SpecialsHelper.FormatSeconds(m_iInboundSeconds), grid), 10);
	}

	//------------------------------------------------------------------------------------------------
	//! Survivors at the LZ while extraction waits get out
	protected void Board(array<int> survivors)
	{
		if (!m_Extraction)
			return;

		vector lz = m_Extraction.GetOrigin();
		foreach (int playerId : survivors)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || vector.DistanceXZ(entity.GetOrigin(), lz) > m_fExtractionRadius)
				continue;

			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!CRF_SpecialsHelper.ExtractPlayer(playerId))
				continue;

			m_iEscaped++;
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "You escaped", "You made it out. Watch the rest from here.", 8);
			CRF_RadioTransmissionEvents.PopupToFaction("", playerName + " escaped", string.Format("%1 out so far.", m_iEscaped), 5);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! "Something is close" for survivors near a killer
	protected void Terrorize(array<int> survivors, array<int> killers)
	{
		if (m_fTerrorRadius <= 0 || killers.IsEmpty())
			return;

		foreach (int playerId : survivors)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || CRF_SpecialsHelper.DistanceToNearestPlayer(entity.GetOrigin(), killers) > m_fTerrorRadius)
				continue;

			float last;
			if (m_mLastTerror.Find(playerId, last) && m_fElapsedSeconds - last < TERROR_COOLDOWN)
				continue;

			m_mLastTerror.Set(playerId, m_fElapsedSeconds);
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Something is close", "Your heart is pounding.", 3);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A dead killer vanishes after a moment, taking the mask and blade with it
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || m_fKillerBodyRemoveDelay <= 0)
			return;

		IEntity victimEntity = instigatorContextData.GetVictimEntity();
		if (!IsKiller(victimEntity))
			return;

		RplComponent bodyRpl = RplComponent.Cast(victimEntity.FindComponent(RplComponent));
		if (bodyRpl)
			GetGame().GetCallqueue().CallLater(RemoveKillerBody, m_fKillerBodyRemoveDelay * 1000, false, bodyRpl.Id());
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveKillerBody(RplId bodyId)
	{
		IEntity body = COA_EntityHelper.GetEntityFromRplId(bodyId);
		if (body)
			SCR_EntityHelper.DeleteEntityAndChildren(body);
	}

	//===================================================================================
	// VICTORY
	//===================================================================================

	protected void EndByEscapes(string reason)
	{
		if (m_iEscaped >= m_iSurvivorsToEscape)
			Victory(m_sSurvivorFaction, string.Format("%1 %2 survivor(s) escaped.", reason, m_iEscaped));
		else
			Victory(m_sKillerFaction, string.Format("%1 Only %2 escaped - nobody leaves.", reason, m_iEscaped));
	}

	//------------------------------------------------------------------------------------------------
	protected void Victory(string factionKey, string reason)
	{
		if (m_bEnded)
			return;

		m_bEnded = true;
		m_bRoundLive = false;
		m_iPhase = CRF_ESlasherPhase.ENDED;
		Replication.BumpMe();
		GetGame().GetCallqueue().Remove(Tick);

		string title = "THE KILLERS WIN";
		if (factionKey == m_sSurvivorFaction)
			title = "THE SURVIVORS WIN";
		CRF_RadioTransmissionEvents.PopupToFaction("", title, reason, 12);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(factionKey, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// CLIENT (1 s): map markers, the survivors' speed limit and the killers' sounds
	//===================================================================================

	protected void ClientUpdate()
	{
		string localFaction = CRF_SpecialsHelper.GetLocalFactionKey();
		UpdateSpeedLimit(localFaction);
		UpdateMarkers(localFaction);
		UpdateKillerSounds();
	}

	//------------------------------------------------------------------------------------------------
	//! Killers on the move are heard: a murmur every so often while they walk or run, a scream while
	//! they sprint. Each sound is played on the killer and moves with them, so it carries direction and
	//! distance. A killer standing still makes no sound.
	//!
	//! Multiplayer: every machine with a player (clients, a listen-server host, Workbench) runs this for
	//! itself, so nothing is sent over the network. It only needs what every client already has: the
	//! killers' characters, streamed in and moved by replication, and their replicated faction. Killers
	//! are found by a world query around the listener rather than through the player manager.
	protected void UpdateKillerSounds()
	{
		if (!m_bRoundLive || m_sKillerSoundProject.IsEmpty())
			return;

		IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
		vector listener;
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (camera)
			listener = camera.GetOrigin();
		else if (localEntity)
			listener = localEntity.GetOrigin();
		else
			return;

		m_aNearbyKillers.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(listener, m_fKillerSoundRange, CollectKiller, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		// Forget killers that died, left or went out of range, so they start fresh when they come back
		array<IEntity> known = {};
		foreach (IEntity tracked, vector unused : m_mKillerLastPosition)
			known.Insert(tracked);
		foreach (IEntity tracked : known)
		{
			if (!tracked || !m_aNearbyKillers.Contains(tracked))
			{
				m_mKillerLastPosition.Remove(tracked);
				m_mKillerNextSound.Remove(tracked);
			}
		}

		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		foreach (IEntity killer : m_aNearbyKillers)
		{
			if (killer == localEntity)
				continue;

			vector position = killer.GetOrigin();
			vector lastPosition;
			bool seenBefore = m_mKillerLastPosition.Find(killer, lastPosition);
			m_mKillerLastPosition.Set(killer, position);
			if (!seenBefore)
				continue;

			float speed = vector.DistanceXZ(position, lastPosition) / CLIENT_UPDATE_SECONDS;
			if (speed < KILLER_MOVING_SPEED)
				continue;

			float nextSound;
			if (m_mKillerNextSound.Find(killer, nextSound) && now < nextSound)
				continue;

			string eventName = m_sKillerMoveEvent;
			float interval = m_fKillerMoveInterval;
			if (speed >= KILLER_SPRINT_SPEED)
			{
				eventName = m_sKillerSprintEvent;
				interval = m_fKillerSprintInterval;
			}

			m_mKillerNextSound.Set(killer, now + interval * Math.RandomFloat(0.7, 1.3));
			PlayKillerSound(killer, eventName);
		}

		m_aNearbyKillers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Query callback: living killers
	protected bool CollectKiller(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character || !IsKiller(entity))
			return true;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (controller && !controller.IsDead() && !m_aNearbyKillers.Contains(entity))
			m_aNearbyKillers.Insert(entity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A sound attached to the killer: not static, so the sound manager keeps moving it with them
	protected void PlayKillerSound(IEntity killer, string eventName)
	{
		if (eventName.IsEmpty())
			return;

		SCR_AudioSourceConfiguration config = new SCR_AudioSourceConfiguration();
		config.m_sSoundProject = m_sKillerSoundProject;
		config.m_sSoundEventName = eventName;
		config.m_eFlags = EAudioSourceConfigurationFlag.EnvironmentSignals;
		SCR_SoundManagerModule.CreateAndPlayAudioSource(killer, config);
	}

	//------------------------------------------------------------------------------------------------
	//! OverrideMaxSpeed runs where the character is controlled, so each client limits its own survivor
	protected void UpdateSpeedLimit(string localFaction)
	{
		IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
		ChimeraCharacter character = ChimeraCharacter.Cast(controlled);
		if (!character)
			return;

		bool limited = m_bRoundLive && localFaction == m_sSurvivorFaction && m_fSurvivorMaxSpeed < 1;
		if (limited == (m_SpeedLimitedCharacter == character))
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		if (limited)
		{
			controller.OverrideMaxSpeed(m_fSurvivorMaxSpeed);
			m_SpeedLimitedCharacter = character;
		}
		else
		{
			controller.OverrideMaxSpeed(-1);
			m_SpeedLimitedCharacter = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateMarkers(string localFaction)
	{
		m_Markers.Begin();

		bool survivor = localFaction == m_sSurvivorFaction;
		bool killer = localFaction == m_sKillerFaction;
		int friendly = ARGB(255, 80, 200, 120);
		int danger = ARGB(255, 220, 60, 60);

		if (m_bRoundLive && (survivor || killer))
		{
			// Generators: survivors see all of them, killers only those being worked on
			if (m_aGeneratorNames)
			{
				foreach (int i, string generatorName : m_aGeneratorNames)
				{
					if (!m_aGeneratorPercent.IsIndexValid(i))
						continue;

					IEntity generator = GetGame().GetWorld().FindEntityByName(generatorName);
					if (!generator)
						continue;

					int percent = m_aGeneratorPercent[i];
					bool worked = m_aGeneratorWorked.IsIndexValid(i) && m_aGeneratorWorked[i];
					if (survivor)
					{
						string state = string.Format("%1%%", percent);
						if (percent >= 100)
							state = "running";
						m_Markers.Want(generator.GetOrigin(), GetGeneratorLabel(i) + " - " + state, m_sMarkerIcon, friendly);
					}
					else if (worked && m_bRevealWork)
					{
						m_Markers.Want(generator.GetOrigin(), GetGeneratorLabel(i) + " - they're here", m_sMarkerIcon, danger);
					}
				}
			}

			// The radio, once it has power
			IEntity radio = GetGame().GetWorld().FindEntityByName(m_sRadioName);
			if (radio && m_iPhase == CRF_ESlasherPhase.RADIO)
			{
				if (survivor)
					m_Markers.Want(radio.GetOrigin(), string.Format("Radio - %1%%", m_iRadioPercent), m_sMarkerIcon, friendly);
				else
					m_Markers.Want(radio.GetOrigin(), "Radio", m_sMarkerIcon, danger);
			}

			// The LZ, once extraction is called (both sides know)
			IEntity lz = GetGame().GetWorld().FindEntityByName(m_sExtractionName);
			if (lz && (m_iPhase == CRF_ESlasherPhase.INBOUND || m_iPhase == CRF_ESlasherPhase.BOARDING))
			{
				string label = "Extraction - lands in " + CRF_SpecialsHelper.FormatSeconds(m_iPhaseSecondsLeft);
				if (m_iPhase == CRF_ESlasherPhase.BOARDING)
					label = "Extraction - leaves in " + CRF_SpecialsHelper.FormatSeconds(m_iPhaseSecondsLeft);

				int color = friendly;
				if (killer)
					color = danger;
				m_Markers.Want(lz.GetOrigin(), label, m_sMarkerIcon, color);
			}
		}

		m_Markers.Apply();
	}

	//===================================================================================
	// HOOKS (stamina and damage overrides)
	//===================================================================================

	//! A character on the killers' faction
	protected bool IsKiller(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		Faction faction = character.GetFaction();
		return faction && faction.GetFactionKey() == m_sKillerFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! Everywhere (the stamina hook runs on the character's owner)
	bool IsTirelessKiller(IEntity character)
	{
		return m_bKillersTireless && IsKiller(character);
	}

	//------------------------------------------------------------------------------------------------
	//! Multiplier for damage dealt to this entity (CRF_Slasher_SCR_DamageManagerComponent)
	float GetDamageScale(IEntity entity)
	{
		if (IsKiller(entity))
			return m_fKillerDamageTaken;

		return 1;
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	protected int GetGeneratorsRequired()
	{
		if (m_iGeneratorsRequired <= 0 || m_iGeneratorsRequired > m_aGenerators.Count())
			return m_aGenerators.Count();

		return m_iGeneratorsRequired;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetGeneratorLabel(int index)
	{
		return string.Format("Generator %1", index + 1);
	}

	//------------------------------------------------------------------------------------------------
	protected int CountNear(vector position, float radius, array<int> playerIds)
	{
		int count = 0;
		foreach (int playerId : playerIds)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (entity && vector.DistanceXZ(entity.GetOrigin(), position) <= radius)
				count++;
		}
		return count;
	}
}
