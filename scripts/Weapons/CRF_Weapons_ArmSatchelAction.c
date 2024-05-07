//------------------------------------------------------------------------------------------------
class CRF_Weapons_ArmSatchelAction : ScriptedUserAction
{	
	SCR_FactionManager factionManager;
	CRF_SearchAndDestroyGameModeComponent gameMode;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		

		
		string sitePlanted = "";
		
		if (pOwnerEntity.GetID() == gameMode.aSiteID)
			sitePlanted = "SiteA";
		else
			sitePlanted = "SiteB";
		
		CRF_GameModePlayerComponent.GetInstance().Owner_ToggleBombPlanted(sitePlanted, true);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
};