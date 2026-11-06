//! Data and hooks for the pause menu's mission info panel (CRF_MissionInfoPanel).
//!
//! Gamemode hook: anything that wants a live line under OBJECTIVES (holdout wave, rally standings,
//! dynamic objective...) can either
//!  - override CRF_AppendMissionInfo on its SCR_BaseGameModeComponent (see the modded class below), or
//!  - subscribe to CRF_MissionInfo.GetOnCollectLines() from anywhere else (remember to Remove it).
//! Both are called on the local machine about once a second while the pause menu is open, so read
//! replicated state only and keep it cheap. Plain text, one short line per entry.

void CRF_MissionInfo_OnCollectLines(notnull array<string> lines);
typedef func CRF_MissionInfo_OnCollectLines;

class CRF_MissionInfo
{
	//! Full community rules page, opened by the panel's FULL RULES button
	static const string RULES_URL = "https://coalitiongroup.net/rules";

	protected static ref ScriptInvokerBase<CRF_MissionInfo_OnCollectLines> s_OnCollectLines;

	//------------------------------------------------------------------------------------------------
	//! Subscribe here to add lines to the panel's OBJECTIVES section (see the header comment)
	static ScriptInvokerBase<CRF_MissionInfo_OnCollectLines> GetOnCollectLines()
	{
		if (!s_OnCollectLines)
			s_OnCollectLines = new ScriptInvokerBase<CRF_MissionInfo_OnCollectLines>();

		return s_OnCollectLines;
	}

	//------------------------------------------------------------------------------------------------
	//! Gathers the live gamemode lines: every SCR_BaseGameModeComponent on the gamemode, then the invoker
	static void CollectLines(notnull array<string> outLines)
	{
		outLines.Clear();

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
		{
			array<Managed> components = {};
			gameMode.FindComponents(SCR_BaseGameModeComponent, components);
			foreach (Managed managed : components)
			{
				SCR_BaseGameModeComponent component = SCR_BaseGameModeComponent.Cast(managed);
				if (component)
					component.CRF_AppendMissionInfo(outLines);
			}
		}

		if (s_OnCollectLines)
			s_OnCollectLines.Invoke(outLines);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info hook: append short status lines for the OBJECTIVES section
	//! (e.g. "Wave 4 of 10"). Runs on the local machine while the pause menu is open.
	void CRF_AppendMissionInfo(notnull array<string> lines);
}
