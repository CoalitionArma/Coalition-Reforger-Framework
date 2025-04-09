//! Base nametag element for text
[BaseContainerProps(), SCR_NameTagElementTitle()]
modded class SCR_NTTextBase : SCR_NTElementBase
{
	//------------------------------------------------------------------------------------------------
	override void SetDefaults(SCR_NameTagData data, int index)
	{
		super.SetDefaults(data, index);

		TextWidget tWidget = TextWidget.Cast(data.m_aNametagElements[index]);
		if (!tWidget)
			return;

		if (data.m_Entity && data.m_Entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			tWidget.SetVisible(false);
	}
}
