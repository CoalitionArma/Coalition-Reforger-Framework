modded class SCR_SliderComponent
{
	// Same hover-steals-focus problem as buttons - see CRF_SCR_ButtonBaseComponent.c.
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
