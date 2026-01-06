#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "2 | Configure Mission Descriptions", 
	description: "Configure Mission Descriptions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0032)
] 
class CRF_MissionDescriptionsPlugin : WorkbenchPlugin
{	
	[Attribute("", category: "CRF Mission Config - General")]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		m_aMissionDescriptors = gamemode.m_aMissionDescriptors;
		
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
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		IEntityComponentSource component = entitySource.GetComponent(0);
		
		component.Set("m_aMissionDescriptors", m_aMissionDescriptors);
		
		return true;
	}
}
#endif