//------------------------------------------------------------------------------------------------
// State data for the CRF gamemode.
//------------------------------------------------------------------------------------------------
class COA_GamemodeStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Preserves the mission phase and clock across a crash resume.
//
// This is what makes "resume directly into GAME" work. The phase value restored here is read back by
// CRF_PersistenceManager.OnPersistenceLoaded(), which then calls COA_Gamemode.ReapplyGamemodeState()
// to re-run that phase's side effects without advancing past it.
//
// Saving is gated to the GAME phase by CRF_PersistenceManager.ShouldSaveNow(), so in practice the
// phase written here is always GAME. It is stored explicitly anyway rather than assumed, because a
// scripted save from anywhere else would otherwise resume into the wrong phase silently.
//------------------------------------------------------------------------------------------------
class COA_GamemodeSerializer : ScriptedStateSerializer
{
	protected const int SERIALIZER_VERSION = 1;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return COA_GamemodeStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull SaveContext context)
	{
		// Logged unconditionally on entry: whether the engine calls this at all is the open question
		// about scripted-state registration. If this line never appears during a save, the state is
		// not registered and no amount of serializer code will help.
		Print("[COA_GamemodeSerializer] Serialize() called - gamemode state IS being written.", LogLevel.NORMAL);

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
		{
			Print("[COA_GamemodeSerializer] COA_Gamemode unavailable - writing nothing.", LogLevel.WARNING);
			return ESerializeResult.DEFAULT;
		}

		context.WriteValue("version", SERIALIZER_VERSION);
		context.WriteValue("gamemodeState", gamemode.m_GamemodeState);

		// World time is what mission timers are measured against; without it a resumed mission would
		// restart its clock and hand both sides a full-length mission again.
		BaseWorld world = GetGame().GetWorld();
		if (world)
			context.WriteValue("worldTime", world.GetWorldTime());

		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull LoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return false;

		int gamemodeState;
		if (!context.ReadValue("gamemodeState", gamemodeState))
			return false;

		// Set the phase directly. Going through AdvanceGamemodeState() would step past it and fire
		// the transition side effects for the wrong phase; CRF_PersistenceManager re-applies them
		// deliberately once the whole load has settled.
		gamemode.m_GamemodeState = gamemodeState;
		Replication.BumpMe();

		Print(string.Format("[COA_GamemodeSerializer] Restored gamemode phase %1.",
			typename.EnumToString(COA_EGamemodeState, gamemodeState)), LogLevel.NORMAL);

		return true;
	}
}
