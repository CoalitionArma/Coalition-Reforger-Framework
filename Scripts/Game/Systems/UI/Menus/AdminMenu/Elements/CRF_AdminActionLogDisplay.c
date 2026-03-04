/**
 * CRF_AdminActionLogDisplay
 *
 * Modular widget component for the persistent admin action-log sidebar
 * (the "List5Box" list box that is always visible in the Admin Menu, not
 * inside any dynamically-loaded panel). Populates and refreshes the
 * reverse-chronological list of CRF_AdminActionLog entries.
 *
 * The component attaches to the root widget that contains "List5Box" and
 * resolves the list-box once in HandlerAttached. The owning menu calls
 * Populate() on open (and can call it again any time the log changes).
 *
 * Expected child widget (on the component root):
 *   "List5Box" — OverlayWidget containing an SCR_ListBoxComponent
 *
 * Usage:
 *   Widget logRoot = m_wRoot.FindAnyWidget("AdminActionLogDisplay");
 *   m_ActionLogDisplay = CRF_AdminActionLogDisplay.Cast(
 *       logRoot.FindHandler(CRF_AdminActionLogDisplay));
 *
 *   // Called once on menu open (after managers are initialised):
 *   if (m_ActionLogDisplay)
 *       m_ActionLogDisplay.Populate();
 */
class CRF_AdminActionLogDisplay : SCR_ScriptedWidgetComponent
{
	protected SCR_ListBoxComponent m_cListBox;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		OverlayWidget listRoot = OverlayWidget.Cast(w.FindAnyWidget("List5Box"));
		if (listRoot)
			m_cListBox = SCR_ListBoxComponent.Cast(listRoot.FindHandler(SCR_ListBoxComponent));
	}

	/**
	 * Clears and re-populates the admin action log list in reverse-chronological
	 * order (newest entry at the top). Call once on menu open and whenever the
	 * log is known to have changed.
	 */
	void Populate()
	{
		if (!m_cListBox)
			return;

		CRF_AdminMenuManager adminMgr = CRF_AdminMenuManager.GetInstance();
		if (!adminMgr)
			return;

		array<ref CRF_AdminActionLog> actions = adminMgr.GetAdminActionLogs();
		if (!actions)
			return;

		m_cListBox.Clear();

		// Iterate in reverse so the most recent action appears first
		for (int i = actions.Count() - 1; i >= 0; i--)
		{
			CRF_AdminActionLog action = actions[i];
			m_cListBox.AddItem(string.Format("%1 - %2", action.timestamp, action.action));
		}
	}
}
