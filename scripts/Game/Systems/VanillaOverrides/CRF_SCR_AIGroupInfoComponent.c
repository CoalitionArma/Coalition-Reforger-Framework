modded class SCR_AIGetCombatMoveRequestParameters_ChangeStance
{//---------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn(PORT_REQUEST, rqBase);
		SCR_AICombatMoveRequest_ChangeStance rq = SCR_AICombatMoveRequest_ChangeStance.Cast(rqBase);
		if (!rq)
			return SCR_AIErrorMessages.NodeErrorCombatMoveRequest(this, owner, rqBase);
			
		if (rq.m_eStance == ECharacterStance.PRONE && CRF_Gamemode.GetInstance().m_bDisableAICrouching)
			SetVariableOut(PORT_STANCE, ECharacterStance.CROUCH);
		else
			SetVariableOut(PORT_STANCE, rq.m_eStance);
		
		return ENodeResult.SUCCESS;
	}
}