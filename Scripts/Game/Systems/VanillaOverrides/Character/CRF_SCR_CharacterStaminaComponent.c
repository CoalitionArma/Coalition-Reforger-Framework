modded class SCR_CharacterStaminaComponent : CharacterStaminaComponent
{
	//-------------------------------------------------------------------------
	// Override: Prevents stamina drain during safestart mode
	//-------------------------------------------------------------------------
	override event void OnStaminaDrain(float pDrain)
	{
		// Get the Safestart Manager instance
		CRF_SafestartManager SafestartManager = CRF_SafestartManager.GetInstance();
		
		// If in play mode and safestart is active, prevent stamina drain
		// by adding back the same amount that was drained
		if (SafestartManager && GetGame().InPlayMode() && SafestartManager.GetSafestartStatus())
		{
			float staminaToRestore = Math.AbsFloat(pDrain);
			AddStamina(staminaToRestore);
			return;
		}

		if (pDrain <= 0)
			return;

		CRF_PlayerCharacter playerCharacter = CRF_PlayerCharacter.Cast(GetOwner());
		if (!playerCharacter)
			return;

		float extraDrainMultiplier = playerCharacter.GetLoneWolfExtraStaminaDrainMultiplier();
		if (extraDrainMultiplier <= 0)
			return;

		AddStamina(-(pDrain * extraDrainMultiplier));
	};
}