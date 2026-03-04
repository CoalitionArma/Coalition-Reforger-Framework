/*
 * CRF_GlobalEventLogDisplay
 *
 * SCR_ScriptedWidgetComponent that renders the global mission event feed inside the
 * spectator layout.  It expects to be attached to a widget named "GlobalEventLog" that
 * contains:
 *
 *   GlobalEventLog            (any container widget — root for this handler)
 *   └─ EventLogScrollPanel    (ScrollLayoutWidget — provides the scrolling container)
 *      └─ EventLogList        (VerticalLayoutWidget — rows are inserted here)
 *
 * Each event is a single TextWidget row prepended at the top of EventLogList so that the
 * most recent entry always appears first.  Rows are capped at MAX_ENTRIES; oldest are
 * removed from the bottom when the cap is exceeded.
 *
 * Colour coding (ARGB):
 *   KILL    — red-tinted        #FFE05050
 *   UNCON   — amber             #FFFFAA33
 *   MISSION — cyan              #FF33CCEE
 *   default — light grey        #FFCCCCCC
 */
class CRF_GlobalEventLogDisplay : SCR_ScriptedWidgetComponent
{
	//=================================================================================================
	// CONSTANTS
	//=================================================================================================

	static const int MAX_ENTRIES = 50; // Max visible rows before oldest are pruned

	//=================================================================================================
	// WIDGET REFERENCES
	//=================================================================================================

	protected ScrollLayoutWidget   m_wScrollPanel;
	protected VerticalLayoutWidget  m_wList;

	//=================================================================================================
	// LIFECYCLE
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wScrollPanel = ScrollLayoutWidget.Cast(w.FindAnyWidget("EventLogScrollPanel"));
		if (m_wScrollPanel)
			m_wList = VerticalLayoutWidget.Cast(m_wScrollPanel.FindAnyWidget("EventLogList"));

		if (!m_wList)
			Print("[CRF_GlobalEventLogDisplay] WARNING: EventLogList widget not found — event feed will not render.", LogLevel.WARNING);
	}

	//=================================================================================================
	// PUBLIC API — called from CRF_SpectatorMenu
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	/**
	 * Append a single formatted event string to the top of the list.
	 * Called each time a new event arrives from the server.
	 * @param entry  Fully-formatted string, e.g. "00:23  [KILL]  Smith (BLUFOR) -> Jones (OPFOR) | M4 | 120m"
	 */
	void AppendEntry(string entry)
	{
		if (!m_wList)
			return;

		// Determine colour from event type tag
		Color colour = Color.FromRGBA(204, 204, 204, 255); // default grey
		if (entry.Contains("[KILL]"))
			colour = Color.FromRGBA(224, 80, 80, 255);   // red
		else if (entry.Contains("[UNCON]"))
			colour = Color.FromRGBA(255, 170, 51, 255);  // amber
		else if (entry.Contains("[MISSION]"))
			colour = Color.FromRGBA(51, 204, 238, 255);  // cyan

		// Create a text row appended to the list (newest at bottom)
		Widget rawRow = GetGame().GetWorkspace().CreateWidget(
			WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, m_wList);

		TextWidget row = TextWidget.Cast(rawRow);
		if (!row)
			return;

		row.SetText(entry);
		row.SetColor(colour);

		// Scroll to bottom so the newest entry is always visible
		if (m_wScrollPanel)
			m_wScrollPanel.SetSliderPos(0, 1);

		// Prune oldest entries when over cap
		EnforceMaxEntries();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Clear all current rows and repopulate from a history array (oldest-first order).
	 * Called once on menu open to fill in catch-up entries from CRF_EventLogManager.
	 * @param history  Array of formatted entries, oldest at index 0
	 */
	void PopulateHistory(notnull array<string> history)
	{
		ClearList();

		// Insert oldest first so newest ends up at the bottom
		foreach (string entry : history)
		{
			AppendEntry(entry);
		}
	}

	//------------------------------------------------------------------------------------------------
	/** Remove all rows from the list. */
	void ClearList()
	{
		if (!m_wList)
			return;

		Widget child = m_wList.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			m_wList.RemoveChild(child);
			child = next;
		}
	}

	//=================================================================================================
	// INTERNAL
	//=================================================================================================

	//------------------------------------------------------------------------------------------------
	/** Remove the bottom-most (oldest) child when MAX_ENTRIES is exceeded. */
	protected void EnforceMaxEntries()
	{
		if (!m_wList)
			return;

		int count = 0;
		Widget child = m_wList.GetChildren();
		while (child)
		{
			count++;
			child = child.GetSibling();
		}

		// Remove from the end (oldest = highest index) until within cap
		while (count > MAX_ENTRIES)
		{
			Widget last = m_wList.GetChildren();
			if (!last)
				break;

			// Walk to the last child
			Widget next = last.GetSibling();
			while (next)
			{
				last = next;
				next = next.GetSibling();
			}

			m_wList.RemoveChild(last);
			count--;
		}
	}
}
