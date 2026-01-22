#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "4 | Configure Slots", 
	description: "Configure Mission Slots", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF00B)
] 
class CRF_MissionSlottingPlugin : WorkbenchPlugin
{	
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
		
		// Only call the quick slot setup if there is no slots setup on the gamemode (this is typically only true when a mission makers is first setting up slots).
		if (gamemode.m_BluforSlots.IsEmpty() && gamemode.m_OpforSlots.IsEmpty() && gamemode.m_IndforSlots.IsEmpty() && gamemode.m_CivSlots.IsEmpty())
		{
			// Create the Quick Slot Setup Dialouge, NOTE: MUST CALL Workbench.ScriptDialog FOR WINDOW TO POP UP.
			CRF_MissionSlottingQuickSetupDialouge dialog = new CRF_MissionSlottingQuickSetupDialouge();
			dialog.Run(); // Have to call Run() Manually since the dialouge isnt a child of WorkbenchPlugin.
			
			// Actually shows the window.
			if (!Workbench.ScriptDialog(
				"Mission Quick Slot Setup", 
				"This allows you to change mission slots at a more basic level so you arent building PLTs from scratch each time\nAll factions have pre-made quickslots set in their quikcslot arrays, please ensure you clear a factions quickslot array if you aren't planning to use that faction \n\n WARNING: THIS EDITOR WILL NOT BE AVALIBLE AFTER INITIAL SLOTS SETUP!", 
				dialog))
				return;
		} else {
			// Create the Regular Slot Setup Dialouge, NOTE: MUST CALL Workbench.ScriptDialog FOR WINDOW TO POP UP.
			CRF_MissionSlottingSetupDialouge dialog = new CRF_MissionSlottingSetupDialouge();
			dialog.Run(); // Have to call Run() Manually since the dialouge isnt a child of WorkbenchPlugin.
			
			// Actually shows the window.
			if (!Workbench.ScriptDialog(
				"Mission Slotting Editor", 
				"This allows you to change mission slots/slotting ratios \n\n If you wish to go back to Quick Slot Setup, you must clear all slot arrays and re-launch the plugin", 
				dialog))
				return;
		};
	}
}

//------------------------------------------------------------------------------------------------
class CRF_MissionSlottingQuickSetupDialouge
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "BLUFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Quick Slot - BLUFOR Slots")]
	protected ref array <ResourceName> m_BLUFORQuickSlots = {};
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "OPFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Quick Slot - OPFOR Slots")]
	protected ref array <ResourceName> m_OPFORQuickSlots = {};
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "INDFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Quick Slot - INDFOR Slots")]
	protected ref array <ResourceName> m_INDFORQuickSlots = {};
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "CIVILIAN Slots", "conf class=CRF_SlottingGroup", category: "CRF Quick Slot - CIVILIAN Slots")]
	protected ref array <ResourceName> m_CIVILIANQuickSlots = {};
	
	// Default slot resources all quick slots get set to.
	static ref array<ResourceName> DEFAULT_SLOTS = {
		"{AA52A4B0CDA99915}Configs/SlottingGroups/Leadership/CRF_COY.conf",
		"{F779F56A759256A8}Configs/SlottingGroups/Leadership/CRF_PLT.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{F779F56A759256A8}Configs/SlottingGroups/Leadership/CRF_PLT.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{496BAA2CA4279ADA}Configs/SlottingGroups/Squads/CRF_RifleSquad.conf",
		"{09D9CFEC656E40FE}Configs/SlottingGroups/Specialties/CRF_MMG.conf",
		"{3C46FECBC2DC1F33}Configs/SlottingGroups/Specialties/CRF_MAT.conf",
		"{C50D6F576810DE04}Configs/SlottingGroups/Crews/CRF_AirCrew.conf",
		"{CF658340A75E8ACA}Configs/SlottingGroups/Crews/CRF_VehicleCrew.conf"
	};
	
	//------------------------------------------------------------------------------------------------
	void Run()
	{
		// This just sets all the DEFAULT_SLOTS to each factions quick slot array.
		SetDefaultSlots(m_BLUFORQuickSlots);
		SetDefaultSlots(m_OPFORQuickSlots);
		SetDefaultSlots(m_INDFORQuickSlots);
		SetDefaultSlots(m_CIVILIANQuickSlots);
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Apply Quick Slots", true)]
	protected bool ButtonNext()
	{	
		// Create the Regular Slot Setup Dialouge, NOTE: MUST CALL Workbench.ScriptDialog FOR WINDOW TO POP UP.
		CRF_MissionSlottingSetupDialouge dialog = new CRF_MissionSlottingSetupDialouge();
		
		// Set Quick Slots to the various factions slots in the regualr slotting dialouge.
		dialog.SetPluginQuickSlots(m_BLUFORQuickSlots, dialog.m_BLUFORSlots);
		dialog.SetPluginQuickSlots(m_OPFORQuickSlots, dialog.m_OPFORSlots);
		dialog.SetPluginQuickSlots(m_INDFORQuickSlots, dialog.m_INDFORSlots);
		dialog.SetPluginQuickSlots(m_CIVILIANQuickSlots, dialog.m_CIVILIANSlots);
		
		dialog.Run(); // Have to call Run() Manually since the dialouge isnt a child of WorkbenchPlugin.
		
		// Actually shows the window.
		if (!Workbench.ScriptDialog(
			"Mission Slotting Editor", 
			"This allows you to change mission slots/slotting ratios", 
			dialog))
			return false;
		
		return true;
	}
	
	protected void SetDefaultSlots(array <ResourceName> quickSlots)
	{
		quickSlots.Clear(); // Due to attributes in plugins being saved when the plugin is closed, we need to clear the array each time so the user gets the same attributes each time.
		
		// To avoid array fuckery, we just manually insert each resource in.
		for ( int i; i < DEFAULT_SLOTS.Count(); i++ )
			quickSlots.Insert(DEFAULT_SLOTS.Get(i));
	}
}

