class CRF_PauseMenuUI: PauseMenuUI
{
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		FrameWidget windowRoot = FrameWidget.Cast(m_wRoot.FindWidget("CoalAdminMenu"));
		if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
			windowRoot.SetVisible(false);
		SCR_ButtonTextComponent.GetButtonText("CoalAdminMenu", windowRoot).m_OnClicked.Insert(OpenAdminMenu);
	}

	
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}