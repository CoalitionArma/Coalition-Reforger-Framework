//------------------------------------------------------------------------------------------------
// CRF persistence - database entities (Enfusion Database Framework)
//
// WHAT THIS COVERS, AND WHAT IT DOES NOT
//   EDF is a database, not a world snapshotter. It stores exactly the fields written here and
//   nothing else. Vehicle positions, damage states, dropped items, corpses and inventories are NOT
//   persisted by this - restoring those is what EnfusionPersistenceFramework (built on top of EDF)
//   or Bohemia's native PersistenceSystem are for.
//   What this gives is mission continuity: after a crash the mission comes back in the same phase,
//   with the same players in the same slots, roles, groups and respawn counts, instead of resetting
//   to briefing.
//
// SHAPE OF THE DATA
//   One record per mission, rewritten in place on every save. A single atomic record avoids the
//   half-written state you get when mission info and player info are separate rows and a crash
//   lands between the two writes.
//
// ID HANDLING
//   Entity IDs are deliberately NOT constructed by hand. EDF ids are a specific 36-character hex
//   format, so inventing one from a mission name risks writing something the driver will not accept.
//   Instead the manager queries for the existing record, keeps the instance it gets back, and writes
//   that same instance again - so the id EDF assigned on first insert is reused and the record is
//   updated rather than duplicated.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! One carried item, and anything inside or attached to it.
//!
//! Stored as a tree rather than a flat list on purpose. A flat list would record WHAT a player was
//! carrying but not WHERE - every optic would come back as a loose item in a pocket instead of on
//! the rifle, and every backpack would come back empty with its contents scattered. The nesting is
//! what makes a restored loadout look like the one that was saved.
class CRF_PersistedItem
{
	ResourceName m_rPrefab;

	//! Rounds left, for anything with a magazine component. -1 means "not ammunition", which is
	//! distinct from 0 ("an empty magazine") - a distinction worth keeping, because restoring an
	//! empty mag as a full one is a meaningful gameplay difference.
	int m_iAmmoCount;

	//! Attachments and contents. Weapon optics live here, as do the items inside a backpack.
	ref array<ref CRF_PersistedItem> m_aChildren;

	//------------------------------------------------------------------------------------------------
	static CRF_PersistedItem Create(ResourceName prefab, int ammoCount)
	{
		CRF_PersistedItem instance();
		instance.m_rPrefab = prefab;
		instance.m_iAmmoCount = ammoCount;
		instance.m_aChildren = {};
		return instance;
	}
}

//------------------------------------------------------------------------------------------------
//! One slot's worth of state.
//!
//! Occupancy is stored as the BI account GUID, never the runtime player id. Player ids are handed
//! out by connection order and mean nothing after a restart - restoring them would hand slots to
//! whoever reconnected in the same sequence. The GUID is stable per account, which is the same key
//! vanilla uses for persistent player data (see SCR_SpawnLogic).
class CRF_PersistedSlot
{
	int m_iSlotId;

	//! BI account GUID of the occupant, or empty for an unoccupied slot.
	string m_sOccupantGuid;

	int m_iRole;
	int m_iFactionEnum;
	int m_iRespawnPoolType;
	int m_iRespawnsRemaining;

	//! Stable group id from the mission's group setup - NOT an RplId, which is session-scoped and
	//! meaningless after a restart.
	int m_iGroupId;

	bool m_bIsDead;
	bool m_bIsLocked;

	//! Where this player's character was standing at the last save, and which way it faced.
	//!
	//! Only meaningful while m_bHasPosition is true. A slot that was empty, dead, or whose occupant
	//! had not spawned yet has no position, and vector.Zero is a real world coordinate rather than a
	//! usable "no value" - hence the explicit flag instead of testing the vector.
	bool m_bHasPosition;
	vector m_vPosition;

	//! Yaw only. Pitch and roll are not restored: a character is placed upright, and persisting a
	//! ragdoll's orientation would put a resumed player at an angle they cannot stand up from.
	float m_fYaw;

	//! What this player was actually carrying, as opposed to what their role's gearscript hands out.
	//!
	//! Separate flag again rather than testing the array: an empty list is a real state (a player
	//! who had been stripped) and must not be confused with "we never captured this", which has to
	//! fall back to the gearscript loadout instead of spawning them with nothing.
	bool m_bHasInventory;
	ref array<ref CRF_PersistedItem> m_aInventory;
}

//------------------------------------------------------------------------------------------------
//! The mission record. One of these exists per mission resource.
//!
//! The explicit name form is used rather than EDF_DbName.Automatic(): the automatic variant is not
//! valid syntax in the installed EDF build (0.6.10) and fails to compile with "Expected attribute
//! call". Naming it explicitly is also the safer choice regardless - this string is what the type is
//! stored as inside the JSON, so pinning it means renaming the script class later does not orphan
//! every record already on disk.
[EDF_DbName("CRF_MissionSave")]
class CRF_MissionSaveEntity : EDF_DbEntity
{
	//! Which mission this record belongs to. Queried on by the manager to find the right record.
	string m_sMissionResource;

	//! COA_EGamemodeState the mission was in when saved. Stored as int so the record does not break
	//! if the enum gains members.
	int m_iGamemodeState;

	//! Wall-clock time of the save, for logging and for picking the newest record if duplicates ever
	//! appear.
	int m_iSavedAtUnix;

	//! World time, so a resumed mission does not restart its clock and hand both sides a full-length
	//! mission again.
	float m_fWorldTime;

	//! True between entering GAME and a clean finish.
	//!
	//! This is the crash-vs-replay discriminator, and it lives in the record rather than in a
	//! separate marker file on purpose: a file can go out of sync with the database (deleted,
	//! copied between servers, left behind by a different mission), whereas this flag is written and
	//! cleared in the same transaction as the data it describes.
	//!   set   + record present -> the previous session never finished -> RESUME
	//!   clear + record present -> it ended on purpose or completed     -> START FRESH
	bool m_bSessionActive;

	ref array<ref CRF_PersistedSlot> m_aSlots;

	//------------------------------------------------------------------------------------------------
	//! EDF entities cannot have constructors with parameters (engine limitation), so construction
	//! goes through a factory.
	static CRF_MissionSaveEntity Create(string missionResource)
	{
		CRF_MissionSaveEntity instance();
		instance.m_sMissionResource = missionResource;
		instance.m_aSlots = {};
		return instance;
	}
}
