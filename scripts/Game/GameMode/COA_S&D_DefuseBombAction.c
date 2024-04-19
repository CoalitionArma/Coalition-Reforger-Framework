//------------------------------------------------------------------------------------------------
class COA_DefuseBombAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	CRF_GameMode_SearchAndDestroyComponent gameMode = CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent));
	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Defuse Bomb";
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		PrintFormat("aSitePlanted: %1\nbSitePlanted: %2",gameMode.aSitePlanted,gameMode.bSitePlanted);
		
		// We only want to defuse if a site is active
		if (!gameMode.countDownActive)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("There is no bomb to defuse", 10);
			return;
		}
		
		// Set which site is planted
		GetGame().GetCallqueue().Remove(gameMode.startCountdown);
		gameMode.countDownActive = false;
		if (gameMode.aSitePlanted)
			gameMode.aSitePlanted = false;
		else	
			gameMode.bSitePlanted = false;
		
		PrintFormat("aSitePlanted: %1\nbSitePlanted: %2",gameMode.aSitePlanted,gameMode.bSitePlanted);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		// Get user faction
		// Get game mode manager
		Faction userFaction = factionManager.GetLocalPlayerFaction();
		
		// Get defender side definition
		if (gameMode.defendingSide != userFaction.GetFactionKey())
			return false;
		
		// else true
		return true;
	}	
	
	/*override bool HasLocalEffectOnlyScript()
	{
		return true;
	}*/

	override bool CanBroadcastScript()
	{
		return true;
	}
};