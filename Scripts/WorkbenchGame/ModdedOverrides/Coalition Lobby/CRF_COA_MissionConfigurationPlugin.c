#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! Callsign-inferred ORBAT tier for the mission synopsis' Slotting section (see GetCallsignTier()).
//! Internal to the synopsis generator - not a runtime/gameplay concept, so it's kept out of COA_Enums.c.
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
modded class COA_MissionConfigurationPlugin
{
	[Attribute("", "auto", "Your BI account GUID for automatic admin privileges (auto-filled from workbench)", category: "CRF Mission Config - Mission Info")]
	protected string m_sMissionAuthorGUID;

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
		
		m_MissionMode = COA_EGamemode.TVT;
		m_sMissionName = "<Name>";
		m_sMissionDescription = "<Description>";

		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"This will automatically generate and sort the mission configuration file. \n\n WARNING: DO NOT RUN THIS TWICE FOR ONE MISSION, SIMPLY GO TO THE ALREADY CREATED CONFIG AND MANUALLY UPDATE IT.", 
		this))
		
			return;
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

		IEntitySource entitySource = api.FindEntityByName("COA_Lobby");
		if (!entitySource)
			return false;

		COA_Gamemode gamemode = COA_Gamemode.Cast(api.SourceToEntity(entitySource));
		if (!gamemode)
			return false;

		string missionMode = SCR_Enum.GetEnumName(COA_EGamemode, m_MissionMode);

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
	protected override bool ButtonNext()
	{
		string missionMode = SCR_Enum.GetEnumName(COA_EGamemode, m_MissionMode);
		int missionPlayercount;
		string worldPath;

		//--- Get mission header from the template config (can't use the class directly, it's engine-controlled class that cannot have reference in script)
		Resource templateResource = Resource.Load("{3D094352621EA88C}Missions/COA_BaseMissionConfig.conf");
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

		IEntitySource entitySource = api.FindEntityByName("COA_Lobby");

		if (!entitySource)
			return false;

		COA_Gamemode gamemode = COA_Gamemode.Cast(api.SourceToEntity(entitySource));

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
	protected void ShowMissionSynopsis(COA_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
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
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION SYNOPSIS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Builds the full markdown mission synopsis from the mission's actual configured data.
	protected array<string> BuildMissionSynopsis(COA_Gamemode gamemode, IEntitySource entitySource, string missionDisplayName, string missionTerrain, string missionMode, int missionPlayercount)
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
	protected void AppendGeneralSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## General");

		if (!gamemode.m_sFactionOneKey.IsEmpty() && !gamemode.m_sFactionTwoKey.IsEmpty())
			lines.Insert(string.Format("- Slotting Ratio: %1 %2:%3 %4", gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey));

		string timeLimit = "No limit";
		if (gamemode.m_iTimeLimitMinutes > 0)
			timeLimit = string.Format("%1 minutes", gamemode.m_iTimeLimitMinutes);
		lines.Insert(string.Format("- Time Limit: %1", timeLimit));

		lines.Insert(string.Format("- Disable JIP: %1", BoolToYesNo(gamemode.m_bLockUnusedSlots)));

		string safestartLimit = "Disabled";
		if (gamemode.m_bUseSafestartTimeLimit)
			safestartLimit = string.Format("%1 minutes", gamemode.m_iSafestartTimeLimit);
		lines.Insert(string.Format("- Safestart Time Limit: %1", safestartLimit));

		lines.Insert(string.Format("- Mission Time Scale: %1x", gamemode.m_fMissionTimeScale));
		lines.Insert(string.Format("- Coalition VON (CVON): %1", BoolToYesNo(gamemode.m_bUseCVON)));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRespawnSection(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("## Respawn");
		lines.Insert(string.Format("- Respawn Enabled: %1", BoolToYesNo(gamemode.m_bRespawnEnabled)));

		if (!gamemode.m_bRespawnEnabled)
		{
			lines.Insert("");
			return;
		}

		bool slotBased = (gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT);

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
	protected void AppendGearscriptSection(array<string> lines, COA_Gamemode gamemode)
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
	protected string GearScriptToString(COA_GearScriptContainer gearScriptSettings)
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
	//! Detects which known CRF gamemode logic components are attached to COA_Lobby, on top of the
	//! single authored COA_EGamemode label. A mission can stack more than one (e.g. Rally + Attrition).
	protected void AppendGameModeComponentSection(array<string> lines, COA_Gamemode gamemode, IEntitySource entitySource, string missionMode)
	{
		lines.Insert("## Game Mode Components");
		lines.Insert(string.Format("- Selected Mode: %1", missionMode));

		array<string> activeComponents = {};

		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_AttritionGamemodeComponent) >= 0)
			activeComponents.Insert("Attrition");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_FrontlineGamemodeManager) >= 0)
			activeComponents.Insert("Frontline");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_HighValueTargetGamemodeManager) >= 0)
			activeComponents.Insert("High Value Target");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_InsurgencyGamemodeManager) >= 0)
			activeComponents.Insert("Insurgency");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RaidGamemodeComponent) >= 0)
			activeComponents.Insert("Raid");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RallyGamemodeComponent) >= 0)
			activeComponents.Insert("Rally");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RushGamemodeManager) >= 0)
			activeComponents.Insert("Rush");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_SearchAndDestroyGamemodeManager) >= 0)
			activeComponents.Insert("Search And Destroy");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_LooterGamemodeComponent) >= 0)
			activeComponents.Insert("Looter");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_PropHuntGamemode) >= 0)
			activeComponents.Insert("Prop Hunt");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_SupplyExtractionGamemodeManager) >= 0)
			activeComponents.Insert("Supply Extraction");

		string componentList = "None detected";
		if (!activeComponents.IsEmpty())
		{
			componentList = activeComponents.Get(0);
			for (int i = 1; i < activeComponents.Count(); i++)
				componentList = componentList + ", " + activeComponents.Get(i);
		}

		lines.Insert(string.Format("- Active Components: %1", componentList));
		lines.Insert("");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendWeatherSection(array<string> lines, COA_Gamemode gamemode)
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
	protected void AppendSlottingSection(array<string> lines, COA_Gamemode gamemode)
	{
		//--- Load the global roles config directly as a resource (not via COA_GearscriptManager.GetRolesConfig(),
		//--- which depends on a live runtime manager singleton that isn't guaranteed to exist in Workbench)
		COA_RolesConfig rolesConfig = COA_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{4388548E9F600148}Configs/Gearscripts/COA_Global_Roles_Config.conf").GetResource().ToBaseContainer()));

		AppendFactionSlotting(lines, "BLUFOR", gamemode.m_BluforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "OPFOR", gamemode.m_OpforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "INDFOR", gamemode.m_IndforSlots, gamemode.m_eRespawnMode, rolesConfig);
		AppendFactionSlotting(lines, "CIV", gamemode.m_CivSlots, gamemode.m_eRespawnMode, rolesConfig);
	}

	//------------------------------------------------------------------------------------------------
	//! Classifies a squad/platoon/company's tier from its callsign, using the same COY / NPLT / N-M
	//! naming convention the Quick Slot Setup tool's auto-numbering already produces (SetPluginQuickSlots
	//! in COA_MissionSlottingPlugin.c). Custom-renamed callsigns that don't match any pattern fall back
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
	protected void AppendFactionSlotting(array<string> lines, string factionName, array<ref COA_SlottingGroup> factionSlots, COA_ERespawnMode respawnMode, COA_RolesConfig rolesConfig)
	{
		if (factionSlots.IsEmpty())
			return;

		bool slotBased = (respawnMode == COA_ERespawnMode.SLOT);

		lines.Insert(string.Format("## Slotting - %1 (%2 slots)", factionName, GetPlayerCount(factionSlots)));

		bool hasActiveCompany = false;
		bool hasActivePlatoon = false;

		foreach (ref COA_SlottingGroup slotGroup : factionSlots)
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
	protected void AppendSlottingGroup(array<string> lines, COA_SlottingGroup slotGroup, int depth, bool slotBased, COA_RolesConfig rolesConfig)
	{
		string indent = "";
		for (int i = 0; i < depth; i++)
			indent = indent + "  ";
		string roleIndent = indent + "  ";

		string flagTypeName = SCR_Enum.GetEnumName(COA_EFlagType, slotGroup.m_FlagType);
		lines.Insert(string.Format("%1- **%2** (%3)", indent, slotGroup.m_sCallsign, flagTypeName));

		if (slotBased && slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_GROUP)
			lines.Insert(string.Format("%1- Respawn Pool (shared): %2", roleIndent, TicketToString(slotGroup.m_iGroupRespawns)));

		// Tally role counts within this squad
		map<COA_EGearRole, int> roleTally = new map<COA_EGearRole, int>();
		array<COA_EGearRole> roleOrder = {};
		foreach (COA_EGearRole role : slotGroup.m_aSlots)
		{
			if (!roleTally.Contains(role))
			{
				roleTally.Set(role, 0);
				roleOrder.Insert(role);
			}

			roleTally.Set(role, roleTally.Get(role) + 1);
		}

		foreach (COA_EGearRole role : roleOrder)
		{
			string roleName = SCR_Enum.GetEnumName(COA_EGearRole, role);
			if (rolesConfig)
			{
				COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
				if (roleConfig && !roleConfig.m_sRoleName.IsEmpty())
					roleName = roleConfig.m_sRoleName;
			}

			string roleLine = string.Format("%1- %2 x%3", roleIndent, roleName, roleTally.Get(role));

			if (slotBased && slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_SLOT)
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