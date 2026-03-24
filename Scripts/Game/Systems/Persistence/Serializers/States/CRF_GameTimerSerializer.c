//------------------------------------------------------------------------------------------------
// State data for CRF Game Timer
class CRF_GameTimerStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Game Timer.
// Persists the remaining mission time so a reloaded server resumes the countdown from where it
// left off rather than restarting the full time limit.
class CRF_GameTimerSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_GameTimerStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_GameTimerManager timerManager = CRF_GameTimerManager.GetInstance();
		if (!timerManager)
			return ESerializeResult.DEFAULT;

		// m_iTimeMissionEnds == 0 means no timed mission is active
		if (timerManager.m_iTimeMissionEnds == 0)
			return ESerializeResult.DEFAULT;

		// Save remaining seconds so we can recompute the absolute end timestamp on restore.
		// We store seconds (not raw ms) to avoid float precision issues across server restarts.
		float worldTime = GetGame().GetWorld().GetWorldTime();
		int missionSecondsRemaining = Math.Max(0, (timerManager.m_iTimeMissionEnds - worldTime) * 0.001);

		context.WriteValue("version", 1);
		context.WriteValue("missionSecondsRemaining", missionSecondsRemaining);

		Print(string.Format("[CRF_GameTimerSerializer] Serialized missionRemaining=%1s", missionSecondsRemaining), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_GameTimerManager timerManager = CRF_GameTimerManager.GetInstance();
		if (!timerManager)
			return false;

		float worldTime = GetGame().GetWorld().GetWorldTime();

		int missionSecondsRemaining;
		if (context.ReadValue("missionSecondsRemaining", missionSecondsRemaining))
			timerManager.m_iTimeMissionEnds = worldTime + (missionSecondsRemaining * 1000);

		Replication.BumpMe();

		Print(string.Format("[CRF_GameTimerSerializer] Restored missionRemaining=%1s", missionSecondsRemaining), LogLevel.NORMAL);
		return true;
	}
}
