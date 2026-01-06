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