modded class SCR_EditorPingInfoDisplay
{
	override protected void OnPingEntityRegister(int reporterID, SCR_EditableEntityComponent pingEntity)
	{
		//--- When outside of the editor, don't create a ping on player
		if (pingEntity.GetOwner() == SCR_PlayerController.GetLocalControlledEntity()) return;
			
		SCR_EditableEntitySceneSlotUIComponent slot;
		if (!m_EntitySlots.Find(pingEntity, slot))
		{
			Widget slotWidget = m_Workspace.CreateWidgets(m_SlotWidgetPrefab, m_wRoot);
			if (!slotWidget) return;
			
			slot = SCR_EditableEntitySceneSlotUIComponent.Cast(slotWidget.FindHandler(SCR_EditableEntitySceneSlotUIComponent));
			if (!slot) return;
			
			slot.InitSlot(pingEntity);
			m_EntitySlots.Insert(pingEntity, slot);
		}
		
		//--- Create ping widget in a slot
		slot.DeleteWidget(null);
		Widget entityWidget;
		if (pingEntity.GetPrefab() == "{744FFD0C5F124A92}PrefabsEditable/System/ACE_Finger_PingEntity.et")
			entityWidget = slot.CreateWidget(pingEntity, "{10826F486211985F}UI/layouts/Editor/EditableEntities/Base/EditableEntity_Base_Pinged_Radius.layout");
		else
			entityWidget = slot.CreateWidget(pingEntity, m_WidgetPrefab);

		m_EntityWidgets.Insert(pingEntity, entityWidget);
		m_bCanUpdate = true;
	}
}