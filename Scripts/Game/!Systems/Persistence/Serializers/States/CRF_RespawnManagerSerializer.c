//------------------------------------------------------------------------------------------------
// State data for CRF Respawn Manager
class COA_RespawnManagerStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for CRF Respawn Manager - preserves respawn tickets and wave respawn state
class COA_RespawnManagerSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return COA_RespawnManagerStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull SaveContext context)
	{
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return ESerializeResult.DEFAULT;

		// Save version for future compatibility
		context.WriteValue("version", 1);

		// Save faction tickets (individual properties)
		context.WriteValue("bluforTickets", respawnManager.m_iBLUFORTickets);
		context.WriteValue("opforTickets", respawnManager.m_iOPFORTickets);
		context.WriteValue("indforTickets", respawnManager.m_iINDFORTickets);
		context.WriteValue("civTickets", respawnManager.m_iCIVTickets);

		// Save wave respawn state
		context.WriteValue("waveRespawnEnabled", respawnManager.m_bCurrentWaveRespawn);
		context.WriteValue("respawnTime", respawnManager.m_iCurrentTimeToRespawn);

		Print("[COA_RespawnManagerSerializer] Serialized respawn state", LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull LoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return false;

		// Restore faction tickets (direct property access)
		int bluforTickets, opforTickets, indforTickets, civTickets;
		
		if (context.ReadValue("bluforTickets", bluforTickets))
			respawnManager.m_iBLUFORTickets = bluforTickets;
		
		if (context.ReadValue("opforTickets", opforTickets))
			respawnManager.m_iOPFORTickets = opforTickets;
		
		if (context.ReadValue("indforTickets", indforTickets))
			respawnManager.m_iINDFORTickets = indforTickets;
		
		if (context.ReadValue("civTickets", civTickets))
			respawnManager.m_iCIVTickets = civTickets;

		// Restore wave respawn state
		bool waveRespawnEnabled;
		if (context.ReadValue("waveRespawnEnabled", waveRespawnEnabled))
			respawnManager.m_bCurrentWaveRespawn = waveRespawnEnabled;

		int respawnTime;
		if (context.ReadValue("respawnTime", respawnTime))
			respawnManager.m_iCurrentTimeToRespawn = respawnTime;

		Print("[COA_RespawnManagerSerializer] Restored respawn state", LogLevel.NORMAL);
		return true;
	}
}
