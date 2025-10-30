class CRF_MiniArsenalItemButton: SCR_ButtonComponent
{
	Widget m_wButtonRoot;
	string m_sResource;
	int m_iSlotId;
	ref array<ResourceName> m_aAttachments = {};
	ref array<ResourceName> m_aMagazines = {};
	ref array<int> m_aMagazineCounts = {};
	bool m_bIsPistol = false;
}