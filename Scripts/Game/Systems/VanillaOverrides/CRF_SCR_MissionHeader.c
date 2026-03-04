modded class SCR_MissionHeader
{
	[Attribute("", desc: "The name of the terrain on the mission")]
	string m_sTerrainName;
	
	[Attribute("", desc: "The BI account GUID of the mission author - used for automatic admin privileges")]
	string m_sAuthorGUID;
}