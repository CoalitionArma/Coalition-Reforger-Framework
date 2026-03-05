#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "3 | Configure Factions", 
	description: "Configure Mission Factions", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF024)
] 
class CRF_MissionFactionsPlugin : WorkbenchPlugin
{		
	// Due to limitations in getting the faction managers faction list (obfuscated) we have to manually point to the CIV faction index.
	static int CIV_FACMANAGER_INDEX = 3;
	
	//------------------------------------------------------------------------------------
	// BLU
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of BLUFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Settings - BLUFOR")]
	protected int m_iBLUFORTickets;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFactions),  desc: "List of faction IDs that are considered friendly for this faction. Note: If factionA has factionB as friendly but FactionB does not have FactionA as friendly then they are still both set as friendly so for init it is only required for one faction.", category: "CRF Faction Settings - BLUFOR")]
	protected ref array<CRF_EFactions> m_aBLUFORFriendlyFactionsIds;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Faction Settings - BLUFOR")]
	protected ref CRF_SimplifiedGearScriptContainer m_BLUFORGearScriptSettings;

	//------------------------------------------------------------------------------------
	// OPF
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of OPFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Settings - OPFOR")]
	protected int m_iOPFORTickets;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFactions), desc: "List of faction IDs that are considered friendly for this faction. Note: If factionA has factionB as friendly but FactionB does not have FactionA as friendly then they are still both set as friendly so for init it is only required for one faction.", category: "CRF Faction Settings - OPFOR")]
	protected ref array<CRF_EFactions> m_aOPFORFriendlyFactionsIds;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Faction Settings - OPFOR")]
	protected ref CRF_SimplifiedGearScriptContainer m_OPFORGearScriptSettings;
	
	//------------------------------------------------------------------------------------
	// IND
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Settings - INDFOR")]
	protected int m_iINDFORTickets;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFactions),  desc: "List of faction IDs that are considered friendly for this faction. Note: If factionA has factionB as friendly but FactionB does not have FactionA as friendly then they are still both set as friendly so for init it is only required for one faction.", category: "CRF Faction Settings - INDFOR")]
	protected ref array<CRF_EFactions> m_aINDFORFriendlyFactionsIds;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Faction Settings - INDFOR")]
	protected ref CRF_SimplifiedGearScriptContainer m_INDFORGearScriptSettings;

	//------------------------------------------------------------------------------------
	// CIV
	//------------------------------------------------------------------------------------
	
	[Attribute("0", UIWidgets.EditBox, "Amount of CIVILIAN Tickets. 0 = disabled/-1 = unlimited", category: "CRF Faction Settings - CIVILIAN")]
	protected int m_iCIVILIANTickets;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFactions),  desc: "List of faction IDs that are considered friendly for this faction. Note: If factionA has factionB as friendly but FactionB does not have FactionA as friendly then they are still both set as friendly so for init it is only required for one faction.", category: "CRF Faction Settings - CIVILIAN")]
	protected ref array<CRF_EFactions> m_aCIVILIANFriendlyFactionsIds;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Faction Settings - CIVILIAN")]
	protected ref CRF_SimplifiedGearScriptContainer m_CIVILIANGearScriptSettings;

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
		
		IEntitySource facManagerSource = GetFactionManager(api, entitySource);
		SCR_FactionManager facManager = SCR_FactionManager.Cast(api.SourceToEntity(facManagerSource));
		
		if (!facManager)
			return;		
		
		m_iBLUFORTickets = gamemode.m_iBLUFORTickets;
		m_iOPFORTickets = gamemode.m_iOPFORTickets;
		m_iINDFORTickets = gamemode.m_iINDFORTickets;
		m_iCIVILIANTickets = gamemode.m_iCIVTickets;
		
		SetPluginFriendlyFactions(facManager, "BLUFOR", m_aBLUFORFriendlyFactionsIds);
		SetPluginFriendlyFactions(facManager, "OPFOR", m_aOPFORFriendlyFactionsIds);
		SetPluginFriendlyFactions(facManager, "INDFOR", m_aINDFORFriendlyFactionsIds);
		SetPluginFriendlyFactions(facManager, "CIV", m_aCIVILIANFriendlyFactionsIds);
		
		m_BLUFORGearScriptSettings = new CRF_SimplifiedGearScriptContainer;
		m_OPFORGearScriptSettings = new CRF_SimplifiedGearScriptContainer;
		m_INDFORGearScriptSettings = new CRF_SimplifiedGearScriptContainer;
		m_CIVILIANGearScriptSettings = new CRF_SimplifiedGearScriptContainer;
		
		SetPluginGearscriptVariables(m_BLUFORGearScriptSettings, gamemode.m_BLUFORGearScriptSettings);
		SetPluginGearscriptVariables(m_OPFORGearScriptSettings, gamemode.m_OPFORGearScriptSettings);
		SetPluginGearscriptVariables(m_INDFORGearScriptSettings, gamemode.m_INDFORGearScriptSettings);
		SetPluginGearscriptVariables(m_CIVILIANGearScriptSettings, gamemode.m_CIVILIANGearScriptSettings);
		
		// Actually shows the window
		if (!Workbench.ScriptDialog(
		"Mission Faction Editor", 
		"This allows you to change basic mission faction settings, for advanced gearscript settings (Mini-Arsenal, Vehicle Gearscript Settings, etc) please go into the CRF_Lobby entity's object properties", 
		this))
			return;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetPluginGearscriptVariables(CRF_SimplifiedGearScriptContainer pluginGearscript, CRF_GearScriptContainer gearscript)
	{			
		pluginGearscript.m_rGearScript = gearscript.m_rGearScript;
		pluginGearscript.m_bEnableShareableMarkers = gearscript.m_bEnableShareableMarkers;
		pluginGearscript.m_bEnableBFT = gearscript.m_bEnableBFT;
		pluginGearscript.m_bEnableLeadershipRadios = gearscript.m_bEnableLeadershipRadios;
		pluginGearscript.m_bEnableGIRadios = gearscript.m_bEnableGIRadios;
		pluginGearscript.m_bEnableRTORadios = gearscript.m_bEnableRTORadios;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetPluginFriendlyFactions(SCR_FactionManager facManager, FactionKey factionKey, array<CRF_EFactions> friendlyFactionArray)
	{
		friendlyFactionArray.Clear();
		
		array<FactionKey> friendlyFactions = SCR_Faction.Cast(facManager.GetFactionByKey(factionKey)).GetDefaultFriendlyFactions();
		foreach(FactionKey facKey : friendlyFactions)
		{
			if (facKey == "BLUFOR" || facKey == "OPFOR" || facKey == "INDFOR" || facKey == "CIV")
				friendlyFactionArray.Insert(typename.StringToEnum(CRF_EFactions, facKey));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Apply Faction Settings", true)]
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
		
		IEntitySource facManagerSource = GetFactionManager(api, entitySource);
		
		if (!IEntitySource)
			return false;		
		
		api.BeginEntityAction();
		api.BeginEditSequence(entitySource);
		// BEGIN EDIT ACTION ON ENTITY
		//-----------------------------------------------------
		
		api.SetVariableValue(entitySource, null, "m_iBLUFORTickets", m_iBLUFORTickets.ToString());
		api.SetVariableValue(entitySource, null, "m_iOPFORTickets", m_iOPFORTickets.ToString());
		api.SetVariableValue(entitySource, null, "m_iINDFORTickets", m_iINDFORTickets.ToString());
		api.SetVariableValue(entitySource, null, "m_iCIVTickets", m_iCIVILIANTickets.ToString());

		SetGamemodeGearscriptVariables(api, entitySource, m_BLUFORGearScriptSettings, "m_BLUFORGearScriptSettings");
		SetGamemodeGearscriptVariables(api, entitySource, m_OPFORGearScriptSettings, "m_OPFORGearScriptSettings");
		SetGamemodeGearscriptVariables(api, entitySource, m_INDFORGearScriptSettings, "m_INDFORGearScriptSettings");
		SetGamemodeGearscriptVariables(api, entitySource, m_CIVILIANGearScriptSettings, "m_CIVILIANGearScriptSettings");
		
		//-----------------------------------------------------
		// END EDIT ACTION ON ENTITY
		api.EndEditSequence(entitySource);
		api.EndEntityAction();
		
		api.BeginEntityAction();
		api.BeginEditSequence(facManagerSource);
		// BEGIN EDIT ACTION ON ENTITY
		//-----------------------------------------------------
		
		SetFactionManagerVariables(api, facManagerSource, "BLUFOR", m_aBLUFORFriendlyFactionsIds);
		SetFactionManagerVariables(api, facManagerSource, "OPFOR", m_aOPFORFriendlyFactionsIds);
		SetFactionManagerVariables(api, facManagerSource, "INDFOR", m_aINDFORFriendlyFactionsIds);
		SetFactionManagerVariables(api, facManagerSource, "CIV", m_aCIVILIANFriendlyFactionsIds);
		
		//-----------------------------------------------------
		// END EDIT ACTION ON ENTITY
		api.EndEditSequence(facManagerSource);
		api.EndEntityAction();
		
		api.UpdateSelectionGui();
		
		worldEditor.Save();
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetGamemodeGearscriptVariables(WorldEditorAPI api, IEntitySource entitySource, CRF_SimplifiedGearScriptContainer pluginGSContainer, string gearscriptContainterToChange)
	{			
		array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry(gearscriptContainterToChange)};
		
		api.SetVariableValue(entitySource, path, "m_rGearScript", pluginGSContainer.m_rGearScript);
		api.SetVariableValue(entitySource, path, "m_bEnableShareableMarkers", pluginGSContainer.m_bEnableShareableMarkers.ToString());
		api.SetVariableValue(entitySource, path, "m_bEnableBFT", pluginGSContainer.m_bEnableBFT.ToString());
		api.SetVariableValue(entitySource, path, "m_bEnableLeadershipRadios", pluginGSContainer.m_bEnableLeadershipRadios.ToString());
		api.SetVariableValue(entitySource, path, "m_bEnableGIRadios", pluginGSContainer.m_bEnableGIRadios.ToString());
		api.SetVariableValue(entitySource, path, "m_bEnableRTORadios", pluginGSContainer.m_bEnableRTORadios.ToString());
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SetFactionManagerVariables(WorldEditorAPI api, IEntitySource facManagerSource, FactionKey factionKey, array<CRF_EFactions> friendlyFactions)
	{
		SCR_FactionManager facManager = SCR_FactionManager.Cast(api.SourceToEntity(facManagerSource));
		
		Faction faction = facManager.GetFactionByKey(factionKey);
		int facCount = facManager.GetFactionsCount();
		int factionIndex;
		
		switch(factionKey)
		{
			// Due to the power or *Hierarchical Structures* in the lobby prefab BLUFOR, OPFOR, and INDFOR will always be the 3rd to last, 2nd to last, and last faction in the faction array.
			case "BLUFOR" : factionIndex = (facCount - 3); break;
			case "OPFOR" : factionIndex = (facCount - 2); break;
			case "INDFOR" : factionIndex = (facCount - 1); break;
			case "CIV" : factionIndex = CIV_FACMANAGER_INDEX; break;
		}
		
		array<ref ContainerIdPathEntry> path = {ContainerIdPathEntry("Factions", factionIndex)};
		
		string finalFactionsArrayStr;
		foreach (int f, CRF_EFactions friendlyFactionEnum : friendlyFactions)
		{
			string friendlyFactionStr = SCR_Enum.GetEnumName(CRF_EFactions, friendlyFactionEnum);
			
			if (f == 0)
				finalFactionsArrayStr = friendlyFactionStr;
			else	
				finalFactionsArrayStr = finalFactionsArrayStr + ", " + friendlyFactionStr;
		}
		
		api.SetVariableValue(facManagerSource, path, "m_aFriendlyFactionsIds", finalFactionsArrayStr);
	}
	
	//------------------------------------------------------------------------------------------------
	protected IEntitySource GetFactionManager(WorldEditorAPI api, IEntitySource gameSource)
	{	
		IEntitySource facManagerSource;
		
		for (int i; i <= gameSource.GetNumChildren(); i++)
		{
			facManagerSource = gameSource.GetChild(i);
			if (SCR_FactionManager.Cast(api.SourceToEntity(facManagerSource)))
				break;
		}
		
		return facManagerSource;
	}
}
#endif