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
		
		// We only want to defuse if a site is active
		if (!gameMode.countDownActive)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("There is no bomb to defuse", 10);
			return;
		}
		
		COA_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(pOwnerEntity.GetID(), false);
		
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
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBroadcastScript()
	{
		return false;
	}
};