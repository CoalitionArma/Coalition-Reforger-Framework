#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Generate Mission World", 
	description: "Automatically Generate Mission World", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0031)
] 
class CRF_MissionWorldCreationPlugin : WorkbenchPlugin
{	
	[Attribute(defvalue: "{1EA95DAE3230BEB0}worlds/Cain/Cain.ent", uiwidget: UIWidgets.ResourceNamePicker, params: "ent")]
	protected ResourceName m_sBaseWorld;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		"Welcome to step-by-step setup of a CRF mission.\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY", 
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
		
		
		
		
		
		
		return true;
	}
}
#endif