modded class COA_Gamemode
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Vehicle Gearscript Enable/Disable per Side
	//------------------------------------------------------------------------------------
	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for BLUFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bBLUFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for OPFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bOPFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for INDFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bINDFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for CIV vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bCIVILIANVehicleGearscriptEnabled;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected CRF_LoggingManager m_LoggingManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 INITIALIZATION AND SETUP
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the gamemode and all required manager instances
	//! \param[in] owner The entity that owns this component
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Load configs on dedicated server
		if (RplSession.Mode() == RplMode.Dedicated) {
			CRF_DonatorConfig.LoadConfig();
			CRF_BugReportConfig.LoadConfig();

			// Initialize sight arsenal registry for optimized RPC
			CRF_SightArsenalRegistry.InitializeRegistry();
		};
		
		m_LoggingManager = CRF_LoggingManager.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/UPDATERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	bool DoesFactionShareMarker(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": 	return m_BLUFORGearScriptSettings.m_bEnableShareableMarkers;
			case "OPFOR": 	return m_OPFORGearScriptSettings.m_bEnableShareableMarkers;
			case "INDFOR": 	return m_INDFORGearScriptSettings.m_bEnableShareableMarkers;
			case "CIV": 	return m_CIVILIANGearScriptSettings.m_bEnableShareableMarkers;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns true when the vehicle gearscript system is enabled for the given faction.
	//! Returns true by default for unknown factions.
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, INDFOR, CIV)
	bool IsVehicleGearscriptEnabled(FactionKey factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return m_bBLUFORVehicleGearscriptEnabled;
			case "OPFOR":  return m_bOPFORVehicleGearscriptEnabled;
			case "INDFOR": return m_bINDFORVehicleGearscriptEnabled;
			case "CIV":    return m_bCIVILIANVehicleGearscriptEnabled;
		}
		return true;
	}
}