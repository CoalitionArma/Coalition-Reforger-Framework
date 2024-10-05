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
		GetGame().GetCallqueue().CallLater(OpenMenu, 3000)
		
	}
	
	void OpenMenu()
	{
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.CRF_GearscriptUI, DialogPriority.WARNING, 0, true);
	}
	
	#endif
	
	
};