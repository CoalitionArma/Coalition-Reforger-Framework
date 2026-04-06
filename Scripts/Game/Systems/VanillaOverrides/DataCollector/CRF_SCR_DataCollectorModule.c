// Exposes the protected RemoveInvokers method so CRF_SCR_DataCollectorComponent can call it
// directly when it needs a clean slate before re-registering invokers (entity reuse on reconnect).
// This avoids calling OnPlayerDisconnected for cleanup, which has unintended side-effects on
// modules such as HealingItemsModule that manage their own internal entity-tracking maps and
// rely on those maps being intact inside their own OnPlayerSpawned cleanup logic.
modded class SCR_DataCollectorModule
{
	void CRF_CleanupInvokers(IEntity entity)
	{
		RemoveInvokers(entity);
	}
}
