#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "4 | Configure Mission Descriptions", 
	description: "Configure Mission Descriptions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0034)
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
		
		if (!entitySource)
			return;		
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		m_aMissionDescriptors.Clear();
		
		for ( int i = 2; i < gamemode.m_aMissionDescriptors.Count(); i++ )
		{
			m_aMissionDescriptors.Insert( gamemode.m_aMissionDescriptors.Get(i) );
		}
		
		if (!Workbench.ScriptDialog(
		"Mission Descriptions Editor", 
		"This allows you to edit all mission descriptions", 
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
		
		WBProgressDialog progress = new WBProgressDialog("Processing...", Workbench.GetModule(WorldEditor));
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		
		if (!entitySource)
			return false;
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		
		for (int i; i < m_aMissionDescriptors.Count(); i++ )
		{
			array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry("m_aMissionDescriptors", i + 2)};
			CRF_MissionDescriptor descriptor = m_aMissionDescriptors[i];
			
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
		
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
}
#endif