class CRF_PauseMenuUI: PauseMenuUI
{
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		FrameWidget windowRoot = FrameWidget.Cast(m_wRoot.FindWidget("CoalAdminMenu"));
		if (!playerManager.HasPlayerRole(SCR_PlayerController.GetLocalPlayerId(), EPlayerRole.ADMINISTRATOR))
			windowRoot.SetVisible(false);
		SCR_ButtonTextComponent.GetButtonText("CoalAdminMenu", windowRoot).m_OnClicked.Insert(OpenAdminMenu);
	}

	
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}