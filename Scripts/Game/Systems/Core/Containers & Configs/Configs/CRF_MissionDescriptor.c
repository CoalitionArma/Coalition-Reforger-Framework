//------------------------------------------------------------------------------------
// Mission briefing descriptor for displaying mission information
//------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sTitle"}, "%1")]
class CRF_MissionDescriptor
{
	[Attribute("")]
	string m_sTitle;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sTextData;

	[Attribute("")]
	ref array<string> m_aFactionKeys;

	[Attribute("")]
	bool m_bShowForAnyFaction;
}