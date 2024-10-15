class CRF_PauseMenuUI: PauseMenuUI
{
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		FrameWidget windowRoot = FrameWidget.Cast(m_wRoot.FindWidget("CoalAdminMenu"));
		SCR_ButtonTextComponent.GetButtonText("CoalAdminMenu", windowRoot).m_OnClicked.Insert(OpenAdminMenu);
	}

	
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}