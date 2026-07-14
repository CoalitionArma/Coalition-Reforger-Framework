#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! Callsign-inferred ORBAT tier for the mission synopsis' Slotting section (see GetCallsignTier()).
//! Internal to the synopsis generator - not a runtime/gameplay concept, so it's kept out of CRF_Enums.c.
enum CRF_ESlottingCallsignTier
{
	COMPANY,
	PLATOON,
	SQUAD,
	OTHER   // Unrecognized/custom callsign - always rendered top-level, un-nested
}

[WorkbenchPluginAttribute(
	name: "6 | Generate Config File",
	description: "Generate Mission Configuration File", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF0C7)
] 
class CRF_MissionConfigurationPlugin : WorkbenchPlugin
{	
	//------------------------------------------------------------------------------------
	[Attribute("<Author>", "auto", "", category: "CRF Mission Config - Mission Info")]
	protected string m_sMissionAuthor;
	
	[Attribute("", "auto", "Your BI account GUID for automatic admin privileges (auto-filled from workbench)", category: "CRF Mission Config - Mission Info")]
	protected string m_sMissionAuthorGUID;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGamemode), category: "CRF Mission Config - Mission Info")]
	CRF_EGamemode m_MissionMode;
	
	[Attribute("<Name>", "auto", "", category: "CRF Mission Config - Mission Info")]
	protected string m_sMissionName;
	
	[Attribute("<Description>", "auto", "", category: "CRF Mission Config - Mission Info")]
	protected string m_sMissionDescription;

	protected const string SCENARIOS_PATH = "Missions";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_sMissionAuthor = "<Author>";
		
		// Auto-fill GUID from currently logged-in Workbench user
		BackendApi backendApi = GetGame().GetBackendApi();
		if (backendApi)
		{
			UUID identityId = BackendAuthenticatorApi.GetIdentityId();
			if (identityId && !identityId.IsNull())
				m_sMissionAuthorGUID = identityId; // UUID extends string, can be assigned directly
			else
				m_sMissionAuthorGUID = "<AuthorGUID - Not logged in to BI account>";
		}
		else
		{
			m_sMissionAuthorGUID = "<AuthorGUID - Backend not available>";
		}
		
		m_MissionMode = CRF_EGamemode.TVT;
		m_sMissionName = "<Name>";
		m_sMissionDescription = "<Description>";

		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"This will automatically generate and sort the mission configuration file. \n\n WARNING: DO NOT RUN THIS TWICE FOR ONE MISSION, SIMPLY GO TO THE ALREADY CREATED CONFIG AND MANUALLY UPDATE IT.", 
		this))
		
			return;
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-shows the synopsis preview (and re-copies it to the clipboard) without regenerating the
	//! mission config - lets a mission maker re-open the synopsis after closing the window, without
	//! risking a second .conf being generated (see the "DO NOT RUN THIS TWICE" warning above).
	[ButtonAttribute("Show Mission Synopsis")]
	protected bool ButtonShowSynopsis()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;

		WorldEditorAPI api = worldEditor.GetApi();

		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		if (!entitySource)
			return false;

		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		if (!gamemode)
			return false;

		string missionMode = SCR_Enum.GetEnumName(CRF_EGamemode, m_MissionMode);

		int missionPlayercount = GetPlayerCount(gamemode.m_BluforSlots);
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_OpforSlots);
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_IndforSlots);
		missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_CivSlots);

		string worldPath;
		api.GetWorldPath(worldPath);

		array<string> strArray = {};
		worldPath.Split("/", strArray, false);
		string missionTerrain = strArray.Get(strArray.Count() - 2);

		string missionDisplayName = string.Format("CRF %1%2 %3", missionMode, missionPlayercount, m_sMissionName);

		ShowMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Generate Mission Config", true)]
	protected bool ButtonNext()
	{
		string missionMode = SCR_Enum.GetEnumName(CRF_EGamemode, m_MissionMode);
		int missionPlayercount;
		string worldPath;

		//--- Get mission header from the template config (can't use the class directly, it's engine-controlled class that cannot have reference in script)
		Resource templateResource = Resource.Load("{3D094352621EA88C}!Missions/CRF_BaseMissionConfig.conf");
		BaseContainer missionHeaderContainer = templateResource.GetResource().ToBaseContainer();

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = worldEditor.GetApi();

		api.GetWorldPath(worldPath);

		//--- Get world path with GUID and save it to the header
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		string absWorldPath;
		Workbench.GetAbsolutePath(worldPath, absWorldPath);
		MetaFile worldMeta = resourceManager.GetMetaFile(absWorldPath);
		string fullWorldPath = worldMeta.GetResourceID();
		missionHeaderContainer.Set("World", fullWorldPath);
		missionHeaderContainer.Set("m_sAuthor", m_sMissionAuthor);
		missionHeaderContainer.Set("m_sAuthorGUID", m_sMissionAuthorGUID);
		missionHeaderContainer.Set("m_sGameMode", missionMode);
		missionHeaderContainer.Set("m_sDescription", m_sMissionDescription);
		missionHeaderContainer.Set("m_iMapMarkerLimitPerPlayer", 256);
		missionHeaderContainer.Set("m_iPlayerCount", 128);

		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");

		if (!entitySource)
			return false;

		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));

		if (gamemode)
		{
			missionPlayercount = GetPlayerCount(gamemode.m_BluforSlots);
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_OpforSlots);
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_IndforSlots);
			missionPlayercount = missionPlayercount + GetPlayerCount(gamemode.m_CivSlots);
		};

		missionHeaderContainer.Set("m_sName", string.Format("CRF %1%2 %3", missionMode, missionPlayercount, m_sMissionName));

		//--- Get target config path
		string fileSystem = FilePath.FileSystemNameFromFileName(worldPath);
		fileSystem = SCR_AddonTool.ToFileSystem(fileSystem);

		array<string> strArray = {};
		worldPath.Split("/", strArray, false);

		string missionTerrain = strArray.Get(strArray.Count() - 2);
		missionHeaderContainer.Set("m_sTerrainName", missionTerrain);

		string relativeDirPath = fileSystem + SCENARIOS_PATH + "/" + missionTerrain;
		string absoluteDirPath;
		if (!Workbench.GetAbsolutePath(relativeDirPath, absoluteDirPath, true)) // the Missions directory does not exist
		{
			if (!Workbench.GetAbsolutePath(relativeDirPath, absoluteDirPath, false))
			{
				Print("Unable to obtain the " + SCENARIOS_PATH + " directory path at " + relativeDirPath, LogLevel.ERROR);
				return false;
			}

			if (!FileIO.MakeDirectory(absoluteDirPath))
			{
				Print("Unable to create the " + SCENARIOS_PATH + " directory at " + absoluteDirPath, LogLevel.ERROR);
				return false;
			}

			Print("Successfully created the " + SCENARIOS_PATH + " directory at " + absoluteDirPath, LogLevel.NORMAL);
		}

		DateTimeUtcAsInt time = Workbench.GetPackedUtcTime();
		string monthFinal;
		int month = time.GetMonth();
		if (month < 10)
			monthFinal = "0";

		monthFinal = monthFinal + month.ToString();

		string dayFinal;
		int day = time.GetDay();
		if (day < 10)
			dayFinal = "0";

		dayFinal = dayFinal + day.ToString();

		string missionBasePath = FilePath.Concat(relativeDirPath, string.Format("%1_%2%3_%4%5_%6", SCR_StringHelper.Filter(m_sMissionAuthor, SCR_StringHelper.ALPHANUMERICAL), monthFinal, dayFinal, missionMode, missionPlayercount, SCR_StringHelper.Filter(m_sMissionName, SCR_StringHelper.ALPHANUMERICAL)));
		string missionHeaderPath = FilePath.AppendExtension(missionBasePath, "conf");

		//--- Create the config
		if (!BaseContainerTools.SaveContainer(missionHeaderContainer, ResourceName.Empty, missionHeaderPath))
		{
			Print(string.Format("Unable to create mission header at %1!", missionHeaderPath), LogLevel.ERROR);
			return false;
		}

		string missionHeaderAbsPath;
		Workbench.GetAbsolutePath(missionHeaderPath, missionHeaderAbsPath, false);
		resourceManager.RegisterResourceFile(missionHeaderAbsPath, false);

		//--- Build the mission synopsis and hand it to the mission maker via clipboard + a preview window
		//--- (NOT written to disk - a file here would get committed and bundled into the mod itself, which we don't want.
		//--- The mission maker pastes this into the PR description instead.)
		string missionDisplayName = string.Format("CRF %1%2 %3", missionMode, missionPlayercount, m_sMissionName);
		ShowMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Builds the synopsis, copies it to the clipboard, and opens the read-only preview dialog.
	protected void ShowMissionSynopsis(CRF_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
	{
		array<string> synopsisLines = BuildMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);
		string synopsisText = SCR_StringHelper.Join("\n", synopsisLines);

		System.ExportToClipboard(synopsisText);

		CRF_MissionSynopsisDialog synopsisDialog = new CRF_MissionSynopsisDialog();
		synopsisDialog.m_sSynopsis = synopsisText;
		Workbench.ScriptDialog(
			"Mission Synopsis (Copied to Clipboard)",
			"This synopsis has already been copied to your clipboard - create your PR and paste it into your PR description now. \n\n You may open the synopsis again via the 'Show Mission Synopsis' button in the Workbench plugin, without regenerating the mission config.",
			synopsisDialog);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetPlayerCount(array<ref CRF_SlottingGroup> factionSlots)
	{
		int missionPlayercount;

		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
			foreach(CRF_EGearRole role : slotGroup.m_aSlots)
				missionPlayercount++;

		return missionPlayercount;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION SYNOPSIS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Builds the full markdown mission synopsis from the mission's actual configured data.
	protected array<string> BuildMissionSynopsis(CRF_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
	{
		array<string> lines = {};

		if (!gamemode)
			return lines;

		// Markers so Coalition_Bot can find this block wherever it's pasted in the PR description,
		// without needing a committed file (this text never touches disk/the mod itself).
		lines.Insert("<!-- CRF_SYNOPSIS_START -->");
		lines.Insert(string.Format("# %1", missionDisplayName));
		lines.Insert("");
		lines.Insert(string.Format("**Author:** %1  **Terrain:** %2  **Game Mode:** %3  **Player Count:** %4", m_sMissionAuthor, missionTerrain, missionMode, missionPlayercount));
		lines.Insert("");

		if (!m_sMissionDescription.IsEmpty() && m_sMissionDescription != "<Description>")
		{
			lines.Insert(m_sMissionDescription);
			lines.Insert("");
		}

		AppendGeneralSection(lines, gamemode);
		AppendRespawnSection(lines, gamemode);
		AppendGearscriptSection(lines, gamemode);
		AppendGameModeComponentSection(lines, gamemode, entitySource, missionMode);
		AppendWeatherSection(lines, gamemode);
		AppendSlottingSection(lines, gamemode);

		lines.Insert("<!-- CRF_SYNOPSIS_END -->");

		return lines;
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendGeneralSection(array<string> lines, CRF_Gamemode gamemode)
	{
		lines.Insert("## General");

		if (!gamemode.m_sFactionOneKey.IsEmpty() && !gamemode.m_sFactionTwoKey.IsEmpty())
			lines.Insert(string.Format("- Slotting Ratio: %1 %2:%3 %4", gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey));

		string timeLimit = "No limit";
		if (gamemode.m_iTimeLimitMinutes > 0)
			timeLimit = string.Format("%1 minutes", gamemode.m_iTimeLimitMinutes);
		lines.Insert(string.Format("- Time Limit: %1", timeLimit));

		lines.Insert(string.Format("- Lock Unused Slots After Safestart: %1", BoolToYesNo(gamemode.m_bLockUnusedSlots)));

		string safestartLimit = "Disabled";
		if (gamemode.m_bUseSafestartTimeLimit)
			safestartLimit = string.Format("%1 minutes", gamemode.m_iSafestartTimeLimit);
		lines.Insert(string.Format("- Safestart Time Limit: %1", safestartLimit));

		lines.Insert(string.Format("- Mission Time Scale: %1x", gamemode.m_fMissionTimeScale));
		lines.Insert(string.Format("- Coalition VON (CVON): %1", BoolToYesNo(gamemode.m_bUseCVON)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRespawnSection(array<string> lines, CRF_Gamemode gamemode)
	{
		lines.Insert("## Respawn");
		lines.Insert(string.Format("- Respawn Enabled: %1", BoolToYesNo(gamemode.m_bRespawnEnabled)));

		if (!gamemode.m_bRespawnEnabled)
		{
			lines.Insert("");
			return;
		}

		bool slotBased = (gamemode.m_eRespawnMode == CRF_ERespawnMode.SLOT);

		string respawnModeName = "Team-Based";
		if (slotBased)
			respawnModeName = "Slot-Based";
		lines.Insert(string.Format("- Mode: %1", respawnModeName));

		if (!slotBased)
		{
			if (!gamemode.m_BluforSlots.IsEmpty())
				lines.Insert(string.Format("- BLUFOR Tickets: %1", TicketToString(gamemode.m_iBLUFORTickets)));
			if (!gamemode.m_OpforSlots.IsEmpty())
				lines.Insert(string.Format("- OPFOR Tickets: %1", TicketToString(gamemode.m_iOPFORTickets)));
			if (!gamemode.m_IndforSlots.IsEmpty())
				lines.Insert(string.Format("- INDFOR Tickets: %1", TicketToString(gamemode.m_iINDFORTickets)));
			if (!gamemode.m_CivSlots.IsEmpty())
				lines.Insert(string.Format("- CIV Tickets: %1", TicketToString(gamemode.m_iCIVTickets)));
		}
		else
		{
			lines.Insert("- Per-squad respawn pools are listed under Slotting below.");
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
	protected void AppendGearscriptSection(array<string> lines, CRF_Gamemode gamemode)
	{
		lines.Insert("## Gearscripts");

		if (!gamemode.m_BluforSlots.IsEmpty())
			lines.Insert(string.Format("- BLUFOR: %1", GearScriptToString(gamemode.m_BLUFORGearScriptSettings)));
		if (!gamemode.m_OpforSlots.IsEmpty())
			lines.Insert(string.Format("- OPFOR: %1", GearScriptToString(gamemode.m_OPFORGearScriptSettings)));
		if (!gamemode.m_IndforSlots.IsEmpty())
			lines.Insert(string.Format("- INDFOR: %1", GearScriptToString(gamemode.m_INDFORGearScriptSettings)));
		if (!gamemode.m_CivSlots.IsEmpty())
			lines.Insert(string.Format("- CIV: %1", GearScriptToString(gamemode.m_CIVILIANGearScriptSettings)));

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected string GearScriptToString(CRF_GearScriptContainer gearScriptSettings)
	{
		if (!gearScriptSettings || gearScriptSettings.m_rGearScript.IsEmpty())
			return "None";

		return string.Format("%1", gearScriptSettings.m_rGearScript);
	}

	//------------------------------------------------------------------------------------------------
	protected string TicketToString(int tickets)
	{
		if (tickets == -1)
			return "Unlimited";

		return tickets.ToString();
	}

	//------------------------------------------------------------------------------------------------
	protected string BoolToYesNo(bool value)
	{
		if (value)
			return "Yes";

		return "No";
	}

	//------------------------------------------------------------------------------------------------
	//! Detects which known CRF gamemode logic components are attached to CRF_Lobby, on top of the
	//! single authored CRF_EGamemode label, and lists each detected component's own mission-tunable
	//! parameters. A mission can stack more than one (e.g. Rally + Attrition).
	//! Array-of-config-object fields (loot tables, HVT entries, zone respawn configs, etc.) are shown
	//! as a count rather than fully expanded - keeps the synopsis readable instead of dumping every
	//! nested sub-object.
	protected void AppendGameModeComponentSection(array<string> lines, CRF_Gamemode gamemode, IEntitySource entitySource, string missionMode)
	{
		lines.Insert("## Game Mode Components");
		lines.Insert(string.Format("- Selected Mode: %1", missionMode));

		bool anyFound = false;

		IEntityComponentSource attritionSrc = GetComponentSource(entitySource, CRF_AttritionGamemodeComponent);
		if (attritionSrc)
		{
			anyFound = true;
			lines.Insert("- **Attrition**");
			AppendAttritionParams(lines, attritionSrc);
		}

		IEntityComponentSource frontlineSrc = GetComponentSource(entitySource, CRF_FrontlineGamemodeManager);
		if (frontlineSrc)
		{
			anyFound = true;
			lines.Insert("- **Frontline**");
			AppendFrontlineParams(lines, frontlineSrc);
		}

		IEntityComponentSource hvtSrc = GetComponentSource(entitySource, CRF_HighValueTargetGamemodeManager);
		if (hvtSrc)
		{
			anyFound = true;
			lines.Insert("- **High Value Target**");
			AppendHVTParams(lines, hvtSrc);
		}

		IEntityComponentSource insurgencySrc = GetComponentSource(entitySource, CRF_InsurgencyGamemodeManager);
		if (insurgencySrc)
		{
			anyFound = true;
			lines.Insert("- **Insurgency**");
			AppendInsurgencyParams(lines, insurgencySrc);
		}

		IEntityComponentSource raidSrc = GetComponentSource(entitySource, CRF_RaidGamemodeComponent);
		if (raidSrc)
		{
			anyFound = true;
			lines.Insert("- **Raid**");
			AppendRaidParams(lines, raidSrc);
		}

		IEntityComponentSource rallySrc = GetComponentSource(entitySource, CRF_RallyGamemodeComponent);
		if (rallySrc)
		{
			anyFound = true;
			lines.Insert("- **Rally**");
			AppendRallyParams(lines, rallySrc);
		}

		IEntityComponentSource rushSrc = GetComponentSource(entitySource, CRF_RushGamemodeManager);
		if (rushSrc)
		{
			anyFound = true;
			lines.Insert("- **Rush**");
			AppendRushParams(lines, rushSrc);
		}

		IEntityComponentSource sndSrc = GetComponentSource(entitySource, CRF_SearchAndDestroyGamemodeManager);
		if (sndSrc)
		{
			anyFound = true;
			lines.Insert("- **Search And Destroy**");
			AppendSearchAndDestroyParams(lines, sndSrc);
		}

		IEntityComponentSource looterSrc = GetComponentSource(entitySource, CRF_LooterGamemodeComponent);
		if (looterSrc)
		{
			anyFound = true;
			lines.Insert("- **Looter**");
			AppendLooterParams(lines, looterSrc);
		}

		IEntityComponentSource propHuntSrc = GetComponentSource(entitySource, CRF_PropHuntGamemode);
		if (propHuntSrc)
		{
			anyFound = true;
			lines.Insert("- **Prop Hunt**");
			AppendPropHuntParams(lines, propHuntSrc);
		}

		IEntityComponentSource supplySrc = GetComponentSource(entitySource, CRF_SupplyExtractionGamemodeManager);
		if (supplySrc)
		{
			anyFound = true;
			lines.Insert("- **Supply Extraction**");
			AppendSupplyExtractionParams(lines, supplySrc);
		}

		if (!anyFound)
			lines.Insert("- Active Components: None detected");

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the IEntityComponentSource for componentType if it's attached to entitySource, or null.
	protected IEntityComponentSource GetComponentSource(IEntitySource entitySource, typename componentType)
	{
		int index = SCR_BaseContainerTools.FindComponentIndex(entitySource, componentType);
		if (index < 0)
			return null;

		return entitySource.GetComponent(index);
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendAttritionParams(array<string> lines, IEntityComponentSource src)
	{
		string teamA, teamB;
		float thresholdA, thresholdB, notifyPct, vehicleGrace;
		bool endOnVictory, countPlayers, countVehicles;
		CRF_EAttritionForceMode forceMode;

		if (src.Get("m_sTeamAFactionKey", teamA))
			lines.Insert(string.Format("  - Team A Faction: %1", teamA));
		if (src.Get("m_sTeamBFactionKey", teamB))
			lines.Insert(string.Format("  - Team B Faction: %1", teamB));
		if (src.Get("m_fTeamAVictoryThreshold", thresholdA))
			lines.Insert(string.Format("  - Team A Victory Threshold: %1%%", thresholdA));
		if (src.Get("m_fTeamBVictoryThreshold", thresholdB))
			lines.Insert(string.Format("  - Team B Victory Threshold: %1%%", thresholdB));
		if (src.Get("m_bEndMissionOnVictory", endOnVictory))
			lines.Insert(string.Format("  - End Mission On Victory: %1", BoolToYesNo(endOnVictory)));
		if (src.Get("m_fNotifyIncrementPercent", notifyPct))
			lines.Insert(string.Format("  - Notify Increment: %1%%", notifyPct));
		if (src.Get("m_eForceCompositionMode", forceMode))
			lines.Insert(string.Format("  - Force Composition Mode: %1", SCR_Enum.GetEnumName(CRF_EAttritionForceMode, forceMode)));
		if (src.Get("m_bCountPlayers", countPlayers))
			lines.Insert(string.Format("  - Count Players: %1", BoolToYesNo(countPlayers)));
		if (src.Get("m_bCountVehicles", countVehicles))
			lines.Insert(string.Format("  - Count Vehicles: %1", BoolToYesNo(countVehicles)));
		if (src.Get("m_fVehicleGracePeriodSeconds", vehicleGrace))
			lines.Insert(string.Format("  - Vehicle Grace Period: %1s", vehicleGrace));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendFrontlineParams(array<string> lines, IEntityComponentSource src)
	{
		FactionKey bluforSide, opforSide;
		string bluforNick, opforNick;
		array<string> zoneObjectNames;
		int captureTime, unlockTime, minPlayers, timeToWin, initialTime;
		bool useRespawns;

		if (src.Get("m_BluforSide", bluforSide))
			lines.Insert(string.Format("  - Blufor Side: %1", bluforSide));
		if (src.Get("m_sBluforSideNickname", bluforNick))
			lines.Insert(string.Format("  - Blufor Nickname: %1", bluforNick));
		if (src.Get("m_OpforSide", opforSide))
			lines.Insert(string.Format("  - Opfor Side: %1", opforSide));
		if (src.Get("m_sOpforSideNickname", opforNick))
			lines.Insert(string.Format("  - Opfor Nickname: %1", opforNick));
		if (src.Get("m_aZoneObjectNames", zoneObjectNames))
			lines.Insert(string.Format("  - Zones: %1", zoneObjectNames.Count()));
		if (src.Get("m_iZoneCaptureTime", captureTime))
			lines.Insert(string.Format("  - Zone Capture Time: %1s", captureTime));
		if (src.Get("m_iZoneUnlockTime", unlockTime))
			lines.Insert(string.Format("  - Zone Unlock Time: %1s", unlockTime));
		if (src.Get("m_iMinNumberOfPlayersNeeded", minPlayers))
			lines.Insert(string.Format("  - Min Players To Capture: %1", minPlayers));
		if (src.Get("m_iTimeToWin", timeToWin))
			lines.Insert(string.Format("  - Time To Win After Full Capture: %1s", timeToWin));
		if (src.Get("m_iInitialTime", initialTime))
			lines.Insert(string.Format("  - Initial Middle Zone Unlock Time: %1s", initialTime));
		if (src.Get("m_bUseRespawns", useRespawns))
			lines.Insert(string.Format("  - Respawn On Zone Capture: %1", BoolToYesNo(useRespawns)));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendHVTParams(array<string> lines, IEntityComponentSource src)
	{
		bool transponderMarker, disableDamage, initialPing, filterFaction, updateDefender;
		CRF_TargetType targetType;
		int timeBetweenPings;
		string searcherFaction;
		CRF_AIHVTState aiState;
		array<ref CRF_HVTEntry> hvtEntries;

		if (src.Get("m_bEnableTransponderMarker", transponderMarker))
			lines.Insert(string.Format("  - Transponder Marker: %1", BoolToYesNo(transponderMarker)));
		if (src.Get("m_eTargetType", targetType))
			lines.Insert(string.Format("  - Target Type: %1", SCR_Enum.GetEnumName(CRF_TargetType, targetType)));
		if (src.Get("m_bDisableDamage", disableDamage))
			lines.Insert(string.Format("  - Disable HVT Damage: %1", BoolToYesNo(disableDamage)));
		if (src.Get("m_timeBetweenPings", timeBetweenPings))
			lines.Insert(string.Format("  - Time Between Pings: %1s", timeBetweenPings));
		if (src.Get("m_bInitialPing", initialPing))
			lines.Insert(string.Format("  - Initial Ping On Start: %1", BoolToYesNo(initialPing)));
		if (src.Get("m_filterFaction", filterFaction))
			lines.Insert(string.Format("  - Filter Marker To Searcher Faction: %1", BoolToYesNo(filterFaction)));
		if (src.Get("m_searcherFactionKey", searcherFaction))
			lines.Insert(string.Format("  - Searcher Faction: %1", searcherFaction));
		if (src.Get("m_updateDefender", updateDefender))
			lines.Insert(string.Format("  - Notify Defending Faction On Ping: %1", BoolToYesNo(updateDefender)));
		if (src.Get("m_eAIHVTState", aiState))
			lines.Insert(string.Format("  - AI HVT State: %1", SCR_Enum.GetEnumName(CRF_AIHVTState, aiState)));
		if (src.Get("m_aHVTEntries", hvtEntries))
			lines.Insert(string.Format("  - HVT Entries: %1", hvtEntries.Count()));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendInsurgencyParams(array<string> lines, IEntityComponentSource src)
	{
		FactionKey attacking, defending;
		int phaseBuffer;

		if (src.Get("m_AttackingSide", attacking))
			lines.Insert(string.Format("  - Attacking Side: %1", attacking));
		if (src.Get("m_DefendingSide", defending))
			lines.Insert(string.Format("  - Defending Side: %1", defending));
		if (src.Get("m_iPhaseBufferMinutes", phaseBuffer))
			lines.Insert(string.Format("  - Phase Buffer: %1 min", phaseBuffer));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRaidParams(array<string> lines, IEntityComponentSource src)
	{
		float winThreshold;
		string attacking, defending;
		bool mapMarkers;

		if (src.Get("m_fWinThresholdPercent", winThreshold))
			lines.Insert(string.Format("  - Win Threshold: %1%%", winThreshold));
		if (src.Get("m_sAttackingSide", attacking))
			lines.Insert(string.Format("  - Attacking Side: %1", attacking));
		if (src.Get("m_sDefendingSide", defending))
			lines.Insert(string.Format("  - Defending Side: %1", defending));
		if (src.Get("m_bEnableMapMarkers", mapMarkers))
			lines.Insert(string.Format("  - Map Markers: %1", BoolToYesNo(mapMarkers)));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRallyParams(array<string> lines, IEntityComponentSource src)
	{
		float checkpointRadius, smokeOffset;
		int laps, maxStandingsRows;
		bool requireVehicle, teamMode;

		if (src.Get("m_fCheckpointRadius", checkpointRadius))
			lines.Insert(string.Format("  - Checkpoint Radius: %1m", checkpointRadius));
		if (src.Get("m_iLapsToComplete", laps))
			lines.Insert(string.Format("  - Laps To Complete: %1", laps));
		if (src.Get("m_bRequireVehicle", requireVehicle))
			lines.Insert(string.Format("  - Require Vehicle: %1", BoolToYesNo(requireVehicle)));
		if (src.Get("m_iMaxStandingsRows", maxStandingsRows))
			lines.Insert(string.Format("  - Max Standings Rows: %1", maxStandingsRows));
		if (src.Get("m_bTeamMode", teamMode))
			lines.Insert(string.Format("  - Team Mode: %1", BoolToYesNo(teamMode)));
		if (src.Get("m_fSmokeOffsetDistance", smokeOffset))
			lines.Insert(string.Format("  - Smoke Marker Offset: %1m", smokeOffset));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRushParams(array<string> lines, IEntityComponentSource src)
	{
		FactionKey attacking, defending;
		ResourceName mcomPrefab;
		bool hideMarkers, dynamicRespawns;
		int mcomTimer, numZones, mcomsPerZone;
		array<ref CRF_Rush_ZoneRespawnConfig> zoneConfigs;

		if (src.Get("m_AttackingSide", attacking))
			lines.Insert(string.Format("  - Attacking Side: %1", attacking));
		if (src.Get("m_DefendingSide", defending))
			lines.Insert(string.Format("  - Defending Side: %1", defending));
		if (src.Get("m_MCOMPrefab", mcomPrefab))
			lines.Insert(string.Format("  - MCOM Prefab: %1", mcomPrefab));
		if (src.Get("m_bHideMapMarkers", hideMarkers))
			lines.Insert(string.Format("  - Hide MCOM Markers: %1", BoolToYesNo(hideMarkers)));
		if (src.Get("m_iMCOMTimer", mcomTimer))
			lines.Insert(string.Format("  - MCOM Destruction Timer: %1s", mcomTimer));
		if (src.Get("m_iNumberOfZones", numZones))
			lines.Insert(string.Format("  - Number Of Zones: %1", numZones));
		if (src.Get("m_iMCOMsPerZone", mcomsPerZone))
			lines.Insert(string.Format("  - MCOMs Per Zone: %1", mcomsPerZone));
		if (src.Get("m_bEnableDynamicRespawns", dynamicRespawns))
			lines.Insert(string.Format("  - Dynamic Respawns By Zone: %1", BoolToYesNo(dynamicRespawns)));
		if (src.Get("m_aZoneRespawnConfigs", zoneConfigs))
			lines.Insert(string.Format("  - Zone Respawn Configs: %1", zoneConfigs.Count()));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendSearchAndDestroyParams(array<string> lines, IEntityComponentSource src)
	{
		FactionKey attacking, defending;
		string bombSitePrefab;
		bool hideMarkers;

		if (src.Get("attackingSide", attacking))
			lines.Insert(string.Format("  - Attacking Side: %1", attacking));
		if (src.Get("defendingSide", defending))
			lines.Insert(string.Format("  - Defending Side: %1", defending));
		if (src.Get("bombSitePrefab", bombSitePrefab))
			lines.Insert(string.Format("  - Bomb Site Prefab: %1", bombSitePrefab));
		if (src.Get("hideMapMarkers", hideMarkers))
			lines.Insert(string.Format("  - Hide Map Markers: %1", BoolToYesNo(hideMarkers)));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendLooterParams(array<string> lines, IEntityComponentSource src)
	{
		float spawnChance;
		int totalSpawnPoints;
		string spawnPrefix;
		bool individualBFT;
		array<ref CRF_WeaponLootEntry> weaponEntries;
		array<ref CRF_GearLootEntry> gearEntries;
		array<ref CRF_BagLootEntry> bagEntries;
		array<ref CRF_KitLootEntry> kitEntries;
		array<ref CRF_MiscLootEntry> miscEntries;

		if (src.Get("m_fGlobalSpawnChance", spawnChance))
			lines.Insert(string.Format("  - Global Spawn Chance: %1", spawnChance));
		if (src.Get("m_iTotalSpawnPoints", totalSpawnPoints))
			lines.Insert(string.Format("  - Total Spawn Points: %1", totalSpawnPoints));
		if (src.Get("m_sSpawnPointPrefix", spawnPrefix))
			lines.Insert(string.Format("  - Spawn Point Prefix: %1", spawnPrefix));
		if (src.Get("m_bEnableIndividualBFT", individualBFT))
			lines.Insert(string.Format("  - Individual BFT: %1", BoolToYesNo(individualBFT)));
		if (src.Get("m_aWeaponEntries", weaponEntries))
			lines.Insert(string.Format("  - Weapon Loot Entries: %1", weaponEntries.Count()));
		if (src.Get("m_aGearEntries", gearEntries))
			lines.Insert(string.Format("  - Gear Loot Entries: %1", gearEntries.Count()));
		if (src.Get("m_aBagEntries", bagEntries))
			lines.Insert(string.Format("  - Bag Loot Entries: %1", bagEntries.Count()));
		if (src.Get("m_aKitEntries", kitEntries))
			lines.Insert(string.Format("  - Kit Loot Entries: %1", kitEntries.Count()));
		if (src.Get("m_aMiscEntries", miscEntries))
			lines.Insert(string.Format("  - Misc Loot Entries: %1", miscEntries.Count()));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendPropHuntParams(array<string> lines, IEntityComponentSource src)
	{
		string propsTeam, huntersTeam, returnSpawn;
		int totalRounds, gracePeriod, huntTimeLimit, hunterMaxHealth, hunterShotPenalty, interRoundPause, warmupSeconds;
		array<string> noiseSounds;
		float noiseCooldown;

		if (src.Get("m_sPropsTeamKey", propsTeam))
			lines.Insert(string.Format("  - Props Team: %1", propsTeam));
		if (src.Get("m_sHuntersTeamKey", huntersTeam))
			lines.Insert(string.Format("  - Hunters Team: %1", huntersTeam));
		if (src.Get("m_iTotalRounds", totalRounds))
			lines.Insert(string.Format("  - Total Rounds: %1", totalRounds));
		if (src.Get("m_iGracePeriodSeconds", gracePeriod))
			lines.Insert(string.Format("  - Grace Period: %1s", gracePeriod));
		if (src.Get("m_iHuntTimeLimitSeconds", huntTimeLimit))
			lines.Insert(string.Format("  - Hunt Time Limit: %1s", huntTimeLimit));
		if (src.Get("m_iHunterMaxHealth", hunterMaxHealth))
			lines.Insert(string.Format("  - Hunter Max Health: %1", hunterMaxHealth));
		if (src.Get("m_iHunterShotPenalty", hunterShotPenalty))
			lines.Insert(string.Format("  - Hunter Shot Penalty: %1", hunterShotPenalty));
		if (src.Get("m_iInterRoundPauseSeconds", interRoundPause))
			lines.Insert(string.Format("  - Inter-Round Pause: %1s", interRoundPause));
		if (src.Get("m_iWarmupSeconds", warmupSeconds))
			lines.Insert(string.Format("  - Warmup: %1s", warmupSeconds));
		if (src.Get("m_sHunterReturnSpawnName", returnSpawn))
			lines.Insert(string.Format("  - Hunter Return Spawn: %1", returnSpawn));
		if (src.Get("m_aNoiseSoundEvents", noiseSounds))
			lines.Insert(string.Format("  - Noise Sound Events: %1", noiseSounds.Count()));
		if (src.Get("m_fPropNoiseCooldown", noiseCooldown))
			lines.Insert(string.Format("  - Prop Noise Cooldown: %1s", noiseCooldown));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendSupplyExtractionParams(array<string> lines, IEntityComponentSource src)
	{
		int totalDepots, winningSupplyCount, manpower, extractionDistance, gameUpdateTime;
		string extractionObject, factionKey, manpowerMsg, suppliesExtractedMsg, gameMsg1, gameMsg2;
		bool enableManpowerMsg, enableSuppliesExtracted, enableGameMsg1, enableGameMsg2;

		if (src.Get("m_totalDepots", totalDepots))
			lines.Insert(string.Format("  - Total Depots: %1", totalDepots));
		if (src.Get("m_winningSupplyCount", winningSupplyCount))
			lines.Insert(string.Format("  - Winning Supply Count: %1", winningSupplyCount));
		if (src.Get("m_manpower", manpower))
			lines.Insert(string.Format("  - Manpower Needed For Retreat: %1", manpower));
		if (src.Get("m_extractionDistance", extractionDistance))
			lines.Insert(string.Format("  - Extraction Distance: %1m", extractionDistance));
		if (src.Get("m_extractionObject", extractionObject))
			lines.Insert(string.Format("  - Extraction Object: %1", extractionObject));
		if (src.Get("m_factionKey", factionKey))
			lines.Insert(string.Format("  - Extracting Faction: %1", factionKey));
		if (src.Get("m_enableManpowerMessage", enableManpowerMsg))
			lines.Insert(string.Format("  - Manpower Message Enabled: %1", BoolToYesNo(enableManpowerMsg)));
		if (src.Get("m_enableSuppliesExtracted", enableSuppliesExtracted))
			lines.Insert(string.Format("  - Supplies Extracted Message Enabled: %1", BoolToYesNo(enableSuppliesExtracted)));
		if (src.Get("m_enableGameMessage1", enableGameMsg1))
			lines.Insert(string.Format("  - Retreat Message Enabled: %1", BoolToYesNo(enableGameMsg1)));
		if (src.Get("m_enableGameMessage2", enableGameMsg2))
			lines.Insert(string.Format("  - Wiped-Out Message Enabled: %1", BoolToYesNo(enableGameMsg2)));
		if (src.Get("m_gameUpdateTime", gameUpdateTime))
			lines.Insert(string.Format("  - Game Status Check Interval: %1ms", gameUpdateTime));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendWeatherSection(array<string> lines, CRF_Gamemode gamemode)
	{
		lines.Insert("## Weather");

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
	protected void AppendSlottingSection(array<string> lines, CRF_Gamemode gamemode)
	{
		//--- Load the global roles config directly as a resource (not via CRF_GearscriptManager.GetRolesConfig(),
		//--- which depends on a live runtime manager singleton that isn't guaranteed to exist in Workbench)
		CRF_RolesConfig rolesConfig = CRF_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf").GetResource().ToBaseContainer()));

		AppendFactionSlotting(lines, "BLUFOR", gamemode.m_BluforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "OPFOR", gamemode.m_OpforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "INDFOR", gamemode.m_IndforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "CIV", gamemode.m_CivSlots, gamemode.m_eRespawnMode, rolesConfig);
	}

	//------------------------------------------------------------------------------------------------
	//! Classifies a squad/platoon/company's tier from its callsign, using the same COY / NPLT / N-M
	//! naming convention the Quick Slot Setup tool's auto-numbering already produces (SetPluginQuickSlots
	//! in CRF_MissionSlottingPlugin.c). Custom-renamed callsigns that don't match any pattern fall back
	//! to CRF_ESlottingCallsignTier.OTHER, which is always rendered top-level/un-nested.
	protected CRF_ESlottingCallsignTier GetCallsignTier(string callsign)
	{
		string upper = callsign;
		upper.ToUpper();

		if (upper == "COY")
			return CRF_ESlottingCallsignTier.COMPANY;

		if (upper.IndexOf("PLT") != -1)
			return CRF_ESlottingCallsignTier.PLATOON;

		if (upper.IndexOf("-") != -1)
			return CRF_ESlottingCallsignTier.SQUAD;

		return CRF_ESlottingCallsignTier.OTHER;
	}

	//------------------------------------------------------------------------------------------------
	//! Renders a faction's slotting groups nested by tier (Company > Platoon > Squad) inferred from
	//! callsign, using array order to track which company/platoon is "current" as we walk the list -
	//! matching how the Quick Slot Setup tool orders company/platoon/squad entries.
	protected void AppendFactionSlotting(array<string> lines, string factionName, array<ref CRF_SlottingGroup> factionSlots, CRF_ERespawnMode respawnMode, CRF_RolesConfig rolesConfig)
	{
		if (factionSlots.IsEmpty())
			return;

		bool slotBased = (respawnMode == CRF_ERespawnMode.SLOT);

		lines.Insert(string.Format("## Slotting - %1 (%2 slots)", factionName, GetPlayerCount(factionSlots)));

		bool hasActiveCompany = false;
		bool hasActivePlatoon = false;

		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
		{
			CRF_ESlottingCallsignTier tier = GetCallsignTier(slotGroup.m_sCallsign);
			int depth = 0;

			switch (tier)
			{
				case CRF_ESlottingCallsignTier.COMPANY:
					depth = 0;
					hasActiveCompany = true;
					hasActivePlatoon = false;
					break;

				case CRF_ESlottingCallsignTier.PLATOON:
					if (hasActiveCompany)
						depth = 1;
					hasActivePlatoon = true;
					break;

				case CRF_ESlottingCallsignTier.SQUAD:
					if (hasActivePlatoon)
					{
						depth = 1;
						if (hasActiveCompany)
							depth = 2;
					}
					break;

				// CRF_ESlottingCallsignTier.OTHER: unrecognized/custom callsign, always top-level -
				// doesn't reset hasActiveCompany/hasActivePlatoon, so it doesn't break the chain
				// (e.g. a standalone vehicle crew group listed between two squads of the same platoon).
			}

			AppendSlottingGroup(lines, slotGroup, depth, slotBased, rolesConfig);
		}

		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	//! Prints one squad/platoon/company as a nested bullet list entry at the given indent depth.
	//! Uses indented "-" bullets (not heading levels) throughout so the hierarchy renders correctly
	//! both on GitHub (PR file diff) and in Discord (which only supports up to H3 headings).
	protected void AppendSlottingGroup(array<string> lines, CRF_SlottingGroup slotGroup, int depth, bool slotBased, CRF_RolesConfig rolesConfig)
	{
		string indent = "";
		for (int i = 0; i < depth; i++)
			indent = indent + "  ";
		string roleIndent = indent + "  ";

		string flagTypeName = SCR_Enum.GetEnumName(CRF_EFlagType, slotGroup.m_FlagType);
		lines.Insert(string.Format("%1- **%2** (%3)", indent, slotGroup.m_sCallsign, flagTypeName));

		if (slotBased && slotGroup.m_eRespawnPoolType == CRF_ERespawnPoolType.PER_GROUP)
			lines.Insert(string.Format("%1- Respawn Pool (shared): %2", roleIndent, TicketToString(slotGroup.m_iGroupRespawns)));

		// Tally role counts within this squad
		map<CRF_EGearRole, int> roleTally = new map<CRF_EGearRole, int>();
		array<CRF_EGearRole> roleOrder = {};
		foreach (CRF_EGearRole role : slotGroup.m_aSlots)
		{
			if (!roleTally.Contains(role))
			{
				roleTally.Set(role, 0);
				roleOrder.Insert(role);
			}

			roleTally.Set(role, roleTally.Get(role) + 1);
		}

		foreach (CRF_EGearRole role : roleOrder)
		{
			string roleName = SCR_Enum.GetEnumName(CRF_EGearRole, role);
			if (rolesConfig)
			{
				CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				if (roleConfig && !roleConfig.m_sRoleName.IsEmpty())
					roleName = roleConfig.m_sRoleName;
			}

			string roleLine = string.Format("%1- %2 x%3", roleIndent, roleName, roleTally.Get(role));

			if (slotBased && slotGroup.m_eRespawnPoolType == CRF_ERespawnPoolType.PER_SLOT)
				roleLine = roleLine + string.Format(" (%1 respawns each)", TicketToString(slotGroup.GetRoleRespawnCount(role)));

			lines.Insert(roleLine);
		}
	}
}

//------------------------------------------------------------------------------------------------
//! Read-only preview window for the generated mission synopsis. The text is already on the
//! clipboard by the time this opens - this just lets the mission maker see/re-select it.
class CRF_MissionSynopsisDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline, desc: "Mission synopsis - already copied to clipboard. Paste this into your PR description.")]
	string m_sSynopsis;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close", true)]
	protected bool ButtonClose()
	{
		return true;
	}
}
#endif