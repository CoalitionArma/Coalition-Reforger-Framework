#ifdef WORKBENCH
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

		//--- Create the mission synopsis (markdown, committed alongside the header .conf so it rides along in the mission's PR)
		string missionDisplayName = string.Format("CRF %1%2 %3", missionMode, missionPlayercount, m_sMissionName);
		array<string> synopsisLines = BuildMissionSynopsis(gamemode, entitySource, missionDisplayName, missionTerrain, missionMode, missionPlayercount);

		string missionSynopsisPath = FilePath.AppendExtension(missionBasePath, "md");
		string missionSynopsisAbsPath;
		Workbench.GetAbsolutePath(missionSynopsisPath, missionSynopsisAbsPath, false);

		FileHandle synopsisFile = FileIO.OpenFile(missionSynopsisAbsPath, FileMode.WRITE);
		if (synopsisFile)
		{
			foreach (string line : synopsisLines)
				synopsisFile.WriteLine(line);

			synopsisFile.Close();
			resourceManager.RegisterResourceFile(missionSynopsisAbsPath, false);
		}
		else
		{
			Print(string.Format("Unable to create mission synopsis at %1!", missionSynopsisPath), LogLevel.ERROR);
		}

		return true;
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

		lines.Insert(string.Format("# %1", missionDisplayName));
		lines.Insert("");
		lines.Insert(string.Format("**Author:** %1  **Terrain:** %2  **Game Mode:** %3  **Player Count:** %4", m_sMissionAuthor, missionTerrain, missionMode, missionPlayercount));
		lines.Insert("");

		AppendGeneralSection(lines, gamemode);
		AppendRespawnSection(lines, gamemode);
		AppendGearscriptSection(lines, gamemode);
		AppendGameModeComponentSection(lines, gamemode, entitySource, missionMode);
		AppendWeatherSection(lines, gamemode);
		AppendSlottingSection(lines, gamemode);

		return lines;
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendGeneralSection(array<string> lines, CRF_Gamemode gamemode)
	{
		lines.Insert("## General");

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
	//! single authored CRF_EGamemode label. A mission can stack more than one (e.g. Rally + Attrition).
	protected void AppendGameModeComponentSection(array<string> lines, CRF_Gamemode gamemode, IEntitySource entitySource, string missionMode)
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
	protected void AppendFactionSlotting(array<string> lines, string factionName, array<ref CRF_SlottingGroup> factionSlots, CRF_ERespawnMode respawnMode, CRF_RolesConfig rolesConfig)
	{
		if (factionSlots.IsEmpty())
			return;

		bool slotBased = (respawnMode == CRF_ERespawnMode.SLOT);

		lines.Insert(string.Format("## Slotting — %1 (%2 slots)", factionName, GetPlayerCount(factionSlots)));

		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
		{
			string flagTypeName = SCR_Enum.GetEnumName(CRF_EFlagType, slotGroup.m_FlagType);
			lines.Insert(string.Format("### %1 (%2)", slotGroup.m_sCallsign, flagTypeName));

			if (slotBased && slotGroup.m_eRespawnPoolType == CRF_ERespawnPoolType.PER_GROUP)
				lines.Insert(string.Format("- Respawn Pool (shared): %1", TicketToString(slotGroup.m_iGroupRespawns)));

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

				string roleLine = string.Format("- %1 x%2", roleName, roleTally.Get(role));

				if (slotBased && slotGroup.m_eRespawnPoolType == CRF_ERespawnPoolType.PER_SLOT)
					roleLine = roleLine + string.Format(" (%1 respawns each)", TicketToString(slotGroup.GetRoleRespawnCount(role)));

				lines.Insert(roleLine);
			}
		}

		lines.Insert("");
	}
}
#endif