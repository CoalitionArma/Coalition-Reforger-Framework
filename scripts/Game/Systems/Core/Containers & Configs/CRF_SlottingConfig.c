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
	static const string m_sPreselectedSlots = "{0,0}";
	
	
	[Attribute()]
	LocalizedString m_sCallsign;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType), category: "Group")]
	CRF_EFlagType m_FlagType;
	
	[Attribute("1", category: "Group")]
	bool m_bBlueForceTrackerEnabled;
	
	[Attribute("", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup_AirCrew : CRF_SlottingGroup
{		
	void CRF_SlottingGroup_AirCrew()
	{
		ref array<ref CRF_EGearRole> temparray;
		
		//if (m_aSlots.IsEmpty())
			temparray.Insert(CRF_EGearRole.PILOT); //CRF_EGearRole.CREW_CHIEF};
		
		m_aSlots = temparray;
		
		Print(m_aSlots);
	}
}