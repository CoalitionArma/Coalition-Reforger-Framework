//------------------------------------------------------------------------------------------------
// CRF Persistence Manager
//
// Crash recovery for event servers. Arma Reforger servers die often, and until now every crash reset
// the mission to briefing. This drives Bohemia's native persistence system (PersistenceSystem /
// SaveGameManager) so a crashed mission resumes exactly where it stopped.
//
// LIFECYCLE
//   boot        -> LookForExistingSave()  asks SaveGameManager for saves belonging to THIS mission
//   found       -> SaveGameManager.Load() the newest, engine reloads the world with save data applied
//   after load  -> OnPersistenceLoaded() restores the gamemode phase and resumes straight into GAME
//   in mission  -> RequestSave() on tracked events + a periodic autosave
//   clean end   -> PurgeSaves() so the next boot starts fresh instead of resuming a finished mission
//
// WHAT IS AND IS NOT COVERED BY THIS FILE
//   Scripted manager state (phase, slots, tickets, objectives) is covered here and by the
//   serializers under Scripts/Game/!Systems/Persistence/Serializers/States.
//   World entities (vehicles, dropped items, corpses) persist through the engine's own entity
//   tracking, which requires PersistenceComponent on the relevant prefabs - that is prefab work in
//   Workbench, not script. See TECHNICAL_README for the checklist.
//------------------------------------------------------------------------------------------------
// Derives from SCR_BaseGameModeComponent, not plain ScriptComponent: OnWorldPostProcess() is
// declared there (forwarded from BaseGameMode), and this component lives on the game mode entity
// anyway - same as every other CRF manager.
class CRF_PersistenceManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_PersistenceManager : SCR_BaseGameModeComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================

	[Attribute("1", UIWidgets.CheckBox, "Master switch. When off this component does nothing at all - no saving, no crash resume.", category: "CRF Persistence")]
	protected bool m_bPersistenceEnabled;

	[Attribute("1", UIWidgets.CheckBox, "On boot, look for a save belonging to this mission and resume it. Turn off to save without ever auto-resuming.", category: "CRF Persistence")]
	protected bool m_bResumeOnBoot;

	[Attribute("120", UIWidgets.EditBox, "Periodic autosave interval in seconds while in GAME. 0 disables periodic saving (event-driven saves still run).", params: "0 1800 1", category: "CRF Persistence")]
	protected float m_fAutoSaveInterval;

	[Attribute("1", UIWidgets.CheckBox, "Also save on tracked gameplay events (phase change, slotting change, objective completion) rather than only on the timer.", category: "CRF Persistence")]
	protected bool m_bEventDrivenSaves;

	[Attribute("15", UIWidgets.EditBox, "Minimum seconds between two event-driven saves, so a burst of events cannot spam the storage layer.", params: "0 300 1", category: "CRF Persistence")]
	protected float m_fEventSaveCooldown;

	[Attribute("1", UIWidgets.CheckBox, "Delete this mission's saves when it finishes cleanly (reaches AAR), so the next boot starts fresh.", category: "CRF Persistence")]
	protected bool m_bPurgeOnCleanEnd;

	[Attribute("1", UIWidgets.CheckBox, "Only resume when the previous session ended unexpectedly. With this off, ANY existing save is resumed - which makes replaying a mission impossible without manually clearing saves.", category: "CRF Persistence")]
	protected bool m_bResumeOnlyAfterCrash;

	[Attribute("0", UIWidgets.CheckBox, "Force this boot to start fresh: discards any existing save for this mission instead of resuming. Use to replay a mission that crashed. Remember to turn it back off.", category: "CRF Persistence")]
	protected bool m_bForceFreshStart;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME
//=============================================================================================================================================================================================================================================================================================================================================================

	protected static CRF_PersistenceManager s_Instance;

	//! The engine subsystem. Typed as the NATIVE PersistenceSystem, not SCR_PersistenceSystem:
	//! SCR_PersistenceSystem declares no InitInfo of its own, so which class the world actually
	//! registers is not guaranteed to be the scripted one. Looking it up via the scripted type would
	//! then return null even though persistence is present and working.
	protected PersistenceSystem m_PersistenceSystem;

	//! Scripted flavour, when available. Only needed for GetOnAfterSave(); everything else this
	//! component does is on the native base. Null is survivable - it costs the completion log, not
	//! the saving itself.
	protected SCR_PersistenceSystem m_ScriptedPersistenceSystem;

	protected COA_Gamemode m_Gamemode;

	//! True when this session was started by loading a save rather than starting the mission fresh.
	protected bool m_bResumedFromSave;

	//! Set once the resume has been applied, so a second load event cannot re-apply it.
	protected bool m_bResumeApplied;

	protected float m_fAutoSaveTick;
	protected float m_fTimeSinceEventSave;

	//! Periodic "here is what I am doing" line, so a stalled or blocked state is visible without
	//! having to reproduce it under a debugger.
	protected float m_fHeartbeatTick;
	protected const float HEARTBEAT_INTERVAL_S = 30.0;

	//! Guards against stacking save requests while one is still committing.
	protected bool m_bSaveInFlight;

	//! Context for the save currently committing, so the completion log can say what it was and how
	//! long it took. OnAfterSave() only receives the save type, so anything else has to be carried
	//! across from the request.
	protected string m_sPendingSaveReason;
	protected int m_iPendingSaveStartTick;
	protected ESaveGameType m_ePendingSaveType;

	//! If a save has been in flight longer than this without reporting back, assume the completion
	//! signal was lost and release the lock. Without this the component would silently stop saving
	//! for the rest of the mission after a single dropped callback.
	protected const int SAVE_IN_FLIGHT_TIMEOUT_MS = 30000;

	//! Running totals for the session, so the console line shows whether saving is healthy overall
	//! rather than just reporting the last one in isolation.
	protected int m_iSaveSuccessCount;
	protected int m_iSaveFailureCount;

	//! Save types we have already reported a fallback for, so the substitution warning appears once
	//! per type instead of on every autosave.
	protected ref array<ESaveGameType> m_aReportedSaveTypeFallbacks = {};
	protected bool m_bNoUsableSaveTypeReported;

	//! Seconds to wait after world load before deciding whether to resume. The persistence system
	//! reports EPersistenceSystemState and needs to reach ACTIVE before WasDataLoaded() is meaningful.
	protected const float RESUME_POLL_INTERVAL_S = 0.5;
	protected const int RESUME_MAX_POLLS = 60;

	//! Marker file used to tell a crash apart from a clean shutdown.
	//!
	//! The problem: a save on disk does not by itself mean the server crashed. A mission that was
	//! stopped deliberately, or one you simply want to replay, also leaves a save - and resuming
	//! those means you can never start that mission fresh again.
	//!
	//! The marker closes that gap. It is written when the mission enters GAME and deleted when the
	//! mission ends cleanly or the server shuts down gracefully. A crash never gets the chance to
	//! delete it, so:
	//!     save + marker present  -> previous session died unexpectedly -> RESUME
	//!     save + no marker       -> previous session ended on purpose  -> START FRESH (and purge)
	//! It deliberately lives outside the save data, because it describes the save rather than being
	//! part of it.
	protected const string SESSION_MARKER_DIR = "$profile:CRF/";
	protected const string SESSION_MARKER_FILE = "$profile:CRF/session_active.marker";

