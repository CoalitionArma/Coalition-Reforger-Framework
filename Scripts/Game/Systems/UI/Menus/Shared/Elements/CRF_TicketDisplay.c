/**
 * CRF_TicketDisplay
 *
 * Modular widget component that manages the four-faction ticket counter panel
 * shown in the Spectator Menu. Each faction's row becomes visible the moment
 * that faction has ever had at least one ticket (latching behaviour) and stays
 * visible even if the count reaches zero.
 *
 * Expected child widget names (root widget must contain):
 *   "BLUFORTickets"  — outer wrapper widget toggled visible/invisible
 *   "BLUFORTicketsText" — TextWidget displaying the ticket string
 *   "OPFORTickets"   / "OPFORTicketsText"
 *   "INDFORTickets"  / "INDFORTicketsText"
 *   "CIVTickets"     / "CIVTicketsText"
 *
 * Usage:
 *   Widget ticketRoot = m_wRoot.FindAnyWidget("TicketDisplay");
 *   m_TicketDisplay = CRF_TicketDisplay.Cast(ticketRoot.FindHandler(CRF_TicketDisplay));
 *
 *   // In OnMenuUpdate (every frame):
 *   m_TicketDisplay.UpdateTickets();
 */
class CRF_TicketDisplay : SCR_ScriptedWidgetComponent
{
	// Wrapper widgets — toggled visible/invisible
	protected Widget     m_wBLUFOR;
	protected Widget     m_wOPFOR;
	protected Widget     m_wINDFOR;
	protected Widget     m_wCIV;

	// Text widgets — display "FACTION Tickets: N"
	protected TextWidget m_wBLUFORText;
	protected TextWidget m_wOPFORText;
	protected TextWidget m_wINDFORText;
	protected TextWidget m_wCIVText;

	// Latching flags — once true the row stays visible
	protected bool m_bBLUFORActive;
	protected bool m_bOPFORActive;
	protected bool m_bINDFORActive;
	protected bool m_bCIVActive;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wBLUFOR     = w.FindAnyWidget("BLUFORTickets");
		m_wBLUFORText = TextWidget.Cast(w.FindAnyWidget("BLUFORTicketsText"));

		m_wOPFOR      = w.FindAnyWidget("OPFORTickets");
		m_wOPFORText  = TextWidget.Cast(w.FindAnyWidget("OPFORTicketsText"));

		m_wINDFOR     = w.FindAnyWidget("INDFORTickets");
		m_wINDFORText = TextWidget.Cast(w.FindAnyWidget("INDFORTicketsText"));

		m_wCIV        = w.FindAnyWidget("CIVTickets");
		m_wCIVText    = TextWidget.Cast(w.FindAnyWidget("CIVTicketsText"));
	}

	/**
	 * Reads the current ticket counts from CRF_RespawnManager and refreshes
	 * the four faction rows. Call every frame from OnMenuUpdate.
	 */
	void UpdateTickets()
	{
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (!rm)
			return;

		// BLUFOR
		if (rm.m_iBLUFORTickets > 0 && !m_bBLUFORActive)
			m_bBLUFORActive = true;

		if (m_bBLUFORActive)
		{
			if (m_wBLUFOR) m_wBLUFOR.SetVisible(true);
			if (m_wBLUFORText) m_wBLUFORText.SetText("BLUFOR Tickets: " + rm.m_iBLUFORTickets.ToString());
		}
		else if (m_wBLUFOR)
			m_wBLUFOR.SetVisible(false);

		// OPFOR
		if (rm.m_iOPFORTickets > 0 && !m_bOPFORActive)
			m_bOPFORActive = true;

		if (m_bOPFORActive)
		{
			if (m_wOPFOR) m_wOPFOR.SetVisible(true);
			if (m_wOPFORText) m_wOPFORText.SetText("OPFOR Tickets: " + rm.m_iOPFORTickets.ToString());
		}
		else if (m_wOPFOR)
			m_wOPFOR.SetVisible(false);

		// INDFOR
		if (rm.m_iINDFORTickets > 0 && !m_bINDFORActive)
			m_bINDFORActive = true;

		if (m_bINDFORActive)
		{
			if (m_wINDFOR) m_wINDFOR.SetVisible(true);
			if (m_wINDFORText) m_wINDFORText.SetText("INDFOR Tickets: " + rm.m_iINDFORTickets.ToString());
		}
		else if (m_wINDFOR)
			m_wINDFOR.SetVisible(false);

		// CIV
		if (rm.m_iCIVTickets > 0 && !m_bCIVActive)
			m_bCIVActive = true;

		if (m_bCIVActive)
		{
			if (m_wCIV) m_wCIV.SetVisible(true);
			if (m_wCIVText) m_wCIVText.SetText("CIV Tickets: " + rm.m_iCIVTickets.ToString());
		}
		else if (m_wCIV)
			m_wCIV.SetVisible(false);
	}
}
