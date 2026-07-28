#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "3 | Configure Factions", 
	description: "Configure Mission Factions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF024)
] 
modded class COA_MissionFactionsPlugin
{
	//------------------------------------------------------------------------------------------------
	protected override void SetPluginGearscriptVariables(COA_SimplifiedGearScriptContainer pluginGearscript, COA_GearScriptContainer gearscript)
	{			
        super.SetPluginGearscriptVariables(pluginGearscript, gearscript);
		pluginGearscript.m_bEnableShareableMarkers = gearscript.m_bEnableShareableMarkers;
	}

	//------------------------------------------------------------------------------------------------
	protected override void SetGamemodeGearscriptVariables(WorldEditorAPI api, IEntitySource entitySource, COA_SimplifiedGearScriptContainer pluginGSContainer, string gearscriptContainterToChange)
	{	
        super.SetGamemodeGearscriptVariables(api, entitySource, pluginGSContainer, gearscriptContainterToChange);

		array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry(gearscriptContainterToChange)};
		api.SetVariableValue(entitySource, path, "m_bEnableShareableMarkers", pluginGSContainer.m_bEnableShareableMarkers.ToString());
	}
}
#endif