//------------------------------------------------------------------------------------
// CRF_CacheHunt_DestroyAction: Lets the attacking side destroy a cache by hand.
//
// Shown only to attackers, and only on a cache the gamemode is still tracking. The
// action itself is purely a request - the client sends the cache's RplId to the server
// through COA_PlayerRplToAuthorityManager, and the server re-checks the faction and the
// player's distance before anything is destroyed. Nothing here is trusted.
//------------------------------------------------------------------------------------

class CRF_CacheHunt_DestroyAction: ScriptedUserAction
{
	protected IEntity m_CacheEntity;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		if (!GetGame().InPlayMode())
			return;

		m_CacheEntity = pOwnerEntity;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		RplId cacheId = GetCacheRplId();
		if (cacheId == RplId.Invalid())
		{
			Print("[CRF_CacheHunt] Destroy action could not resolve the cache's RplId.", LogLevel.WARNING);
			return;
		}

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!authorityManager)
			return;

		authorityManager.CacheHuntDestroyCache(cacheId, SCR_PlayerController.GetLocalPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_CacheHuntGamemodeManager gamemode = CRF_CacheHuntGamemodeManager.GetInstance();
		if (!gamemode || !user)
			return false;

		// Defenders rearm from the cache; only the hunting side gets to blow it up
		return gamemode.IsAttacker(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (GetCacheRplId() == RplId.Invalid())
		{
			SetCannotPerformReason(NOT_READY_REASON);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = ACTION_NAME;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The request is sent to the server explicitly, so the action must not also be
	//! broadcast or replicated by the action system.
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The cache is identified to the server by RplId rather than by index, because the
	//! gamemode's cache bookkeeping only exists server-side.
	protected RplId GetCacheRplId()
	{
		if (!m_CacheEntity)
			return RplId.Invalid();

		IEntity cache = m_CacheEntity;

		RplComponent rplComponent = RplComponent.Cast(cache.FindComponent(RplComponent));
		if (!rplComponent)
		{
			// The action may sit on a child of the cache
			cache = m_CacheEntity.GetRootParent();
			if (!cache)
				return RplId.Invalid();

			rplComponent = RplComponent.Cast(cache.FindComponent(RplComponent));
			if (!rplComponent)
				return RplId.Invalid();
		}

		return rplComponent.Id();
	}

	//===================================================================================
	// CONSTANTS
	//===================================================================================

	protected static const string ACTION_NAME		= "Destroy Cache";
	protected static const string NOT_READY_REASON	= "Cache is not ready to be destroyed";
}
