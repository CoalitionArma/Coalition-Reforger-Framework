modded class CSI_HUD
{
	/**
	 * Updates the compass visibility based on player state.
	 * @param owner The entity that owns this component
	 * @param timeSlice Time elapsed since last update
	 */
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		// Call the parent class implementation first
		super.UpdateValues(owner, timeSlice);
		
		// Early exit if no local player entity exists
		if (!SCR_PlayerController.GetLocalMainEntity())
			return;
		
		// Get references
		bool isSpectator = CRF_GamemodeManager.IsSpectator();
		bool isWidgetVisible = m_wRoot.IsVisible();
		
		// Handle visibility based on player state:
		// - Hide compass when in spectator mode
		// - Show compass when in normal player mode
		if (isSpectator && isWidgetVisible)
		{
			m_wRoot.SetVisible(false);
		}
		else if (!isSpectator && !isWidgetVisible)
		{
			m_wRoot.SetVisible(true);
		}
	}
}