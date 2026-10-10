
//! Custom Map Menu UI class for the Coalition Reforger Framework
//! Extends the default map menu to provide mission description functionality
//! This class initializes the mission description list when clients open their map
//! Actual list/text panel logic lives in COA_MissionDescriptionUI (COALITION-Lobby), shared with
//! the lobby's own briefing screen so fixes only need to be made in one place.

modded class SCR_MapMenuUI
{
	protected ref COA_MissionDescriptionUI m_MissionDescriptionUI = new COA_MissionDescriptionUI();
	protected bool m_bMissionDescriptionsInitialized = false; // Flag to track if descriptions have been initialized

	// The briefing panel is a drawer on the right edge (UI/layouts/Map/MapMenu.layout "MissionDescription"):
	// PositionX with only its 44px tab on screen, and fully out
	protected static const float BRIEFING_DRAWER_CLOSED_X = -44;
	protected static const float BRIEFING_DRAWER_OPEN_X = -524;
	protected ref COA_HoverDrawer m_BriefingDrawer;

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

		// Starts tucked away; slides out while the cursor is over it
		m_BriefingDrawer = new COA_HoverDrawer(missionDescriptionWidget, BRIEFING_DRAWER_CLOSED_X, BRIEFING_DRAWER_OPEN_X);
		m_BriefingDrawer.m_OnOpenChanged.Insert(OnBriefingDrawerChanged);

		// Open only from the tab: the drawer runs almost the full screen height, and its edge would
		// otherwise catch the cursor over the mission timer / tickets drawer in the bottom-right corner
		m_BriefingDrawer.SetClosedHitArea(missionDescriptionWidget.FindAnyWidget("BriefingTabBG"));
		OnBriefingDrawerChanged(false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_BriefingDrawer)
			m_BriefingDrawer.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	//! Flip the drawer tab's arrow
	protected void OnBriefingDrawerChanged(bool open)
	{
		TextWidget arrow = TextWidget.Cast(GetRootWidget().FindAnyWidget("BriefingTabArrow"));
		if (!arrow)
			return;

		if (open)
			arrow.SetText("›");
		else
			arrow.SetText("‹");
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
		m_BriefingDrawer = null;
	}
}
