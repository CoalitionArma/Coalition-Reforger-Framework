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
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		CRF_GameModePlayerComponent.GetInstance().Owner_ToggleRushPlanted(pOwnerEntity.GetName(), true);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
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
}