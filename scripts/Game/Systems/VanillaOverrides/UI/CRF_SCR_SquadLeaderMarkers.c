modded class SCR_MapMarkerSquadLeader
{
	/**
	 * Override method called when player ID is updated.
	 * Makes the squad leader marker visible to the local player.
	 */
	override void OnPlayerIdUpdate()
	{
		// Get the reference to the player controller from the game
		PlayerController pController = GetGame().GetPlayerController();
		
		// If no valid player controller is found, exit the method
		if (!pController)
			return;
		
		// Set this squad leader marker to be visible to the local player
		SetLocalVisible(true);
	}
}