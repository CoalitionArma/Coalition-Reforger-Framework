[BaseContainerProps()]
modded class SCR_FactionCallsignInfo
{
	void SetSquadArray(array<ref SCR_CallsignInfo> names)
	{
		m_aSquadNames = names;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sCallsign")]
modded class SCR_CallsignInfo
{
	void SetCallsign(LocalizedString callsign)
	{
		m_sCallsign = callsign;
	}
};