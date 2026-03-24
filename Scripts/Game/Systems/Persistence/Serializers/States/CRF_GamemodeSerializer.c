//------------------------------------------------------------------------------------------------
// State data for CRF Gamemode
class CRF_GamemodeStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for CRF Gamemode system state.
// Restores m_GamemodeState and m_SlottingState so a reloaded mission resumes at the correct
// phase rather than rewinding to BRIEFING/SLOTTING.
class CRF_GamemodeSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_GamemodeStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return ESerializeResult.DEFAULT;

		// Skip saving if the game hasn't started yet — nothing meaningful to restore
		if (gamemode.m_GamemodeState == CRF_EGamemodeState.BRIEFING)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValue("gamemodeState", gamemode.m_GamemodeState);
		context.WriteValue("slottingState",  gamemode.m_SlottingState);

		Print(string.Format("[CRF_GamemodeSerializer] Serialized state=%1 slotting=%2",
			gamemode.m_GamemodeState, gamemode.m_SlottingState), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return false;

		// Restore raw values directly — do NOT call AdvanceGamemodeState() as that
		// triggers side-effects (vehicle spawns, data collector calls, etc.) that should
		// only fire during live gameplay, not on save reload.
		int gamemodeState;
		if (context.ReadValue("gamemodeState", gamemodeState))
			gamemode.m_GamemodeState = gamemodeState;

		int slottingState;
		if (context.ReadValue("slottingState", slottingState))
			gamemode.m_SlottingState = slottingState;

		// Bump replication so clients receive the restored values
		Replication.BumpMe();

		Print(string.Format("[CRF_GamemodeSerializer] Restored state=%1 slotting=%2",
			gamemode.m_GamemodeState, gamemode.m_SlottingState), LogLevel.NORMAL);
		return true;
	}
}
