#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Generate Config File", 
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
		m_MissionMode = CRF_EGamemode.TVT;
		m_sMissionName = "<Name>";
		m_sMissionDescription = "<Description>";
		
		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"This will automatically generate and sort the mission configuration file.\nPlease do NOT include any special characters in your text for any input, this can cause issues. \n\n WARNING: DO NOT RUN THIS TWICE FOR ONE MISSION, SIMPLY GO TO THE ALREADY CREATED CONFIG AND MANUALLY UPDATE IT.", 
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
		Resource templateResource = Resource.Load("{3D094352621EA88C}Missions/ACRF_BaseMissionConfig.conf");
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
		missionHeaderContainer.Set("m_iPlayerCount", missionPlayercount);
		
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
		
		string missionHeaderPath = FilePath.Concat(relativeDirPath, string.Format("%1_%2_%3%4_%5%6_%7", m_sMissionAuthor, missionTerrain, monthFinal, dayFinal, missionMode, missionPlayercount, m_sMissionName));
		missionHeaderPath = FilePath.AppendExtension(missionHeaderPath, "conf");

		//--- Create the config
		if (!BaseContainerTools.SaveContainer(missionHeaderContainer, ResourceName.Empty, missionHeaderPath))
		{
			Print(string.Format("Unable to create mission header at %1!", missionHeaderPath), LogLevel.ERROR);
			return false;
		}
		
		worldEditor.Close();
		
		return true;
	}
	
	protected int GetPlayerCount(array<ref CRF_SlottingGroup> factionSlots)
	{
		int missionPlayercount;
		
		foreach (ref CRF_SlottingGroup slotGroup : factionSlots)
			foreach(CRF_EGearRole role : slotGroup.m_aSlots)
				missionPlayercount++;
		
		return missionPlayercount;
	}
}
#endif