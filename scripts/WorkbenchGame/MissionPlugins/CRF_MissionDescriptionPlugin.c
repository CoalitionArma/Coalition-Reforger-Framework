#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "4 | Configure Descriptions", 
	description: "Configure Mission Descriptions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0034)
] 
class CRF_MissionDescriptionsPlugin : WorkbenchPlugin
{	
	[Attribute("", category: "CRF Mission Config - General")]
	protected ref array<ref CRF_MissionDescriptor> m_aMissionDescriptors;
	
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
	[ButtonAttribute("Apply Mission Descriptions", true)]
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
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		
		for (int r; r < gamemode.m_aMissionDescriptors.Count(); r++)
			api.RemoveObjectArrayVariableMember(entitySource, null, "m_aMissionDescriptors", 0);
		
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

		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
}
#endif