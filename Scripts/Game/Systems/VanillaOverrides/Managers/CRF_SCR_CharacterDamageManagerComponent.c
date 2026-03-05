 /*
 * CRF_SCR_CharacterDamageManagerComponent
 * Tracks damage events for weapon logging to fix issues with incorrect weapons being reported.
 * Also broadcasts cause-of-death damage type to all clients when a player dies.
 */
modded class SCR_CharacterDamageManagerComponent
{
	protected ref BaseDamageEffect m_eFatalDamageEffect;
	
	//------------------------------------------------------------------------------------------------
	BaseDamageEffect GetFatalDamageEffect()
	{
		return m_eFatalDamageEffect;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CRF_HandleDamageTracking(notnull BaseDamageEffect damageEffect)
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
		int damageType = damageEffect.GetDamageType();
		
		// Get killer entity
		IEntity killerEntity = null;
		int killerId = -1;
		
		// Get instigator information
		Instigator instigator = damageEffect.GetInstigator();
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
	//!	Invoked when damage state changes.
	protected override void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		
		if (newState == EDamageState.DESTROYED)
		{
			BaseDamageEffect lastValidDamageEffect;
			array<ref BaseDamageEffect> baseDamageEffects = {};
			GetDamageHistory(baseDamageEffects);
			
			if (!baseDamageEffects.IsEmpty())
			{
				// Iterate in reverse — history is oldest-first, so we walk backwards
				// to find the most recent meaningful damage (the killing blow).
				for (int i = baseDamageEffects.Count() - 1; i >= 0; i--)
				{
					BaseDamageEffect damageEffect = baseDamageEffects.Get(i);
					EDamageType dt = damageEffect.GetDamageType();
					if (dt != EDamageType.TRUE && dt != EDamageType.REGENERATION && dt != EDamageType.HEALING)
					{
						lastValidDamageEffect = damageEffect;
						break;
					}
				}
				
				// If every entry was TRUE/HEALING/REGEN, fall back to the most recent entry
				if (!lastValidDamageEffect)
					lastValidDamageEffect = baseDamageEffects.Get(baseDamageEffects.Count() - 1);
			};
			
			// Track damage for weapon logging (server-only, handled inside)
			if (lastValidDamageEffect)
			{
				m_eFatalDamageEffect = lastValidDamageEffect;
				CRF_HandleDamageTracking(lastValidDamageEffect);
			}
		}
	}
}
