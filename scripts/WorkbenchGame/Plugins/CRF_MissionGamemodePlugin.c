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
	
	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bRespawnEnabled;

	[Attribute("0", "auto", "", category: "CRF Mission Config - Respawn")]
	bool m_bWaveRespawn;

	[Attribute("60", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Mission Config - Respawn")]
	int m_iTimeToRespawn;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"This will automatically generate and sort the mission configuration file", 
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