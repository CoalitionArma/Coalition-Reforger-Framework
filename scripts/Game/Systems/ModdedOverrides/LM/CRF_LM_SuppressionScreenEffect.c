/*modded class LM_SuppressionScreenEffect
{
	/**
	 * Checks if the local player entity is valid and not in spectator mode
	 * @return True if the player exists and is not a spectator
	 
	private bool IsValidLocalPlayer()
	{
		// Check if player controller exists
		if (!m_pPlayerController)
			return false;
			
		// Check if local entity exists
		IEntity localEntity = m_pPlayerController.GetLocalMainEntity();
		if (!localEntity)
			return false;
			
		// Check if the player is not a spectator
		if (CRF_GamemodeManager.IsSpectator(localEntity))
			return false;
			
		return true;
	}
	
	/**
	 * Override: Gets the current suppression amount for the local player
	 * @return Suppression amount or 0 if player is invalid or spectating
	 
	override private float GetSuppressionAmount()
	{
		if (IsValidLocalPlayer())
		{
			return m_pPlayerController.GetSuppressionAmount();
		}
		return 0;
	}
	
	/**
	 * Override: Handles suppression flinch effect
	 * Only applies the effect if player is valid and not spectating
	 
	override private void OnSuppressionFlinch()
	{
		if (IsValidLocalPlayer())
		{
			FlinchEffect();
		}
	}
}*/