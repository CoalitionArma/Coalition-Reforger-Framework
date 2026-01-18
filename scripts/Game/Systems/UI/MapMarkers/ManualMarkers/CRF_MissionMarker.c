class CRF_MissionMarkerClass: CRF_ManualMarkerClass
{
}

class CRF_MissionMarker: CRF_ManualMarker
{
	[Attribute("", category: "Objective Briefing")]
	string m_sMissionMarkerDescription;
	
	[Attribute(params: "edds", category: "Objective Briefing")]
	ref array<ResourceName> m_aMissionMarkerImages;
}