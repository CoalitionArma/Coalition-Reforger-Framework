/**
 * CRF_MissionTimerDisplay
 *
 * Modular widget component that manages the spectator menu's mission-time
 * countdown. It mirrors the logic that was previously inlined in
 * CRF_SpectatorMenu: polling CRF_GameTimerManager every second, playing
 * audio warnings at 15 min / 5 min / expiry, and formatting the time
 * string with colour coding.
 *
 * Expected child widget (root widget must contain):
 *   "Timer" — TextWidget that shows "Mission End: MM:SS" (or the full
 *              HH:MM:SS when the hour bucket is non-zero)
 *
 * Usage:
 *   Widget timerRoot = m_wRoot.FindAnyWidget("MissionTimerDisplay");
 *   m_MissionTimerDisplay = CRF_MissionTimerDisplay.Cast(
 *       timerRoot.FindHandler(CRF_MissionTimerDisplay));
 *
 *   // In OnMenuUpdate (every frame):
 *   m_MissionTimerDisplay.UpdateTimer();
 */
class CRF_MissionTimerDisplay : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wTimer;

	// Last displayed value — used to skip redundant updates
	protected string m_sStoredTime;

	// Reference to the pop-up notification singleton (set via Init)
	protected SCR_PopUpNotification m_PopUpNotification;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wTimer = TextWidget.Cast(w.FindAnyWidget("Timer"));
	}

	/**
	 * Provides the pop-up notification singleton used for time warnings.
	 * Call once from the owning menu's OnMenuOpen.
	 *
	 * @param popUp  Reference to SCR_PopUpNotification.GetInstance().
	 */
	void Init(SCR_PopUpNotification popUp)
	{
		m_PopUpNotification = popUp;
	}

	/**
	 * Checks the server world time and refreshes the timer widget when the
	 * value changes. Safe to call every frame — internal diffing prevents
	 * redundant work.
	 */
	void UpdateTimer()
	{
		if (!m_wTimer)
			return;

		CRF_GameTimerManager timerMgr = CRF_GameTimerManager.GetInstance();
		if (!timerMgr)
			return;

		CRF_SafestartManager safestartMgr = CRF_SafestartManager.GetInstance();

		string serverTime = timerMgr.GetServerWorldTime();

		// Skip while in safestart, or if the value is empty / unchanged / N/A
		if (serverTime == "N/A" || serverTime.IsEmpty() || m_sStoredTime == serverTime)
			return;

		if (safestartMgr && safestartMgr.GetSafestartStatus())
			return;

		m_sStoredTime = serverTime;

		// Fire threshold warnings
		HandleTimeWarnings(serverTime);

		// Format and colour the widget
		UpdateTimeDisplay(serverTime);
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleTimeWarnings(string serverTime)
	{
		if (serverTime != "00:15:00" &&
			serverTime != "00:05:00" &&
			serverTime != "Mission Time Expired!")
			return;

		AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");

		if (!m_PopUpNotification)
			return;

		if (serverTime == "00:15:00")
			m_PopUpNotification.PopupMsg("Mission Ends In 15 Minutes!", 10);
		else if (serverTime == "00:05:00")
			m_PopUpNotification.PopupMsg("Mission Ends In 5 Minutes!", 10);
		else if (serverTime == "Mission Time Expired!")
		{
			m_PopUpNotification.PopupMsg(serverTime, 10);
			m_wTimer.SetText(serverTime);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTimeDisplay(string serverTime)
	{
		array<string> parts = {};
		serverTime.Split(":", parts, false);

		// Drop the leading "00:" hour bucket for cleaner display
		string displayTime = serverTime;
		if (parts.Count() >= 3 && parts[0] == "00")
			displayTime = string.Format("%1:%2", parts[1], parts[2]);

		m_wTimer.SetText("Mission End: " + displayTime);

		// Colour coding based on remaining time
		if (parts.Count() >= 3 && parts[0] == "00" && parts[1].ToInt() < 5)
			m_wTimer.SetColorInt(ARGB(255, 200, 65, 65));       // < 5 min — red
		else if (parts.Count() >= 3 && parts[0] == "00" && parts[1].ToInt() < 15)
			m_wTimer.SetColorInt(ARGB(255, 230, 230, 0));       // < 15 min — yellow
		else
			m_wTimer.SetColorInt(ARGB(255, 215, 215, 215));     // normal — light grey
	}
}
