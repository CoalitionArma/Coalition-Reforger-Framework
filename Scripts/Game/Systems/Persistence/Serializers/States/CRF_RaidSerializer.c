//------------------------------------------------------------------------------------------------
// State data for CRF Raid Gamemode Component
class CRF_RaidStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Raid Gamemode Component.
// Persists the number of objective points destroyed and the current phase so that a reloaded
// server resumes at the correct raid progression stage.
class CRF_RaidSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_RaidStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_RaidGamemodeComponent raid = CRF_RaidGamemodeComponent.GetInstance();
		if (!raid)
			return ESerializeResult.DEFAULT;

		// Nothing to save in the default starting state
		if (raid.m_iPointsDestroyed == 0 && raid.m_iCurrentPhase == 1)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValue("pointsDestroyed", raid.m_iPointsDestroyed);
		context.WriteValue("currentPhase", raid.m_iCurrentPhase);

		Print(string.Format("[CRF_RaidSerializer] Serialized pointsDestroyed=%1 phase=%2",
			raid.m_iPointsDestroyed, raid.m_iCurrentPhase), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_RaidGamemodeComponent raid = CRF_RaidGamemodeComponent.GetInstance();
		if (!raid)
			return false;

		int pointsDestroyed;
		if (context.ReadValue("pointsDestroyed", pointsDestroyed))
			raid.m_iPointsDestroyed = pointsDestroyed;

		int currentPhase;
		if (context.ReadValue("currentPhase", currentPhase))
			raid.m_iCurrentPhase = currentPhase;

		Replication.BumpMe();

		Print(string.Format("[CRF_RaidSerializer] Restored pointsDestroyed=%1 phase=%2",
			pointsDestroyed, currentPhase), LogLevel.NORMAL);
		return true;
	}
}
