modded class SCR_PlayerControllerCommandingComponent
{
	override bool AddElementsFromCategoryToMap(notnull SCR_PlayerCommandingMenuCategoryElement category, SCR_SelectionMenuCategoryEntry parentCategory = null)
	{
		SCR_MapMarkerMenuEntry shareMenuEntry = new SCR_MapMarkerMenuEntry();
		shareMenuEntry.SetName("Share Map Markers");
		shareMenuEntry.GetOnPerform().Insert(ShareMapMarkers);
		shareMenuEntry.SetIcon("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset", "FrequencyBig");
		
		m_MapContextualMenu.InsertCustomRadialEntry(shareMenuEntry, parentCategory);
		
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		SCR_AIGroup playerGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (!playerGroup)
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!CRF_SafestartManager.GetInstance())
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!playerGroup.IsPlayerLeader(playerId) || !CRF_SafestartManager.GetInstance().GetSafestartStatus())
		    return super.AddElementsFromCategoryToMap(category, parentCategory);
		
		SCR_MapMarkerMenuEntry menuEntry = new SCR_MapMarkerMenuEntry();
		menuEntry.SetName("Forward Deploy Element");
		menuEntry.GetOnPerform().Insert(CheckIfValidSpawn);
		menuEntry.SetIcon("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset", "RespawnSmall");
		
		m_MapContextualMenu.InsertCustomRadialEntry(menuEntry, parentCategory);
		return super.AddElementsFromCategoryToMap(category, parentCategory);
	}
	
	void ShareMapMarkers()
	{
		CRF_RplToAuthorityManager.GetInstance().ShareMapMarkers();
	}
	
	void CheckIfValidSpawn()
	{
	    int playerId = SCR_PlayerController.GetLocalPlayerId();
	    Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
	    if (!faction)
	        return;
	    
	    string factionKey = faction.GetFactionKey();
	    CRF_RplToAuthorityManager.GetInstance().RequestForwardDeploy(m_MapContextualMenu.GetMenuWorldPosition(), factionKey, playerId);
	}
}