//------------------------------------------------------------------------------------------------
class CRF_MissionSlottingSetupDialouge
{
	[Attribute("1", "auto", "", category: "CRF Slotting Settings - Slotting Ratio")]
	int m_iFactionOneRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Settings - Slotting Ratio")]
	string m_sFactionOneKey;

	[Attribute("1", "auto", "", category: "CRF Slotting Settings - Slotting Ratio")]
	int m_iFactionTwoRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Settings - Slotting Ratio")]
	string m_sFactionTwoKey;
	
	[Attribute("", UIWidgets.Auto, desc: "BLUFOR Slots", category: "CRF Slotting Settings - BLUFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_BLUFORSlots = {};
	
	[Attribute("", UIWidgets.Auto, desc: "OPFOR Slots", category: "CRF Slotting Settings - OPFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_OPFORSlots = {};
	
	[Attribute("", UIWidgets.Auto, desc: "INDFOR Slots", category: "CRF Slotting Settings - INDFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_INDFORSlots = {};

	[Attribute("", UIWidgets.Auto, desc: "CIVILIAN Slots", category: "CRF Slotting Settings - CIVILIAN Slots")]
	ref array <ref CRF_SlottingGroup> m_CIVILIANSlots = {};

	//------------------------------------------------------------------------------------------------
	void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		
		WorldEditorAPI api = worldEditor.GetApi();
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby"); // Getting the IEntitySource of the lobby, this allows us to set variables on the lobby just like if you were changing Object Properties.
		
		if (!entitySource)
			return;		
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource)); // Get the actual Gamemode so we can pull any variables we need.
		
		m_iFactionOneRatio = gamemode.m_iFactionOneRatio;
		m_sFactionOneKey = gamemode.m_sFactionOneKey;
		m_iFactionTwoRatio = gamemode.m_iFactionTwoRatio;
		m_sFactionTwoKey = gamemode.m_sFactionTwoKey;
		
		// Only use the quick slot setup variables if there is no slots setup on the gamemode.
		if (m_BLUFORSlots.IsEmpty() && m_OPFORSlots.IsEmpty() && m_INDFORSlots.IsEmpty() && m_CIVILIANSlots.IsEmpty())
		{
			SetPluginSlottingVariables(m_BLUFORSlots, gamemode.m_BluforSlots);
			SetPluginSlottingVariables(m_OPFORSlots, gamemode.m_OpforSlots);
			SetPluginSlottingVariables(m_INDFORSlots, gamemode.m_IndforSlots);
			SetPluginSlottingVariables(m_CIVILIANSlots, gamemode.m_CivSlots);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetPluginSlottingVariables(array <ref CRF_SlottingGroup> pluginSlots, array <ref CRF_SlottingGroup> gamemodeSlots)
	{
		pluginSlots.Clear(); // Due to attributes in plugins being saved when the plugin is closed, we need to clear the array each time so the user gets the same attributes each time.
		
		// To avoid array fuckery, we just manually insert each resource in.
		for ( int i; i < gamemodeSlots.Count(); i++ )
			pluginSlots.Insert(gamemodeSlots.Get(i));
	}
	
	//------------------------------------------------------------------------------------------------
	void SetPluginQuickSlots(array <ResourceName> quickSlots, array <ref CRF_SlottingGroup> slots)
	{
		array <LocalizedString> storedPLTCallsigns = {};
		int latestRifleSquad;
		int latestPLT;
		
		foreach (int i, ResourceName slotConfig : quickSlots)
		{
			CRF_SlottingGroup slotGroup = CRF_SlottingGroup.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(slotConfig).GetResource().ToBaseContainer()));
			LocalizedString slotGroupCallsign = slotGroup.m_sCallsign;
			
			if (slotGroupCallsign == "1-1")
			{
				latestRifleSquad++;
				slotGroupCallsign = string.Format("%1-%2", storedPLTCallsigns.Count(), latestRifleSquad);
			}
			
			if (slotGroupCallsign == "PLT")
			{
				latestPLT++;
				latestRifleSquad = 0;
				storedPLTCallsigns.Insert(slotGroupCallsign);
				slotGroupCallsign = string.Format("%1PLT", latestPLT);
			};
			
			slotGroup.m_sCallsign = slotGroupCallsign;
			slots.Insert(slotGroup);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Apply Slots", true)]
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
		
		api.SetVariableValue(entitySource, null, "m_iFactionOneRatio", m_iFactionOneRatio.ToString());
		api.SetVariableValue(entitySource, null, "m_sFactionOneKey", m_sFactionOneKey);
		api.SetVariableValue(entitySource, null, "m_iFactionTwoRatio", m_iFactionTwoRatio.ToString());
		api.SetVariableValue(entitySource, null, "m_sFactionTwoKey", m_sFactionTwoKey);
		
		SetGamemodeSlottingVariables(api, entitySource, m_BLUFORSlots, "m_BluforSlots", gamemode.m_BluforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_OPFORSlots, "m_OpforSlots", gamemode.m_OpforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_INDFORSlots, "m_IndforSlots", gamemode.m_IndforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_CIVILIANSlots, "m_CivSlots", gamemode.m_CivSlots);
		
		//-----------------------------------------------------
		// END EDIT ACTION ON ENTITY
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetGamemodeSlottingVariables(WorldEditorAPI api, IEntitySource entitySource, array <ref CRF_SlottingGroup> pluginSlots, string slotsToChange, array <ref CRF_SlottingGroup> gamemodeSlots)
	{			
		for (int r; r < gamemodeSlots.Count(); r++)
			api.RemoveObjectArrayVariableMember(entitySource, null, slotsToChange, 0);
		
		foreach (int i, CRF_SlottingGroup slotGroup : pluginSlots)
		{
			array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry(slotsToChange, i)};
			
			api.CreateObjectArrayVariableMember(entitySource, null, slotsToChange, "CRF_SlottingGroup", i);
			
			api.SetVariableValue(entitySource, path, "m_sCallsign", slotGroup.m_sCallsign);
			api.SetVariableValue(entitySource, path, "m_FlagType", slotGroup.m_FlagType.ToString());
			
			string finalSlotsArrayStr;
			foreach (int f, CRF_EGearRole role : slotGroup.m_aSlots)
			{
				if (f == 0)
					finalSlotsArrayStr = role.ToString();
				else	
					finalSlotsArrayStr = finalSlotsArrayStr + ", " + role.ToString();
			}
			
			api.SetVariableValue(entitySource, path, "m_aSlots", finalSlotsArrayStr);
		}
	}
}
#endif