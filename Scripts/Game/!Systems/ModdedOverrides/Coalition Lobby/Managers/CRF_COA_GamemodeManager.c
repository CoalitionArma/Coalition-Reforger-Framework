modded class COA_GamemodeManager
{
	// Keyed by the awaited entity's RplId so each in-flight wait stays alive (ref-counted) until it
	// resolves or times out - see CRF_StatsNotifyWaiter below.
	protected ref map<RplId, ref CRF_StatsNotifyWaiter> m_mStatsNotifyWaiters = new map<RplId, ref CRF_StatsNotifyWaiter>();

	//------------------------------------------------------------------------------------------------
	//! Base COA_GamemodeManager.InitilizePlayer already handles spawn/possession/gear ordering
	//! (including the GM-possession short-circuit) for both the normal and possession paths - this
	//! hook just needs to know once a non-spectator player has been initialized, to start CRF's stats
	//! tracking. See COA_GamemodeManager.OnPlayerInitialized for where this is called from.
	override void OnPlayerInitialized(int playerId, IEntity playerCharacter, RplComponent playerRplComp, bool isSpectator)
	{
		if (isSpectator || !playerRplComp)
			return;

		CRF_StatsNotifyWaiter waiter = new CRF_StatsNotifyWaiter();
		waiter.Setup(this, playerId, playerRplComp.Id());
		m_mStatsNotifyWaiters.Set(playerRplComp.Id(), waiter);
		waiter.Start(CRF_StatsNotifyWaiter.INTERVAL_MS, CRF_StatsNotifyWaiter.MAX_ATTEMPTS, string.Format("Stats tracking for player %1", playerId));
	}

	//------------------------------------------------------------------------------------------------
	//! Called by CRF_StatsNotifyWaiter once it resolves, times out, or abandons, so it can be released.
	void RemoveStatsNotifyWaiter(RplId playerEntityRplId)
	{
		m_mStatsNotifyWaiters.Remove(playerEntityRplId);
	}
}

//! Resolves delayed stats-manager availability and delayed replicated-entity availability before
//! starting CRF stats tracking for a newly-initialized player. See COA_GamemodeManager.OnPlayerInitialized.
class CRF_StatsNotifyWaiter : COA_RetryWaiter
{
	static const int INTERVAL_MS = 250;
	static const int MAX_ATTEMPTS = 20; // ~5 seconds at 250ms interval

	protected COA_GamemodeManager m_Owner;
	protected int m_iPlayerId;
	protected RplId m_PlayerEntityRplId;
	protected IEntity m_ResolvedEntity;
	protected CRF_ServerStatsManager m_ResolvedStatsManager;

	//------------------------------------------------------------------------------------------------
	void Setup(COA_GamemodeManager owner, int playerId, RplId playerEntityRplId)
	{
		m_Owner = owner;
		m_iPlayerId = playerId;
		m_PlayerEntityRplId = playerEntityRplId;
	}

	//------------------------------------------------------------------------------------------------
	protected override bool IsConditionMet()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(m_iPlayerId))
		{
			// Player disconnected before stats tracking could start - abandon quietly, not a timeout.
			Cancel();
			return false;
		}

		m_ResolvedStatsManager = CRF_ServerStatsManager.GetInstance();

		m_ResolvedEntity = null;
		RplComponent playerRplComp = RplComponent.Cast(Replication.FindItem(m_PlayerEntityRplId));
		if (playerRplComp)
			m_ResolvedEntity = playerRplComp.GetEntity();

		if (!m_ResolvedStatsManager || !m_ResolvedEntity)
			return false;

		// Control assignment (AssignCharacterToPlayer) doesn't necessarily reflect in
		// GetPlayerControlledEntity() on the very same tick it's set - that's normal, not staleness,
		// so keep retrying rather than abandoning. A genuinely stale wait (player respawned into a
		// different character before this caught up) just times out instead, which is fine: it's rare
		// and OnTimeout() already logs it.
		if (playerManager.GetPlayerControlledEntity(m_iPlayerId) != m_ResolvedEntity)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnReady()
	{
		m_ResolvedStatsManager.NotifyPlayerSpawned(m_iPlayerId, m_ResolvedEntity);

		if (m_Owner)
			m_Owner.RemoveStatsNotifyWaiter(m_PlayerEntityRplId);
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnTimeout()
	{
		Print(string.Format("[CRF_COA_GamemodeManager] WARNING: Failed to initialize stats tracking for player %1 after %2 attempts", m_iPlayerId, MAX_ATTEMPTS), LogLevel.WARNING);

		if (m_Owner)
			m_Owner.RemoveStatsNotifyWaiter(m_PlayerEntityRplId);
	}
}
