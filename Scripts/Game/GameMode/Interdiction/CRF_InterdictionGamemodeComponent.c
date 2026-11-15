//------------------------------------------------------------------------------------
// CRF_InterdictionGamemodeComponent: hunt a convoy, take its officer, use what he knows to find the HQ
//
// The hunting side (players) looks for an AI convoy on a marked route and stops it. The officer riding
// with it surrenders when a hunter gets close while he's on foot or his vehicle has stopped; zip-tied
// (ACE Captives) he gives up the HQ's exact position, killed first his documents only give a large area
// around it. The hunters then secure the HQ and everyone still alive comes home to base to win.
// The defending side wins when the clock runs out, or when the hunters are wiped out with no respawns left.
//
// The server decides everything. Clients get only the round phase, the officer's state and - once it's
// revealed, never before - the HQ position or the hint circle, which the hunters' own machines turn into
// map markers.
//
// Setup for mission makers (full guide: !Docs/MissionMaking/Gamemodes/CRF_INTERDICTION_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the COA_Lobby gamemode entity.
//   2. Place the convoy as one SCR_AIGroup named "interdiction_convoy"; one member is the officer prefab.
//   3. Place named entities "interdiction_hq" (HQ centre) and "interdiction_base" (hunters' base centre).
//------------------------------------------------------------------------------------

enum CRF_EInterdictionPhase
{
	WAITING,	//!< the round isn't live yet
	HUNT,		//!< convoy, officer and HQ
	EXTRACT,	//!< HQ secured: everyone back to base
	ENDED
}

enum CRF_EInterdictionOfficerState
{
	UNKNOWN,		//!< not found (yet)
	FREE,
	SURRENDERING,	//!< being taken out of his vehicle / about to put his hands up
	SURRENDERED,	//!< hands up, waiting to be zip-tied
	CAPTURED,
	KILLED
}

enum CRF_EInterdictionReveal
{
	NONE,
	HINT,	//!< a large circle somewhere around the HQ
	EXACT
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Interdiction: stop the convoy, take its officer alive, secure the HQ he gives up and get home.")]
class CRF_InterdictionGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_InterdictionGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the hunting side (players)", category: "Interdiction")]
	protected string m_sBluforFaction;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the insurgents (the AI and their player element)", category: "Interdiction")]
	protected string m_sOpforFaction;

	[Attribute("interdiction_convoy", UIWidgets.EditBox, "Name of the placed SCR_AIGroup that is the convoy", category: "Interdiction")]
	protected string m_sConvoyGroupName;

	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab of the officer: the convoy group's member spawned from it is the officer", params: "et", category: "Interdiction")]
	protected ResourceName m_rOfficerPrefab;

	[Attribute("40", UIWidgets.EditBox, "The officer surrenders when a living hunter is within this many metres while he's on foot or his vehicle has stopped", params: "5 300 1", category: "Interdiction")]
	protected float m_fSurrenderRadius;

	[Attribute("3", UIWidgets.EditBox, "Seconds the surrender condition must hold", params: "0 60 0.5", category: "Interdiction")]
	protected float m_fSurrenderDelay;

	[Attribute("interdiction_hq", UIWidgets.EditBox, "Name of an entity at the centre of the HQ", category: "Interdiction")]
	protected string m_sHQAnchorName;

	[Attribute("150", UIWidgets.EditBox, "Metres from the HQ entity that count as the HQ", params: "20 1000 1", category: "Interdiction")]
	protected float m_fHQRadius;

	[Attribute("2", UIWidgets.EditBox, "The HQ is secured while at most this many conscious insurgents (AI and players) are inside it and at least one living hunter is", params: "0 50 1", category: "Interdiction")]
	protected int m_iHQClearedMaxOpfor;

	[Attribute("30", UIWidgets.EditBox, "Seconds the HQ must stay cleared to count as secured. Once secured it stays secured.", params: "0 600 1", category: "Interdiction")]
	protected float m_fHQSecureSeconds;

	[Attribute("900", UIWidgets.EditBox, "Radius of the area the hunters get if the officer dies before he's captured. It always contains the HQ; its centre is up to 60% of the radius away from it.", params: "100 5000 10", category: "Interdiction")]
	protected float m_fHintRadius;

	[Attribute("interdiction_base", UIWidgets.EditBox, "Name of an entity at the centre of the hunters' base", category: "Interdiction")]
	protected string m_sBaseAnchorName;

	[Attribute("350", UIWidgets.EditBox, "Metres from the base entity that count as back at base", params: "20 3000 1", category: "Interdiction")]
	protected float m_fBaseRadius;

	[Attribute("120", UIWidgets.EditBox, "Minutes from the round going live until the insurgents win. 0: no limit.", params: "0 600 1", category: "Interdiction")]
	protected int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Interdiction")]
	protected bool m_bEndMissionOnVictory;

	// Replicated: what clients need for the hunters' markers and the mission info. The HQ position is
	// only ever in m_vRevealPosition, and only once it's revealed.
	[RplProp()]
	protected int m_iPhase;
	[RplProp()]
	protected int m_iOfficerState;
	[RplProp(onRplName: "OnRevealReplicated")]
	protected int m_iRevealState;
	[RplProp(onRplName: "OnRevealReplicated")]
	protected vector m_vRevealPosition;	//!< HINT: circle centre; EXACT: the HQ
	[RplProp(onRplName: "OnRevealReplicated")]
	protected float m_fRevealRadius;
	[RplProp()]
	protected bool m_bHQSecured;
	[RplProp()]
	protected bool m_bBluforWon;

