#ifdef WORKBENCH

//------------------------------------------------------------------------------------------------
//! Suite for framework tests that need a loaded world and a live COA_Gamemode.
//!
//! Run it:
//!   Workbench  - put the cursor in a test case class (or in this suite) and press F4.
//!   Headless   - ArmaReforgerSteam.exe -autotest CRF_FrameworkTestSuite
//!                Results land in autotest.log.
//!
//! The suite boots the world below before any case runs, so cases can assume the
//! world exists but must NOT assume the gamemode has finished initialising -
//! poll for that in the Main step, which re-runs each frame until it returns true.
//!
//! Everything here is WORKBENCH-only and never ships to a server.
//------------------------------------------------------------------------------------------------
class CRF_FrameworkTestSuite : SCR_AutotestSuiteBase
{
	//! Arland lobby test world. Swap for another mission world to validate that one instead.
	static const ResourceName TEST_WORLD = "{06EB09DA0FBAD133}Worlds/Arland/COA_LobbyTestWorld.ent";

	//------------------------------------------------------------------------------------------------
	override ResourceName GetWorldFile()
	{
		return TEST_WORLD;
	}
}

//------------------------------------------------------------------------------------------------
//! Shared helpers for framework test cases.
//------------------------------------------------------------------------------------------------
class CRF_TestHelper
{
	//------------------------------------------------------------------------------------------------
	//! Faction keys the framework configures gearscripts for.
	static void GetFactionKeys(out notnull array<string> keys)
	{
		keys.Clear();
		keys.Insert("BLUFOR");
		keys.Insert("OPFOR");
		keys.Insert("INDFOR");
		keys.Insert("CIV");
	}

	//------------------------------------------------------------------------------------------------
	//! True when a ResourceName points at a config the engine can actually open.
	//!
	//! Mirrors how COA_SlottingMenu and COA_GearscriptManager load gearscripts, so
	//! a renamed or deleted config fails here instead of in a live mission.
	static bool CanLoadContainer(ResourceName resource)
	{
		if (resource.IsEmpty())
			return false;

		Resource holder = BaseContainerTools.LoadContainer(resource);
		if (!holder || !holder.IsValid())
			return false;

		BaseResourceObject object = holder.GetResource();
		if (!object)
			return false;

		return object.ToBaseContainer() != null;
	}
}

#endif
