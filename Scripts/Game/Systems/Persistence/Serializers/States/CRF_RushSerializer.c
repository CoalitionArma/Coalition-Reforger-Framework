//------------------------------------------------------------------------------------------------
// State data for CRF Rush Gamemode Manager
class CRF_RushStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for the CRF Rush Gamemode Manager.
// Persists the active zone index and which MCOM sites have already been destroyed so that a
// reloaded server resumes zone progression rather than restarting at Zone 1.
//
// Note: MCOM entities are re-spawned fresh by InitializeMCOMSites() on server restart. The
// serializer restores the logical destruction tracking state; previously-destroyed MCOM entities
// will be physically present again but the game will consider them already cleared.
// For the current zone the live MCOM entities remain interactive as expected.
class CRF_RushSerializer : ScriptedStateSerializer
{
	// Sequential MCOM identifiers used by the Rush manager
	static const array<string> MCOM_IDS = {"MCOMA", "MCOMB", "MCOMC", "MCOMD", "MCOME", "MCOMF"};

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_RushStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_RushGamemodeManager rush = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (!rush)
			return ESerializeResult.DEFAULT;

		// Zone 1 with no destructions is the default initial state
		if (rush.GetCurrentZone() == 1)
		{
			bool anyDestroyed = false;
			foreach (string id : MCOM_IDS)
			{
				if (rush.IsMCOMDestroyedByIdentifier(id))
				{
					anyDestroyed = true;
					break;
				}
			}
			if (!anyDestroyed)
				return ESerializeResult.DEFAULT;
		}

		context.WriteValue("version", 1);
		context.WriteValue("currentZone", rush.GetCurrentZone());

		// Save destroyed status for each MCOM site
		for (int i = 0; i < MCOM_IDS.Count(); i++)
			context.WriteValue(MCOM_IDS[i] + "_destroyed", rush.IsMCOMDestroyedByIdentifier(MCOM_IDS[i]));

		Print(string.Format("[CRF_RushSerializer] Serialized currentZone=%1", rush.GetCurrentZone()), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_RushGamemodeManager rush = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (!rush)
			return false;

		int currentZone;
		if (context.ReadValue("currentZone", currentZone))
			rush.SetCurrentZoneForPersistence(currentZone);

		// Restore logical destruction state for each MCOM
		foreach (string id : MCOM_IDS)
		{
			bool destroyed;
			if (context.ReadValue(id + "_destroyed", destroyed) && destroyed)
				rush.SetMCOMDestroyedStatusFromRPC(id, true);
		}

		Replication.BumpMe();

		Print(string.Format("[CRF_RushSerializer] Restored currentZone=%1", currentZone), LogLevel.NORMAL);
		return true;
	}
}
