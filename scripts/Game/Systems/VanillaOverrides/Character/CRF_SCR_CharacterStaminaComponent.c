modded class SCR_CharacterStaminaComponent : CharacterStaminaComponent
{
	override event void OnStaminaDrain(float pDrain)
	{
		CRF_SafestartManager SafestartManager = CRF_SafestartManager.GetInstance();
		
		if(!SafestartManager)
			return;
		
		if(GetGame().InPlayMode() && SafestartManager.GetSafestartStatus())
			AddStamina(Math.AbsFloat(pDrain));
	};
}