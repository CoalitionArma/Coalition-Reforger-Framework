#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "3 | Configure Slots", 
	description: "Configure Mission Slots", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0033)
] 
class CRF_MissionSlottingPlugin : WorkbenchPlugin
{	
	[Attribute("1", "auto", "", category: "CRF Slotting Config - Slotting Ratio")]
	int m_iFactionOneRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Config - Slotting Ratio")]
	string m_sFactionOneKey;

	[Attribute("1", "auto", "", category: "CRF Slotting Config - Slotting Ratio")]
	int m_iFactionTwoRatio;

	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Slotting Config - Slotting Ratio")]
	string m_sFactionTwoKey;
	
	//------------------------------------------------------------------------------------------------
	// BLU
	//------------------------------------------------------------------------------------------------
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=CRF_SlottingGroup")]
	ref array <ResourceName> m_TestSlots;
	
	[Attribute("", UIWidgets.Auto, desc: "BLUFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Mission Config - BLUFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_BLUFORSlots;
	
	//------------------------------------------------------------------------------------------------
	// OPF
	//------------------------------------------------------------------------------------------------
	
	[Attribute("", UIWidgets.Auto, desc: "OPFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Mission Config - OPFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_OPFORSlots;
	
	//------------------------------------------------------------------------------------------------
	// IND
	//------------------------------------------------------------------------------------------------
	
	[Attribute("", UIWidgets.Auto, desc: "INDFOR Slots", "conf class=CRF_SlottingGroup", category: "CRF Mission Config - INDFOR Slots")]
	ref array <ref CRF_SlottingGroup> m_INDFORSlots;
	
	//------------------------------------------------------------------------------------------------
	// CIV
	//------------------------------------------------------------------------------------------------

	[Attribute("", UIWidgets.Auto, desc: "CIVILIAN Slots", "conf class=CRF_SlottingGroup", category: "CRF Mission Config - CIVILIAN Slots")]
	ref array <ref CRF_SlottingGroup> m_CIVILIANSlots;

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
		
		m_iFactionOneRatio = gamemode.m_iFactionOneRatio;
		m_sFactionOneKey = gamemode.m_sFactionOneKey;
		m_iFactionTwoRatio = gamemode.m_iFactionTwoRatio;
		m_sFactionTwoKey = gamemode.m_sFactionTwoKey;
		
		if (m_BLUFORSlots.IsEmpty())
			m_BLUFORSlots = gamemode.m_BluforSlots;
		
		if (m_OPFORSlots.IsEmpty())
			m_OPFORSlots = gamemode.m_OpforSlots;
		
		if (m_INDFORSlots.IsEmpty())
			m_INDFORSlots = gamemode.m_IndforSlots;
		
		if (m_CIVILIANSlots.IsEmpty())
			m_CIVILIANSlots = gamemode.m_CivSlots;
		
		if (!Workbench.ScriptDialog(
		"Mission Slotting Editor", 
		"This allows you to change mission slots/slotting ratios", 
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
	[ButtonAttribute("Apply Slots", true)]
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
		
		api.SetVariableValue(entitySource, null, "m_iFactionOneRatio", m_iFactionOneRatio.ToString());
		api.SetVariableValue(entitySource, null, "m_sFactionOneKey", m_sFactionOneKey);
		api.SetVariableValue(entitySource, null, "m_iFactionTwoRatio", m_iFactionTwoRatio.ToString());
		api.SetVariableValue(entitySource, null, "m_sFactionTwoKey", m_sFactionTwoKey);
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		
		SetGamemodeSlottingVariables(api, entitySource, m_BLUFORSlots, "m_BluforSlots", gamemode.m_BluforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_OPFORSlots, "m_OpforSlots", gamemode.m_OpforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_INDFORSlots, "m_IndforSlots", gamemode.m_IndforSlots);
		SetGamemodeSlottingVariables(api, entitySource, m_CIVILIANSlots, "m_CivSlots", gamemode.m_CivSlots);
		
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
	
	protected void SetGamemodeSlottingVariables(WorldEditorAPI api, IEntitySource entitySource, array <ref CRF_SlottingGroup> pluginSlots, string slotsToChange, array <ref CRF_SlottingGroup> gamemodeSlots)
	{			
		for (int i; i < pluginSlots.Count(); i++ )
		{
			array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry(slotsToChange, i)};
			
			if (i > (gamemodeSlots.Count() - 1))
				api.CreateObjectArrayVariableMember(entitySource, null, slotsToChange, "CRF_SlottingGroup", i);
			
			CRF_SlottingGroup slotGroup = pluginSlots[i];
			int flagTypeInt = slotGroup.m_FlagType;
			
			api.SetVariableValue(entitySource, path, "m_sCallsign", slotGroup.m_sCallsign);
			api.SetVariableValue(entitySource, path, "m_FlagType", flagTypeInt.ToString());
			
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