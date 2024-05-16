class CRF_RushPlantAction : ScriptedUserAction
{
	SCR_FactionManager factionManager;
	CRF_RushGameModeComponent gameMode;
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode()) return;
		
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		gameMode = CRF_RushGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGameModeComponent));
	}
}