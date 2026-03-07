/**
 * CRF_TimeDisplay
 *
 * Modular widget component that formats and displays the current in-game
 * time as "Time: HH:MM". Designed to be polled every frame from the parent
 * menu's OnMenuUpdate. Expects a child widget named "TimeText".
 *
 * Usage:
 *   m_TimeDisplay = CRF_TimeDisplay.Cast(
 *       m_wRoot.FindAnyWidget("TimeDisplay").FindHandler(CRF_TimeDisplay));
 *
 *   // In OnMenuUpdate:
 *   m_TimeDisplay.UpdateTimeDisplay();
 */
class CRF_TimeDisplay : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wTimeText;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wTimeText = TextWidget.Cast(w.FindAnyWidget("TimeText"));
	}

	/**
	 * Reads the world time and refreshes the text widget.
	 * Call every frame (or on a tick) from the owning menu's OnMenuUpdate.
	 */
	void UpdateTimeDisplay()
	{
		if (!m_wTimeText)
			return;

		TimeContainer time = ChimeraWorld.CastFrom(GetGame().GetWorld())
			.GetTimeAndWeatherManager()
			.GetTime();

		string hours = time.m_iHours.ToString();
		if (time.m_iHours < 10)
			hours = "0" + hours;
		
		string minutes = time.m_iMinutes.ToString();
		if (time.m_iMinutes < 10)
			minutes = "0" + minutes;

		m_wTimeText.SetText("Time: " + hours + ":" + minutes);
	}
}
