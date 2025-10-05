modded class EditorMenuUI
{
	
	SCR_PlayerController m_PlayerController;
	SCR_ButtonComponent m_ButtonComponent;
	Widget m_wCrossWidget;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		m_ButtonComponent = SCR_ButtonComponent.Cast(GetRootWidget().FindAnyWidget("ListenButton").FindHandler(SCR_ButtonComponent));
		m_wCrossWidget = GetRootWidget().FindAnyWidget("ListeningCross");
		m_ButtonComponent.m_OnClicked.Insert(ToggleListen);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!m_PlayerController.m_bIsListeningToSpec)
			m_wCrossWidget.SetVisible(true);
		else
			m_wCrossWidget.SetVisible(false);
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		m_PlayerController.m_bIsListeningToSpec = false;
		CRF_RplToAuthorityManager.GetInstance().TogglePlayerListening(SCR_PlayerController.GetLocalPlayerId(), false);
	}
	
	void ToggleListen()
	{
		m_PlayerController.m_bIsListeningToSpec = !m_PlayerController.m_bIsListeningToSpec;
		CRF_RplToAuthorityManager.GetInstance().TogglePlayerListening(SCR_PlayerController.GetLocalPlayerId(), m_PlayerController.m_bIsListeningToSpec);
	}
}