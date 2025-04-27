modded class CSI_Compass
{
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		if (!SCR_PlayerController.GetLocalMainEntity())
			return;
		if (CRF_GamemodeManager.IsSpectator() && GetRootWidget().IsVisible())
			GetRootWidget().SetVisible(false);
		else if (!CRF_GamemodeManager.IsSpectator() && !GetRootWidget().IsVisible())
			GetRootWidget().SetVisible(true);
		
	}
}