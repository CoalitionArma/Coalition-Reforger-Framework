//------------------------------------------------------------------------------------------------
// CRF Persistence Manager
//
// Crash recovery for event servers. Reforger servers die often, and a crash otherwise resets the
// mission to briefing. This keeps a single database record per mission so a crashed session comes
// back in the same phase with the same players in the same slots.
//
//
// SCOPE - READ THIS BEFORE ASSUMING WHAT COMES BACK
//   Restored: mission phase, world time, the slot table (occupant, role, faction, group, respawns,
//   dead/locked), each living player's position and facing, and each living player's inventory
//   including nested contents, weapon attachments and magazine round counts.
//
//   NOT restored: anything that is not a player. Vehicles, their positions and damage, dropped
//   items, corpses, deployed objects and AI are all absent - EDF stores the fields declared in
//   CRF_MissionSaveEntity and nothing else. A resumed mission puts the same people back in the same
//   places with the same kit; it does not reproduce the battlefield around them.
//
//   Dead players are deliberately excluded from position and inventory capture. Their kit belongs
//   to their corpse, and respawning them onto the spot they were killed would drop them back onto
//   whatever killed them.
//
// DEPENDENCY
//   Requires the EnfusionDatabaseFramework addon to be loaded. If it is absent the scripts here will
//   not compile, so it must be a hard dependency of this addon.
//------------------------------------------------------------------------------------------------

// Derives from SCR_BaseGameModeComponent, not plain ScriptComponent: OnWorldPostProcess() is
// declared there (forwarded from BaseGameMode), and this lives on the game mode entity anyway.
class CRF_PersistenceManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_PersistenceManager : SCR_BaseGameModeComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================

	[Attribute("1", UIWidgets.CheckBox, "Master switch. When off this component does nothing - no saving, no crash resume.", category: "CRF Persistence")]
	protected bool m_bPersistenceEnabled;

	[Attribute("CRF_MissionPersistence", UIWidgets.EditBox, "Database name. With the JSON driver this becomes a folder under the server profile directory.", category: "CRF Persistence")]
	protected string m_sDatabaseName;

	[Attribute("1", UIWidgets.CheckBox, "On boot, look for a record for this mission and resume it.", category: "CRF Persistence")]
	protected bool m_bResumeOnBoot;

	[Attribute("1", UIWidgets.CheckBox, "Only resume when the previous session ended unexpectedly. With this off, ANY record is resumed, which makes replaying a mission impossible.", category: "CRF Persistence")]
	protected bool m_bResumeOnlyAfterCrash;

	[Attribute("0", UIWidgets.CheckBox, "Force this boot to ignore and clear any existing record. Use to replay a mission that crashed. Remember to turn it back off.", category: "CRF Persistence")]
	protected bool m_bForceFreshStart;

	[Attribute("60", UIWidgets.EditBox, "Periodic save interval in seconds while in GAME. 0 disables periodic saving; event-driven saves still run.", params: "0 1800 1", category: "CRF Persistence")]
	protected float m_fAutoSaveInterval;

	[Attribute("10", UIWidgets.EditBox, "Minimum seconds between event-driven saves, so a burst of slot changes cannot hammer the database.", params: "0 300 1", category: "CRF Persistence")]
	protected float m_fEventSaveCooldown;

	[Attribute("1", UIWidgets.CheckBox, "Clear the record when the mission finishes cleanly, so the next boot starts fresh.", category: "CRF Persistence")]
	protected bool m_bClearOnCleanEnd;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME
