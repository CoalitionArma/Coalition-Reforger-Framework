//------------------------------------------------------------------------------------------------
//! The mission info card on the right of the Lobby's pause menu (mission, objectives, time left, admins
//! online, rules button). Added by the CRF: other communities use the Lobby, and the button is our rules page.
modded class COA_PauseMenuUI
{
	protected ref CRF_MissionInfoPanel m_MissionInfoPanel;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_MissionInfoPanel = new CRF_MissionInfoPanel();
		if (!m_MissionInfoPanel.Init(GetRootWidget()))
			m_MissionInfoPanel = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps the card's timer and admin list current
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_MissionInfoPanel)
			m_MissionInfoPanel.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_MissionInfoPanel)
		{
			m_MissionInfoPanel.Clear();
			m_MissionInfoPanel = null;
		}

		super.OnMenuClose();
	}
}
