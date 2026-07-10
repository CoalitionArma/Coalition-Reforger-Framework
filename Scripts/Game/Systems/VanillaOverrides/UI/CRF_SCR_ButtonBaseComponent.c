modded class SCR_ButtonBaseComponent
{
	// Buttons auto-focus themselves on mere mouse hover, which steals focus off the marker
	// text field mid-typing. Suppress that while a marker edit dialog is open.
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (SCR_MapMarkersUI.CRF_IsMarkerEditDialogOpen())
			return false;

		return super.OnMouseEnter(w, x, y);
	}

	override bool OnFocus(Widget w, int x, int y)
	{
		if (SCR_MapMarkersUI.CRF_IsMarkerEditDialogOpen())
			return false;

		return super.OnFocus(w, x, y);
	}
}