	// Every machine with a local player
	protected ref CRF_SpecialsMarkerSet m_Markers;
	protected IEntity m_LocalHintCircle;
	protected vector m_vLocalHintCenter;
	protected bool m_bLocalHintPlaced;	//!< tried for m_vLocalHintCenter, even if the spawn failed: no retry every poll

	// Server
	protected COA_Gamemode m_Gamemode;
	protected bool m_bRoundLive;
	protected bool m_bEndRequested;
	protected vector m_vHQPosition;
	protected vector m_vBasePosition;
	protected vector m_vHintCenter;
	protected float m_fDeadline;			//!< world time (ms) the clock runs out, 0 = none
	protected float m_fHQSecureProgress;
	protected bool m_bHQProgressShown;
	protected float m_fWipeSeconds;
	protected int m_iLastAtBase = -1;
	protected int m_iLastAlive = -1;
	protected ref array<int> m_aHunters = {};		//!< living hunters, gathered once per tick
	protected ref array<IEntity> m_aQueryResults = {};

	// Server: the officer
	protected RplId m_OfficerRplId = RplId.Invalid();	//!< not a pointer: his body can be deleted
	protected SCR_ChimeraCharacter m_Officer;			//!< resolved from m_OfficerRplId at the start of each tick
	protected int m_iOfficerSearches;
	protected bool m_bOfficerSearchWarned;
	protected float m_fSurrenderHeld;		//!< seconds the surrender condition has held
	protected float m_fSurrenderStep;		//!< seconds since the surrender began
	protected float m_fNextDismountOrder;
	protected float m_fOnFootSince = -1;
	protected bool m_bWeaponDropped;
	protected float m_fSurrenderRepair;

	// Server: task ids (CRF_ObjectiveTasks)
	protected string m_sTaskIntercept;
	protected string m_sTaskTakeAlive;
	protected string m_sTaskFindHQ;
	protected string m_sTaskReturn;
	protected string m_sTaskProtectConvoy;
	protected string m_sTaskDefendHQ;

	protected static const int TICK_MS = 500;
	protected static const float TICK_SECONDS = 0.5;
	protected static const int OFFICER_SEARCH_MS = 1000;
	protected static const int OFFICER_SEARCH_WARN_AFTER = 60;	//!< searches (seconds) before warning
	protected static const float STOPPED_SPEED = 2;				//!< m/s a vehicle counts as stopped below
	protected static const float DISMOUNT_TELEPORT_SECONDS = 6;	//!< after this, he's teleported out of a vehicle he can't climb out of
	protected static const float DISMOUNT_RETRY_SECONDS = 2;
	protected static const float SURRENDER_AFTER_DISMOUNT = 1;	//!< a beat between dropping his weapon and raising his hands
	protected static const float SURRENDER_REPAIR_SECONDS = 5;
	protected static const float HINT_MAX_OFFSET = 0.6;
	protected static const float WIPE_SECONDS = 60;
	protected static const float HQ_QUERY_MARGIN = 30;
	protected static const int MARKER_POLL_MS = 2000;
	protected static const int MARKER_WIDGET_DELAY_MS = 100;
	protected static const ResourceName HQ_MARKER_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";
	protected static const ResourceName SHAPE_MARKER_PREFAB = "{B1A844B7FEF4BC41}PrefabsMissionMaking/Markers/ShapeMarkers/ShapeMarker_Base.et";
	protected static const string LOG = "[CRF_Interdiction]";
	protected static const string MODE = "Interdiction";

	protected static CRF_InterdictionGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_InterdictionGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_InterdictionGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_InterdictionGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		if (!System.IsConsoleApp())
		{
			m_Markers = new CRF_SpecialsMarkerSet();
			GetGame().GetCallqueue().CallLater(UpdateLocalMarkers, MARKER_POLL_MS, true);
		}

		if (!Replication.IsServer())
			return;

		// Polled: the Lobby managers may not exist yet this early, and a mission without safestart
		// goes live on the gamemode state instead
		GetGame().GetCallqueue().CallLater(WaitForRoundLive, 1000, true);

