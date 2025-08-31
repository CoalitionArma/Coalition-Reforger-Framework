modded class SCR_CharacterStaminaComponent : CharacterStaminaComponent
{
	//-------------------------------------------------------------------------
	// Override: Prevents stamina drain during safestart mode
	//-------------------------------------------------------------------------
	override event void OnStaminaDrain(float pDrain)
	{
		// Get the Safestart Manager instance
		CRF_SafestartManager SafestartManager = CRF_SafestartManager.GetInstance();
		
		// If Safestart Manager doesn't exist, exit early
		if (!SafestartManager)
			return;
		
		// If in play mode and safestart is active, prevent stamina drain
		// by adding back the same amount that was drained
		if (GetGame().InPlayMode() && SafestartManager.GetSafestartStatus())
		{
			float staminaToRestore = Math.AbsFloat(pDrain);
			AddStamina(staminaToRestore);
		}
	};
}