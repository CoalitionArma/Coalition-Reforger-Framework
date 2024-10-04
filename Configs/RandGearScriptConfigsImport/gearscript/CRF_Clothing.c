[BaseContainerProps(configRoot: true), BaseContainerCustomTitleField("RoleSpecificClothing")]
class CRF_Clothing
{
	[Attribute(category: "slot")]
	string m_Slot;
	[Attribute(category: "Loadout")]
	array<ref ResourceName> m_Prefab;
}
