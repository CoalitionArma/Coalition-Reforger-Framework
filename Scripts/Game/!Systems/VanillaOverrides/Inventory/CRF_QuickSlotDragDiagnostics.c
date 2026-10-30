//------------------------------------------------------------------------------------------------
//! TEMPORARY diagnostics - remove once the hotbar drag bug is fixed.
//! Dragging the medical kit onto the hotbar does nothing, while hovering it and pressing a number key
//! assigns it. Both end in the same quick-slot call; dropping also needs the game to see the hotbar slot
//! under the cursor and that slot to pass IsCompatible() and the quick-slot size check. This logs, for every
//! drop, what was dragged, where the game thinks it was dropped and how those checks came out, so one test
//! in game shows which step fails. Search the client's script.log for "[CRF_QuickSlotDrag]".
modded class SCR_InventoryMenuUI
{
	//------------------------------------------------------------------------------------------------
	override protected void Action_Drop()
	{
		CRF_LogQuickSlotDrop();
		super.Action_Drop();
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_LogQuickSlotDrop()
	{
		if (!m_pSelectedSlotUI)
			return;

		string item = "?";
		bool fitsQuickSlot;
		InventoryItemComponent itemComponent = m_pSelectedSlotUI.GetInventoryItemComponent();
		if (itemComponent)
		{
			fitsQuickSlot = CanItemFitIntoQuickSlots(itemComponent);
			IEntity owner = itemComponent.GetOwner();
			if (owner && owner.GetPrefabData())
				item = owner.GetPrefabData().GetPrefabName();
		}

		string from = "none";
		if (m_pSelectedSlotUI.GetStorageUI())
			from = m_pSelectedSlotUI.GetStorageUI().Type().ToString();

		string target = "none";
		string targetStorage = "none";
		bool compatible;
		int slotIndex = -1;
		if (m_pFocusedSlotUI)
		{
			target = m_pFocusedSlotUI.Type().ToString();
			compatible = m_pFocusedSlotUI.IsCompatible();
			slotIndex = m_pFocusedSlotUI.GetSlotIndex();
			if (m_pFocusedSlotUI.GetStorageUI())
				targetStorage = m_pFocusedSlotUI.GetStorageUI().Type().ToString();
		}

		string hovered = "none";
		if (m_pActiveHoveredStorageUI)
			hovered = m_pActiveHoveredStorageUI.Type().ToString();

		string underCursor = "none";
		Widget widget = WidgetManager.GetWidgetUnderCursor();
		if (widget)
			underCursor = widget.GetName();

		Print(string.Format("[CRF_QuickSlotDrag] item=%1 from=%2 | target=%3 in %4 (index %5) | hoveredStorage=%6 underCursor=%7 | compatible=%8 fitsQuickSlot=%9",
			item, from, target, targetStorage, slotIndex, hovered, underCursor, compatible, fitsQuickSlot), LogLevel.NORMAL);
	}
}
