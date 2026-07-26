class CRF_MiniArsenalCategoryButton: SCR_ButtonComponent
{
	int m_iCategoryIndex = 0;
	ref array<ref COA_Weapon_Class> m_Weapons = {};
	ref array<ref COA_Inventory_Item> m_Items = {};
	ref array<ref COA_Spec_Weapon_Class> m_SpecWeapons = {};
	ref array<ref COA_Magazine_Class> m_Magazines = {};
	bool m_bIsPistol = false;
}