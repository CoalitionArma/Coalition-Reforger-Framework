modded class SCR_CharacterStaminaComponent : CharacterStaminaComponent
{
		override event void OnStaminaDrain(float pDrain)
		{
			CRF_GamemodeComponent gamemodeComp = CRF_GamemodeComponent.GetInstance();
		
			if(!gamemodeComp)
				return;
		
			if(GetGame().InPlayMode() && gamemodeComp.GetSafestartStatus())
				AddStamina(Math.AbsFloat(pDrain));
		};
}