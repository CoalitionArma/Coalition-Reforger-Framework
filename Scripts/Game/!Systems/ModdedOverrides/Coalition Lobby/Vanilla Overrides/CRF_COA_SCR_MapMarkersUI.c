modded class SCR_PlayerControllerCommandingComponent
{
	override bool AddElementsFromCategoryToMap(notnull SCR_PlayerCommandingMenuCategoryElement category, SCR_SelectionMenuCategoryEntry parentCategory = null)
	{
		super.AddElementsFromCategoryToMap(category, parentCategory);
		
		if (!COA_Gamemode.GetInstance())
		{
			return super.AddElementsFromCategoryToMap(category, parentCategory);
		}
		
		//Buh...
		COA_GamemodeManager gamemodeManager = COA_GamemodeManager.GetInstance();
		if (!gamemodeManager)
			return super.AddElementsFromCategoryToMap(category, parentCategory);

		SCR_MapMarkerMenuEntry shareMenuEntry = new SCR_MapMarkerMenuEntry();
		shareMenuEntry.SetName("Share Map Markers");
		shareMenuEntry.GetOnPerform().Insert(ShareMapMarkers);
		shareMenuEntry.SetIcon("{F7E8D4834A3AFF2F}UI/Imagesets/Conflict/conflict-icons-bw.imageset", "FrequencyBig");
		
		if (!m_MapContextualMenu)
			return super.AddElementsFromCategoryToMap(category, parentCategory);

		m_MapContextualMenu.InsertCustomRadialEntry(shareMenuEntry, parentCategory);
        
		return super.AddElementsFromCategoryToMap(category, parentCategory);
	}
	
	void ShareMapMarkers()
	{
		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.ShareMapMarkers();
	}
}