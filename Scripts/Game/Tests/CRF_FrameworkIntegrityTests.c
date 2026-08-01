#ifdef WORKBENCH

//------------------------------------------------------------------------------------------------
//! Every faction's configured gearscript must resolve to a loadable config.
//!
//! COA_SlottingMenu.SetupFactionUIDisplay feeds this exact ResourceName into
//! BaseContainerTools.LoadContainer(...).GetResource().ToBaseContainer(). When the
//! config has been renamed or deleted the load returns null and that chain throws
//! "NULL pointer to instance. Variable '#return'" while the slotting menu is opening.
//!
//! Tools/Validation/validate_resources.py catches the same class of breakage
//! statically; this catches gearscripts assigned at runtime, which the static
//! check cannot see.
//------------------------------------------------------------------------------------------------
[Test(suite: CRF_FrameworkTestSuite, timeoutMs: 60000)]
class CRF_Test_FactionGearscripts_Resolve : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
		{
			PrintOnce("Waiting for COA_Gamemode...");
			return false; // re-run next frame until the timeout above
		}

		array<string> factions = {};
		CRF_TestHelper.GetFactionKeys(factions);

		int checkedCount;
		foreach (string faction : factions)
		{
			ResourceName gearScript = gamemode.GetGearScriptResource(faction);

			// A faction the mission leaves unassigned is legitimate - the slotting
			// menu skips it. Only an assigned-but-unloadable gearscript is a fault.
			if (gearScript.IsEmpty())
			{
				PrintFormat("%1: no gearscript assigned, skipped", faction);
				continue;
			}

			checkedCount++;
			AssertTrue(
				CRF_TestHelper.CanLoadContainer(gearScript),
				string.Format("%1 gearscript failed to load: %2", faction, gearScript));
		}

		AssertTrue(checkedCount > 0, "No faction had a gearscript assigned - is the world configured?");

		SetResultSuccess();
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Framework managers must actually be attached to the gamemode prefab.
//!
//! Each of these guards a static singleton set from its own constructor. If the
//! prefab stops attaching one, the class still compiles, GetInstance() returns
//! null forever and every call site silently no-ops - which is how stat tracking
//! and AAR replay saving were lost in the CRF/Lobby separation without a single
//! error being logged.
//!
//! Runs on the authority: these are server-side managers and are not replicated.
//------------------------------------------------------------------------------------------------
[Test(suite: CRF_FrameworkTestSuite, timeoutMs: 60000)]
class CRF_Test_GamemodeManagers_Attached : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		if (!COA_Gamemode.GetInstance())
		{
			PrintOnce("Waiting for COA_Gamemode...");
			return false;
		}

		if (RplSession.Mode() == RplMode.Client)
		{
			PrintOnce("Client session, server-side managers not expected here");
			SetResultSuccess();
			return true;
		}

		AssertTrue(CRF_LoggingManager.GetInstance() != null, "CRF_LoggingManager not attached to the gamemode prefab");
		AssertTrue(CRF_MissionValidatorManager.GetInstance() != null, "CRF_MissionValidatorManager not attached to the gamemode prefab");
		AssertTrue(CRF_ServerStatsManager.GetInstance() != null, "CRF_ServerStatsManager not attached to the gamemode prefab");
		AssertTrue(CRF_SlotLottery.GetInstance() != null, "CRF_SlotLottery not attached to the gamemode prefab");
		AssertTrue(CRF_VAAR_GamemodeComponent.GetInstance() != null, "CRF_VAAR_GamemodeComponent not attached to the gamemode prefab");
		AssertTrue(CRF_VehicleGearscriptManager.GetInstance() != null, "CRF_VehicleGearscriptManager not attached to the gamemode prefab");

		SetResultSuccess();
		return true;
	}
}

#endif
