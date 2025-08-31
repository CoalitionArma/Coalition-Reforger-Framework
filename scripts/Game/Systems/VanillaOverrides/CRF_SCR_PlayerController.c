modded class SCR_PlayerController
{
	/**
	 * Called when the player controller updates (typically whenever a player joins/rejoins)
	 */
	override protected void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();
		
		if (RplSession.Mode() == RplMode.Dedicated || !CRF_Gamemode.GetInstance() || !CRF_PlayerControllerManager.GetInstance())
			return;
		
		CRF_PlayerControllerManager.GetInstance().InitilizePlayerControllerComp();
	}
	
	/**
	 * Called when the entity controlled by this player controller changes
	 * @param from The previous entity being controlled
	 * @param to The new entity being controlled
	 */
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{	
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
		{
			// Call the parent implementation
			super.OnControlledEntityChanged(from, to);
			return;
		};
		
		// Get the CRF player controller comp
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
		{
			// Apply HDR settings if the player controller component is active
			if (playerControllerComp.m_bActivated)
				SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
	
			// Reset activation status
			playerControllerComp.m_bActivated = false;
		};

		// Handle race condition: If player is being assigned initial entity when they should have a playable character
		if (to && to.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource() && 
			CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			int playerId = GetPlayerId();
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			
			// Check if this player should have a proper character instead of initial entity
			if (slottingManager && slottingManager.IsPlayerInASlot(playerId) && !slottingManager.IsPlayerConsideredDead(playerId))
			{
				// Request re-initialization from server to fix race condition
				CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
				if (rplManager)
				{
					GetGame().GetCallqueue().CallLater(rplManager.RequestInitilizePlayer, 250, false, playerId);
				}
			}
		}

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
		{
			// Call the parent implementation
			super.DisconnectFromGame();
			return;
		};

		// Get the CRF player controller comp
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
			// Reset settings to previously stored values
			playerControllerComp.ResetSettingsToStoredValues();
		
		super.DisconnectFromGame();
	}
}