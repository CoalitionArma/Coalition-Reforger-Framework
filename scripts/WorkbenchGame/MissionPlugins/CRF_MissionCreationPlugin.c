#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "0 | Spawn Initial Mission Layers/Objects", 
	description: "Automatically Generate Mission Layers", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0030)
] 
class CRF_MissionWorldCreationPlugin : WorkbenchPlugin
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
		
		WorldEditorAPI api = worldEditor.GetApi();
		
		api.CreateSubsceneLayer(1, "_INIT");
		api.CreateSubsceneLayer(1, "SPAWNPOINTS");
		api.CreateSubsceneLayer(1, "OBJECTIVES");
		api.CreateSubsceneLayer(1, "POLYZONES");
		api.CreateSubsceneLayer(1, "VEHICLES");
		api.CreateSubsceneLayer(1, "PROPS");
		
		api.SetActiveSubsceneLayer(1, "_INIT");
		
		worldEditor.Save();
		
		return true;
	}
}
#endif