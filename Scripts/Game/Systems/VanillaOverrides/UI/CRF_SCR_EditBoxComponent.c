modded class SCR_EditBoxComponent
{
	// Identifies the marker name edit box (vanilla SCR_MapMarkersUI.USERID_EDITBOX / _MIL)
	// among all edit boxes, so this guard only applies to the marker dialog, not every text field.
	protected bool CRF_IsMarkerEditBox()
	{
		if (!m_wEditBox)
			return false;

		if (!GetGame().GetInputManager().IsUsingMouseAndKeyboard())
			return false;

		int userID = m_wEditBox.GetUserID();
		return userID == 1000 || userID == 1001; // SCR_MapMarkersUI.USERID_EDITBOX / USERID_EDITBOX_MIL
	}

	// Swallow focus-lost caused by the mouse merely wandering onto another widget while typing.
	override bool OnFocusLost(Widget w, int x, int y)
	{
		if (CRF_IsMarkerEditBox())
			return true;

		return super.OnFocusLost(w, x, y);
	}

	override void OnHandlerFocusLost()
	{
		if (CRF_IsMarkerEditBox())
			return;

		super.OnHandlerFocusLost();
	}

	// Safety net: if the native widget still exits write mode despite the guards above,
	// re-arm it here instead of tearing down the typing state.
	override void UpdateInteractionState(bool forceDisabled)
	{
		if (CRF_IsMarkerEditBox() && !forceDisabled)
		{
			if (m_wEditBoxWidget && !m_wEditBoxWidget.IsInWriteMode())
				ActivateWriteMode(true);

			string currentText = GetEditBoxText();
			if (currentText != m_sTextPrevious)
			{
				m_sTextPrevious = currentText;
				m_OnTextChange.Invoke(m_sTextPrevious);
			}

			return;
		}

		super.UpdateInteractionState(forceDisabled);
	}
}
