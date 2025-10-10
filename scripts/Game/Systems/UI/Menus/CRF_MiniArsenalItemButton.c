class CRF_MiniArsenalItemButton: SCR_ButtonComponent
{
	string m_sResource;
	int m_iSlotId;
	ref array<ResourceName> m_aAttachments = {};
	ref array<ResourceName> m_aMagazines = {};
	ref array<int> m_aMagazineCounts = {};
	bool m_bIsPistol = false;
}