//------------------------------------------------------------------------------------------------
class COA_DefuseBombAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	CRF_GameMode_SearchAndDestroyComponent gameMode = CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent));
	EntityID siteID = null;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) {
		siteID = pOwnerEntity.GetID();
	};
	
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
		
		string stieDefused = "";
		
		if (pOwnerEntity.GetID() == gameMode.aSiteID)
			stieDefused = "SiteA";
		else
			stieDefused = "SiteB";
		
		COA_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(stieDefused, false);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		// Get user faction
		// Get game mode manager
		Faction userFaction = factionManager.GetLocalPlayerFaction();
		
		// Get defender side definition
		if (gameMode.defendingSide != userFaction.GetFactionKey() || !gameMode.countDownActive || ((siteID == gameMode.bSiteID) && !gameMode.bSitePlanted) || ((siteID == gameMode.aSiteID) && !gameMode.aSitePlanted))
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