
//! Custom Map Menu UI class for the Coalition Reforger Framework
//! Extends the default map menu to provide mission description functionality
//! This class initializes the mission description list when clients open their map
//! Actual list/text panel logic lives in COA_MissionDescriptionUI (COALITION-Lobby), shared with
//! the lobby's own briefing screen so fixes only need to be made in one place.

modded class SCR_MapMenuUI
{
	protected ref COA_MissionDescriptionUI m_MissionDescriptionUI = new COA_MissionDescriptionUI();
	protected bool m_bMissionDescriptionsInitialized = false; // Flag to track if descriptions have been initialized

	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Called when the map menu is opened
	//! Initializes mission description functionality only on first open
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Don't initialize on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			return;
		}

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode) {
			return;
		}

		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		if (!m_bMissionDescriptionsInitialized)
		{
			// First open: find and cache widgets
			if (!m_MissionDescriptionUI.Init(missionDescriptionWidget, gamemode)) {
				return;
			}
			m_bMissionDescriptionsInitialized = true;
		}
		else
		{
			// Subsequent opens: re-show/re-enable the widget that was hidden on close
			missionDescriptionWidget.SetVisible(true);
			missionDescriptionWidget.SetEnabled(true);
		}

		m_MissionDescriptionUI.ShowList();
	}

	//------------------------------------------------------------------------------------------------
	//! Called when the map menu is closed
	//! Cleanup mission description components
	override void OnMenuClose()
	{
		super.OnMenuClose();

		// Hide and disable the MissionDescription widget to prevent invisible input blocking
		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (missionDescriptionWidget)
		{
			missionDescriptionWidget.SetVisible(false);
			missionDescriptionWidget.SetEnabled(false);
		}

		// Clear mission description state and event handlers
		m_MissionDescriptionUI.Clear();
	}
}
