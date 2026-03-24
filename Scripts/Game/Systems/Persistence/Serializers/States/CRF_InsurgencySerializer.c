//------------------------------------------------------------------------------------------------
// State data for CRF Insurgency Gamemode Manager
class CRF_InsurgencyStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Insurgency Gamemode Manager.
// Persists the current phase and the phase zone-reveal timer so that a reloaded server resumes
// at the correct cache phase rather than restarting from phase 1.
//
// Note: individual cache entity destroyed-states cannot be persisted via RplId (runtime-only).
// Cache entity states are re-populated by CRF_InsDestructiveComponent on spawn. Only the phase
// progression and reveal timer are written here.
class CRF_InsurgencySerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_InsurgencyStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_InsurgencyGamemodeManager insurgency = CRF_InsurgencyGamemodeManager.GetInstance();
		if (!insurgency)
			return ESerializeResult.DEFAULT;

		// Phase 1 with no reveal timer is the initial state — nothing meaningful to restore
		if (insurgency.m_iCurrentPhase == 1 && insurgency.m_iTimePhaseZoneReveals < 0)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValue("currentPhase", insurgency.m_iCurrentPhase);
		context.WriteValue("timePhaseZoneReveals", insurgency.m_iTimePhaseZoneReveals);

		Print(string.Format("[CRF_InsurgencySerializer] Serialized phase=%1 revealTimer=%2",
			insurgency.m_iCurrentPhase, insurgency.m_iTimePhaseZoneReveals), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_InsurgencyGamemodeManager insurgency = CRF_InsurgencyGamemodeManager.GetInstance();
		if (!insurgency)
			return false;

		int currentPhase;
		if (context.ReadValue("currentPhase", currentPhase))
			insurgency.m_iCurrentPhase = currentPhase;

		float timeReveal;
		if (context.ReadValue("timePhaseZoneReveals", timeReveal))
			insurgency.m_iTimePhaseZoneReveals = timeReveal;

		Replication.BumpMe();

		Print(string.Format("[CRF_InsurgencySerializer] Restored phase=%1 revealTimer=%2",
			currentPhase, timeReveal), LogLevel.NORMAL);
		return true;
	}
}
