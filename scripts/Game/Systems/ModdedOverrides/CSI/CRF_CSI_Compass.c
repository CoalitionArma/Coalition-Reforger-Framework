modded class CSI_Compass
{
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		if (!SCR_PlayerController.GetLocalMainEntity())
			return;
		if (SCR_PlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && GetRootWidget().IsVisible())
			GetRootWidget().SetVisible(false);
		else if (SCR_PlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && !GetRootWidget().IsVisible())
			GetRootWidget().SetVisible(true);
		
	}
}