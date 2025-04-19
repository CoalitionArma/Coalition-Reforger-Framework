modded class SCR_CharacterStaminaComponent : CharacterStaminaComponent
{
	override event void OnStaminaDrain(float pDrain)
	{
		CRF_SafestartComponent safestartComponent = CRF_SafestartComponent.GetInstance();
		
		if(!safestartComponent)
			return;
		
		if(GetGame().InPlayMode() && safestartComponent.GetSafestartStatus())
			AddStamina(Math.AbsFloat(pDrain));
	};
}