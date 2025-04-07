class CRF_PauseMenuUI: PauseMenuUI
{
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		SCR_ButtonTextComponent comp = SCR_ButtonTextComponent.GetButtonText("CoalAdminMenuButton", GetRootWidget());
		FrameWidget frame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("CoalAdminMenu"));
		
		if ((!SCR_Global.IsAdmin() && !SCR_Global.IsModerator()) || !CRF_GamemodeComponent.GetInstance() || !CRF_Gamemode.GetInstance())
			frame.SetVisible(false);
		
		comp.m_OnClicked.Insert(OpenAdminMenu);
	}
	
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}