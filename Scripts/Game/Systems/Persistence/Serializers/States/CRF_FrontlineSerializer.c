//------------------------------------------------------------------------------------------------
// State data for CRF Frontline Gamemode Manager
class CRF_FrontlineStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Frontline Gamemode Manager.
// Persists zone ownership/capture state, zone timers, and whether the game has started so that a
// reloaded server resumes frontline at the correct zone-control positions.
class CRF_FrontlineSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_FrontlineStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_FrontlineGamemodeManager frontline = CRF_FrontlineGamemodeManager.GetInstance();
		if (!frontline)
			return ESerializeResult.DEFAULT;

		// Skip if the game hasn't initialised zones yet
		array<string> zonesStatus = frontline.GetZonesStatus();
		if (!zonesStatus || zonesStatus.IsEmpty())
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValue("gameStarted", frontline.m_bGameStarted);
		context.WriteValue("zoneCount", zonesStatus.Count());

		for (int i = 0; i < zonesStatus.Count(); i++)
			context.WriteValue(string.Format("zone_%1", i), zonesStatus[i]);

		// Persist countdown iterators so timers resume correctly
		context.WriteValue("zoneUnlockTimeIter", frontline.m_iZoneUnlockTimeIteratorInt);
		context.WriteValue("timeToWinIter", frontline.m_iTimeToWinIteratorInt);
		context.WriteValue("initialTimeIter", frontline.m_iInitialTimeIteratorInt);

		Print(string.Format("[CRF_FrontlineSerializer] Serialized %1 zones, gameStarted=%2",
			zonesStatus.Count(), frontline.m_bGameStarted), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_FrontlineGamemodeManager frontline = CRF_FrontlineGamemodeManager.GetInstance();
		if (!frontline)
			return false;

		bool gameStarted;
		if (context.ReadValue("gameStarted", gameStarted))
			frontline.m_bGameStarted = gameStarted;

		int zoneCount;
		if (context.ReadValue("zoneCount", zoneCount) && zoneCount > 0)
		{
			array<string> statuses = {};
			for (int i = 0; i < zoneCount; i++)
			{
				string status;
				if (context.ReadValue(string.Format("zone_%1", i), status))
					statuses.Insert(status);
			}

			if (!statuses.IsEmpty())
				frontline.RestoreZonesStatusForPersistence(statuses);
		}

		int zoneUnlockIter;
		if (context.ReadValue("zoneUnlockTimeIter", zoneUnlockIter))
			frontline.m_iZoneUnlockTimeIteratorInt = zoneUnlockIter;

		int timeToWinIter;
		if (context.ReadValue("timeToWinIter", timeToWinIter))
			frontline.m_iTimeToWinIteratorInt = timeToWinIter;

		int initialTimeIter;
		if (context.ReadValue("initialTimeIter", initialTimeIter))
			frontline.m_iInitialTimeIteratorInt = initialTimeIter;

		Replication.BumpMe();

		Print(string.Format("[CRF_FrontlineSerializer] Restored %1 zone statuses, gameStarted=%2",
			zoneCount, gameStarted), LogLevel.NORMAL);
		return true;
	}
}
