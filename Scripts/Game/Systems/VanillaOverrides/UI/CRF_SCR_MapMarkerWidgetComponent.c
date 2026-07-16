modded class SCR_MapMarkerWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	//! Vanilla always hid marker text regardless of state; fix it and keep it visible without needing hover
	override void SetTextVisible(bool state)
	{
		m_bShowText = state;
		m_wMarkerText.SetVisible(state);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla re-hides marker text on mouse leave; keep it shown when it's supposed to be always visible
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		bool result = super.OnMouseLeave(w, enterW, x, y);

		if (m_bShowText)
			m_wMarkerText.SetVisible(true);

		return result;
	}
}
