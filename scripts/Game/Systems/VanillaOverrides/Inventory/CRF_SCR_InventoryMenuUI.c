modded class SCR_InventoryMenuUI
{
	override void OnItemAddedListener( IEntity item, notnull BaseInventoryStorageComponent storage )
	{
		super.OnItemAddedListener(item, storage);
		
		// If the item is from an arsenal
		if (MoveItemToStorageSlot_VirtualArsenal()) {
			// Grab item and player information
			CRF_AdminMenuManager adminMenu = CRF_AdminMenuManager.GetInstance();
			CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();
			string name = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
			InventoryItemComponent itemIIC = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			UIInfo itemUiInfo = itemIIC.GetUIInfo();
			RplComponent rplComponent = RplComponent.Cast(m_Player.FindComponent(RplComponent));
			if (!rplComponent)
				return;
			CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotDataFromCharacter(rplComponent.Id());
			
			// Log to admin menu
			adminMenu.LogAdminAction(name + "(" + slotData.GetSlotName() + ")" + " took a(n) " + string.Format(itemUiInfo.GetName()) + " from an arsenal");
		}
		
	}
}