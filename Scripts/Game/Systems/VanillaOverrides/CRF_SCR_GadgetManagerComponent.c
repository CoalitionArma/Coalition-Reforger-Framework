modded class SCR_GadgetManagerComponent
{
	override protected void OnGadgetInput(float value, EActionTrigger reason)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		CRF_ParachutePlayerComponent paraComp = CRF_ParachutePlayerComponent.Cast(pc.FindComponent(CRF_ParachutePlayerComponent));
		if (!paraComp)
			return;
		
		if (paraComp.m_DeployedChuteEntity)
			return;
		
		super.OnGadgetInput(value, reason);

	}
}