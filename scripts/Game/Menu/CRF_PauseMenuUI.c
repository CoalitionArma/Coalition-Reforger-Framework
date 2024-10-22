class CRF_PauseMenuUI: PauseMenuUI
{
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		SCR_ButtonTextComponent comp = SCR_ButtonTextComponent.GetButtonText("CoalAdminMenuButton", GetRootWidget());
		FrameWidget frame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("CoalAdminMenu"));
		
		if (!SCR_Global.IsAdmin() || !CRF_AdminMenuGameComponent.GetInstance())
			frame.SetVisible(false);
		
		comp.m_OnClicked.Insert(OpenAdminMenu);
	}
	
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}