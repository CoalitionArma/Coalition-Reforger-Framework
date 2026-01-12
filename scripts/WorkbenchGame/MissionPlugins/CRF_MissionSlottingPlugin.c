#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "3 | Configure Mission Slots", 
	description: "Configure Mission Slots", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0033)
] 
class CRF_MissionSlottingPlugin : WorkbenchPlugin
{	
	[Attribute("1", "auto", "", category: "CRF Slotting Config - Slotting Ratio")]
	int m_iFactionOneRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Config - Slotting Ratio")]
	string m_sFactionOneKey;

	[Attribute("1", "auto", "", category: "CRF Slotting Config - Slotting Ratio")]
	int m_iFactionTwoRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Config - Slotting Ratio")]
	string m_sFactionTwoKey;
	
	//------------------------------------------------------------------------------------------------
	// BLU
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// OPF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// IND
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// CIV
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Slotting Editor", 
		"This allows you to change mission slots/slotting ratios", 
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