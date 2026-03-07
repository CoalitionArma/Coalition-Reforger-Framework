/**
 * CRF_PhaseIndicatorDisplay
 *
 * Modular widget component for the four phase-indicator border images shown
 * across the top of the Slotting and AAR menus
 * (Preview → Slotting → Game → AAR).
 *
 * The component finds the four image widgets by name, then highlights the
 * one that matches the current CRF_EGamemodeState.
 *
 * Expected child widget names on the root widget:
 *   "PreviewBorder", "SlottingBorder", "GameBorder", "AARBorder"
 *
 * Usage:
 *   Widget phaseRoot = m_wRoot.FindAnyWidget("PhaseIndicator");
 *   m_PhaseDisplay = CRF_PhaseIndicatorDisplay.Cast(phaseRoot.FindHandler(CRF_PhaseIndicatorDisplay));
 *   m_PhaseDisplay.UpdatePhaseIndicator();
 */
class CRF_PhaseIndicatorDisplay : SCR_ScriptedWidgetComponent
{
	protected ImageWidget m_wPreview;
	protected ImageWidget m_wSlotting;
	protected ImageWidget m_wGame;
	protected ImageWidget m_wAAR;

	protected static const ref Color COLOR_ACTIVE   = Color.FromRGBA(122, 0, 0, 255);
	protected static const ref Color COLOR_INACTIVE  = Color.FromRGBA(63,  63, 63, 255);

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wPreview  = ImageWidget.Cast(w.FindAnyWidget("PreviewBorder"));
		m_wSlotting = ImageWidget.Cast(w.FindAnyWidget("SlottingBorder"));
		m_wGame     = ImageWidget.Cast(w.FindAnyWidget("GameBorder"));
		m_wAAR      = ImageWidget.Cast(w.FindAnyWidget("AARBorder"));
	}

	/**
	 * Reads the current gamemode state and highlights the matching phase
	 * indicator, dimming the others. Safe to call every frame or once on open.
	 */
	void UpdatePhaseIndicator()
	{
		if (!m_wPreview || !m_wSlotting || !m_wGame || !m_wAAR)
			return;

		// Reset all to inactive
		m_wPreview.SetColor(COLOR_INACTIVE);
		m_wSlotting.SetColor(COLOR_INACTIVE);
		m_wGame.SetColor(COLOR_INACTIVE);
		m_wAAR.SetColor(COLOR_INACTIVE);

		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return;

		switch (gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: m_wPreview.SetColor(COLOR_ACTIVE);  break;
			case CRF_EGamemodeState.SLOTTING: m_wSlotting.SetColor(COLOR_ACTIVE); break;
			case CRF_EGamemodeState.GAME:     m_wGame.SetColor(COLOR_ACTIVE);     break;
			case CRF_EGamemodeState.AAR:      m_wAAR.SetColor(COLOR_ACTIVE);      break;
		}
	}
}