//=============================================================================================================================================================================================================================================================================================================================================================

	protected static CRF_PersistenceManager s_Instance;

	protected ref EDF_DbContext m_DbContext;

	//! The record for this mission. Held across the session so writes reuse the id EDF assigned on
	//! first insert, updating the row instead of inserting a duplicate every save.
	protected ref CRF_MissionSaveEntity m_SaveRecord;

	protected COA_Gamemode m_Gamemode;
	protected COA_SlottingManager m_SlottingManager;

	//! True when this session resumed a record rather than starting fresh.
	protected bool m_bResumedFromSave;

	//! Set once the load has been applied, so a second callback cannot re-apply it.
	protected bool m_bResumeApplied;

	protected float m_fAutoSaveTick;
	protected float m_fTimeSinceEventSave;
	protected float m_fHeartbeatTick;

	//! Guards against issuing another write while one is still committing. Safe to have again now
	//! that a completion callback exists to clear it - without one it would latch on the first save
	//! and silently block every later one.
	protected bool m_bSaveInFlight;

	protected int m_iSaveSuccessCount;
	protected int m_iSaveFailureCount;

	protected const float HEARTBEAT_INTERVAL_S = 30.0;

	//! How far from a saved position to search for clear ground when putting a player back.
	protected const float POSITION_RESTORE_SEARCH_RADIUS_M = 5.0;

	//! Bounds for waiting on the slot table before applying a loaded record.
	protected const int RESUME_RETRY_MS = 250;
	protected const int RESUME_MAX_ATTEMPTS = 80;	// ~20s, enough for a large mission to build slots

	//! Bounds for waiting on the gearscript before swapping in a saved loadout.
	protected const int INVENTORY_RESTORE_RETRY_MS = 250;
	protected const int INVENTORY_RESTORE_MAX_ATTEMPTS = 40;	// ~10s

	//! Saved positions waiting for their player to reconnect after a resume, keyed on account GUID.
	//! Entries are consumed on use, so this empties as players return.
	protected ref map<string, ref CRF_PersistedSlot> m_mPendingPositions = new map<string, ref CRF_PersistedSlot>();

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
		// Cancel the pending resume retry. The call queue keeps running after this component is
		// destroyed, so a scheduled call that is not cancelled fires against freed memory.
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(WaitForSlotsThenApply);
			GetGame().GetCallqueue().Remove(WaitForGearThenRestoreInventory);
		}

		if (s_Instance == this)
			s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		if (!m_bPersistenceEnabled)
		{
			Print("[CRF_Persistence] DISABLED on the game mode component. No saving, no crash resume.", LogLevel.WARNING);
			return;
		}

		// Authority only - the database is server state. RplMode.None is Workbench play mode without
		// hosting, which IS authority, so only Client is excluded here.
		if (RplSession.Mode() == RplMode.Client)
			return;

		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SlottingManager = COA_SlottingManager.GetInstance();

		if (!ConnectToDatabase())
			return;

		LogStartupDiagnostics();
		LookForExistingRecord();
	}

	//------------------------------------------------------------------------------------------------
	//! Open the database connection.
	//!
	//! Uses the JSON file driver deliberately. It writes human-readable files under the server
	//! profile, so when a resume misbehaves the saved record can be opened and read directly instead
	//! of inferred from logs. Swap EDF_JsonFileDbConnectionInfo for EDF_BinaryFileDbConnectionInfo if
	//! record size ever matters more than being able to inspect it.
	protected bool ConnectToDatabase()
	{
		if (m_sDatabaseName.IsEmpty())
		{
			Print("[CRF_Persistence] No database name configured - persistence is off for this session.", LogLevel.ERROR);
			return false;
		}

		EDF_JsonFileDbConnectionInfo connectInfo();
		connectInfo.m_sDatabaseName = m_sDatabaseName;

		m_DbContext = EDF_DbContext.Create(connectInfo);
		if (!m_DbContext)
		{
			Print(string.Format("[CRF_Persistence] Failed to open database '%1'. Is the EnfusionDatabaseFramework addon loaded?", m_sDatabaseName), LogLevel.ERROR);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! One block at startup naming every precondition. Persistence has several dependencies outside
	//! this component and any one being wrong produces the same symptom - nothing happens - so state
	//! the whole chain rather than making it a guessing game.
	protected void LogStartupDiagnostics()
	{
		Print("[CRF_Persistence] ---------- persistence status (EDF) ----------", LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   replication mode : %1", typename.EnumToString(RplMode, RplSession.Mode())), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   database         : '%1' (JSON driver)", m_sDatabaseName), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   mission key      : '%1'", GetMissionKey()), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   resume on boot   : %1 (only after crash: %2)", m_bResumeOnBoot, m_bResumeOnlyAfterCrash), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   force fresh      : %1", m_bForceFreshStart), LogLevel.NORMAL);
		Print(string.Format("[CRF_Persistence]   autosave         : %1s | event cooldown %2s", m_fAutoSaveInterval, m_fEventSaveCooldown), LogLevel.NORMAL);
		Print("[CRF_Persistence]   NOTE: saves only run while the gamemode is in the GAME phase.", LogLevel.NORMAL);
		Print("[CRF_Persistence]   NOTE: world entities (vehicles, bodies, dropped items) are NOT persisted.", LogLevel.NORMAL);
		Print("[CRF_Persistence] ----------------------------------------------", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Key identifying this mission's record. Falls back to the world name if no mission resource is
	//! set, which happens when a world is launched directly rather than through a scenario.
	protected string GetMissionKey()
	{
		// Preferred: the scenario's own .conf resource. This is the mission's real identity - two
		// scenarios can share a world, and keying on the world would silently merge their records.
		MissionHeader header = GetGame().GetMissionHeader();
		if (header)
		{
			ResourceName headerResource = header.GetHeaderResourceName();
			if (!headerResource.IsEmpty())
				return headerResource;
		}

		// Fallback: what the save-game system considers the current mission. Verified to return a
		// non-empty value on this server, so it is a dependable second choice when a scenario is
		// running without a resolvable header.
		string saveGameMission = SaveGameManager.GetCurrentMissionResource();
		if (!saveGameMission.IsEmpty())
			return saveGameMission;

		// Last resort: the world path. Only reached when a world is launched directly rather than
		// through a scenario, in which case there is no mission identity to be had.
		if (header)
			return header.GetWorldPath();

		return string.Empty;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 BOOT: RESUME OR START FRESH
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Ask the database whether this mission already has a record.
	protected void LookForExistingRecord()
	{
		string missionKey = GetMissionKey();
		if (missionKey.IsEmpty())
		{
			Print("[CRF_Persistence] No mission key - cannot identify a record. Persistence is inactive this session.", LogLevel.ERROR);
			return;
		}

		EDF_DbFindCondition condition = EDF_DbFind.Field("m_sMissionResource").Equals(missionKey);
		EDF_DbFindCallbackSingle<CRF_MissionSaveEntity> callback(this, "OnRecordFound");

		EDF_DbRepository<CRF_MissionSaveEntity> repository = EDF_DbEntityHelper<CRF_MissionSaveEntity>.GetRepository(m_DbContext);
		repository.FindFirstAsync(condition, callback: callback);
	}

	//------------------------------------------------------------------------------------------------
	//! Result handler for the record lookup.
	//!
	//! Handler signature is (statusCode, result, context), dictated by EDF_DbFindCallbackSingle.Invoke().
	//!
	//! MUST BE PUBLIC. EDF dispatches these through GetGame().GetScriptModule().Call(), which cannot
	//! reach a protected or private method - it fails at runtime with
	//! "ScriptModule.Call: method 'OnRecordFound' is private/protected", not at compile time.
	void OnRecordFound(EDF_EDbOperationStatusCode statusCode, CRF_MissionSaveEntity result, Managed context = null)
	{
		if (statusCode != EDF_EDbOperationStatusCode.SUCCESS)
		{
			Print(string.Format("[CRF_Persistence] Record lookup failed (%1). Starting fresh; this session will still save.",
				typename.EnumToString(EDF_EDbOperationStatusCode, statusCode)), LogLevel.ERROR);

			StartFreshRecord();
			return;
		}

		if (!result)
		{
			Print("[CRF_Persistence] No existing record for this mission - starting fresh.", LogLevel.NORMAL);
			StartFreshRecord();
			return;
		}

		Print(string.Format("[CRF_Persistence] Found record | phase %1 | saved %2 | sessionActive %3 | %4 slot(s)",
			typename.EnumToString(COA_EGamemodeState, result.m_iGamemodeState),
			result.m_iSavedAtUnix,
			result.m_bSessionActive,
			result.m_aSlots.Count()), LogLevel.NORMAL);

		// Keep the instance either way, so later writes update this row rather than inserting a new
		// one. Even when we decline to resume, the record is reused and overwritten.
		m_SaveRecord = result;

		if (m_bForceFreshStart)
		{
			Print("[CRF_Persistence] m_bForceFreshStart is set - ignoring the record and starting fresh.", LogLevel.NORMAL);
			ResetRecordForFreshRun();
			return;
		}

		if (!m_bResumeOnBoot)
		{
			Print("[CRF_Persistence] Resume on boot is off - record kept but not applied.", LogLevel.NORMAL);
			ResetRecordForFreshRun();
			return;
		}

		// A record whose session was closed belongs to a mission that finished or was stopped on
		// purpose. Resuming it is what would make replaying a mission impossible.
		if (m_bResumeOnlyAfterCrash && !result.m_bSessionActive)
		{
			Print("[CRF_Persistence] Record exists but the previous session ended cleanly - treating this as a replay and starting fresh.", LogLevel.NORMAL);
			ResetRecordForFreshRun();
			return;
		}

		ApplyRecord(result);
	}

	//------------------------------------------------------------------------------------------------
	protected void StartFreshRecord()
	{
		m_SaveRecord = CRF_MissionSaveEntity.Create(GetMissionKey());
	}

	//------------------------------------------------------------------------------------------------
	//! Keep the row but wipe its contents, so a fresh run overwrites rather than leaving stale slots
	//! that a later save might partially merge with.
	protected void ResetRecordForFreshRun()
	{
		if (!m_SaveRecord)
		{
			StartFreshRecord();
			return;
		}

		m_SaveRecord.m_iGamemodeState = COA_EGamemodeState.BRIEFING;
		m_SaveRecord.m_bSessionActive = false;
		m_SaveRecord.m_aSlots = {};
	}

	//------------------------------------------------------------------------------------------------
	//! Apply a loaded record to the live mission.
	//! Begin applying a loaded record.
	//!
	//! Deliberately deferred. The database lookup completes during world post-process, which is
	//! BEFORE COA_SlottingManager has built its slot table - so applying immediately found an empty
	//! live map, matched none of the saved slots, and restored nothing. Wait for the table to exist.
	protected void ApplyRecord(notnull CRF_MissionSaveEntity record)
	{
		if (m_bResumeApplied)
			return;

		m_bResumeApplied = true;
		m_bResumedFromSave = true;

		WaitForSlotsThenApply(0);
	}

	//------------------------------------------------------------------------------------------------
	//! Poll until the slotting manager has built its table, then apply. Retries on a condition rather
	//! than guessing a delay, because how long the table takes to appear varies with mission size.
	protected void WaitForSlotsThenApply(int attempt)
	{
		int liveSlotCount = GetLiveSlotCount();

		if (liveSlotCount == 0 && attempt < RESUME_MAX_ATTEMPTS)
		{
			GetGame().GetCallqueue().CallLater(WaitForSlotsThenApply, RESUME_RETRY_MS, false, attempt + 1);
			return;
		}

		if (liveSlotCount == 0)
			Print("[CRF_Persistence] Slot table never appeared - restoring the phase only. Players will need to re-slot.", LogLevel.ERROR);

		DoApplyRecord();
	}

	//------------------------------------------------------------------------------------------------
	protected int GetLiveSlotCount()
	{
		if (!m_SlottingManager)
			m_SlottingManager = COA_SlottingManager.GetInstance();

		if (!m_SlottingManager)
			return 0;

		map<int, ref COA_SlotData> slotsMap = m_SlottingManager.GetSlotMap();
		if (!slotsMap)
			return 0;

		return slotsMap.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected void DoApplyRecord()
	{
		if (!m_SaveRecord)
			return;

		if (!m_Gamemode)
			m_Gamemode = COA_Gamemode.GetInstance();

		if (!m_Gamemode)
		{
			Print("[CRF_Persistence] Resumed but COA_Gamemode is unavailable - phase not restored.", LogLevel.ERROR);
			return;
		}

		int restored = RestoreSlots(m_SaveRecord);

		// Phase last: restoring it re-runs the phase side effects (including the mission-start save),
		// so the slot table must already be in place when that happens.
		m_Gamemode.m_GamemodeState = m_SaveRecord.m_iGamemodeState;
		Replication.BumpMe();
		m_Gamemode.ReapplyGamemodeState();

		Print(string.Format("[CRF_Persistence] RESUMED into phase %1 | %2 of %3 saved slot(s) restored | %4 pending position(s).",
			typename.EnumToString(COA_EGamemodeState, m_SaveRecord.m_iGamemodeState),
			restored,
			m_SaveRecord.m_aSlots.Count(),
			m_mPendingPositions.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Put the saved slot table back.
	//!
	//! Occupancy is handed to COA_Gamemode's existing GUID -> slot reconnect map rather than being
	//! applied directly, because on a crash resume the server is up before anyone has reconnected -
	//! there is no player id to assign yet. Feeding the reconnect path means a returning player lands
	//! in their old slot through code that already runs every session.
	protected int RestoreSlots(notnull CRF_MissionSaveEntity record)
	{
		if (!m_SlottingManager)
			m_SlottingManager = COA_SlottingManager.GetInstance();

		if (!m_SlottingManager || !record.m_aSlots)
			return 0;

		map<int, ref COA_SlotData> slotsMap = m_SlottingManager.GetSlotMap();
		if (!slotsMap)
			return 0;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		int restored = 0;

		foreach (CRF_PersistedSlot saved : record.m_aSlots)
		{
			if (!saved)
				continue;

			COA_SlotData slotData = slotsMap.Get(saved.m_iSlotId);
			if (!slotData)
				continue;	// mission changed since the save - skip rather than fabricate a slot

			slotData.SetIsLockedSlot(saved.m_bIsLocked);
			slotData.SetIsDeadSlot(saved.m_bIsDead);
			slotData.SetSlotRespawnsRemaining(saved.m_iRespawnsRemaining);
			slotData.SetSlotRole(saved.m_iRole);
			slotData.SetSlotFactionEnum(saved.m_iFactionEnum);
			slotData.SetRespawnPoolType(saved.m_iRespawnPoolType);

			// Map the stable group id back onto whatever RplId that group holds this session.
			if (groupsManager && saved.m_iGroupId >= 0)
			{
				SCR_AIGroup group = groupsManager.FindGroup(saved.m_iGroupId);
				if (group)
				{
					RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
					if (groupRpl)
						slotData.SetSlotCurrentGroup(groupRpl.Id());
				}
			}

			if (!saved.m_sOccupantGuid.IsEmpty() && m_Gamemode)
			{
				m_Gamemode.RestoreReconnectSlot(saved.m_sOccupantGuid, saved.m_iSlotId);

				// Positions cannot be applied here - on a crash resume the server is up before
				// anyone has reconnected, so there is no character to move yet. Park it against the
				// account GUID and apply it when that player is initialized.
				if (saved.m_bHasPosition)
					m_mPendingPositions.Set(saved.m_sOccupantGuid, saved);
			}

			restored++;
		}

		Print(string.Format("[CRF_Persistence] Restored %1 slot(s) from the record.", restored), LogLevel.NORMAL);
		return restored;
	}

	//------------------------------------------------------------------------------------------------
	bool WasResumedFromSave()
	{
		return m_bResumedFromSave;
	}

	//------------------------------------------------------------------------------------------------
	//! Put a resuming player back where they were when the server died.
	//!
	//! Called by CRF_COA_GamemodeManager once the player has been initialized and controls a
	//! character. One-shot: the entry is consumed whether or not the move succeeds, so a later death
	//! and respawn puts the player at a normal spawn point rather than teleporting them back to a
	//! position from before the crash.
	//! \param[in] playerId the player who just finished initializing
	void RestorePlayerPosition(int playerId)
	{
		if (!m_bResumedFromSave || playerId <= 0 || m_mPendingPositions.IsEmpty())
			return;

		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (guid.IsEmpty())
			return;

		CRF_PersistedSlot saved = m_mPendingPositions.Get(guid);
		if (!saved)
			return;

		// Consume up front so a failure below cannot leave the entry to fire on a later respawn.
		m_mPendingPositions.Remove(guid);

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		// Make sure the saved spot is still clear. The world has moved on since the crash - another
		// player may be standing there, or a vehicle may have been parked on it - and dropping a
		// character inside a solid is how you get a player stuck in geometry.
		vector target = saved.m_vPosition;
		vector clearPosition;
		if (SCR_WorldTools.FindEmptyTerrainPosition(clearPosition, target, POSITION_RESTORE_SEARCH_RADIUS_M))
			target = clearPosition;

		SCR_Global.TeleportPlayer(playerId, target, SCR_EPlayerTeleportedReason.DEFAULT);

		// Restore facing. Yaw only, with pitch and roll forced flat - a character has to stand
		// upright, and feeding back a stored pitch would leave a resumed player tilted.
		character.SetYawPitchRoll(Vector(saved.m_fYaw, 0, 0));

		Print(string.Format("[CRF_Persistence] Restored player %1 to their pre-crash position %2 (yaw %3).",
			playerId, target, saved.m_fYaw), LogLevel.NORMAL);

		// Inventory cannot go back yet. The gearscript runs from a deferred call queued in
		// COA_GearscriptCharacter.EOnInit and begins with ClearEntityGear(), so anything restored
		// before it lands is wiped. Wait for the character to report its gear applied.
		if (saved.m_bHasInventory)
			WaitForGearThenRestoreInventory(playerId, saved, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Wait until the role gearscript has been applied, then replace it with the saved loadout.
	//!
	//! Sequencing matters more than it looks. COA_GearscriptManager.SetEntityGear() starts by
	//! calling ClearEntityGear(), so restoring first and equipping second loses everything. Polling
	//! IsGearApplied() rather than guessing a delay keeps this correct on a loaded server where the
	//! gearscript pass can be slow.
	protected void WaitForGearThenRestoreInventory(int playerId, CRF_PersistedSlot saved, int attempt)
	{
		if (!saved)
			return;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;	// player left during initialization

		COA_GearscriptCharacter gearscriptCharacter = COA_GearscriptCharacter.Cast(character);
		if (gearscriptCharacter && !gearscriptCharacter.IsGearApplied())
		{
			if (attempt < INVENTORY_RESTORE_MAX_ATTEMPTS)
			{
				GetGame().GetCallqueue().CallLater(WaitForGearThenRestoreInventory, INVENTORY_RESTORE_RETRY_MS, false, playerId, saved, attempt + 1);
				return;
			}

			Print(string.Format("[CRF_Persistence] Gearscript never reported applied for player %1 - restoring inventory anyway; it may be overwritten.", playerId), LogLevel.WARNING);
		}

		RestoreInventory(playerId, character, saved);
	}

	//------------------------------------------------------------------------------------------------
	//! Swap the gearscript loadout for the one this player actually had.
	protected void RestoreInventory(int playerId, notnull IEntity character, notnull CRF_PersistedSlot saved)
	{
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return;

		// Clear what the gearscript just handed out. Without this the player ends up with both
		// loadouts and an overweight character.
		array<IEntity> existing = {};
		inventoryManager.GetAllRootItems(existing);
		foreach (IEntity item : existing)
		{
			if (item)
				SCR_EntityHelper.DeleteEntityAndChildren(item);
		}

		int restored = 0;
		foreach (CRF_PersistedItem persisted : saved.m_aInventory)
		{
			if (SpawnPersistedItem(persisted, inventoryManager, null, character))
				restored++;
		}

		Print(string.Format("[CRF_Persistence] Restored %1 of %2 saved top-level item(s) for player %3.",
			restored, saved.m_aInventory.Count(), playerId), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Recreate one persisted item and everything it contained.
	//!
	//! \param[in] parentStorage storage to insert into, or null to let the inventory manager choose
	//!            (used for top-level items, where the manager works out whether something is a
	//!            weapon, a uniform or cargo)
	//! \return true if the item was created
	protected bool SpawnPersistedItem(CRF_PersistedItem persisted, notnull SCR_InventoryStorageManagerComponent inventoryManager, BaseInventoryStorageComponent parentStorage, notnull IEntity character)
	{
		if (!persisted || persisted.m_rPrefab.IsEmpty())
			return false;

		// Spawn into the world first, then insert.
		//
		// TrySpawnPrefabToStorage() would be the obvious call, but it returns a bool - there is no
		// way to get at the entity it created, and this needs the reference to set the round count
		// and to place the item's contents inside it. Spawning explicitly is also the pattern CRF
		// already uses elsewhere for exactly this reason (see RpcAsk_ConvertItem).
		Resource resource = Resource.Load(persisted.m_rPrefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("[CRF_Persistence] Saved item prefab '%1' could not be loaded - skipping. Check the mod set matches the one the save was made on.", persisted.m_rPrefab), LogLevel.WARNING);
			return false;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = character.GetOrigin();

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!spawned)
			return false;

		bool inserted;
		if (parentStorage)
			inserted = inventoryManager.TryInsertItemInStorage(spawned, parentStorage);
		else
			inserted = inventoryManager.TryInsertItem(spawned);

		if (!inserted)
		{
			// Nowhere to put it - delete rather than leaving a loose item on the ground under the
			// player, which would look like a duplication bug.
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			return false;
		}

		ApplyAmmoCount(spawned, persisted.m_iAmmoCount);

		if (!persisted.m_aChildren || persisted.m_aChildren.IsEmpty())
			return true;

		// Put the contents back. Children go into the first storage on the spawned item - a weapon's
		// attachment storage, a backpack's cargo.
		//
		// KNOWN LIMITATION: attachments are inserted in capture order, not dependency order. Vanilla
		// sorts them by how many other attachments they require (see
		// SCR_WeaponAttachmentsStorageComponentSerializer.HandleInsertionOrder), so an optic needing
		// a rail goes on after it. Without that sort, a dependent attachment can fail to attach and
		// is dropped. Single-optic setups are unaffected; stacked attachment chains may not restore
		// fully.
		array<Managed> storages = {};
		spawned.FindComponents(BaseInventoryStorageComponent, storages);

		foreach (CRF_PersistedItem child : persisted.m_aChildren)
		{
			foreach (Managed storageManaged : storages)
			{
				BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(storageManaged);
				if (!storage)
					continue;

				if (SpawnPersistedItem(child, inventoryManager, storage, character))
					break;	// placed - do not try the item's other storages
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Put a magazine back to the round count it had. -1 means the item is not ammunition.
	protected void ApplyAmmoCount(notnull IEntity item, int ammoCount)
	{
		if (ammoCount < 0)
			return;

		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
		if (magazine)
			magazine.SetAmmoCount(ammoCount);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SAVING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_fTimeSinceEventSave += timeSlice;
		m_fHeartbeatTick += timeSlice;

		string blockReason = GetSaveBlockReason();
		if (blockReason != string.Empty)
		{
			// Report periodically instead of sitting silent - "no save logs" otherwise means any of
			// half a dozen conditions with no way to tell them apart from the console.
			if (m_fHeartbeatTick >= HEARTBEAT_INTERVAL_S)
			{
				m_fHeartbeatTick = 0;
				Print(string.Format("[CRF_Persistence] Not saving: %1.", blockReason), LogLevel.NORMAL);
			}

			return;
		}

		if (m_fAutoSaveInterval <= 0)
			return;

		m_fAutoSaveTick += timeSlice;

		if (m_fHeartbeatTick >= HEARTBEAT_INTERVAL_S)
		{
			m_fHeartbeatTick = 0;
			Print(string.Format("[CRF_Persistence] Armed - next save in %1s.", Math.Round(m_fAutoSaveInterval - m_fAutoSaveTick)), LogLevel.NORMAL);
		}

		if (m_fAutoSaveTick < m_fAutoSaveInterval)
			return;

		m_fAutoSaveTick = 0;
		WriteRecord("Autosave");
	}

	//------------------------------------------------------------------------------------------------
	//! Save because something meaningful changed. Rate-limited so a burst of slot changes commits
	//! once rather than forty times.
	void RequestEventSave(string reason)
	{
		if (m_fTimeSinceEventSave < m_fEventSaveCooldown)
			return;

		m_fTimeSinceEventSave = 0;
		WriteRecord(reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Save regardless of cooldown. For phase transitions and other one-off moments.
	void RequestImmediateSave(string reason)
	{
		m_fTimeSinceEventSave = 0;
		WriteRecord(reason);
	}

	//------------------------------------------------------------------------------------------------
	protected void WriteRecord(string reason)
	{
		string blockReason = GetSaveBlockReason();
		if (blockReason != string.Empty)
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: %2.", reason, blockReason), LogLevel.WARNING);
			return;
		}

		if (m_bSaveInFlight)
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: a write is still committing.", reason), LogLevel.WARNING);
			return;
		}

		if (!m_SaveRecord)
			StartFreshRecord();

		// REGRESSION GUARD - do not remove.
		//
		// If the live slot table is empty but the record holds slots, something is wrong with the
		// live state, not with the record. Saving here overwrites good data with nothing.
		//
		// This is not hypothetical: on the first working resume, the mission-start save fired while
		// the slotting manager had not yet built its table, and wrote a 0-slot record over a saved
		// 134-slot one. The resume was destroyed by the act of resuming.
		int liveSlotCount = GetLiveSlotCount();
		if (liveSlotCount == 0 && m_SaveRecord.m_aSlots && !m_SaveRecord.m_aSlots.IsEmpty())
		{
			Print(string.Format("[CRF_Persistence] Save '%1' REFUSED: no live slots, but the record holds %2. Refusing to overwrite good data with an empty table.",
				reason, m_SaveRecord.m_aSlots.Count()), LogLevel.ERROR);
			return;
		}

		CaptureState();

		EDF_DbRepository<CRF_MissionSaveEntity> repository = EDF_DbEntityHelper<CRF_MissionSaveEntity>.GetRepository(m_DbContext);
		if (!repository)
		{
			Print(string.Format("[CRF_Persistence] Save '%1' skipped: could not obtain the repository.", reason), LogLevel.ERROR);
			return;
		}

		m_bSaveInFlight = true;
		Print(string.Format("[CRF_Persistence] Save starting | reason '%1' | phase %2 | %3 slot(s)",
			reason,
			typename.EnumToString(COA_EGamemodeState, m_SaveRecord.m_iGamemodeState),
			m_SaveRecord.m_aSlots.Count()), LogLevel.NORMAL);

		// EDF_DbOperationStatusOnlyCallback invokes the named method as
		// Call(instance, method, ..., code, context), so the handler takes
		// (EDF_EDbOperationStatusCode, Managed).
		EDF_DbOperationStatusOnlyCallback callback(this, "OnRecordWritten");
		repository.AddOrUpdateAsync(m_SaveRecord, callback);
	}

	//------------------------------------------------------------------------------------------------
	//! Write completion handler.
	//! Signature is dictated by EDF_DbOperationStatusOnlyCallback.Invoke().
	//! MUST BE PUBLIC - dispatched via ScriptModule.Call(), same as OnRecordFound above.
	void OnRecordWritten(EDF_EDbOperationStatusCode statusCode, Managed context = null)
	{
		m_bSaveInFlight = false;

		if (statusCode != EDF_EDbOperationStatusCode.SUCCESS)
		{
			m_iSaveFailureCount++;
			Print(string.Format("[CRF_Persistence] SAVE FAILED (%1) | %2 failed / %3 ok this session. The mission is NOT resumable from this point.",
				typename.EnumToString(EDF_EDbOperationStatusCode, statusCode),
				m_iSaveFailureCount,
				m_iSaveSuccessCount), LogLevel.ERROR);
			return;
		}

		m_iSaveSuccessCount++;
		Print(string.Format("[CRF_Persistence] Save complete | phase %1 | %2 slot(s) | %3 saved this session",
			typename.EnumToString(COA_EGamemodeState, m_SaveRecord.m_iGamemodeState),
			m_SaveRecord.m_aSlots.Count(),
			m_iSaveSuccessCount), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Copy live mission state into the record.
	protected void CaptureState()
	{
		m_SaveRecord.m_sMissionResource = GetMissionKey();
		m_SaveRecord.m_iSavedAtUnix = System.GetUnixTime();
		m_SaveRecord.m_bSessionActive = true;

		if (m_Gamemode)
			m_SaveRecord.m_iGamemodeState = m_Gamemode.m_GamemodeState;

		BaseWorld world = GetGame().GetWorld();
		if (world)
			m_SaveRecord.m_fWorldTime = world.GetWorldTime();

		CaptureSlots();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild the slot table from scratch each save.
	//!
	//! Rebuilding rather than patching means a slot that was freed, relocked or reassigned cannot
	//! leave a stale entry behind - the record always describes the slot table as it is now.
	protected void CaptureSlots()
	{
		m_SaveRecord.m_aSlots = {};

		if (!m_SlottingManager)
			m_SlottingManager = COA_SlottingManager.GetInstance();

		if (!m_SlottingManager)
			return;

		map<int, ref COA_SlotData> slotsMap = m_SlottingManager.GetSlotMap();
		if (!slotsMap)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();

		foreach (int slotId, COA_SlotData slotData : slotsMap)
		{
			if (!slotData)
				continue;

			CRF_PersistedSlot saved();
			saved.m_iSlotId = slotId;
			saved.m_iRole = slotData.GetSlotRole();
			saved.m_iFactionEnum = slotData.GetSlotFactionEnum();
			saved.m_iRespawnPoolType = slotData.GetRespawnPoolType();
			saved.m_iRespawnsRemaining = slotData.GetSlotRespawnsRemaining();
			saved.m_bIsDead = slotData.GetIsDeadSlot();
			saved.m_bIsLocked = slotData.GetIsLockedSlot();
			saved.m_iGroupId = ResolveGroupId(slotData);

			// Account GUID, not the runtime player id - see the note on CRF_PersistedSlot.
			saved.m_sOccupantGuid = string.Empty;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0 && playerManager && playerManager.IsPlayerConnected(playerId))
			{
				saved.m_sOccupantGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
				CapturePosition(saved, playerManager, playerId);
				CaptureInventory(saved, playerManager, playerId);
			}

			m_SaveRecord.m_aSlots.Insert(saved);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Record where this player's character is standing.
	//!
	//! Only captured for a live, alive character. A dead player is deliberately left without a
	//! position - restoring them to the spot they were killed would drop a freshly respawned player
	//! back onto whatever killed them.
	protected void CapturePosition(notnull CRF_PersistedSlot saved, notnull PlayerManager playerManager, int playerId)
	{
		saved.m_bHasPosition = false;

		IEntity character = playerManager.GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
		if (controller && controller.IsDead())
			return;

		saved.m_vPosition = character.GetOrigin();

		// Yaw from the character's forward vector. Stored in degrees to match the angle helpers used
		// when placing the character again on load.
		vector angles = character.GetYawPitchRoll();
		saved.m_fYaw = angles[0];

		saved.m_bHasPosition = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Record what this player is carrying, as a tree.
	//!
	//! Only captured for a live, alive character - the same rule as position. A dead player's
	//! inventory belongs to their corpse; on respawn they should get the role's gearscript loadout,
	//! not the kit they died holding.
	protected void CaptureInventory(notnull CRF_PersistedSlot saved, notnull PlayerManager playerManager, int playerId)
	{
		saved.m_bHasInventory = false;
		saved.m_aInventory = {};

		IEntity character = playerManager.GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
		if (controller && controller.IsDead())
			return;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return;

		array<IEntity> rootItems = {};
		inventoryManager.GetAllRootItems(rootItems);

		foreach (IEntity item : rootItems)
		{
			CRF_PersistedItem persisted = CaptureItem(item);
			if (persisted)
				saved.m_aInventory.Insert(persisted);
		}

		saved.m_bHasInventory = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Turn one item and everything inside it into a persisted tree node.
	protected CRF_PersistedItem CaptureItem(IEntity item)
	{
		if (!item)
			return null;

		EntityPrefabData prefabData = item.GetPrefabData();
		if (!prefabData)
			return null;

		ResourceName prefab = prefabData.GetPrefabName();
		if (prefab.IsEmpty())
			return null;	// runtime-created entity with no prefab - cannot be recreated

		CRF_PersistedItem persisted = CRF_PersistedItem.Create(prefab, GetAmmoCount(item));

		// Recurse into anything this item carries: a weapon's attachment storage, a backpack's
		// contents, a vest's pouches. GetAll with includeChildComponents=false keeps this to the
		// storage's DIRECT children, because the recursion below handles the deeper levels - asking
		// for children here as well would record every nested item twice.
		array<Managed> storages = {};
		item.FindComponents(BaseInventoryStorageComponent, storages);

		foreach (Managed storageManaged : storages)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(storageManaged);
			if (!storage)
				continue;

			array<IEntity> contained = {};
			storage.GetAll(contained, false);

			foreach (IEntity child : contained)
			{
				CRF_PersistedItem childPersisted = CaptureItem(child);
				if (childPersisted)
					persisted.m_aChildren.Insert(childPersisted);
			}
		}

		return persisted;
	}

	//------------------------------------------------------------------------------------------------
	//! Rounds remaining, or -1 when the item is not ammunition.
	protected int GetAmmoCount(notnull IEntity item)
	{
		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
		if (!magazine)
			return -1;

		return magazine.GetAmmoCount();
	}

	//------------------------------------------------------------------------------------------------
	//! Turn a slot's group RplId into the stable group id for storage. -1 when the slot has no group.
	protected int ResolveGroupId(notnull COA_SlotData slotData)
	{
		RplId groupRplId = slotData.GetSlotCurrentGroup();
		if (!groupRplId.IsValid())
			return -1;

		RplComponent groupRpl = RplComponent.Cast(Replication.FindItem(groupRplId));
		if (!groupRpl)
			return -1;

		SCR_AIGroup group = SCR_AIGroup.Cast(groupRpl.GetEntity());
		if (!group)
			return -1;

		return group.GetGroupID();
	}

	//------------------------------------------------------------------------------------------------
	//! Write completion handler.
	//! NOTE: there is deliberately no write-completion handler here.
	//! See the comment in WriteRecord() - EDF's write callback type could not be verified, so writes
	//! are fire-and-forget and failures are not observable from script.

	//------------------------------------------------------------------------------------------------
	//! Why saving is currently impossible, or empty string if it is fine.
	//! Split out so the reason can be reported rather than silently swallowed.
	protected string GetSaveBlockReason()
	{
		if (!m_bPersistenceEnabled)
			return "persistence disabled on the component";

		if (!m_DbContext)
			return "no database connection";

		if (RplSession.Mode() == RplMode.Client)
			return "not authority";

		if (!m_Gamemode)
			m_Gamemode = COA_Gamemode.GetInstance();

		if (!m_Gamemode)
			return "COA_Gamemode unavailable";

		if (m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return string.Format("gamemode is in %1, saves only run in GAME", typename.EnumToString(COA_EGamemodeState, m_Gamemode.m_GamemodeState));

		return string.Empty;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SESSION LIFECYCLE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called by COA_Gamemode when the mission enters GAME. Opens the session and takes a first save,
	//! so a crash from this point on is resumable.
	void OnMissionStarted()
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client)
			return;

		RequestImmediateSave("Mission start");
	}

	//------------------------------------------------------------------------------------------------
	//! Called by COA_Gamemode when the mission reaches AAR under its own power.
	//! Closes the session so the next boot does not resume a mission that already finished.
	void OnMissionCompleted()
	{
		if (!m_bPersistenceEnabled || RplSession.Mode() == RplMode.Client || !m_DbContext)
			return;

		if (!m_SaveRecord)
			return;

		if (m_bClearOnCleanEnd)
			m_SaveRecord.m_aSlots = {};

		// Clearing this flag is what tells the next boot "this ended on purpose". It is written even
		// when the slot table is kept, so the record stays for inspection without being resumed.
		m_SaveRecord.m_bSessionActive = false;
		m_SaveRecord.m_iGamemodeState = COA_EGamemodeState.AAR;

		Print("[CRF_Persistence] Mission completed cleanly - closing the session record.", LogLevel.NORMAL);

		EDF_DbRepository<CRF_MissionSaveEntity> repository = EDF_DbEntityHelper<CRF_MissionSaveEntity>.GetRepository(m_DbContext);
		repository.AddOrUpdateAsync(m_SaveRecord);
	}

	//------------------------------------------------------------------------------------------------
	//! Admin entry point: abandon this run so the next boot starts from the beginning.
	//! Does not interrupt the session in progress.
	void AdminStartFresh()
	{
		if (RplSession.Mode() == RplMode.Client || !m_DbContext || !m_SaveRecord)
			return;

		Print("[CRF_Persistence] Session record cleared by admin - the next boot will not resume this run.", LogLevel.NORMAL);

		ResetRecordForFreshRun();

		EDF_DbRepository<CRF_MissionSaveEntity> repository = EDF_DbEntityHelper<CRF_MissionSaveEntity>.GetRepository(m_DbContext);
		repository.AddOrUpdateAsync(m_SaveRecord);
	}
}
