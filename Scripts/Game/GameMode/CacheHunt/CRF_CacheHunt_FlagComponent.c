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
