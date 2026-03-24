//------------------------------------------------------------------------------------------------
// State data for CRF Safestart Manager
class CRF_SafestartStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Safestart Manager.
// Persists whether safestart is active, the remaining countdown, and the countdown mode so that
// a reloaded server correctly resumes either mid-safestart or post-safestart.
class CRF_SafestartSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_SafestartStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValue("safeStartEnabled", safestart.GetSafestartStatus());
		context.WriteValue("timeRemaining", safestart.GetSafeStartTimeRemaining());
		context.WriteValue("countdownMode", safestart.GetCountdownMode());

		Print(string.Format("[CRF_SafestartSerializer] Serialized enabled=%1 timeRemaining=%2 countdown=%3",
			safestart.GetSafestartStatus(), safestart.GetSafeStartTimeRemaining(), safestart.GetCountdownMode()), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart)
			return false;

		bool safeStartEnabled;
		context.ReadValue("safeStartEnabled", safeStartEnabled);
		safestart.SetSafeStartEnabledForPersistence(safeStartEnabled);

		int timeRemaining;
		context.ReadValue("timeRemaining", timeRemaining);
		safestart.SetSafeStartTimeRemainingForPersistence(timeRemaining);

		bool countdownMode;
		context.ReadValue("countdownMode", countdownMode);
		safestart.SetCountdownModeForPersistence(countdownMode);

		// If safestart was already completed when we saved, prevent the frame-handler's one-shot
		// init from re-enabling it on reload.
		if (!safeStartEnabled)
			safestart.SetInitCompleteForPersistence(true);

		Print(string.Format("[CRF_SafestartSerializer] Restored enabled=%1 timeRemaining=%2 countdown=%3",
			safeStartEnabled, timeRemaining, countdownMode), LogLevel.NORMAL);
		return true;
	}
}