		// Looked for from the start, not from round live: a member that dies leaves the group's agents,
		// so the earlier he's found the better
		if (m_rOfficerPrefab.IsEmpty())
			Print(LOG + " No officer prefab set: there is no officer to take. The HQ and the extraction still work.", LogLevel.WARNING);
		else
			GetGame().GetCallqueue().CallLater(SearchForOfficer, OFFICER_SEARCH_MS, true);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(WaitForRoundLive);
			GetGame().GetCallqueue().Remove(SearchForOfficer);
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(UpdateLocalMarkers);
			GetGame().GetCallqueue().Remove(EnsureMarkerWidget);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Markers)
			m_Markers.Clear();

		ClearLocalHintCircle();

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

		if (m_bRoundLive && m_iPhase != CRF_EInterdictionPhase.ENDED)
		{
			m_bEndRequested = true;
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(SearchForOfficer);
			m_iPhase = CRF_EInterdictionPhase.ENDED;
			Replication.BumpMe();
		}
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		IEntity hq = CRF_SpecialsHelper.FindNamedEntity(m_sHQAnchorName, LOG);
		IEntity base = CRF_SpecialsHelper.FindNamedEntity(m_sBaseAnchorName, LOG);
		if (!hq || !base)
		{
			Print(LOG + " The round went live without its HQ or base entity: the interdiction is not running.", LogLevel.ERROR);
			return;
		}

		m_bRoundLive = true;
		m_vHQPosition = hq.GetOrigin();
		m_vBasePosition = base.GetOrigin();
		m_vHintCenter = PickHintCenter(m_vHQPosition);

		if (m_iTimeLimitMinutes > 0)
			m_fDeadline = GetGame().GetWorld().GetWorldTime() + m_iTimeLimitMinutes * 60000;

		m_iPhase = CRF_EInterdictionPhase.HUNT;
		Replication.BumpMe();

		CreateTasks();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sBluforFaction, "INTERDICT THE CONVOY", "Find the convoy on its marked route and stop it. Take the officer alive: he knows where the HQ is.", 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sOpforFaction, "PROTECT THE CONVOY AND THE HQ", "The enemy is hunting the convoy's officer. If they take him, they will come for the HQ.", 10);

		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//! Decided once, on the server: anywhere up to HINT_MAX_OFFSET of the radius from the HQ, so the HQ
	//! is always inside the circle but never obviously at its centre
	protected vector PickHintCenter(vector hq)
	{
		float angle = Math.RandomFloat(0, Math.PI2);
		float distance = Math.Sqrt(Math.RandomFloat01()) * HINT_MAX_OFFSET * m_fHintRadius;

		vector center = hq;
		center[0] = hq[0] + Math.Cos(angle) * distance;
		center[2] = hq[2] + Math.Sin(angle) * distance;
		center[1] = SCR_TerrainHelper.GetTerrainY(center);
		return center;
	}

	//===================================================================================
	// SERVER: TASKS
	//===================================================================================

	protected void CreateTasks()
	{
		m_sTaskIntercept = CRF_ObjectiveTasks.MakeId(MODE, "Intercept", m_sBluforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskIntercept, m_sBluforFaction, "Intercept the convoy",
			"The convoy drives the route marked on your map, round and round. Find it and stop it.",
			CRF_ObjectiveTasks.ICON_KILL, SCR_ETaskState.FAILED);

		m_sTaskTakeAlive = CRF_ObjectiveTasks.MakeId(MODE, "TakeAlive", m_sBluforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskTakeAlive, m_sBluforFaction, "Take the officer alive",
			"Get close to the officer once he's on foot or his vehicle has stopped and he will surrender. Zip-tie him: alive, he gives up the HQ's exact location. Dead, his documents only point to a large area.",
			CRF_ObjectiveTasks.ICON_CAPTURE, SCR_ETaskState.FAILED);

		m_sTaskFindHQ = CRF_ObjectiveTasks.MakeId(MODE, "FindHQ", m_sBluforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskFindHQ, m_sBluforFaction, "Find the insurgent HQ",
			"Its location is unknown. Take the officer to find out where it is, then clear it and hold it.",
			CRF_ObjectiveTasks.ICON_SEARCH, SCR_ETaskState.FAILED);

		m_sTaskProtectConvoy = CRF_ObjectiveTasks.MakeId(MODE, "ProtectConvoy", m_sOpforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskProtectConvoy, m_sOpforFaction, "Protect the convoy",
			"Keep the convoy's officer out of enemy hands. If they take him alive, he gives up the HQ.",
			CRF_ObjectiveTasks.ICON_PROTECT, SCR_ETaskState.COMPLETED);

		m_sTaskDefendHQ = CRF_ObjectiveTasks.MakeId(MODE, "DefendHQ", m_sOpforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskDefendHQ, m_sOpforFaction, "Defend the HQ",
			"Hold the HQ until the clock runs out, or wipe the enemy out.",
			CRF_ObjectiveTasks.ICON_DEFEND, SCR_ETaskState.COMPLETED);
	}

	//===================================================================================
	// SERVER: THE ROUND
	//===================================================================================

	protected void Tick()
	{
		if (m_bEndRequested)
			return;

		CRF_SpecialsHelper.GetAlivePlayers(m_sBluforFaction, m_aHunters);

		TickOfficer();

		if (m_iPhase == CRF_EInterdictionPhase.HUNT)
			TickHQ();
		else if (m_iPhase == CRF_EInterdictionPhase.EXTRACT)
			TickExtraction();

		if (m_bEndRequested)
			return;

		TickWipe();
		if (m_bEndRequested)
			return;

		if (m_fDeadline > 0 && GetGame().GetWorld().GetWorldTime() >= m_fDeadline)
			EndRound(false, "Time ran out before the operation was complete.");
	}

	//===================================================================================
	// SERVER: THE OFFICER
	//===================================================================================

	//! The convoy group's member spawned from the officer prefab. Retried until the group has spawned it.
	protected void SearchForOfficer()
	{
		if (ResolveOfficer() || m_bEndRequested)
		{
			GetGame().GetCallqueue().Remove(SearchForOfficer);
			return;
		}

		m_iOfficerSearches++;

		IEntity groupEntity = GetGame().GetWorld().FindEntityByName(m_sConvoyGroupName);
		SCR_AIGroup convoyGroup = SCR_AIGroup.Cast(groupEntity);

		int agentCount;
		if (convoyGroup)
		{
			array<AIAgent> agents = {};
			agentCount = convoyGroup.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;

				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(agent.GetControlledEntity());
				if (!character || !IsOfficerPrefab(character))
					continue;

				SetOfficer(character);
				return;
			}
		}

		if (m_bOfficerSearchWarned || m_iOfficerSearches < OFFICER_SEARCH_WARN_AFTER)
			return;

		// Keeps looking quietly after this: a group spawning late still works
		m_bOfficerSearchWarned = true;
		if (!groupEntity)
			Print(string.Format("%1 No entity named '%2': there is no convoy officer to take. The HQ and the extraction still work.", LOG, m_sConvoyGroupName), LogLevel.WARNING);
		else if (!convoyGroup)
			Print(string.Format("%1 '%2' is not an SCR_AIGroup: there is no convoy officer to take.", LOG, m_sConvoyGroupName), LogLevel.WARNING);
		else
			Print(string.Format("%1 The convoy group '%2' has %3 members and none was spawned from the officer prefab '%4'. Check its unit prefab slots.", LOG, m_sConvoyGroupName, agentCount, m_rOfficerPrefab), LogLevel.WARNING);
	}

	protected bool IsOfficerPrefab(notnull IEntity entity)
	{
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return false;

		// Compare the GUIDs: a moved or renamed prefab keeps its GUID
		string prefab = prefabData.GetPrefabName();
		string officer = m_rOfficerPrefab;
		if (prefab.Length() >= 18 && officer.Length() >= 18)
			return prefab.Substring(0, 18) == officer.Substring(0, 18);

		return prefab == officer;
	}

	protected void SetOfficer(notnull SCR_ChimeraCharacter officer)
	{
		RplComponent rpl = officer.GetRplComponent();
		if (!rpl)
		{
			Print(LOG + " The convoy officer has no RplComponent: can't track him.", LogLevel.ERROR);
			return;
		}

		m_OfficerRplId = rpl.Id();
		m_Officer = officer;
		GetGame().GetCallqueue().Remove(SearchForOfficer);

		if (m_iOfficerState == CRF_EInterdictionOfficerState.UNKNOWN)
		{
			m_iOfficerState = CRF_EInterdictionOfficerState.FREE;
			Replication.BumpMe();
		}

		Print(string.Format("%1 Found the convoy officer (%2).", LOG, officer), LogLevel.NORMAL);
	}

	protected void TickOfficer()
	{
		if (m_iOfficerState == CRF_EInterdictionOfficerState.KILLED)
			return;

		m_Officer = ResolveOfficer();
		if (!m_Officer)
		{
			LoseOfficer();
			return;
		}

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(m_Officer.GetCharacterController());
		if (!controller)
			return;

		if (m_iOfficerState == CRF_EInterdictionOfficerState.CAPTURED)
		{
			// He was taken, so his death changes nothing now
			if (!controller.IsDead())
				KeepAIOff();
			return;
		}

		if (controller.IsDead())
		{
			OnOfficerKilled();
			return;
		}

		// However it happened (also tied while unconscious, without surrendering)
		if (controller.ACE_Captives_IsCaptive())
		{
			OnOfficerCaptured();
			return;
		}

		switch (m_iOfficerState)
		{
			case CRF_EInterdictionOfficerState.FREE:
			{
				if (SurrenderConditionHolds(controller))
					m_fSurrenderHeld += TICK_SECONDS;
				else
					m_fSurrenderHeld = 0;

				if (m_fSurrenderHeld >= m_fSurrenderDelay)
					BeginSurrender();
				break;
			}
			case CRF_EInterdictionOfficerState.SURRENDERING:
			{
				ContinueSurrender(controller);
				break;
			}
			case CRF_EInterdictionOfficerState.SURRENDERED:
			{
				KeepSurrendered(controller);
				break;
			}
		}
	}

	//! His entity is gone without us seeing him die (deleted, or replaced): look for him again
	protected void LoseOfficer()
	{
		if (m_iOfficerState == CRF_EInterdictionOfficerState.UNKNOWN)
			return;

		if (m_iOfficerState != CRF_EInterdictionOfficerState.CAPTURED)
		{
			Print(LOG + " The convoy officer's entity is gone: looking for him in the convoy group again.", LogLevel.WARNING);
			m_iOfficerState = CRF_EInterdictionOfficerState.UNKNOWN;
			m_fSurrenderHeld = 0;
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(SearchForOfficer);
			GetGame().GetCallqueue().CallLater(SearchForOfficer, OFFICER_SEARCH_MS, true);
		}

		m_Officer = null;
		m_OfficerRplId = RplId.Invalid();
	}

	protected SCR_ChimeraCharacter ResolveOfficer()
	{
		if (!m_OfficerRplId.IsValid())
			return null;

		RplComponent rpl = RplComponent.Cast(Replication.FindItem(m_OfficerRplId));
		if (!rpl)
			return null;

		IEntity entity = rpl.GetEntity();
		if (!entity || entity.IsDeleted())
			return null;

		return SCR_ChimeraCharacter.Cast(entity);
	}

	//! A living hunter close by, and the officer on foot or his vehicle stopped or wrecked
	protected bool SurrenderConditionHolds(notnull SCR_CharacterControllerComponent controller)
	{
		if (controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

		// Already in an ACE helper (carried, or put in a pose by Zeus): leave him be
		if (ACE_AnimationTools.GetHelperCompartment(m_Officer))
			return false;

		if (!IsAnyPlayerWithin(m_aHunters, m_Officer.GetOrigin(), m_fSurrenderRadius))
			return false;

		if (!m_Officer.IsInVehicle())
			return true;

		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(m_Officer);
		if (!vehicle)
			return true;

		return IsDestroyed(vehicle) || GetSpeed(vehicle) <= STOPPED_SPEED;
	}

	protected void BeginSurrender()
	{
		m_iOfficerState = CRF_EInterdictionOfficerState.SURRENDERING;
		m_fSurrenderStep = 0;
		m_fNextDismountOrder = 0;
		m_fOnFootSince = -1;
		m_bWeaponDropped = false;
		Replication.BumpMe();

		StopOfficerFighting();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sBluforFaction, "THE OFFICER IS SURRENDERING", "Restrain him with zip ties. Alive, he can tell us where the HQ is.", 8);

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(m_Officer.GetCharacterController());
		if (controller)
			ContinueSurrender(controller);
	}

	//! Out of the convoy's group, so it doesn't wait for him or order him back aboard, and his AI off,
	//! so he neither shoots nor runs
	protected void StopOfficerFighting()
	{
		// Looked up again: an emptied group can delete itself
		SCR_AIGroup convoyGroup = SCR_AIGroup.Cast(GetGame().GetWorld().FindEntityByName(m_sConvoyGroupName));
		AIControlComponent aiControl = AIControlComponent.Cast(m_Officer.FindComponent(AIControlComponent));
		if (aiControl && convoyGroup)
		{
			AIAgent agent = aiControl.GetControlAIAgent();
			if (agent && agent.GetParentGroup() == convoyGroup)
				convoyGroup.RemoveAgentFromControlledEntity(m_Officer);
		}

		KeepAIOff();
	}

	//! Vanilla switches a character's AI back on whenever it comes round from unconsciousness
	protected void KeepAIOff()
	{
		if (!m_Officer)
			return;

		AIControlComponent aiControl = AIControlComponent.Cast(m_Officer.FindComponent(AIControlComponent));
		if (aiControl && aiControl.IsAIActivated())
			aiControl.DeactivateAI();
	}

	//! Out of his vehicle first: ACE's surrender pose is a helper compartment, so he has to be on foot
	protected void ContinueSurrender(notnull SCR_CharacterControllerComponent controller)
	{
		m_fSurrenderStep += TICK_SECONDS;
		KeepAIOff();

		// Unconscious: he can be zip-tied as he is, and he carries on when he comes round
		if (controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return;

		SCR_CompartmentAccessComponent compartmentAccess = SCR_CompartmentAccessComponent.Cast(m_Officer.GetCompartmentAccessComponent());

		if (ACE_AnimationTools.GetHelperCompartment(m_Officer))
		{
			// Something else put him in an ACE pose; if it's the surrender, that's all we wanted
			if (controller.ACE_Captives_HasSurrendered())
				SetSurrendered();
			return;
		}

		if (m_Officer.IsInVehicle())
		{
			m_fOnFootSince = -1;
			if (!compartmentAccess || compartmentAccess.IsGettingOut() || m_fSurrenderStep < m_fNextDismountOrder)
				return;

			// A wreck on its side or a jammed door can stop the animated exit
			EGetOutType getOutType = EGetOutType.ANIMATED;
			if (m_fSurrenderStep >= DISMOUNT_TELEPORT_SECONDS)
				getOutType = EGetOutType.TELEPORT;

			compartmentAccess.GetOutVehicle(getOutType, -1, ECloseDoorAfterActions.INVALID, false);
			m_fNextDismountOrder = m_fSurrenderStep + DISMOUNT_RETRY_SECONDS;
			return;
		}

		if (compartmentAccess && (compartmentAccess.IsGettingOut() || compartmentAccess.ACE_IsGettingIn()))
			return;

		if (!m_bWeaponDropped)
		{
			DropWeaponInHand(controller);
			m_bWeaponDropped = true;
			m_fOnFootSince = m_fSurrenderStep;
			return;
		}

		if (m_fOnFootSince < 0)
			m_fOnFootSince = m_fSurrenderStep;

		if (m_fSurrenderStep - m_fOnFootSince < SURRENDER_AFTER_DISMOUNT)
			return;

		controller.ACE_Captives_SetSurrender(true);
		SetSurrendered();
	}

	protected void SetSurrendered()
	{
		m_iOfficerState = CRF_EInterdictionOfficerState.SURRENDERED;
		m_fSurrenderRepair = 0;
		Replication.BumpMe();
		Print(LOG + " The convoy officer has surrendered.", LogLevel.NORMAL);
	}

	protected void DropWeaponInHand(notnull SCR_CharacterControllerComponent controller)
	{
		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		WeaponSlotComponent slot = weaponManager.GetCurrentSlot();
		if (slot)
			controller.DropWeapon(slot);
	}

	//! ACE drops the surrender pose when he's knocked out (or Zeus moves him): put his hands back up once
	//! he's conscious and on foot again
	protected void KeepSurrendered(notnull SCR_CharacterControllerComponent controller)
	{
		KeepAIOff();

		if (controller.GetLifeState() != ECharacterLifeState.ALIVE || controller.ACE_Captives_HasSurrendered())
		{
			m_fSurrenderRepair = 0;
			return;
		}

		// In a vehicle or another ACE helper: whoever put him there meant it
		if (m_Officer.IsInVehicle())
			return;

		m_fSurrenderRepair += TICK_SECONDS;
		if (m_fSurrenderRepair < SURRENDER_REPAIR_SECONDS)
			return;

		m_fSurrenderRepair = 0;
		controller.ACE_Captives_SetSurrender(true);
		Print(LOG + " The convoy officer lost his surrender pose: put his hands back up.", LogLevel.WARNING);
	}

	protected void OnOfficerCaptured()
	{
		m_iOfficerState = CRF_EInterdictionOfficerState.CAPTURED;
		Replication.BumpMe();
		KeepAIOff();
		Print(LOG + " The convoy officer has been captured.", LogLevel.NORMAL);

		CRF_ObjectiveTasks.Finish(m_sTaskIntercept, SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_sTaskTakeAlive, SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_sTaskProtectConvoy, SCR_ETaskState.FAILED);

		if (m_iRevealState == CRF_EInterdictionReveal.NONE)
		{
			Reveal(CRF_EInterdictionReveal.EXACT, m_vHQPosition, 0);
			CRF_ObjectiveTasks.SetText(m_sTaskFindHQ, "Secure the insurgent HQ", string.Format("The officer gave up the HQ: grid %1, marked on your map. Clear it and hold it.", CRF_RadioTransmissionEvents.GetGrid100(m_vHQPosition)));
			CRF_ObjectiveTasks.Move(m_sTaskFindHQ, m_vHQPosition);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sBluforFaction, "INTEL: THE HQ IS LOCATED", string.Format("The officer talked: the insurgent HQ is at grid %1. It's marked on your map.", CRF_RadioTransmissionEvents.GetGrid100(m_vHQPosition)), 10);
		}

		CRF_RadioTransmissionEvents.PopupToFaction(m_sOpforFaction, "THE OFFICER HAS BEEN TAKEN", "He will talk. Expect the enemy at the HQ.", 10);
	}

	protected void OnOfficerKilled()
	{
		m_iOfficerState = CRF_EInterdictionOfficerState.KILLED;
		Replication.BumpMe();
		Print(LOG + " The convoy officer was killed before he was captured.", LogLevel.NORMAL);

		CRF_ObjectiveTasks.Finish(m_sTaskIntercept, SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_sTaskTakeAlive, SCR_ETaskState.FAILED);
		CRF_ObjectiveTasks.Finish(m_sTaskProtectConvoy, SCR_ETaskState.FAILED);

		if (m_iRevealState == CRF_EInterdictionReveal.NONE)
		{
			Reveal(CRF_EInterdictionReveal.HINT, m_vHintCenter, m_fHintRadius);
			CRF_ObjectiveTasks.SetText(m_sTaskFindHQ, "Find the insurgent HQ", string.Format("The officer's documents point to an area around grid %1, marked on your map. The HQ is somewhere inside it.", CRF_RadioTransmissionEvents.GetGrid1000(m_vHintCenter)));
			CRF_ObjectiveTasks.Move(m_sTaskFindHQ, m_vHintCenter);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sBluforFaction, "THE OFFICER IS DEAD", string.Format("His documents point to an area around grid %1. It's marked on your map: the HQ is somewhere inside.", CRF_RadioTransmissionEvents.GetGrid1000(m_vHintCenter)), 10);
		}

		CRF_RadioTransmissionEvents.PopupToFaction(m_sOpforFaction, "THE OFFICER IS DEAD", "The enemy may have found his documents. Expect them around the HQ.", 10);
	}

	//! The only place the HQ position (or the hint) is handed to clients
	protected void Reveal(CRF_EInterdictionReveal reveal, vector position, float radius)
	{
		m_iRevealState = reveal;
		m_vRevealPosition = position;
		m_fRevealRadius = radius;
		Replication.BumpMe();
	}

	//===================================================================================
	// SERVER: HQ AND EXTRACTION
	//===================================================================================

	//! Secured while it holds few enough conscious insurgents and at least one living hunter, for long enough.
	//! Revealed or not.
	protected void TickHQ()
	{
		bool huntersInside = CRF_SpecialsHelper.CountPlayersWithin(m_aHunters, m_vHQPosition, m_fHQRadius) > 0;
		if (huntersInside && CountConsciousInHQ(m_sOpforFaction) <= m_iHQClearedMaxOpfor)
			m_fHQSecureProgress += TICK_SECONDS;
		else
			m_fHQSecureProgress = 0;

		// Progress bar on the hunters' task only while it means something
		if (m_fHQSecureProgress > 0 || m_bHQProgressShown)
		{
			m_bHQProgressShown = m_fHQSecureProgress > 0;
			CRF_ObjectiveTasks.SetProgress(m_sTaskFindHQ, m_fHQSecureProgress / Math.Max(m_fHQSecureSeconds, 0.1));
		}

		if (m_fHQSecureProgress >= m_fHQSecureSeconds)
			OnHQSecured();
	}

	protected void OnHQSecured()
	{
		m_bHQSecured = true;
		m_iPhase = CRF_EInterdictionPhase.EXTRACT;
		Replication.BumpMe();
		Print(LOG + " The HQ is secured.", LogLevel.NORMAL);

		CRF_ObjectiveTasks.Finish(m_sTaskFindHQ, SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_sTaskDefendHQ, SCR_ETaskState.FAILED);

		m_sTaskReturn = CRF_ObjectiveTasks.MakeId(MODE, "Return", m_sBluforFaction);
		CRF_ObjectiveTasks.Create(m_sTaskReturn, m_sBluforFaction, "Return to base",
			"Everyone still alive has to be back at base to end the operation.",
			CRF_ObjectiveTasks.ICON_SEIZE, SCR_ETaskState.FAILED, m_vBasePosition);

		CRF_RadioTransmissionEvents.PopupToFaction(m_sBluforFaction, "HQ SECURED", "Return to base. The operation ends when everyone still alive is back.", 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sOpforFaction, "THE HQ HAS FALLEN", "Stop them getting home: hold out until the clock runs out.", 10);
	}

	//! Every living hunter at base (at least one) ends it
	protected void TickExtraction()
	{
		int alive = m_aHunters.Count();
		int atBase = CRF_SpecialsHelper.CountPlayersWithin(m_aHunters, m_vBasePosition, m_fBaseRadius);
		if (atBase != m_iLastAtBase || alive != m_iLastAlive)
		{
			m_iLastAtBase = atBase;
			m_iLastAlive = alive;
			CRF_ObjectiveTasks.SetText(m_sTaskReturn, string.Format("Return to base (%1/%2 back)", atBase, alive));
		}

		if (alive > 0 && atBase == alive)
			EndRound(true, "The HQ is secured and everyone is home.");
	}

	//! No living hunters for a while, and none of them can come back
	protected void TickWipe()
	{
		if (!HasSlottedPlayers(m_sBluforFaction))
		{
			m_fWipeSeconds = 0;
			return;
		}

		if (!m_aHunters.IsEmpty() || CanFactionRespawn(m_sBluforFaction))
		{
			m_fWipeSeconds = 0;
			return;
		}

		m_fWipeSeconds += TICK_SECONDS;
		if (m_fWipeSeconds >= WIPE_SECONDS)
			EndRound(false, string.Format("%1 has nobody left alive and no respawns.", CRF_SpecialsHelper.GetFactionName(m_sBluforFaction)));
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

	//! Conscious characters of a faction in the HQ, AI and players alike (captives are CIV to ACE)
	protected int CountConsciousInHQ(string factionKey)
	{
		m_aQueryResults.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(m_vHQPosition, m_fHQRadius + HQ_QUERY_MARGIN, AddCharacterResult, null, EQueryEntitiesFlags.DYNAMIC);

		int count;
		foreach (IEntity entity : m_aQueryResults)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character || COA_EntityHelper.IsSpectator(character))
				continue;

			if (vector.DistanceXZ(character.GetOrigin(), m_vHQPosition) > m_fHQRadius || character.GetFactionKey() != factionKey)
				continue;

			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller && controller.GetLifeState() == ECharacterLifeState.ALIVE)
				count++;
		}

		m_aQueryResults.Clear();
		return count;
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

	//! 3D, so a helicopter hovering high over him doesn't count
	protected bool IsAnyPlayerWithin(notnull array<int> playerIds, vector center, float radius)
	{
		foreach (int playerId : playerIds)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (entity && vector.Distance(entity.GetOrigin(), center) <= radius)
				return true;
		}

		return false;
	}

	protected bool IsDestroyed(notnull IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		return damageManager != null && damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected float GetSpeed(notnull IEntity entity)
	{
		Physics physics = entity.GetPhysics();
		if (!physics)
			return 0;

		return physics.GetVelocity().Length();
	}

	//===================================================================================
	// SERVER: THE END
	//===================================================================================

	protected void EndRound(bool bluforWon, string reason)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().Remove(SearchForOfficer);

		m_iPhase = CRF_EInterdictionPhase.ENDED;
		m_bBluforWon = bluforWon;
		Replication.BumpMe();

		string winner = m_sOpforFaction;
		if (bluforWon)
			winner = m_sBluforFaction;

		FinishTasks(bluforWon);
		AnnounceResult(bluforWon, reason);
		Print(string.Format("%1 Round over, %2 wins: %3", LOG, winner, reason), LogLevel.NORMAL);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//! Whatever is still open: the winner's done, the loser's failed. Hunter objectives a win made moot
	//! (the HQ was found without the officer) are cancelled rather than failed.
	protected void FinishTasks(bool bluforWon)
	{
		if (bluforWon)
		{
			CRF_ObjectiveTasks.Finish(m_sTaskReturn, SCR_ETaskState.COMPLETED);
			CRF_ObjectiveTasks.Finish(m_sTaskIntercept, SCR_ETaskState.CANCELLED);
			CRF_ObjectiveTasks.Finish(m_sTaskTakeAlive, SCR_ETaskState.CANCELLED);
			CRF_ObjectiveTasks.Finish(m_sTaskProtectConvoy, SCR_ETaskState.FAILED);
			CRF_ObjectiveTasks.Finish(m_sTaskDefendHQ, SCR_ETaskState.FAILED);
			return;
		}

		CRF_ObjectiveTasks.Finish(m_sTaskIntercept, SCR_ETaskState.FAILED);
		CRF_ObjectiveTasks.Finish(m_sTaskTakeAlive, SCR_ETaskState.FAILED);
		CRF_ObjectiveTasks.Finish(m_sTaskFindHQ, SCR_ETaskState.FAILED);
		CRF_ObjectiveTasks.Finish(m_sTaskReturn, SCR_ETaskState.FAILED);
		CRF_ObjectiveTasks.Finish(m_sTaskProtectConvoy, SCR_ETaskState.COMPLETED);
		CRF_ObjectiveTasks.Finish(m_sTaskDefendHQ, SCR_ETaskState.COMPLETED);
	}

	protected void AnnounceResult(bool bluforWon, string reason)
	{
		string bluforTitle = "MISSION FAILED";
		string opforTitle = "VICTORY";
		string winnerName = CRF_SpecialsHelper.GetFactionName(m_sOpforFaction);
		if (bluforWon)
		{
			bluforTitle = "MISSION COMPLETE";
			opforTitle = "DEFEAT";
			winnerName = CRF_SpecialsHelper.GetFactionName(m_sBluforFaction);
		}

		string neutralTitle = string.Format("Winner: %1", winnerName);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string faction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string title = neutralTitle;
			if (faction == m_sBluforFaction)
				title = bluforTitle;
			else if (faction == m_sOpforFaction)
				title = opforTitle;

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, title, reason, 12);
		}
	}

	//===================================================================================
	// EVERY MACHINE: THE HUNTERS' MAP
	//===================================================================================

	protected void OnRevealReplicated()
	{
		UpdateLocalMarkers();
	}

	//! Polled as well as on replication: the local player's faction changes with slotting
	protected void UpdateLocalMarkers()
	{
		if (!m_Markers)
			return;

		bool show = m_iRevealState != CRF_EInterdictionReveal.NONE && CRF_SpecialsHelper.GetLocalFactionKey() == m_sBluforFaction;

		m_Markers.Begin();
		if (show && m_iRevealState == CRF_EInterdictionReveal.EXACT)
			m_Markers.Want(Flat(m_vRevealPosition), "Insurgent HQ", HQ_MARKER_ICON, ARGB(255, 225, 25, 0));
		m_Markers.Apply();

		UpdateLocalHintCircle(show && m_iRevealState == CRF_EInterdictionReveal.HINT);
	}

	//! COA_ShapeMarker has no RplComponent, so each hunter spawns their own copy where the server put it
	protected void UpdateLocalHintCircle(bool show)
	{
		if (!show)
		{
			ClearLocalHintCircle();
			return;
		}

		if (m_bLocalHintPlaced && vector.DistanceSq(m_vLocalHintCenter, m_vRevealPosition) < 0.01)
			return;

		ClearLocalHintCircle();
		m_vLocalHintCenter = m_vRevealPosition;
		m_bLocalHintPlaced = true;

		Resource markerResource = Resource.Load(SHAPE_MARKER_PREFAB);
		if (!markerResource || !markerResource.IsValid())
		{
			Print(string.Format("%1 Hint circle prefab '%2' could not be loaded.", LOG, SHAPE_MARKER_PREFAB), LogLevel.ERROR);
			return;
		}

		vector position = m_vRevealPosition;
		position[1] = SCR_TerrainHelper.GetTerrainY(position);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = position;

		IEntity markerEntity = GetGame().SpawnEntityPrefab(markerResource, GetGame().GetWorld(), spawnParams);
		COA_ShapeMarker marker = COA_ShapeMarker.Cast(markerEntity);
		if (!marker)
		{
			Print(string.Format("%1 Hint circle prefab '%2' is not a COA_ShapeMarker.", LOG, SHAPE_MARKER_PREFAB), LogLevel.ERROR);
			if (markerEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(markerEntity);
			return;
		}

		// Only a hunter's machine ever has this entity, so it can show for any faction
		marker.m_bShowForAnyFaction = true;
		marker.m_bVisibleForEmptyFaction = true;
		marker.m_bUseWorldScale = true;
		marker.m_MarkerColor = new Color(1, 0.25, 0.25, 1);

		marker.SetShapeType(COA_EMapShapeMarkerType.CIRCLE);
		marker.SetShapeBorderWidth(2.5);
		marker.SetShapeRadius(m_fRevealRadius);
		marker.SetShapeName("The officer's documents: the HQ is somewhere in here");

		m_LocalHintCircle = markerEntity;

		// A manual marker builds its widget only on the map-open event it subscribes to in EOnInit, so one
		// spawned while the map is open stays invisible until it's reopened. A beat later: EOnInit hasn't run yet.
		GetGame().GetCallqueue().CallLater(EnsureMarkerWidget, MARKER_WIDGET_DELAY_MS, false, markerEntity);
	}

	protected void EnsureMarkerWidget(IEntity markerEntity)
	{
		COA_ManualMarker marker = COA_ManualMarker.Cast(markerEntity);
		if (!marker)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		marker.CreateMapWidget(mapEntity.GetMapConfig());
	}

	protected void ClearLocalHintCircle()
	{
		if (m_LocalHintCircle)
			SCR_EntityHelper.DeleteEntityAndChildren(m_LocalHintCircle);

		m_LocalHintCircle = null;
		m_bLocalHintPlaced = false;
	}

	protected vector Flat(vector position)
	{
		return Vector(Math.Round(position[0]), 0, Math.Round(position[2]));
	}

	//===================================================================================
	// ANY MACHINE (replicated)
	//===================================================================================

	CRF_EInterdictionPhase GetPhase()
	{
		return m_iPhase;
	}

	CRF_EInterdictionOfficerState GetOfficerState()
	{
		return m_iOfficerState;
	}

	CRF_EInterdictionReveal GetRevealState()
	{
		return m_iRevealState;
	}

	bool IsHQSecured()
	{
		return m_bHQSecured;
	}

	bool DidBluforWin()
	{
		return m_bBluforWon;
	}

	string GetBluforFactionKey()
	{
		return m_sBluforFaction;
	}

	string GetOpforFactionKey()
	{
		return m_sOpforFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iPhase == CRF_EInterdictionPhase.WAITING)
		{
			lines.Insert("Interdiction: starts when the round goes live");
			return;
		}

		if (m_iPhase == CRF_EInterdictionPhase.ENDED)
		{
			string winner = CRF_SpecialsHelper.GetFactionName(m_sOpforFaction);
			if (m_bBluforWon)
				winner = CRF_SpecialsHelper.GetFactionName(m_sBluforFaction);
			lines.Insert(string.Format("Interdiction: round over, %1 won", winner));
			return;
		}

		string officer = "at large";
		switch (m_iOfficerState)
		{
			case CRF_EInterdictionOfficerState.UNKNOWN: officer = "unaccounted for"; break;
			case CRF_EInterdictionOfficerState.SURRENDERING: officer = "surrendering"; break;
			case CRF_EInterdictionOfficerState.SURRENDERED: officer = "surrendered, not yet restrained"; break;
			case CRF_EInterdictionOfficerState.CAPTURED: officer = "captured"; break;
			case CRF_EInterdictionOfficerState.KILLED: officer = "dead"; break;
		}

		string hq = "location unknown";
		if (m_bHQSecured)
			hq = "secured - return to base";
		else if (m_iRevealState == CRF_EInterdictionReveal.EXACT)
			hq = "location marked";
		else if (m_iRevealState == CRF_EInterdictionReveal.HINT)
			hq = "rough area marked";

		lines.Insert(string.Format("Interdiction: officer %1", officer));
		lines.Insert(string.Format("Interdiction: HQ %1", hq));
	}
}
