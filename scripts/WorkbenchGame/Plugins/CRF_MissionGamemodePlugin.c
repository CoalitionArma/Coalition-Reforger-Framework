#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "3 | Configure Mission Settings", 
	description: "Configure Mission Gamemode Settings", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0033)
] 
class CRF_MissionGamemodePlugin : WorkbenchPlugin
{	
	[Attribute("45", "auto", "Mission Time (Minutes) (set to -1 to disable)", category: "CRF Mission Config - General")]
	int m_iMissionTimeLimit;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Mission Config - General")]
	bool m_bMissionAllowsEspionage;

	[Attribute("true", "auto", "Should we lock all JIP slots after SafeStart turns off? COOP = FALSE", category: "CRF Mission Config - General")]
	bool m_bLockUnusedSlots;
	
	[Attribute("60", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Mission Config - Respawn")]
	int m_iTimeToRespawn;
	
	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bRespawnEnabled;

	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bWaveRespawn;
	
	[Attribute("0", UIWidgets.EditBox, "Amount of BLUFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Mission Config - Respawn")]
	int m_iBLUFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of OPFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Mission Config - Respawn")]
	int m_iOPFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Mission Config - Respawn")]
	int m_iINDFORTickets;

	[Attribute("0", UIWidgets.EditBox, "Amount of CIVILIAN Tickets. 0 = disabled/-1 = unlimited", category: "CRF Mission Config - Respawn")]
	int m_iCIVTickets;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		m_iMissionTimeLimit = gamemode.m_iMissionTimeLimit;
		m_bMissionAllowsEspionage = gamemode.m_bMissionAllowsEspionage;
		m_bLockUnusedSlots = gamemode.m_bLockUnusedSlots;
		m_bRespawnEnabled = gamemode.m_bRespawnEnabled;
		m_bWaveRespawn = gamemode.m_bWaveRespawn;
		m_iTimeToRespawn = gamemode.m_iTimeToRespawn;
		
		if (!Workbench.ScriptDialog(
		"Mission Gamemode Settings", 
		"This is a simplified settings window for settings that most mission makers will use, for more advanced settings go to the CRF_Lobby entitiy in your world file", 
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
		return true;
	}
}
#endif