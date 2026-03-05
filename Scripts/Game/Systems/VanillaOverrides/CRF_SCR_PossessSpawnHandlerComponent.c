modded class SCR_PossessSpawnHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnFinalizeDone_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnData data, IEntity entity)
	{
		// Assign possessed entity's faction to the player
		PlayerController pc = requestComponent.GetPlayerController();
		SCR_PlayerFactionAffiliationComponent playerFactionComp = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		Faction targetFaction = GetFaction(entity);
		if (!targetFaction || targetFaction == playerFactionComp.GetAffiliatedFaction())
			return;

		playerFactionComp.SetAffiliatedFaction(targetFaction);

		// Notify faction manager
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		factionManager.UpdatePlayerFaction_S(playerFactionComp);

		// Notify game mode
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.OnPlayerFactionSet_S(playerFactionComp, targetFaction);
	}

	//------------------------------------------------------------------------------------------------
	protected Faction GetFaction(IEntity entity)
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!facComp)
			return null;
		
		return facComp.GetAffiliatedFaction();
	}
}