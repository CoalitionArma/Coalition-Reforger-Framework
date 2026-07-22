modded class EditorMenuUI
{
	
	CRF_PlayerControllerManager m_PlayerControllerManager;
	SCR_ButtonComponent m_ButtonComponent;
	SCR_ButtonComponent m_CleanUpBodiesButton;
	Widget m_wCrossWidget;
	Widget m_wConfirmMenu;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_PlayerControllerManager = CRF_PlayerControllerManager.GetInstance();
		m_ButtonComponent = SCR_ButtonComponent.Cast(GetRootWidget().FindAnyWidget("ListenButton").FindHandler(SCR_ButtonComponent));
		m_wCrossWidget = GetRootWidget().FindAnyWidget("ListeningCross");
		m_ButtonComponent.m_OnClicked.Insert(ToggleListen);
		
		if (SCR_EditorManagerEntity.GetInstance().GetCurrentMode() != EEditorMode.EDIT)
			GetRootWidget().FindAnyWidget("DeadBodyCleanupFrame").SetVisible(false);
		
		m_CleanUpBodiesButton = SCR_ButtonComponent.Cast(GetRootWidget().FindAnyWidget("DeadBodyCleanupButton").FindHandler(SCR_ButtonComponent));
		m_CleanUpBodiesButton.m_OnClicked.Insert(ConfirmAction);
	}
	
	void CleanUpBodies()
	{
		CloseConfirmAction();
		CRF_GameplayRplToAuthorityManager.GetInstance().CleanUpBodies();
	}
	
	void ConfirmAction()
	{
		// Load menu content widget
		m_wConfirmMenu = GetGame().GetWorkspace().CreateWidgets("{905BF1B70A9A44AC}UI/layouts/Menus/PauseMenu/AdminMenuWidgets/ConfirmationMenu.layout");

		// Get menu buttons
		SCR_ButtonTextComponent runButton = SCR_ButtonTextComponent.Cast(m_wConfirmMenu.FindAnyWidget("ExcuteButton").FindHandler(SCR_ButtonTextComponent));
		SCR_ButtonTextComponent cancelButton = SCR_ButtonTextComponent.Cast(m_wConfirmMenu.FindAnyWidget("CancelButton").FindHandler(SCR_ButtonTextComponent));

		// Setup script invokers
		cancelButton.m_OnClicked.Insert(CloseConfirmAction);
		runButton.m_OnClicked.Insert(CleanUpBodies);
	}
	
	void CloseConfirmAction()
	{
		if (m_wConfirmMenu)
		 delete m_wConfirmMenu;
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!m_PlayerControllerManager.m_bIsListeningToSpec)
			m_wCrossWidget.SetVisible(true);
		else
			m_wCrossWidget.SetVisible(false);
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		// Always remove the click handler to prevent double-registration on re-open
		if (m_ButtonComponent)
			m_ButtonComponent.m_OnClicked.Remove(ToggleListen);
		
		if (!CRF_Gamemode.GetInstance())
			return;
		
		m_PlayerControllerManager.m_bIsListeningToSpec = false;
		CRF_GameplayRplToAuthorityManager.GetInstance().TogglePlayerListening(SCR_PlayerController.GetLocalPlayerId(), false);
		CloseConfirmAction();
	}
	
	void ToggleListen()
	{
		if (!CRF_Gamemode.GetInstance())
			return;
		
		m_PlayerControllerManager.m_bIsListeningToSpec = !m_PlayerControllerManager.m_bIsListeningToSpec;
		CRF_GameplayRplToAuthorityManager.GetInstance().TogglePlayerListening(SCR_PlayerController.GetLocalPlayerId(), m_PlayerControllerManager.m_bIsListeningToSpec);
	}
}