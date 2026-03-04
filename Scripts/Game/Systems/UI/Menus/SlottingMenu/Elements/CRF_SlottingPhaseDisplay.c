/**
 * CRF_SlottingPhaseDisplay
 *
 * Modular widget component that shows the current slotting phase label
 * ("Leaders and Medics" / "Specialties" / "Everyone") and plays the
 * default notification sound whenever the phase changes.
 *
 * Expected child widget name (relative to the root widget):
 *   "CurrentSlotPhase" — TextWidget
 *
 * Usage:
 *   Widget phaseRoot = m_wRoot.FindAnyWidget("SlottingPhaseDisplay");
 *   m_SlottingPhaseDisplay = CRF_SlottingPhaseDisplay.Cast(
 *       phaseRoot.FindHandler(CRF_SlottingPhaseDisplay));
 *   m_SlottingPhaseDisplay.Init(m_Gamemode.m_SlottingState);  // once on open
 *
 *   // In OnMenuUpdate:
 *   m_SlottingPhaseDisplay.UpdateSlottingPhaseDisplay();
 */
class CRF_SlottingPhaseDisplay : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wCurrentSlotPhase;

	protected int m_iLocalSlottingState = -1;

	protected static const string SOUND_PHASE_CHANGED =
		"{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav";

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wCurrentSlotPhase = TextWidget.Cast(w.FindAnyWidget("CurrentSlotPhase"));
	}

	/**
	 * Seeds the cached state so the first frame does not trigger a false
	 * "phase changed" sound. Call once after the menu opens.
	 *
	 * @param initialSlottingState Current value of CRF_Gamemode.m_SlottingState.
	 */
	void Init(int initialSlottingState)
	{
		m_iLocalSlottingState = initialSlottingState;
		RefreshPhaseText();
	}

	/**
	 * Checks for a phase change, plays a notification sound when detected,
	 * and refreshes the phase label. Call every frame from OnMenuUpdate.
	 */
	void UpdateSlottingPhaseDisplay()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return;

		if (m_iLocalSlottingState != gamemode.m_SlottingState)
		{
			m_iLocalSlottingState = gamemode.m_SlottingState;
			AudioSystem.PlaySound(SOUND_PHASE_CHANGED);
		}

		RefreshPhaseText();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshPhaseText()
	{
		if (!m_wCurrentSlotPhase)
			return;

		string phaseText;
		switch (m_iLocalSlottingState)
		{
			case 0:  phaseText = "Leaders and Medics"; break;
			case 1:  phaseText = "Specialties";        break;
			default: phaseText = "Everyone";            break;
		}

		m_wCurrentSlotPhase.SetText(phaseText);
	}
}
