#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Configure Descriptions", 
	description: "Configure Mission Descriptions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF15C)
] 
class CRF_MissionDescriptionsPlugin : WorkbenchPlugin
{	
	[Attribute("", category: "CRF Mission Settings - Descriptors")]
	protected ref array<ref CRF_MissionDescriptor> m_aMissionDescriptors;
	
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby"); // Getting the IEntitySource of the lobby, this allows us to set variables on the lobby just like if you were changing Object Properties.
		
		if (!entitySource)
			return;		
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource)); // Get the actual Gamemode so we can pull any variables we need.
		
		m_aMissionDescriptors.Clear(); // Due to attributes in plugins being saved when the plugin is closed, we need to clear the array each time so the user gets the same attributes each time.
		
		// Dont want to show the controls or welcome descriptors since those should rarely change, so we simply offset and insert the gamemodes descriptors into the array.
		for ( int i = 2; i < gamemode.m_aMissionDescriptors.Count(); i++ )
			m_aMissionDescriptors.Insert( gamemode.m_aMissionDescriptors.Get(i) );
		
		// Actually shows the window
		if (!Workbench.ScriptDialog(
		"Mission Descriptions Editor", 
		"This allows you to edit all mission descriptions.", 
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
	[ButtonAttribute("Apply Mission Descriptions", true)]
	protected bool ButtonNext()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return false;
		
		WBProgressDialog progress = new WBProgressDialog("Processing...", worldEditor); // Show a simple processing window.
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby"); // Getting the IEntitySource of the lobby, this allows us to set variables on the lobby just like if you were changing Object Properties.
		
		if (!entitySource)
			return false;
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource)); // Get the actual Gamemode so we can pull any variables we need.
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		// BEGIN EDIT ACTION ON ENTITY
		//-----------------------------------------------------
		
		foreach (int i, CRF_MissionDescriptor descriptor : m_aMissionDescriptors)
		{
			int descriptorOffset = i + 2;
			array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry("m_aMissionDescriptors", descriptorOffset)};
			
			if (descriptorOffset > (gamemode.m_aMissionDescriptors.Count() - 1))
				api.CreateObjectArrayVariableMember(entitySource, null, "m_aMissionDescriptors", "CRF_MissionDescriptor", descriptorOffset);
			
			api.SetVariableValue(entitySource, path, "m_sTitle", descriptor.m_sTitle);
			api.SetVariableValue(entitySource, path, "m_sTextData", descriptor.m_sTextData);
			api.SetVariableValue(entitySource, path, "m_bShowForAnyFaction", descriptor.m_bShowForAnyFaction.ToString());
			
			string finalFactionsArrayStr;
			foreach (int f, FactionKey factionKey : descriptor.m_aFactionKeys)
			{
				if (f == 0)
					finalFactionsArrayStr = factionKey;
				else	
					finalFactionsArrayStr = finalFactionsArrayStr + ", " + factionKey;
			}
			
			api.SetVariableValue(entitySource, path, "m_aFactionKeys", finalFactionsArrayStr);
		}
		
		//-----------------------------------------------------
		// END EDIT ACTION ON ENTITY
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
}
#endif