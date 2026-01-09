#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "1 | Configure Mission Factions", 
	description: "Configure Mission Factions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0031)
] 
class CRF_MissionFactionsPlugin : WorkbenchPlugin
{	
	[Attribute("1", "auto", "", category: "CRF Mission Config - Slotting Ratio")]
	int m_iFactionOneRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Mission Config - Slotting Ratio")]
	string m_sFactionOneKey;

	[Attribute("1", "auto", "", category: "CRF Mission Config - Slotting Ratio")]
	int m_iFactionTwoRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Mission Config - Slotting Ratio")]
	string m_sFactionTwoKey;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Layer Generator", 
		"This Automatically generates the missions layers to keep consistancy across all CRF missions", 
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