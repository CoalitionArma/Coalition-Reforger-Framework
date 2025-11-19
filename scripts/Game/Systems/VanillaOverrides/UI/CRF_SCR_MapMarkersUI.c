modded class SCR_PlayerControllerCommandingComponent
{
	override bool AddElementsFromCategoryToMap(notnull SCR_PlayerCommandingMenuCategoryElement category, SCR_SelectionMenuCategoryEntry parentCategory = null)
	{
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		if (!SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId))
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!CRF_SafestartManager.GetInstance())
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId).IsPlayerLeader(playerId) || !CRF_SafestartManager.GetInstance().GetSafestartStatus())
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		SCR_MapMarkerMenuEntry menuEntry = new SCR_MapMarkerMenuEntry();
		menuEntry.SetName("Forward Deploy Element");
		menuEntry.GetOnPerform().Insert(CheckIfValidSpawn);
		menuEntry.SetIcon("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset", "RespawnSmall");
		
		m_MapContextualMenu.InsertCustomRadialEntry(menuEntry, parentCategory);
		return super.AddElementsFromCategoryToMap(category, parentCategory);
	}
	
	void CheckIfValidSpawn()
	{
		string factionKey = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey();
		CRF_RplToAuthorityManager.GetInstance().RequestForwardDeploy(m_MapContextualMenu.GetMenuWorldPosition(), factionKey, SCR_PlayerController.GetLocalPlayerId());
	}
}