//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup
{			
	[Attribute()]
	LocalizedString m_sCallsign;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType))]
	CRF_EFlagType m_FlagType;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;
}