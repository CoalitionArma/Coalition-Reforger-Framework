class CRF_MissionMarkerClass: COA_ManualMarkerClass
{
}

class CRF_MissionMarker: COA_ManualMarker
{
	[Attribute("", category: "Objective Briefing")]
	string m_sMissionMarkerDescription;
	
	[Attribute(params: "edds", category: "Objective Briefing")]
	ref array<ResourceName> m_aMissionMarkerImages;
}