 /*
 * CRF_SCR_CharacterDamageManagerComponent
 * Tracks damage events for weapon logging to fix issues with incorrect weapons being reported.
 * Also broadcasts cause-of-death damage type to all clients when a player dies.
 */
modded class SCR_CharacterDamageManagerComponent
{
	// Cause-of-death damage type, broadcast to all clients via RPC when the player dies.
	// Stored locally so the spectator UI can read it from charDmg at any time after death.
	EDamageType m_eCRF_FatalDamageType = EDamageType.KINETIC;

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
	// Called when damage is received — track the last harmful damage type on all machines.
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		// Track on every machine so the local copy is always up to date regardless of authority.
		EDamageType dt = damageContext.damageType;
		if (dt != EDamageType.HEALING && dt != EDamageType.REGENERATION)
			m_eCRF_FatalDamageType = dt;

		super.OnDamage(damageContext);

		// Track damage for weapon logging (server-only, handled inside)
		CRF_HandleDamageTracking(damageContext);
	}

	//------------------------------------------------------------------------------------------------
	// When the character is destroyed (killed), broadcast the fatal damage type to all clients.
	override protected void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);

		if (Replication.IsServer() && newState == EDamageState.DESTROYED)
			Rpc(RpcDo_SetFatalDamageType, m_eCRF_FatalDamageType);
	}

	//------------------------------------------------------------------------------------------------
	// Received on all clients — store the fatal damage type so the spectator UI can display it.
	// EDamageType is passed as int because Enfusion RPCs cannot serialise enum types directly.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetFatalDamageType(int damageType)
	{
		m_eCRF_FatalDamageType = damageType;
	}
}
