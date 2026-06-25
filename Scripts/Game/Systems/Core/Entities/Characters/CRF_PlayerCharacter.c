class CRF_PlayerCharacterClass : SCR_ChimeraCharacterClass
{
}

class CRF_PlayerCharacter : SCR_ChimeraCharacter
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 DISABLE AI METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void DisableAI()
	{
		AIControlComponent aiComponent = AIControlComponent.Cast(this.FindComponent(AIControlComponent));
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (!agent)
			return;
		
		agent.DeactivateAI();
		
		// Double-check deactivation next frame
		GetGame().GetCallqueue().Call(DisableAIWrap, aiComponent);
	}

	//------------------------------------------------------------------------------------------------
	protected void DisableAIWrap(AIControlComponent aiComponent)
	{
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
	}
}