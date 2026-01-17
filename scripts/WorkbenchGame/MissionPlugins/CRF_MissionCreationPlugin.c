#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Automatically Create Initial Layers/Objects", 
	description: "Automatically Generate Mission Layers", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF1B3)
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
	[ButtonAttribute("Generate Layers/Objects", true)]
	protected bool ButtonNext()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;
		
		WorldEditorAPI api = worldEditor.GetApi();
		
		api.CreateSubsceneLayer(1, "_INIT");
		api.CreateSubsceneLayer(1, "SPAWNPOINTS");
		api.CreateSubsceneLayer(1, "SPAWNPOINTSGROUPS");
		api.CreateSubsceneLayer(1, "OBJECTIVES");
		api.CreateSubsceneLayer(1, "POLYZONES");
		api.CreateSubsceneLayer(1, "VEHICLES");
		api.CreateSubsceneLayer(1, "PROPS");
		
		api.SetActiveSubsceneLayer(1, "_INIT");

		api.CreateEntity("{6A996BBFCEB37E78}Prefabs/MP/Modes/Lobby/CRF_Lobby.et", "CRF_Lobby", 1, null, vector.Zero, vector.Zero);
		
		worldEditor.Save();
		
		string worldPath;
		api.GetWorldPath(worldPath);
		
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		string absWorldPath;
		Workbench.GetAbsolutePath(worldPath, absWorldPath);
		MetaFile worldMeta = resourceManager.GetMetaFile(absWorldPath);
		string fullWorldPath = worldMeta.GetResourceID();
		
		worldEditor.SetOpenedResource(fullWorldPath);
		
		return true;
	}
}
#endif