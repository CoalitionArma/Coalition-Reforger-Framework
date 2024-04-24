//------------------------------------------------------------------------------------------------
class CRF_DefuseBombAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager;
	CRF_SearchAndDestroyGameModeComponent gameMode;
	EntityID siteID = null;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) {
		if (!GetGame().InPlayMode()) return;
		
		siteID = pOwnerEntity.GetID();
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		gameMode = CRF_SearchAndDestroyGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGameModeComponent));
	};
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		string siteDefused = "";
		
		if (pOwnerEntity.GetID() == gameMode.aSiteID)
			siteDefused = "SiteA";
		else
			siteDefused = "SiteB";
		
		CRF_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(siteDefused, false);
		
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