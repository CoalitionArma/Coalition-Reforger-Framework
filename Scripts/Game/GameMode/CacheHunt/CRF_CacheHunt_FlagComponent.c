//------------------------------------------------------------------------------------
// CRF_CacheHunt_FlagComponent: Marks a CRF flag pole as one end of a Cache Hunt
// teleport link and carries the replicated "enemies are nearby" state that gates the
// teleport actions.
//
// Two kinds of flag exist:
//  - The defender home flag, placed by the mission maker at their main spawn and named
//    to match the gamemode's 'Defender Home Flag Name'. Its cache index is -1.
//  - A cache flag, auto-spawned by the gamemode next to each cache. Its cache index is
//    the 0-based index of the cache it serves.
//
// The proximity check itself lives on the server in CRF_CacheHuntGamemodeManager, which
// walks every registered flag on a timer. Clients only read the replicated result, so a
// client can never see enemy positions it should not know about.
//------------------------------------------------------------------------------------

class CRF_CacheHunt_FlagComponentClass: ScriptComponentClass {}

class CRF_CacheHunt_FlagComponent: ScriptComponent
{
	//! Flags start unassigned so a freshly spawned cache flag is never mistaken for the home
	//! flag during the moment between spawning and the gamemode tagging it.
	static const int UNASSIGNED_INDEX = -99;

	//! -1 for the defender home flag, otherwise the 0-based index of the cache this flag serves.
	//! Assigned by the gamemode on the server and replicated so client-side user actions can
	//! work out where they lead.
	[RplProp()]
	protected int m_iCacheIndex = UNASSIGNED_INDEX;

	//! True while an attacking player stands inside the gamemode's enemy proximity radius.
	[RplProp()]
	protected bool m_bEnemiesNear = false;

	//! Every flag in the world, on both server and client. Populated in OnPostInit so the
	//! user actions can resolve their partner flag without needing replicated entity refs.
	protected static ref array<CRF_CacheHunt_FlagComponent> m_aRegisteredFlags = {};

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		if (!m_aRegisteredFlags.Contains(this))
			m_aRegisteredFlags.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		m_aRegisteredFlags.RemoveItem(this);
		super.OnDelete(owner);
	}

	//===================================================================================
	// JIP REPLICATION
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Cache flags are spawned by the server at runtime, so a client receives the entity
	//! already built and never sees the RplProp writes that happened before it arrived.
	//! Without this the index stays UNASSIGNED on every client: no flag reports itself as
	//! the home flag, GetHomeFlag() and GetCacheFlag() find nothing, and every teleport
	//! action hides. That failure is invisible in Workbench, where the host is also the
	//! client and reads the server's own values.
	//!
	//! RplSave and RplLoad must stay symmetric - add fields to both, in the same order.
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iCacheIndex);
		writer.WriteBool(m_bEnemiesNear);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadInt(m_iCacheIndex);
		reader.ReadBool(m_bEnemiesNear);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_CacheHunt_FlagComponent()
	{
		if (m_aRegisteredFlags)
			m_aRegisteredFlags.RemoveItem(this);
	}

	//===================================================================================
	// STATE
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Server-side setter for which end of the link this flag is.
	//! \param[in] index -1 for the defender home flag, otherwise the served cache index
	void SetCacheIndex(int index)
	{
		if (!Replication.IsServer() || m_iCacheIndex == index)
			return;

		m_iCacheIndex = index;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Sets the index on this machine only, with no replication.
	//!
	//! Cache flags are spawned at runtime, and component-level replication has not proved
	//! reliable for getting their initial state onto clients of a dedicated server. The
	//! gamemode component replicates the flag roster instead - it is world-baked, so its
	//! RplProps are dependable - and each client stamps its own flags through here.
	//! \param[in] index -1 for the defender home flag, otherwise the served cache index
	void SetCacheIndexLocal(int index)
	{
		m_iCacheIndex = index;
	}

	//------------------------------------------------------------------------------------------------
	//! Sets the enemy-proximity state on this machine only, with no replication. See
	//! SetCacheIndexLocal for why the gamemode drives this rather than the component.
	void SetEnemiesNearLocal(bool enemiesNear)
	{
		m_bEnemiesNear = enemiesNear;
	}

	//------------------------------------------------------------------------------------------------
	int GetCacheIndex()
	{
		return m_iCacheIndex;
	}

	//------------------------------------------------------------------------------------------------
	bool IsHomeFlag()
	{
		return m_iCacheIndex == CRF_CacheHuntGamemodeManager.HOME_FLAG_INDEX;
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side setter used by the gamemode's proximity sweep. Only replicates on change
	//! so a busy flag does not spam the network every tick.
	void SetEnemiesNear(bool enemiesNear)
	{
		if (!Replication.IsServer() || m_bEnemiesNear == enemiesNear)
			return;

		m_bEnemiesNear = enemiesNear;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	bool AreEnemiesNear()
	{
		return m_bEnemiesNear;
	}

	//===================================================================================
	// REGISTRY
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! \return Every Cache Hunt flag currently in the world
	static array<CRF_CacheHunt_FlagComponent> GetRegisteredFlags()
	{
		array<CRF_CacheHunt_FlagComponent> flags = {};
		foreach (CRF_CacheHunt_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag)
				flags.Insert(flag);
		}

		return flags;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The defender home flag, or null when the mission maker has not placed one
	static CRF_CacheHunt_FlagComponent GetHomeFlag()
	{
		foreach (CRF_CacheHunt_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag && flag.IsHomeFlag())
				return flag;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] cacheIndex 0-based cache index
	//! \return The flag serving that cache, or null when the cache is gone or never existed
	static CRF_CacheHunt_FlagComponent GetCacheFlag(int cacheIndex)
	{
		if (cacheIndex < 0)
			return null;

		foreach (CRF_CacheHunt_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag && flag.GetCacheIndex() == cacheIndex)
				return flag;
		}

		return null;
	}
}
