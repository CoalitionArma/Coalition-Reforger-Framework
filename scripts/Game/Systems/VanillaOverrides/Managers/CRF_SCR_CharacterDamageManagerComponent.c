 /*
 * CRF_SCR_CharacterDamageManagerComponent
 * Tracks damage events for weapon logging to fix issues with incorrect weapons being reported
 */
modded class SCR_CharacterDamageManagerComponent
{
	protected void CRF_HandleDamageTracking(notnull BaseDamageContext damageContext)
	{
		// Only run on server
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		// Get victim player ID
		IEntity victim = GetOwner();
		if (!victim)
			return;
			
		int victimId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(victim);
		if (victimId <= 0)
			return; // Not a player
			
		// Get damage type
		int damageType = damageContext.damageType;
		
		// Get killer entity
		IEntity killerEntity = null;
		int killerId = -1;
		
		// Get instigator information
		Instigator instigator = damageContext.instigator;
		if (instigator)
		{
			// Try to get player ID from instigator
			killerId = instigator.GetInstigatorPlayerID();
			
			// If we have a player ID, get their entity
			if (killerId > 0)
				killerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(killerId);
				
			// If instigator isn't a player but has an entity reference, use it
			if (!killerEntity && instigator.GetInstigatorEntity())
				killerEntity = instigator.GetInstigatorEntity();
		}
		
		// If we still don't have a killer entity, return
		if (!killerEntity)
			return;
		
		// Get data collector to track damage
		SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
		if (!dataCollector)
			return;
		
		// Notify data collector about damage
		dataCollector.OnPlayerDamageReceived(victimId, killerEntity, damageType);
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when damage is received
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		
		// Track damage for weapon logging
		CRF_HandleDamageTracking(damageContext);
	}
}
