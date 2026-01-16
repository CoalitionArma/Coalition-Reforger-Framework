//------------------------------------------------------------------------------------------------
// State data for CRF Gamemode
// TODO: Uncomment when gamemode persistence is ready
/*
class CRF_GamemodeStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for CRF Gamemode system state
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

		// Save gamemode state
		context.WriteValue("version", 1);
		context.WriteValueDefault("gamemodeState", gamemode.m_GamemodeState, CRF_EGamemodeState.BRIEFING);
		
		// Note: Safestart state and mission time are managed by separate systems
		// and should be serialized by their own serializers
		
		// Save any other critical gamemode data
		// context.WriteValue("customData", gamemode.m_CustomData);

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

		// Load gamemode state
		CRF_EGamemodeState state;
		
		context.ReadValueDefault("gamemodeState", state, CRF_EGamemodeState.BRIEFING);
		
		// Restore gamemode state
		gamemode.m_GamemodeState = state;

		return true;
	}
}
*/
