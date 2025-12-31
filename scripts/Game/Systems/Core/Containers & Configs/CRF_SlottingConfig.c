//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup
{		
	[Attribute()]
	LocalizedString m_sCallsign;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType))]
	CRF_EFlagType m_FlagType;
	
	[Attribute()]
	ref CRF_SlottingSpawnPoint m_sSpawnpoint;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_SlottingSpawnPoint
{		
	[Attribute("0", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("DEFAULT", "0"), ParamEnum("CUSTOM", "1")})]
	int m_bStartingSpawnPoint;
	
	[Attribute("")]
	ref PointInfo m_CustomPosition;
}