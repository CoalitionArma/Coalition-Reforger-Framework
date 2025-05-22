class CRF_CharacterLoading: ChimeraMenuBase
{
	protected bool m_bFailsafeActive;
	
	/**
	 * Called when the menu is opened
	 */
	override void OnMenuOpen()
	{	
		super.OnMenuOpen()
		
		GetGame().GetCallqueue().CallLater(ActivateFailsafe, 5000, false);
	}
	
	/**
	 * Cleans up resources when the menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose()
		
		GetGame().GetCallqueue().Remove(ActivateFailsafe);
	};
	
	/**
	 * Updates the menu state during each frame
	 * @param tDelta - Time elapsed since last frame
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		
		// If we have a valid main entity OR the menu has been up for more than 5 seconds;
		if ((mainEntity && !CRF_GamemodeManager.IsSpectator(mainEntity)) || m_bFailsafeActive)
			GetGame().GetMenuManager().CloseMenu(this);
	}
	
	protected void ActivateFailsafe()
	{
		m_bFailsafeActive = true;
	}
}