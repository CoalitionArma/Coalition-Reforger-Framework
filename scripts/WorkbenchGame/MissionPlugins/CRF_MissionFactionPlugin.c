#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "2 | Configure Mission Factions", 
	description: "Configure Mission Factions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0032)
] 
class CRF_MissionFactionsPlugin : WorkbenchPlugin
{		
	//------------------------------------------------------------------------------------
	// BLU
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of BLUFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Config - BLUFOR"), RplProp()]
	int m_iBLUFORTickets;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Faction Config - BLUFOR")]
	ref CRF_SimplifiedGearScriptContainer m_BLUFORGearScriptSettings;

	//------------------------------------------------------------------------------------
	// OPF
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of OPFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Config - OPFOR"), RplProp()]
	int m_iOPFORTickets;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Faction Config - OPFOR")]
	ref CRF_SimplifiedGearScriptContainer m_OPFORGearScriptSettings;
	
	//------------------------------------------------------------------------------------
	// IND
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Config - INDFOR"), RplProp()]
	int m_iINDFORTickets;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Faction Config - INDFOR")]
	ref CRF_SimplifiedGearScriptContainer m_INDFORGearScriptSettings;

	//------------------------------------------------------------------------------------
	// CIV
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of CIVILIAN Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Config - CIVILIAN"), RplProp()]
	int m_iCIVTickets;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Faction Config - CIVILIAN")]
	ref CRF_SimplifiedGearScriptContainer m_CIVILIANGearScriptSettings;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Faction Editor", 
		"This allows you to change all mission faction related settings", 
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
		
		return true;
	}
}
#endif