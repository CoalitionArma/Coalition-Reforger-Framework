//------------------------------------------------------------------------------------------------
//! Adds the Game Master player list (CRF_ZeusPlayerList) to the editor UI
modded class EditorMenuUI
{
	protected ref CRF_ZeusPlayerList m_ZeusPlayerList;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		if (m_ZeusPlayerList)
			m_ZeusPlayerList.Destroy();

		// Game Masters only - a limited editor (e.g. the Lobby's Tactical Camera for platoon
		// leaders) must not get the full player list and its admin actions
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && editorManager.IsLimited())
			return;

		m_ZeusPlayerList = new CRF_ZeusPlayerList(GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_ZeusPlayerList)
			m_ZeusPlayerList.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_ZeusPlayerList)
		{
			m_ZeusPlayerList.Destroy();
			m_ZeusPlayerList = null;
		}

		super.OnMenuClose();
	}
}
