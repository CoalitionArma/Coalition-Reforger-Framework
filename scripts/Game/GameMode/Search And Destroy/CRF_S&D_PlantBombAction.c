//------------------------------------------------------------------------------------------------
class CRF_PlantBombAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager;
	CRF_SearchAndDestroyGameModeComponent gameMode;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode()) return;
		
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		gameMode = CRF_SearchAndDestroyGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGameModeComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		string sitePlanted = "";
		
		if (pOwnerEntity.GetID() == gameMode.aSiteID)
			sitePlanted = "SiteA";
		else
			sitePlanted = "SiteB";
		
		CRF_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(sitePlanted, true);
		
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
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (gameMode.countDownActive)
			return false;
		
		// else true
		return true;
	}	
	
	//------------------------------------------------------------------------------------------------
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