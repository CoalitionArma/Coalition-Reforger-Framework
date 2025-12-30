[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomTitleEnum(CRF_EFactions, "m_Faction")]
class CRF_SlottingConfig
{	
	[Attribute()]
	ref CRF_SlottingGroup m_FactionGroups;	
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup
{		
	[Attribute()]
	LocalizedString m_sCallsign;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType))]
	CRF_EFlagType m_FlagType;
	
	[Attribute("1")]
	bool m_bBlueForceTrackerEnabled;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup_AirCrew : CRF_SlottingGroup
{		
	void CRF_SlottingGroup_AirCrew()
	{
		m_FlagType = CRF_EFlagType.HELICOPER;
		m_aSlots = {CRF_EGearRole.PILOT, CRF_EGearRole.CREW_CHIEF};
	}
}