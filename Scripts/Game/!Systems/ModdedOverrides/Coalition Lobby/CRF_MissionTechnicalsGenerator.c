//------------------------------------------------------------------------------------------------
//! Builds the text for the "Mission Technicals" descriptor
//!
//! Unlike COA_MissionSynopsisGenerator (Workbench-only, produces a markdown PR blurb), this runs at
//! runtime on every machine so the descriptor text always reflects the mission as actually loaded,
//! with no manual upkeep required from the mission maker.
class CRF_MissionTechnicalsGenerator
{
	//------------------------------------------------------------------------------------------------
	//! Builds the full "Mission Technicals" descriptor text from the gamemode's live settings.
	static string BuildText(COA_Gamemode gamemode)
	{
		array<string> lines = {};

		if (!gamemode)
			return "";

		AppendGeneralSection(lines, gamemode);
		AppendRespawnSection(lines, gamemode);
		AppendWeatherSection(lines, gamemode);
		AppendRadioSection(lines, gamemode);

		return SCR_StringHelper.Join("\n", lines);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendGeneralSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("GENERAL");

		string timeLimit = "No limit";
		if (gamemode.m_iTimeLimitMinutes > 0)
			timeLimit = string.Format("%1 minutes", gamemode.m_iTimeLimitMinutes);
		lines.Insert(string.Format("- Time Limit: %1", timeLimit));

		lines.Insert(string.Format("- Join In Progress: %1", BoolToYesNo(!gamemode.m_bLockUnusedSlots)));

		string safestartLimit = "Disabled";
		if (gamemode.m_bUseSafestartTimeLimit)
			safestartLimit = string.Format("%1 minutes", gamemode.m_iSafestartTimeLimit);
		lines.Insert(string.Format("- Safestart Time Limit: %1", safestartLimit));

		lines.Insert(string.Format("- Mission Time Scale: %1x", gamemode.m_fMissionTimeScale));
		lines.Insert(string.Format("- Coalition VON (CVON): %1", BoolToYesNo(gamemode.m_bUseCVON)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendRespawnSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("RESPAWN");
		lines.Insert(string.Format("- Respawn Enabled: %1", BoolToYesNo(gamemode.m_bRespawnEnabled)));

		if (!gamemode.m_bRespawnEnabled)
		{
			lines.Insert("");
			return;
		}

		bool slotBased = (gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT);

		string respawnModeName = "Team-Based (faction ticket pool)";
		if (slotBased)
			respawnModeName = "Slot-Based (per-squad respawn pools)";
		lines.Insert(string.Format("- Mode: %1", respawnModeName));

		if (!slotBased)
		{
			if (!gamemode.GetSlots("BLUFOR").IsEmpty())
				lines.Insert(string.Format("- BLUFOR Tickets: %1", TicketToString(gamemode.m_iBLUFORTickets)));
			if (!gamemode.GetSlots("OPFOR").IsEmpty())
				lines.Insert(string.Format("- OPFOR Tickets: %1", TicketToString(gamemode.m_iOPFORTickets)));
			if (!gamemode.GetSlots("INDFOR").IsEmpty())
				lines.Insert(string.Format("- INDFOR Tickets: %1", TicketToString(gamemode.m_iINDFORTickets)));
			if (!gamemode.GetSlots("CIV").IsEmpty())
				lines.Insert(string.Format("- CIV Tickets: %1", TicketToString(gamemode.m_iCIVTickets)));
		}

		lines.Insert(string.Format("- Wave Respawn: %1", BoolToYesNo(gamemode.m_bWaveRespawn)));
		lines.Insert(string.Format("- Time To Respawn: %1s", gamemode.m_iTimeToRespawn));

		string cutoff = "Never disables";
		if (gamemode.m_iRespawnCutoffMinutes > 0)
			cutoff = string.Format("Disables %1 minutes before mission end", gamemode.m_iRespawnCutoffMinutes);
		lines.Insert(string.Format("- Respawn Cutoff: %1", cutoff));

		lines.Insert(string.Format("- Rally Points Enabled: %1", BoolToYesNo(gamemode.m_bRallyPointsEnabled)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendWeatherSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("WEATHER & TIME");

		SCR_TimeAndWeatherHandlerComponent timeAndWeatherComp = SCR_TimeAndWeatherHandlerComponent.Cast(gamemode.FindComponent(SCR_TimeAndWeatherHandlerComponent));
		if (!timeAndWeatherComp || timeAndWeatherComp.GetStartingWeatherAndTime().IsEmpty())
		{
			lines.Insert("- Not configured");
			lines.Insert("");
			return;
		}

		string startingWeather = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetWeatherPresetName();
		int startingHour = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingHour();
		int startingMinutes = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingMinutes();

		lines.Insert(string.Format("- Starting Weather: %1", startingWeather));
		lines.Insert(string.Format("- Starting Time: %1:%2", startingHour.ToString(), startingMinutes.ToString()));
		lines.Insert(string.Format("- Random Starting Weather: %1", BoolToYesNo(timeAndWeatherComp.GetRandomStartingWeather())));
		lines.Insert(string.Format("- Random Weather Changes: %1", BoolToYesNo(timeAndWeatherComp.GetRandomWeatherChanges())));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendRadioSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("RADIOS");

		bool any = false;
		if (AppendFactionRadios(lines, "BLUFOR", gamemode))
			any = true;
		if (AppendFactionRadios(lines, "OPFOR", gamemode))
			any = true;
		if (AppendFactionRadios(lines, "INDFOR", gamemode))
			any = true;
		if (AppendFactionRadios(lines, "CIV", gamemode))
			any = true;

		if (!any)
			lines.Insert("- Not configured");
	}

	//------------------------------------------------------------------------------------------------
	//! Appends one faction's radio line if that faction has any slots configured. Returns whether a
	//! line was appended, so the caller can tell whether any faction had radio settings to show.
	protected static bool AppendFactionRadios(array<string> lines, FactionKey factionKey, COA_Gamemode gamemode)
	{
		if (gamemode.GetSlots(factionKey).IsEmpty())
			return false;

		COA_GearScriptContainer gearScript = gamemode.GetGearScriptSettings(factionKey);
		if (!gearScript)
			return false;

		lines.Insert(string.Format(
			"- %1: BFT %2, Leadership Radios %3, GI Radios %4, RTO Radios %5",
			factionKey,
			BoolToYesNo(gearScript.m_bEnableBFT),
			BoolToYesNo(gearScript.m_bEnableLeadershipRadios),
			BoolToYesNo(gearScript.m_bEnableGIRadios),
			BoolToYesNo(gearScript.m_bEnableRTORadios)
		));

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static string TicketToString(int tickets)
	{
		if (tickets == -1)
			return "Unlimited";

		return tickets.ToString();
	}

	//------------------------------------------------------------------------------------------------
	protected static string BoolToYesNo(bool value)
	{
		if (value)
			return "Yes";

		return "No";
	}
}
