/*
 * CRF_SCR_SpawnRequestComponent.c
 *
 * Fixes a NULL pointer error in SCR_SpawnRequestComponent.OnPostInit that occurs when
 * SCR_RespawnSystemComponent is not yet available (e.g. during server startup or mission
 * transition). The vanilla code null-checks 'respawnSystem' and logs an error but
 * immediately dereferences it anyway, throwing a bunch of errors.
 *
 * Vanilla bug (SCR_SpawnRequestComponent.c ~line 150):
 *   SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
 *   if (!respawnSystem)
 *       Print(...);                          // logs but does NOT return
 *   m_HandlerComponent = ...respawnSystem.FindComponent(...);  // NULL dereference → error
 *
 * Fix: return early and schedule a deferred retry so m_HandlerComponent is resolved once
 * SCR_RespawnSystemComponent becomes available, ensuring the full vanilla possess-spawn path
 * (including OnPlayerSpawnFinalize_S / data collector notification) is used when possible.
 */
modded class SCR_SpawnRequestComponent
{
	//------------------------------------------------------------------------------------------------
	protected override void OnPostInit(IEntity owner)
	{
		// NOTE: We intentionally do NOT call super.OnPostInit(owner) here.
		// The vanilla super implementation is the one that contains the NULL dereference bug.
		// Engine-level ScriptComponent initialization occurs natively before this callback,
		// so omitting the super call does not affect engine-managed component lifecycle.

		if (!GetGame().InPlayMode())
			return;

		m_PlayerController = SCR_PlayerController.Cast(owner);
		if (!m_PlayerController)
		{
			Print(string.Format("%1 is not attached in %2 hierarchy! (%1 should be a child of %3!)",
				Type().ToString(), SCR_PlayerController, SCR_RespawnComponent),
				LogLevel.ERROR);
		}

		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_RplComponent)
		{
			Print(string.Format("%1 could not find %2!",
				Type().ToString(), RplComponent),
				LogLevel.ERROR);
		}

		m_RespawnComponent = SCR_RespawnComponent.Cast(owner.FindComponent(SCR_RespawnComponent));
		if (!m_RespawnComponent)
		{
			Print(string.Format("%1 could not find %2!",
				Type().ToString(), SCR_RespawnComponent),
				LogLevel.ERROR);
		}

		m_LockComponent = SCR_SpawnLockComponent.Cast(owner.FindComponent(SCR_SpawnLockComponent));
		if (!m_LockComponent)
		{
			Print(string.Format("%1 could not find %2!",
				Type().ToString(), SCR_SpawnLockComponent),
				LogLevel.ERROR);
		}

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			// CRF: RespawnSystemComponent not ready yet (game mode still initializing).
			// Schedule a deferred retry so m_HandlerComponent is resolved once available,
			// ensuring the full vanilla possess-spawn path (including OnPlayerSpawnFinalize_S)
			// works correctly rather than permanently falling back to SetInitialMainEntity.
			GetGame().GetCallqueue().CallLater(TryResolveHandlerComponent, 100, false);
			return;
		}

		m_HandlerComponent = SCR_SpawnHandlerComponent.Cast(respawnSystem.FindComponent(GetHandlerType()));
		if (!m_HandlerComponent)
		{
			Print(string.Format("%1 could not find %2!",
				Type().ToString(), GetHandlerType().ToString()),
				LogLevel.ERROR);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Deferred retry: attempt to resolve m_HandlerComponent after game mode initialization completes.
	//! Called via CallLater when SCR_RespawnSystemComponent was not available during OnPostInit.
	protected void TryResolveHandlerComponent()
	{
		if (m_HandlerComponent)
			return;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			// Still not ready — retry again
			GetGame().GetCallqueue().CallLater(TryResolveHandlerComponent, 100, false);
			return;
		}

		m_HandlerComponent = SCR_SpawnHandlerComponent.Cast(respawnSystem.FindComponent(GetHandlerType()));
		if (!m_HandlerComponent)
		{
			Print(string.Format("%1 could not find %2! (deferred retry also failed)",
				Type().ToString(), GetHandlerType().ToString()),
				LogLevel.ERROR);
		}
	}
}
