#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "2 | Configure Settings", 
	description: "Configure Mission Gamemode Settings", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF013)
] 
class CRF_MissionGamemodePlugin : WorkbenchPlugin
{	
	[Attribute("45", "auto", "Mission Time (Minutes) (set to -1 to disable)", category: "CRF Mission Settings - General")]
	protected int m_iMissionTimeLimit;
	
	[Attribute("false", "auto", "Enable safestart time limit countdown (forces mission start after time expires)", category: "CRF Mission Settings - Safestart")]
	protected bool m_bUseSafestartTimeLimit;
	
	[Attribute("10", UIWidgets.EditBox, "Safestart Time Limit (Minutes) - Only used if time limit is enabled", category: "CRF Mission Settings - Safestart")]
	protected int m_iSafestartTimeLimit;
	
	[Attribute("true", "auto", "Disables JIP (Join In Progress). When enabled, all non-slotted slots lock once SafeStart turns off, so late joiners can't take an empty seat. When disabled, late joiners may forward deploy to their unit's live position instead. COOP = FALSE", category: "CRF Mission Settings - General")]
	protected bool m_bDisableJIP;
	
	[Attribute("60", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Mission Settings - Respawn")]
	protected int m_iTimeToRespawn;
	
	[Attribute("0", "auto", "", category: "CRF Mission Settings - Respawn")]
	protected bool m_bRespawnEnabled;
	
	[Attribute("0", "auto", "", category: "CRF Mission Settings - Respawn")]
	protected bool m_bRallyPointsEnabled;

	[Attribute("0", "auto", "", category: "CRF Mission Settings - Respawn")]
	protected bool m_bWaveRespawn;

	[Attribute("0", UIWidgets.EditBox, "Minutes before mission end when respawns disable (0 = never disable)", category: "CRF Mission Settings - Respawn")]
	protected int m_iRespawnCutoffMinutes;

	[Attribute("0", UIWidgets.SearchComboBox, "Team-Based shares one ticket pool per faction (configured on the Configure Factions plugin). Slot-Based gives each squad's roles their own respawn counts (configured per-squad on the Configure Slots plugin).", enums: ParamEnumArray.FromEnum(CRF_ERespawnMode), category: "CRF Mission Settings - Respawn")]
	protected CRF_ERespawnMode m_eRespawnMode;
	
	[Attribute("", desc: "Starting Weather", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("Clear", "Clear"), ParamEnum("Cloudy", "Cloudy"), ParamEnum("Overcast", "Overcast"), ParamEnum("Rainy", "Rainy")}, category: "CRF Mission Settings - Weather & Time")]
	protected string m_sMissionWeather;
	
	[Attribute("12", UIWidgets.Slider, desc: "Starting time of day (hour)", "0 23 1", category: "CRF Mission Settings - Weather & Time")]
	protected int m_iStartingHour;

	[Attribute("0", UIWidgets.Slider, "Starting time of day (minutes)", "0 59 1", category: "CRF Mission Settings - Weather & Time")]
	protected int m_iStartingMinutes;
	
	[Attribute("0", category: "CRF Mission Settings - Weather & Time")]
	protected bool m_bRandomStartingWeather;
	
	[Attribute("0", desc: "Weather can change during gameplay", category: "CRF Mission Settings - Weather & Time")]
	protected bool m_bRandomWeatherChanges;

	[Attribute("1", UIWidgets.Slider, "Time scale applied once the mission starts (after briefing/slotting). 1 = normal speed, 2 = twice as fast, etc.", "0.1 12 0.1", category: "CRF Mission Settings - Weather & Time")]
	protected float m_fMissionTimeScale;

	[Attribute("true", desc: "Use Coalition VON (CVON) for voice communication via TeamSpeak. Uncheck to use the default Arma Reforger in-game VON instead.", category: "CRF Mission Settings - VON")]
	protected bool m_bUseCVON;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby"); // Getting the IEntitySource of the lobby, this allows us to set variables on the lobby just like if you were changing Object Properties.
		
		if (!entitySource)
			return;		
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource)); // Get the actual Gamemode so we can pull any variables we need.
		
		//Gamemode
		m_iMissionTimeLimit = gamemode.m_iTimeLimitMinutes;
		m_bDisableJIP = gamemode.m_bLockUnusedSlots;
		m_bRespawnEnabled = gamemode.m_bRespawnEnabled;
		m_bRallyPointsEnabled = gamemode.m_bRallyPointsEnabled;
		m_bWaveRespawn = gamemode.m_bWaveRespawn;
		m_iTimeToRespawn = gamemode.m_iTimeToRespawn;
		m_iRespawnCutoffMinutes = gamemode.m_iRespawnCutoffMinutes;
		m_eRespawnMode = gamemode.m_eRespawnMode;
		m_bUseSafestartTimeLimit = gamemode.m_bUseSafestartTimeLimit;
		m_iSafestartTimeLimit = gamemode.m_iSafestartTimeLimit;
		m_bUseCVON = gamemode.m_bUseCVON;
		m_fMissionTimeScale = gamemode.m_fMissionTimeScale;

		// Weather
		SCR_TimeAndWeatherHandlerComponent timeAndWeatherComp = SCR_TimeAndWeatherHandlerComponent.Cast(gamemode.FindComponent(SCR_TimeAndWeatherHandlerComponent));
		
		m_bRandomStartingWeather = timeAndWeatherComp.GetRandomStartingWeather();
		m_bRandomWeatherChanges = timeAndWeatherComp.GetRandomWeatherChanges();
		
		m_sMissionWeather = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetWeatherPresetName();
		m_iStartingHour = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingHour();
		m_iStartingMinutes = timeAndWeatherComp.GetStartingWeatherAndTime()[0].GetStartingMinutes();
		
		// Actually shows the window
		if (!Workbench.ScriptDialog(
		"Mission Gamemode Settings", 
		"This is a settings window for basic settings that most mission makers will use. \n - Advanced Gamemode Settings can be found on the CRF_Lobby entity object properties. \n - Advanced Weather Settings can be found on the SCR_TimeAndWeatherHandlerComponent in the lobby's object properties.", 
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
	[ButtonAttribute("Apply Gamemode Settings", true)]
	protected bool ButtonNext()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;
		
		WBProgressDialog progress = new WBProgressDialog("Processing...", worldEditor); // Show a simple processing window.
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby"); // Getting the IEntitySource of the lobby, this allows us to set variables on the lobby just like if you were changing Object Properties.
		
		if (!entitySource)
			return false;
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource)); // Get the actual Gamemode so we can pull any variables we need.
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		// BEGIN EDIT ACTION ON ENTITY
		//-----------------------------------------------------
		
		//Gamemode
		api.SetVariableValue(entitySource, null, "m_iTimeLimitMinutes", m_iMissionTimeLimit.ToString());
		api.SetVariableValue(entitySource, null, "m_bLockUnusedSlots", m_bDisableJIP.ToString());
		api.SetVariableValue(entitySource, null, "m_bRespawnEnabled", m_bRespawnEnabled.ToString());
		api.SetVariableValue(entitySource, null, "m_bRallyPointsEnabled", m_bRallyPointsEnabled.ToString());
		api.SetVariableValue(entitySource, null, "m_bWaveRespawn", m_bWaveRespawn.ToString());
		api.SetVariableValue(entitySource, null, "m_iTimeToRespawn", m_iTimeToRespawn.ToString());
		api.SetVariableValue(entitySource, null, "m_iRespawnCutoffMinutes", m_iRespawnCutoffMinutes.ToString());
		api.SetVariableValue(entitySource, null, "m_eRespawnMode", m_eRespawnMode.ToString());
		api.SetVariableValue(entitySource, null, "m_bUseSafestartTimeLimit", m_bUseSafestartTimeLimit.ToString());
		api.SetVariableValue(entitySource, null, "m_iSafestartTimeLimit", m_iSafestartTimeLimit.ToString());
		api.SetVariableValue(entitySource, null, "m_bUseCVON", m_bUseCVON.ToString());
		api.SetVariableValue(entitySource, null, "m_fMissionTimeScale", m_fMissionTimeScale.ToString());

		// Weather
		int componentIndex = SCR_BaseContainerTools.FindComponentIndex(entitySource, SCR_TimeAndWeatherHandlerComponent);
		array<ref ContainerIdPathEntry> path = {new ContainerIdPathEntry("SCR_TimeAndWeatherHandlerComponent", componentIndex)};
		
		api.SetVariableValue(entitySource, path, "m_bRandomWeatherChanges", m_bRandomWeatherChanges.ToString());	
		api.SetVariableValue(entitySource, path, "m_bRandomStartingWeather", m_bRandomStartingWeather.ToString());	
		
		path.Insert(ContainerIdPathEntry("m_aStartingWeatherAndTime", 0));
		api.SetVariableValue(entitySource, path, "m_sWeatherPresetName", m_sMissionWeather);	
		api.SetVariableValue(entitySource, path, "m_iStartingHour", m_iStartingHour.ToString());	
		api.SetVariableValue(entitySource, path, "m_iStartingMinutes", m_iStartingMinutes.ToString());	
		
		//-----------------------------------------------------
		// END EDIT ACTION ON ENTITY
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
}
#endif