//=============================================================================================================================================================================================================================================================================================================================================================
//	 INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	static CRF_PersistenceManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	void CRF_PersistenceManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_PersistenceManager()
	{
		if (s_Instance == this)
			s_Instance = null;

		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(PollForResume);
			GetGame().GetCallqueue().Remove(LookForExistingSave);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		// Every early return below announces itself. This component has several preconditions and
		// silently doing nothing when one fails is indistinguishable from it not being installed -
		// which is exactly how it behaved before, and cost a debugging session to work out.
		if (!m_bPersistenceEnabled)
		{
			Print("[CRF_Persistence] DISABLED: m_bPersistenceEnabled is off on the game mode's CRF_PersistenceManager. No saving, no crash resume.", LogLevel.WARNING);
			return;
		}

		// Authority only. Saves are server state; a client has nothing to persist.
		// RplMode.None is Workbench play mode without hosting - that IS authority, so it must not be
		// excluded here or persistence would never run in the editor.
		if (RplSession.Mode() == RplMode.Client)
		{
			Print("[CRF_Persistence] Not authority (RplMode.Client) - persistence is server-side only. This is expected on a client.", LogLevel.NORMAL);
			return;
		}

		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		m_Gamemode = COA_Gamemode.GetInstance();

		// Native static first - returns the instance whichever class the world registered.
		m_PersistenceSystem = PersistenceSystem.GetInstance();
		m_ScriptedPersistenceSystem = SCR_PersistenceSystem.Cast(m_PersistenceSystem);

		LogStartupDiagnostics();

		if (!m_PersistenceSystem)
			return;

		// The save-completion log rides on the scripted subclass's invoker. If the world registered
		// the plain native system there is no invoker to hook, which costs the log line but not the
		// saving, so carry on rather than bailing.
		if (m_ScriptedPersistenceSystem)
			m_ScriptedPersistenceSystem.GetOnAfterSave().Insert(OnAfterSave);
		else
			Print("[CRF_Persistence] Native PersistenceSystem present but not SCR_PersistenceSystem - saves will still run, but completion logging is unavailable.", LogLevel.WARNING);

		// Decide, once the system is live, whether this boot is a fresh start or a crash resume.
		GetGame().GetCallqueue().Call(PollForResume, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Report whether the engine has instantiated one of our PersistentState classes.
	//! A null result means the state is not in the persistence config, so its serializer will never
	//! run - which is invisible without this check, because saves still succeed.
	protected void ReportScriptedStateRegistration(string stateName, typename stateType)
	{
		if (!m_PersistenceSystem)
			return;

		Managed state = m_PersistenceSystem.GetPersistentState(stateType);
		if (!state)
		{
			// NOT conclusive. PersistenceSystem.StartTracking() documents lazy registration
			// ("Allow lazy registration at a later point to reduce performance impact"), so a state
			// that is configured correctly can still be absent at boot and only be created when it
			// is first needed - i.e. during the first save.
			// The only conclusive test is whether the serializer's Serialize() logs during a save.
			Print(string.Format("[CRF_Persistence]   state %1 : not instantiated yet (may register lazily - not necessarily an error)", stateName), LogLevel.WARNING);
			return;
		}

		// Being in the config list is necessary but NOT sufficient. Each entry carries its own
		// PersistenceConfig, and two fields on it decide whether the serializer actually runs:
		//   m_Collection - which collection the record goes in. SetConfig() is documented to reject
		//                  a config with no collection assigned, so an unassigned one is broken.
		//   m_eSaveMask  - which save TYPES this entry activates for. If the mask excludes the type
		//                  of the save being taken, this state is skipped for that save and the
		//                  serializer is never called - silently, and only for this entry.
		PersistenceConfig config = m_PersistenceSystem.GetConfig(state);
		if (!config)
		{
			Print(string.Format("[CRF_Persistence]   state %1 : registered, but has NO CONFIG - it will not be saved.", stateName), LogLevel.ERROR);
			return;
		}

		bool hasCollection = (config.m_Collection != null);

		Print(string.Format("[CRF_Persistence]   state %1 : registered | collection %2 | saveMask %3",
			stateName,
			hasCollection,
			config.m_eSaveMask), LogLevel.NORMAL);

		if (!hasCollection)
		{
			Print(string.Format("[CRF_Persistence]   %1 has NO COLLECTION assigned - assign one in the persistence config or it will never be written.", stateName), LogLevel.ERROR);
		}

		// Compare the entry's mask against the save types this component will actually request.
		SaveGameManager saveManager = SaveGameManager.Get();
		if (saveManager && config.m_eSaveMask != 0)
		{
			ESaveGameType enabledTypes = saveManager.GetEnabledSaveTypes();
			if (!(config.m_eSaveMask & enabledTypes))
			{
				Print(string.Format("[CRF_Persistence]   %1 saveMask (%2) does not overlap the mission's enabled save types (%3) - this state will be skipped on every save.",
					stateName, config.m_eSaveMask, enabledTypes), LogLevel.ERROR);
			}
		}
		else if (config.m_eSaveMask == 0)
		{
			Print(string.Format("[CRF_Persistence]   %1 saveMask is 0 - it will be skipped on every save. Set it to include the save types you use.", stateName), LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! One block at startup naming every precondition and whether it is met.
	//!
	//! Persistence depends on several things that live outside this component - a world-level systems
	//! config, a mission header flag, an engine subsystem - and any one of them being wrong produces
	//! the same symptom: nothing happens. Rather than make that a guessing game, state the whole
	//! chain up front so the console says which link is broken.
	protected void LogStartupDiagnostics()
	{
		Print("[CRF_Persistence] ---------- persistence status ----------", LogLevel.NORMAL);

		Print(string.Format("[CRF_Persistence]   replication mode      : %1", typename.EnumToString(RplMode, RplSession.Mode())), LogLevel.NORMAL);

		// The scenario this session belongs to. Everything in the persistence system is scoped to a
		// mission resource - saves are filed under it, and playthroughs are counted per mission.
		// Empty means there is no scenario, which is what running a WORLD directly in Workbench gives
		// you. That is not a misconfiguration to fix in a file; it means persistence has nothing to
		// attach to and cannot work in that mode at all.
		string missionResource = SaveGameManager.GetCurrentMissionResource();
		if (missionResource.IsEmpty())
		{
			Print("[CRF_Persistence]   mission resource      : *** EMPTY ***", LogLevel.ERROR);
			Print("[CRF_Persistence]   No scenario is active, so there is nothing for saves to belong to.", LogLevel.ERROR);
			Print("[CRF_Persistence]   This is normal when running a WORLD directly in Workbench.", LogLevel.ERROR);
			Print("[CRF_Persistence]   Fix: launch a SCENARIO (a .conf mission with a MissionHeader), not the bare world.", LogLevel.ERROR);
		}
		else
		{
			Print(string.Format("[CRF_Persistence]   mission resource      : '%1'", missionResource), LogLevel.NORMAL);
		}

		// The engine subsystem itself. Null means the world is not running SCR_PersistenceSystem at
		// all, which is a world/config problem rather than anything this component can fix.
		if (!m_PersistenceSystem)
		{
			Print("[CRF_Persistence]   SCR_PersistenceSystem : *** MISSING ***", LogLevel.ERROR);
			Print("[CRF_Persistence]   The world is not running the persistence system, so nothing will ever save.", LogLevel.ERROR);
			Print("[CRF_Persistence]   Fix: the world must reference a SystemSettings config with SCR_PersistenceSystem Enabled 1.", LogLevel.ERROR);
			Print("[CRF_Persistence]   Note: CRF ships Configs/Systems/MissionSystems.conf for this, but assigning it to a world is Workbench work - editing that file alone has no effect if no world points at it.", LogLevel.ERROR);
		}
		else
		{
			Print(string.Format("[CRF_Persistence]   SCR_PersistenceSystem : present (state %1)", typename.EnumToString(EPersistenceSystemState, m_PersistenceSystem.GetState())), LogLevel.NORMAL);
		}

		// Save types come from the mission header. All-off disables persistence engine-side no matter
		// what the systems config says.
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
		{
			Print("[CRF_Persistence]   SaveGameManager       : *** MISSING ***", LogLevel.ERROR);
		}
		else
		{
			bool savingEnabled = saveManager.IsSavingEnabled();
			ESaveGameType enabledTypes = saveManager.GetEnabledSaveTypes();
			int playthrough = saveManager.GetCurrentPlaythroughNumber();

			Print(string.Format("[CRF_Persistence]   saving enabled        : %1", savingEnabled), LogLevel.NORMAL);
			Print(string.Format("[CRF_Persistence]   enabled save types    : %1", enabledTypes), LogLevel.NORMAL);
			Print(string.Format("[CRF_Persistence]   playthrough           : %1", playthrough), LogLevel.NORMAL);

			// Distinguish the two very different reasons saving can be off. Blaming m_eSaveTypes when
			// it is already set sends you editing a header that was never the problem.
			if (!savingEnabled)
			{
				if (enabledTypes == 0)
					Print("[CRF_Persistence]   Saving is off because NO save types are enabled. Fix: set m_eSaveTypes on the mission header.", LogLevel.ERROR);
				else
					Print(string.Format("[CRF_Persistence]   Saving is off even though save types are set (%1). The scenario itself does not support saving - almost always because no scenario is active (see mission resource above).", enabledTypes), LogLevel.ERROR);
			}

			// -1 means no playthrough has been started, which again points at there being no scenario.
			if (playthrough < 0)
				Print("[CRF_Persistence]   No playthrough is active (-1) - consistent with no scenario being loaded.", LogLevel.WARNING);
		}

		Print(string.Format("[CRF_Persistence]   autosave interval     : %1s", m_fAutoSaveInterval), LogLevel.NORMAL);
		if (m_fAutoSaveInterval <= 0)
			Print("[CRF_Persistence]   Periodic autosave is OFF (interval 0). Only event-driven saves will run.", LogLevel.WARNING);

		Print(string.Format("[CRF_Persistence]   event-driven saves    : %1 (cooldown %2s)", m_bEventDrivenSaves, m_fEventSaveCooldown), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   resume on boot        : %1 (only after crash: %2)", m_bResumeOnBoot, m_bResumeOnlyAfterCrash), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   crash marker present  : %1", HasSessionMarker()), LogLevel.NORMAL);

		// NOTE: the scripted-state registration check is NOT done here. This block runs during
		// OnWorldPostProcess while the persistence system is still in INIT, and states are not
		// instantiated until it comes up - so a check here reports NOT REGISTERED for everything,
		// including states that are correctly configured. It runs from PollForResume() instead,
		// once the system reports ACTIVE.

		// The gate that most often explains "it never saves": saving only runs during GAME.
		Print("[CRF_Persistence]   NOTE: saves only run while the gamemode is in the GAME phase.", LogLevel.NORMAL);
		Print("[CRF_Persistence]         In BRIEFING/SLOTTING/AAR nothing is saved by design.", LogLevel.NORMAL);
		Print("[CRF_Persistence] ----------------------------------------", LogLevel.NORMAL);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 BOOT: RESUME OR START FRESH
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Wait for the persistence system to finish coming up, then branch.
	//! Polls on a condition instead of guessing a delay - world load time varies wildly by terrain.
	protected void PollForResume(int attempt)
	{
		if (!m_PersistenceSystem)
			return;

		if (m_PersistenceSystem.GetState() < EPersistenceSystemState.ACTIVE)
		{
			if (attempt >= RESUME_MAX_POLLS)
			{
				Print("[CRF_Persistence] Persistence system never reached ACTIVE. Continuing without persistence.", LogLevel.ERROR);
				return;
			}

			GetGame().GetCallqueue().CallLater(PollForResume, RESUME_POLL_INTERVAL_S * 1000, false, attempt + 1);
			return;
		}

		// Checked before anything else. This used to live inside OnSavesObtained(), which is only
		// reached when the world came up WITHOUT save data - so a server launched with a save already
		// active resumed it and never consulted the flag at all.
		if (m_bForceFreshStart)
		{
			StartFreshPlaythrough();
			return;
		}

		// System is up, so persistent states now exist if they are going to. Checking earlier (during
		// OnWorldPostProcess, at state INIT) reported false negatives for everything.
		ReportScriptedStateRegistration("COA_GamemodeStateData", COA_GamemodeStateData);
		ReportScriptedStateRegistration("COA_SlottingManagerStateData", COA_SlottingManagerStateData);

		// The engine already loaded save data into this world - this boot IS the resume.
		if (m_PersistenceSystem.WasDataLoaded())
		{
			m_bResumedFromSave = true;
			OnPersistenceLoaded();
			return;
		}

		// Fresh world. Is there a save for this mission we should be resuming instead?
		if (m_bResumeOnBoot)
			LookForExistingSave();
	}

	//------------------------------------------------------------------------------------------------
	//! Begin a new playthrough of this mission, abandoning any existing save without deleting it.
	//!
	//! Uses SaveGameManager.StartPlaythrough() rather than purging. The engine tracks a playthrough
	//! number per mission and stamps it onto every save point; starting a new playthrough bumps that
	//! number, and OnSavesObtained() only ever resumes saves from the CURRENT playthrough. So the old
	//! run's saves become invisible to the resume logic while still existing on disk for review.
	//!
	//! That also makes this loop-proof. Purging and reloading would leave m_bForceFreshStart still
	//! set on the next boot, so the server would force-fresh again forever; with playthroughs, the
	//! second boot simply finds no save for the current playthrough and starts normally.
	protected void StartFreshPlaythrough()
	{
		ClearSessionMarker();

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return;

		string missionResource = SaveGameManager.GetCurrentMissionResource();
		if (missionResource.IsEmpty())
			return;

		Print(string.Format("[CRF_Persistence] Fresh playthrough requested - abandoning playthrough %1 of this mission. Previous saves are kept but will not be resumed.",
			saveManager.GetCurrentPlaythroughNumber()), LogLevel.NORMAL);

		saveManager.StartPlaythrough(missionResource);
	}

	//------------------------------------------------------------------------------------------------
	//! Ask SaveGameManager whether this mission has any saves on disk.
	//! Filtered by mission resource, so a save from a different mission is never picked up - this is
	//! the "check if that mission data exists server side" step.
	protected void LookForExistingSave()
	{
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return;

		if (!saveManager.IsSavingEnabled())
		{
			Print("[CRF_Persistence] Saving is disabled for this scenario - no crash resume will be possible. Set save types in the mission header.", LogLevel.WARNING);
			return;
		}

		string missionResource = SaveGameManager.GetCurrentMissionResource();
		if (missionResource.IsEmpty())
			return;

		SaveGameObtainCallback callback = new SaveGameObtainCallback(OnSavesObtained);
		saveManager.GetSaves(missionResource, callback);
	}

	//------------------------------------------------------------------------------------------------
	//! Result handler for the save lookup. Decides between resuming and replaying, then acts.
	protected void OnSavesObtained(bool success, array<SaveGame> saves, Managed context = null)
	{
		// Separated so a failed lookup is not reported as "no saves exist" - they mean very different
		// things and only one of them is normal.
		if (!success)
		{
			Print(string.Format("[CRF_Persistence] Save lookup FAILED for mission '%1'. Cannot tell whether a save exists; starting fresh.", SaveGameManager.GetCurrentMissionResource()), LogLevel.ERROR);
			return;
		}

		if (!saves || saves.IsEmpty())
		{
			Print(string.Format("[CRF_Persistence] No saves on disk for mission '%1' - starting fresh.", SaveGameManager.GetCurrentMissionResource()), LogLevel.NORMAL);
			Print("[CRF_Persistence] If a previous session reported 'Save complete', the save is not reaching disk or is filed under a different mission resource.", LogLevel.NORMAL);
			ClearSessionMarker();
			return;
		}

		// A save exists, but the previous session ended on purpose - the mission was completed,
		// stopped, or is simply being replayed. Resuming here is what would make replaying a mission
		// impossible, so begin a new playthrough instead.
		if (m_bResumeOnlyAfterCrash && !HasSessionMarker())
		{
			Print("[CRF_Persistence] A save exists but the previous session ended cleanly - treating this as a replay and starting a new playthrough.", LogLevel.NORMAL);
			StartFreshPlaythrough();
			return;
		}

		SaveGameManager manager = SaveGameManager.Get();
		if (!manager)
			return;

		string currentMission = SaveGameManager.GetCurrentMissionResource();
		int currentPlaythrough = manager.GetCurrentPlaythroughNumber();

		// Log every candidate. When a resume does not happen, the useful question is always "what did
		// the engine actually return, and why was none of it acceptable" - which was previously
		// invisible, leaving only a flat "no save available".
		Print(string.Format("[CRF_Persistence] %1 save(s) returned for this mission. Current playthrough is %2.", saves.Count(), currentPlaythrough), LogLevel.NORMAL);

		SaveGame newest = null;
		int newestUnix = -1;

		foreach (SaveGame save : saves)
		{
			if (!save)
				continue;

			Print(string.Format("[CRF_Persistence]   candidate: savePoint %1 '%2' | playthrough %3 | type %4 | created %5 | mission '%6'",
				save.GetSavePointNumber(),
				save.GetSavePointName(),
				save.GetPlaythroughNumber(),
				typename.EnumToString(ESaveGameType, save.GetType()),
				save.GetSavePointCreatedUnix(),
				save.GetMissionResource()), LogLevel.NORMAL);

			// Defensive: GetSaves() is already filtered by mission, but resuming the wrong mission
			// would be a spectacular failure so it is worth re-checking.
			if (save.GetMissionResource() != currentMission)
			{
				Print("[CRF_Persistence]     rejected: different mission.", LogLevel.NORMAL);
				continue;
			}

			// NOTE: deliberately NOT filtered by playthrough.
			// An earlier version required save.GetPlaythroughNumber() == currentPlaythrough, which
			// broke the entire point of this feature: relaunching a scenario after a crash starts a
			// NEW playthrough, so the crashed session's saves were always from an older one and were
			// silently discarded. Every boot then reported "no save available" despite a good save
			// sitting on disk.
			// Replay protection does not need this filter - it is already handled above by the
			// session marker, which is the thing that actually distinguishes a crash from a
			// deliberate stop.
			int created = save.GetSavePointCreatedUnix();
			if (created > newestUnix)
			{
				newestUnix = created;
				newest = save;
			}
		}

		if (!newest)
		{
			Print("[CRF_Persistence] None of the returned saves belong to this mission - starting fresh.", LogLevel.NORMAL);
			return;
		}

		Print(string.Format("[CRF_Persistence] Resuming from save point %1 ('%2', playthrough %3). The world will reload.",
			newest.GetSavePointNumber(), newest.GetSavePointName(), newest.GetPlaythroughNumber()), LogLevel.NORMAL);

		SaveGameManager saveManager = SaveGameManager.Get();
		if (saveManager)
			saveManager.Load(newest, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Called once the world has come up WITH save data applied. Restores the gamemode phase.
	//!
	//! The gamemode phase itself is restored by COA_GamemodeStateSerializer as part of the save data;
	//! this exists to drive the side effects of being in that phase (safestart off, HUD, spawning
	//! enabled) which normally only run through AdvanceGamemodeState().
	protected void OnPersistenceLoaded()
	{
		if (m_bResumeApplied)
			return;

		m_bResumeApplied = true;

		if (!m_Gamemode)
			m_Gamemode = COA_Gamemode.GetInstance();

		if (!m_Gamemode)
		{
			Print("[CRF_Persistence] Resumed from save but COA_Gamemode is unavailable - phase not restored.", LogLevel.ERROR);
			return;
		}

		Print(string.Format("[CRF_Persistence] Resumed from save. Restoring gamemode phase %1.",
			typename.EnumToString(COA_EGamemodeState, m_Gamemode.m_GamemodeState)), LogLevel.NORMAL);

		// Re-run the phase side effects for the restored phase without advancing it.
		m_Gamemode.ReapplyGamemodeState();
	}

	//------------------------------------------------------------------------------------------------
	//! True when this session came back from a save rather than starting fresh. Other systems can use
	//! this to skip intro/briefing behaviour that should not replay on a crash resume.
	bool WasResumedFromSave()
	{
		return m_bResumedFromSave;
	}

	//------------------------------------------------------------------------------------------------
	//! Admin entry point: abandon the current playthrough so the NEXT boot of this mission starts
	//! from the beginning. Does not interrupt the session in progress, and does not delete anything.
	//!
	//! This is the in-session equivalent of the m_bForceFreshStart attribute, for the case where you
	//! decide mid-event that the mission should be replayed rather than resumed.
	void AdminStartFreshPlaythrough()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		Print("[CRF_Persistence] New playthrough started by admin request - the next boot will not resume this run.", LogLevel.NORMAL);
		StartFreshPlaythrough();
	}

	//------------------------------------------------------------------------------------------------
	//! Permanently delete every save for this mission, across all playthroughs.
	//! Only for genuinely clearing history - AdminStartFreshPlaythrough() is the safer option for
	//! "I want to play this again", because it keeps the old run's data.
	void AdminPurgeAllSaves()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		ClearSessionMarker();

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return;

		string missionResource = SaveGameManager.GetCurrentMissionResource();
		if (missionResource.IsEmpty())
			return;

		Print("[CRF_Persistence] Purging ALL saves for this mission by admin request.", LogLevel.WARNING);
		saveManager.Purge(missionResource);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SESSION MARKER (crash vs clean shutdown)
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Written when the mission reaches GAME. Its presence at boot means the previous session did not
	//! get to shut down, i.e. it crashed.
	protected void WriteSessionMarker()
	{
		if (FileIO.FileExists(SESSION_MARKER_FILE))
			return;

		FileIO.MakeDirectory(SESSION_MARKER_DIR);

		FileHandle handle = FileIO.OpenFile(SESSION_MARKER_FILE, FileMode.WRITE);
		if (!handle)
		{
			Print("[CRF_Persistence] Could not write the session marker - crash detection will not work, and any save will be treated as a clean shutdown.", LogLevel.WARNING);
			return;
		}

		handle.WriteLine(SaveGameManager.GetCurrentMissionResource());
		handle.Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearSessionMarker()
	{
		if (FileIO.FileExists(SESSION_MARKER_FILE))
			FileIO.DeleteFile(SESSION_MARKER_FILE);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasSessionMarker()
	{
		return FileIO.FileExists(SESSION_MARKER_FILE);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SAVING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Periodic autosave while the mission is actually running.
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_fTimeSinceEventSave += timeSlice;
		m_fHeartbeatTick += timeSlice;

		string blockReason = GetSaveBlockReason();

		if (blockReason != string.Empty)
		{
			// Say so periodically rather than sitting silent. Without this, "no save logs" could mean
			// the wrong phase, a missing subsystem, or a mission header flag, with no way to tell
			// them apart from the console.
			if (m_fHeartbeatTick >= HEARTBEAT_INTERVAL_S)
			{
				m_fHeartbeatTick = 0;
				Print(string.Format("[CRF_Persistence] Not saving: %1.", blockReason), LogLevel.NORMAL);
			}

			return;
		}

		if (m_fAutoSaveInterval <= 0)
			return;	// periodic saving off by config; event-driven saves still work

		m_fAutoSaveTick += timeSlice;

		if (m_fHeartbeatTick >= HEARTBEAT_INTERVAL_S)
		{
			m_fHeartbeatTick = 0;
			Print(string.Format("[CRF_Persistence] Armed - next autosave in %1s.", Math.Round(m_fAutoSaveInterval - m_fAutoSaveTick)), LogLevel.NORMAL);
		}

		if (m_fAutoSaveTick < m_fAutoSaveInterval)
			return;

		m_fAutoSaveTick = 0;
		RequestSave(ESaveGameType.AUTO, "Autosave");
	}

	//------------------------------------------------------------------------------------------------
	//! Event-driven save. Call from anywhere a meaningful bit of mission state changed.
	//! Rate-limited by m_fEventSaveCooldown so a burst (e.g. mass slotting) commits once, not 40 times.
	//! \param[in] reason short label shown in the save point name and the log
	void RequestEventSave(string reason)
	{
		if (!m_bEventDrivenSaves)
			return;

		if (m_fTimeSinceEventSave < m_fEventSaveCooldown)
			return;

		m_fTimeSinceEventSave = 0;
		RequestSave(ESaveGameType.SCRIPTED, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Force a save regardless of cooldown. For genuinely important moments (phase transitions).
	void RequestImmediateSave(string reason)
	{
		m_fTimeSinceEventSave = 0;
		RequestSave(ESaveGameType.SCRIPTED, reason);
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestSave(ESaveGameType saveType, string reason)
	{
		// Every rejection below says why. These were silent, which meant a save that never happened
		// looked exactly like a save that happened and never reported back.
		string blockReason = GetSaveBlockReason();
		if (blockReason != string.Empty)
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: %2.", reason, blockReason), LogLevel.WARNING);
			return;
		}

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: SaveGameManager unavailable.", reason), LogLevel.ERROR);
			return;
		}

		if (m_bSaveInFlight)
		{
			// Guard against the in-flight flag latching forever. It is cleared by the completion
			// callback, so if a save is dropped without ever reporting back, every later save would
			// be skipped silently and the mission would quietly stop being resumable.
			int inFlightMs = System.GetTickCount() - m_iPendingSaveStartTick;
			if (inFlightMs < SAVE_IN_FLIGHT_TIMEOUT_MS)
			{
				Print(string.Format("[CRF_Persistence] Save '%1' skipped: previous save ('%2') still committing after %3 ms.", reason, m_sPendingSaveReason, inFlightMs), LogLevel.WARNING);
				return;
			}

			Print(string.Format("[CRF_Persistence] Previous save ('%1') never reported completion after %2 ms - clearing the in-flight lock and continuing. Saves may not be committing.", m_sPendingSaveReason, inFlightMs), LogLevel.ERROR);
			m_bSaveInFlight = false;
		}

		if (saveManager.IsBusy())
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: SaveGameManager is busy.", reason), LogLevel.WARNING);
			return;
		}

		// The mission header decides which save TYPES are permitted, as a bitmask. Requesting a type
		// that is not enabled is rejected instantly by the engine - which looks identical to a real
		// save failure, and is what made every AUTO autosave fail while the SCRIPTED mission-start
		// save succeeded on a header set to MANUAL|SCRIPTED.
		saveType = ResolveUsableSaveType(saveType);
		if (saveType == 0)
		{
			if (!m_bNoUsableSaveTypeReported)
			{
				m_bNoUsableSaveTypeReported = true;
				Print(string.Format("[CRF_Persistence] Cannot save: the mission header enables no usable save types (m_eSaveTypes = %1). Fix: enable at least AUTO or SCRIPTED on the mission header. Suppressing further reports.",
					saveManager.GetEnabledSaveTypes()), LogLevel.ERROR);
			}

			return;
		}

		m_bSaveInFlight = true;
		m_sPendingSaveReason = reason;
		m_ePendingSaveType = saveType;

		// Real ticks, not world time: world time is scaled by ApplyMissionTimeScale(), so a mission
		// running at anything other than 1x would report a misleading save duration.
		m_iPendingSaveStartTick = System.GetTickCount();

		Print(string.Format("[CRF_Persistence] Save starting | reason '%1' | type %2",
			reason, typename.EnumToString(ESaveGameType, saveType)), LogLevel.NORMAL);

		// Completion is reported through this per-request callback rather than only through
		// SCR_PersistenceSystem's OnAfterSave invoker. That invoker only exists if the world happens
		// to register the SCRIPTED persistence class; this callback is handed directly to the request
		// and fires either way, so the completion log and the in-flight unlock no longer depend on it.
		SaveGameOperationCallback callback = new SaveGameOperationCallback(OnSaveOperationCompleted);
		saveManager.RequestSavePoint(saveType, reason, 0, callback);
	}

	//------------------------------------------------------------------------------------------------
	//! Pick a save type the mission header actually permits.
	//!
	//! ESaveGameType is a BITMASK on the mission header (MANUAL=1, AUTO=2, SCRIPTED=4, SHUTDOWN=8;
	//! see SCR_PauseMenuUI's `GetEnabledSaveTypes() & ESaveGameType.SHUTDOWN`). A header set to 5 is
	//! MANUAL|SCRIPTED, so AUTO requests are refused - instantly, with no reason given.
	//!
	//! Rather than fail every autosave, fall back to another permitted type: what matters for crash
	//! recovery is that a save point exists, not which label it carries. The substitution is reported
	//! once so it is a visible choice rather than silent magic.
	//! \return a permitted save type, or 0 if the header permits nothing usable
	protected ESaveGameType ResolveUsableSaveType(ESaveGameType desired)
	{
		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return desired;

		ESaveGameType enabled = saveManager.GetEnabledSaveTypes();

		if (enabled & desired)
			return desired;

		// Preference order for a stand-in. SHUTDOWN is deliberately excluded - it carries "the
		// application is closing" meaning and is not appropriate for a periodic save point.
		if (enabled & ESaveGameType.SCRIPTED)
			return ReportSaveTypeFallback(desired, ESaveGameType.SCRIPTED, enabled);

		if (enabled & ESaveGameType.AUTO)
			return ReportSaveTypeFallback(desired, ESaveGameType.AUTO, enabled);

		if (enabled & ESaveGameType.MANUAL)
			return ReportSaveTypeFallback(desired, ESaveGameType.MANUAL, enabled);

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Announce a save-type substitution once per desired type, then stay quiet about it.
	protected ESaveGameType ReportSaveTypeFallback(ESaveGameType desired, ESaveGameType substitute, ESaveGameType enabled)
	{
		if (!m_aReportedSaveTypeFallbacks.Contains(desired))
		{
			m_aReportedSaveTypeFallbacks.Insert(desired);

			Print(string.Format("[CRF_Persistence] Save type %1 is not enabled on the mission header (m_eSaveTypes = %2) - using %3 instead. Enable %1 on the header to silence this.",
				typename.EnumToString(ESaveGameType, desired),
				enabled,
				typename.EnumToString(ESaveGameType, substitute)), LogLevel.WARNING);
		}

		return substitute;
	}

	//------------------------------------------------------------------------------------------------
	//! Per-request completion handler. Routes into the same reporting as the invoker path, with a
	//! guard so a save reported by both routes is only logged once.
	protected void OnSaveOperationCompleted(bool success, Managed context = null)
	{
		if (!m_bSaveInFlight)
			return;	// already reported via OnAfterSave

		ReportSaveResult(m_ePendingSaveType, success, "request callback");
	}

	//------------------------------------------------------------------------------------------------
	//! Gate on mission phase. Saving during BRIEFING/SLOTTING would let a crash resume into a
	//! half-slotted lobby, which is worse than just restarting; and saving during AAR would resurrect
	//! a finished mission on the next boot.
	protected bool ShouldSaveNow()
	{
		return GetSaveBlockReason() == string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Why saving is currently not possible, or empty string if it is.
	//! Split out from ShouldSaveNow() so the reason can be reported instead of silently swallowed -
	//! "no logs at all" was previously the only symptom of every one of these conditions.
	protected string GetSaveBlockReason()
	{
		if (!m_bPersistenceEnabled)
			return "persistence disabled on the component";

		if (!m_PersistenceSystem)
			return "PersistenceSystem missing from the world";

		if (RplSession.Mode() == RplMode.Client)
			return "not authority";

		if (!m_Gamemode)
			m_Gamemode = COA_Gamemode.GetInstance();

		if (!m_Gamemode)
			return "COA_Gamemode unavailable";

		if (m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return string.Format("gamemode is in %1, saves only run in GAME", typename.EnumToString(COA_EGamemodeState, m_Gamemode.m_GamemodeState));

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return "SaveGameManager unavailable";

		if (!saveManager.IsSavingEnabled())
			return "saving disabled for this scenario (mission header m_eSaveTypes is 0)";

		if (!saveManager.IsSavingAllowed())
			return "saving temporarily disallowed";

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Reports every save to the server console.
	//!
	//! Deliberately LogLevel.NORMAL rather than VERBOSE: the point of this line is that an admin
	//! watching the console can tell at a glance that crash recovery is actually working. A VERBOSE
	//! line does not appear in a default server log, so the previous version was effectively silent
	//! on success and only spoke up on failure - which reads identically to persistence being off.
	//!
	//! OnAfterSave() is given only the save type, so the reason and start tick are carried over from
	//! RequestSave() via m_sPendingSaveReason / m_iPendingSaveStartTick.
	protected void OnAfterSave(ESaveGameType saveType, bool success)
	{
		if (!m_bSaveInFlight)
			return;	// already reported via the per-request callback

		ReportSaveResult(saveType, success, "system invoker");
	}

	//------------------------------------------------------------------------------------------------
	//! Single place that reports a finished save and releases the in-flight lock.
	//!
	//! Two independent routes can land here - the per-request SaveGameOperationCallback and
	//! SCR_PersistenceSystem's OnAfterSave invoker - because neither is guaranteed on its own: the
	//! invoker only exists when the world registered the scripted persistence class. Whichever
	//! arrives first reports; the m_bSaveInFlight check in both entry points makes the second a
	//! no-op, so a save is never double-counted.
	//! \param[in] source which route reported this, for diagnosing a missing signal
	protected void ReportSaveResult(ESaveGameType saveType, bool success, string source)
	{
		m_bSaveInFlight = false;

		int durationMs = 0;
		if (m_iPendingSaveStartTick != 0)
			durationMs = System.GetTickCount() - m_iPendingSaveStartTick;

		string reason = m_sPendingSaveReason;
		if (reason.IsEmpty())
			reason = "unspecified";

		m_sPendingSaveReason = string.Empty;
		m_iPendingSaveStartTick = 0;

		if (!success)
		{
			m_iSaveFailureCount++;

			SaveGameManager failManager = SaveGameManager.Get();
			ESaveGameType enabledTypes = 0;
			bool savingEnabled = false;
			if (failManager)
			{
				enabledTypes = failManager.GetEnabledSaveTypes();
				savingEnabled = failManager.IsSavingEnabled();
			}

			// The live state is included because an instant failure is almost always a rejection
			// rather than a genuine write error, and these three values identify which rejection.
			Print(string.Format("[CRF_Persistence] SAVE FAILED | reason '%1' | type %2 | after %3 ms | %4 failed / %5 ok this session | via %6",
				reason,
				typename.EnumToString(ESaveGameType, saveType),
				durationMs,
				m_iSaveFailureCount,
				m_iSaveSuccessCount,
				source), LogLevel.ERROR);

			Print(string.Format("[CRF_Persistence]   state at failure: savingEnabled=%1 enabledSaveTypes=%2 mission='%3'",
				savingEnabled,
				enabledTypes,
				SaveGameManager.GetCurrentMissionResource()), LogLevel.ERROR);

			return;
		}

		m_iSaveSuccessCount++;

		SaveGameManager saveManager = SaveGameManager.Get();
		int playthrough = -1;
		int savePoint = -1;
		if (saveManager)
		{
			playthrough = saveManager.GetCurrentPlaythroughNumber();

			SaveGame activeSave = saveManager.GetActiveSave();
			if (activeSave)
				savePoint = activeSave.GetSavePointNumber();
		}

		Print(string.Format("[CRF_Persistence] Save complete | reason '%1' | type %2 | %3 ms | playthrough %4, save point %5 | %6 saved this session | via %7",
			reason,
			typename.EnumToString(ESaveGameType, saveType),
			durationMs,
			playthrough,
			savePoint,
			m_iSaveSuccessCount,
			source), LogLevel.NORMAL);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLEAN END
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called by COA_Gamemode when the mission enters GAME. Opens the session: from here on, an
	//! absent marker at next boot means we got here and never left cleanly.
	void OnMissionStarted()
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		WriteSessionMarker();
	}

	//------------------------------------------------------------------------------------------------
	//! Called by COA_Gamemode when the mission reaches AAR under its own power.
	//! Without this, a mission that finished normally would be "resumed" on the next server boot.
	void OnMissionCompleted()
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		// Close the session first. Even if the purge below is disabled or fails, the missing marker
		// is enough to stop the next boot resuming a mission that already finished.
		ClearSessionMarker();

		if (!m_bPurgeOnCleanEnd)
			return;

		SaveGameManager saveManager = SaveGameManager.Get();
		if (!saveManager)
			return;

		// Stop any queued autosave from re-creating a save point after the purge.
		saveManager.SetSavingAllowed(false);

		string missionResource = SaveGameManager.GetCurrentMissionResource();
		if (missionResource.IsEmpty())
			return;

		Print("[CRF_Persistence] Mission completed cleanly - purging its saves so the next boot starts fresh.", LogLevel.NORMAL);
		saveManager.Purge(missionResource);
	}

	//------------------------------------------------------------------------------------------------
	//! Graceful server shutdown or mission end. Distinct from OnMissionCompleted(): the mission may
	//! not have finished, but it IS ending on purpose, so the next boot should not treat it as a
	//! crash. The save is deliberately left on disk so an admin can still resume it by hand.
	void OnGracefulShutdown()
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		Print("[CRF_Persistence] Graceful shutdown - clearing the session marker. Any save is kept but will not auto-resume.", LogLevel.NORMAL);
		ClearSessionMarker();
	}
}
