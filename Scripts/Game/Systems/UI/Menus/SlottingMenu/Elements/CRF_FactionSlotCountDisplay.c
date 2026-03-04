/**
 * CRF_FactionSlotCountDisplay
 *
 * Modular widget component that updates the per-faction slot-count text badges
 * and the faction lock visuals shown in the Slotting Menu header strip.
 *
 * Expected child widget names (relative to the root widget):
 *   Slot count text:  "SlotsBlufor", "SlotsOpfor", "SlotsIndfor", "SlotsCiv"
 *   Lock overlay bg:  "BluforFactionLockBG", "OpforFactionLockBG",
 *                     "IndforFactionLockBG", "CivFactionLockBG"
 *   Lock overlay icon:"BluforFactionLock",   "OpforFactionLock",
 *                     "IndforFactionLock",   "CivFactionLock"
 *   Faction buttons:  "ButtonBlufor", "ButtonOpfor", "ButtonIndfor", "ButtonCiv"
 *
 * Usage:
 *   Widget slotCountRoot = m_wRoot.FindAnyWidget("FactionSlotCounts");
 *   m_FactionSlotCount = CRF_FactionSlotCountDisplay.Cast(
 *       slotCountRoot.FindHandler(CRF_FactionSlotCountDisplay));
 *
 *   // In OnMenuUpdate or after slot data changes:
 *   m_FactionSlotCount.UpdateFactionSlotCounts(
 *       takenBlu, totalBlu, takenOpf, totalOpf,
 *       takenInd, totalInd, takenCiv, totalCiv);
 */
class CRF_FactionSlotCountDisplay : SCR_ScriptedWidgetComponent
{
	protected TextWidget  m_wSlotsBlufor;
	protected TextWidget  m_wSlotsOpfor;
	protected TextWidget  m_wSlotsIndfor;
	protected TextWidget  m_wSlotsCiv;

	protected ImageWidget m_wBluforLockBG;
	protected ImageWidget m_wOpforLockBG;
	protected ImageWidget m_wIndforLockBG;
	protected ImageWidget m_wCivLockBG;

	protected ImageWidget m_wBluforLock;
	protected ImageWidget m_wOpforLock;
	protected ImageWidget m_wIndforLock;
	protected ImageWidget m_wCivLock;

	protected ButtonWidget m_wButtonBlufor;
	protected ButtonWidget m_wButtonOpfor;
	protected ButtonWidget m_wButtonIndfor;
	protected ButtonWidget m_wButtonCiv;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wSlotsBlufor = TextWidget.Cast(w.FindAnyWidget("SlotsBlufor"));
		m_wSlotsOpfor  = TextWidget.Cast(w.FindAnyWidget("SlotsOpfor"));
		m_wSlotsIndfor = TextWidget.Cast(w.FindAnyWidget("SlotsIndfor"));
		m_wSlotsCiv    = TextWidget.Cast(w.FindAnyWidget("SlotsCiv"));

		m_wBluforLockBG = ImageWidget.Cast(w.FindAnyWidget("BluforFactionLockBG"));
		m_wOpforLockBG  = ImageWidget.Cast(w.FindAnyWidget("OpforFactionLockBG"));
		m_wIndforLockBG = ImageWidget.Cast(w.FindAnyWidget("IndforFactionLockBG"));
		m_wCivLockBG    = ImageWidget.Cast(w.FindAnyWidget("CivFactionLockBG"));

		m_wBluforLock = ImageWidget.Cast(w.FindAnyWidget("BluforFactionLock"));
		m_wOpforLock  = ImageWidget.Cast(w.FindAnyWidget("OpforFactionLock"));
		m_wIndforLock = ImageWidget.Cast(w.FindAnyWidget("IndforFactionLock"));
		m_wCivLock    = ImageWidget.Cast(w.FindAnyWidget("CivFactionLock"));

		m_wButtonBlufor = ButtonWidget.Cast(w.FindAnyWidget("ButtonBlufor"));
		m_wButtonOpfor  = ButtonWidget.Cast(w.FindAnyWidget("ButtonOpfor"));
		m_wButtonIndfor = ButtonWidget.Cast(w.FindAnyWidget("ButtonIndfor"));
		m_wButtonCiv    = ButtonWidget.Cast(w.FindAnyWidget("ButtonCiv"));
	}

	/**
	 * Refreshes all four faction slot-count badges.
	 *
	 * @param takenBlu   Taken BLUFOR slots
	 * @param totalBlu   Total BLUFOR slots
	 * @param takenOpf   Taken OPFOR slots
	 * @param totalOpf   Total OPFOR slots
	 * @param takenInd   Taken INDFOR slots
	 * @param totalInd   Total INDFOR slots
	 * @param takenCiv   Taken CIV slots
	 * @param totalCiv   Total CIV slots
	 */
	void UpdateFactionSlotCounts(
		int takenBlu, int totalBlu,
		int takenOpf, int totalOpf,
		int takenInd, int totalInd,
		int takenCiv, int totalCiv)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();

		UpdateFactionBadge(m_wSlotsBlufor, m_wBluforLockBG, m_wBluforLock, m_wButtonBlufor,
			takenBlu, totalBlu, slottingManager.IsFactionValid("BLUFOR"));

		UpdateFactionBadge(m_wSlotsOpfor, m_wOpforLockBG, m_wOpforLock, m_wButtonOpfor,
			takenOpf, totalOpf, slottingManager.IsFactionValid("OPFOR"));

		UpdateFactionBadge(m_wSlotsIndfor, m_wIndforLockBG, m_wIndforLock, m_wButtonIndfor,
			takenInd, totalInd, slottingManager.IsFactionValid("INDFOR"));

		UpdateFactionBadge(m_wSlotsCiv, m_wCivLockBG, m_wCivLock, m_wButtonCiv,
			takenCiv, totalCiv, slottingManager.IsFactionValid("CIV"));
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateFactionBadge(
		TextWidget  wSlots,
		ImageWidget wLockBG,
		ImageWidget wLock,
		ButtonWidget wButton,
		int taken, int total, bool isValid)
	{
		if (!isValid)
			return;

		if (wSlots)
			wSlots.SetText(taken.ToString() + "/" + total);

		if (wLockBG)
			wLockBG.SetColor(Color.FromRGBA(63, 63, 63, 0));

		if (wLock)
			wLock.SetColor(Color.FromRGBA(255, 255, 255, 0));

		if (wButton)
			wButton.SetEnabled(true);
	}
}
