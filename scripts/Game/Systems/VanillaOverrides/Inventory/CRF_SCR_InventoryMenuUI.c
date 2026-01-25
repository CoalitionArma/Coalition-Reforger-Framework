modded class SCR_InventoryMenuUI
{
	CRF_GearscriptManager m_GearscriptManager;
	CRF_SafestartManager m_SafeStartManager;

	override void OnItemAddedListener( IEntity item, notnull BaseInventoryStorageComponent storage )
	{
		super.OnItemAddedListener(item, storage);
		
		// If the item is from an arsenal
		if (MoveItemToStorageSlot_VirtualArsenal()) {
			// Grab item and player information
			CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
			CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();

			if (!sm || !rplManager)
				return;

			string name = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
			InventoryItemComponent itemIIC = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			UIInfo itemUiInfo = itemIIC.GetUIInfo();
			RplComponent rplComponent = RplComponent.Cast(m_Player.FindComponent(RplComponent));
			if (!rplComponent)
				return;
			CRF_SlotDataContainer slotData = sm.GetSlotDataFromCharacter(rplComponent.Id());
			
			// Log to admin menu
			rplManager.LogAdminAction(name + "(" + slotData.GetSlotName() + ")" + " took a(n) " + string.Format(itemUiInfo.GetName()) + " from an arsenal", -1, false);
		}
		
	}
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		SCR_ButtonComponent.Cast(GetRootWidget().FindWidget("MiniArsenal").FindHandler(SCR_ButtonComponent)).m_OnClicked.Insert(OpenMiniArsenal);
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_SafeStartManager = CRF_SafestartManager.GetInstance();
		
		if (!m_GearscriptManager || !m_SafeStartManager)
			return;
		
		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!playerFaction)
			return;
		
		CRF_GearScriptContainer container = m_GearscriptManager.GetGearScriptSettings(playerFaction.GetFactionKey());
		if (!container)
			return;
		
		if (!container.m_bEnableMiniArsenal || !m_SafeStartManager.GetSafestartStatus())
			GetRootWidget().FindWidget("MiniArsenal").SetVisible(false);
		
	}
	
	void OpenMiniArsenal()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_MiniArsenal);
	}
}