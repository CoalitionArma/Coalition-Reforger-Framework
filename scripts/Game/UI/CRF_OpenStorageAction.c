class CRF_OpenStorageAction : SCR_OpenStorageAction
{
	#ifndef DISABLE_INVENTORY
	//------------------------------------------------------------------------------------------------
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CRF_Gearscript_OverlayClass m_GearScriptUI;
		auto vicinity = CharacterVicinityComponent.Cast( pUserEntity .FindComponent( CharacterVicinityComponent ));
		if ( !vicinity )
			return;
		
		manager.SetStorageToOpen(pOwnerEntity);
		manager.OpenInventory();
		Print(ChimeraMenuPreset.CRF_GearscriptUI);
		Print("Opening next menu");
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.CRF_GearscriptUI, DialogPriority.INFORMATIVE, 0, true);
	}
	
	void CloseMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		ChimeraMenuPreset menu = ChimeraMenuPreset.CRF_GearscriptUI;
		
		MenuBase gsMenu = menuManager.FindMenuByPreset(menu);
		GetGame().GetMenuManager().CloseMenu(gsMenu);
	}
	
	#endif
	
	
};