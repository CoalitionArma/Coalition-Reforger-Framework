//------------------------------------------------------------------------------------------------
class COA_PlantBombAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	CRF_GameMode_SearchAndDestroyComponent gameMode = CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent));
	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Plant Bomb";
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
		
		// We only want one site planted at a time
		if (gameMode.countDownActive)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("A Bomb Is Already Active!", 10, "Only one bomb site can be active at a time");
			return;
		}
		
		COA_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(pOwnerEntity.GetID(), true);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		// Get user faction
		// Get game mode manager
		Faction userFaction = factionManager.GetLocalPlayerFaction();
		
		// Get attacker side definition
		if (gameMode.attackingSide != userFaction.GetFactionKey())
			return false;
		
		// else true
		return true;
	}	
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
};