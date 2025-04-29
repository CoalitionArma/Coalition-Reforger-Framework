modded class SCR_PlayerController
{
	/**
	 * Called when the entity controlled by this player controller changes
	 * @param from The previous entity being controlled
	 * @param to The new entity being controlled
	 */
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
			return;
		
		// Get the CRF player controller comp
		CRF_PlayerControllerComponent playerControllerComp = CRF_PlayerControllerComponent.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
		{
			// Apply HDR settings if the player controller component is active
			if (playerControllerComp.m_bActivated)
				SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
	
			// Reset activation status
			playerControllerComp.m_bActivated = false;
		};

		// Call the parent implementation
		super.OnControlledEntityChanged(from, to);
	}

	/**
	 * Called when the player disconnects from the game
	 * Ensures settings are reset to their stored values
	 */
	override void DisconnectFromGame()
	{
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
			return;
		
		// Call the parent implementation
		super.DisconnectFromGame();

		// Get the CRF player controller comp
		CRF_PlayerControllerComponent playerControllerComp = CRF_PlayerControllerComponent.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
			// Reset settings to previously stored values
			playerControllerComp.ResetSettingsToStoredValues();
	}
}