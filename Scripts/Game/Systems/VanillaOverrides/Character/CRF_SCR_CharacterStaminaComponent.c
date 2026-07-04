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
		{
			// Recovery event — cancel proportionally to lone-wolf penalty intensity
			if (pDrain < 0)
			{
				CRF_LoneWolfPenaltyManager loneWolfPenaltyManager = CRF_LoneWolfPenaltyManager.GetForCharacter(GetOwner());
				if (loneWolfPenaltyManager)
				{
					float intensity = loneWolfPenaltyManager.GetLoneWolfPenaltyIntensity();
					if (intensity > 0)
						AddStamina(pDrain * intensity);
				}
			}
			return;
		}

		CRF_LoneWolfPenaltyManager loneWolfPenaltyManager = CRF_LoneWolfPenaltyManager.GetForCharacter(GetOwner());
		if (!loneWolfPenaltyManager)
			return;

		float extraDrainMultiplier = loneWolfPenaltyManager.GetLoneWolfExtraStaminaDrainMultiplier();
		if (extraDrainMultiplier <= 0)
			return;

		AddStamina(-(pDrain * extraDrainMultiplier));
	};
}