#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "1 | Configure Mission Settings", 
	description: "Configure Mission Gamemode Settings", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0031)
] 
class CRF_MissionGamemodePlugin : WorkbenchPlugin
{	
	[Attribute("45", "auto", "Mission Time (Minutes) (set to -1 to disable)", category: "CRF Mission Config - General")]
	int m_iMissionTimeLimit;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Mission Config - General")]
	bool m_bMissionAllowsEspionage;

	[Attribute("true", "auto", "If safestart turns on instantly after the lobby screen.", category: "CRF Mission Config - General")]
	bool m_bSafestartEnabledOnMissionStart;
	
	[Attribute("true", "auto", "Should we lock all non-slotted slots after SafeStart turns off? COOP = FALSE", category: "CRF Mission Config - General")]
	bool m_bLockUnusedSlots;
	
	[Attribute("60", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Mission Config - Respawn")]
	int m_iTimeToRespawn;
	
	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bRespawnEnabled;

	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bWaveRespawn;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");

		if (!entitySource)
			return;		

		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		m_iMissionTimeLimit = gamemode.m_iTimeLimitMinutes;
		m_bMissionAllowsEspionage = gamemode.m_bAllowEspionage;
		m_bLockUnusedSlots = gamemode.m_bLockUnusedSlots;
		m_bRespawnEnabled = gamemode.m_bRespawnEnabled;
		m_bWaveRespawn = gamemode.m_bWaveRespawn;
		m_iTimeToRespawn = gamemode.m_iTimeToRespawn;
		m_bSafestartEnabledOnMissionStart = gamemode.m_bSafestartInstantlyEnabled;
		
		if (!Workbench.ScriptDialog(
		"Mission Gamemode Settings", 
		"This is a settings window for settings that most mission makers will use, for more advanced settings go to the CRF_Lobby entitiy in your world file", 
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
	[ButtonAttribute("Next", true)]
	protected bool ButtonNext()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;
		
		WBProgressDialog progress = new WBProgressDialog("Processing...", Workbench.GetModule(WorldEditor));
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		
		api.SetVariableValue(entitySource, null, "m_iTimeLimitMinutes", m_iMissionTimeLimit.ToString());
		api.SetVariableValue(entitySource, null, "m_bAllowEspionage", m_bMissionAllowsEspionage.ToString());
		api.SetVariableValue(entitySource, null, "m_bLockUnusedSlots", m_bLockUnusedSlots.ToString());
		api.SetVariableValue(entitySource, null, "m_bRespawnEnabled", m_bRespawnEnabled.ToString());
		api.SetVariableValue(entitySource, null, "m_bWaveRespawn", m_bWaveRespawn.ToString());
		api.SetVariableValue(entitySource, null, "m_iTimeToRespawn", m_iTimeToRespawn.ToString());
		api.SetVariableValue(entitySource, null, "m_bSafestartInstantlyEnabled", m_bSafestartEnabledOnMissionStart.ToString());		
		
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
}
#endif