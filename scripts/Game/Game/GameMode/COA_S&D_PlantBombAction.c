//------------------------------------------------------------------------------------------------
class COA_PlantBombAction : ScriptedUserAction
{	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Plant Bomb";
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		Print("PerformAction");
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		SCR_PopUpNotification.GetInstance().PopupMsg("Bomb planted");
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		// Get user side
		
		// Get attacker side definition
		
		// If user side != attacker side definition, return false
		
		// else true
		return true;
	}	
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
	
	override bool CanBroadcastScript()
	{
		return true;
	}
};