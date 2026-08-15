modded class SCR_InventoryMenuUI
{
	COA_Gamemode m_Gamemode;
	COA_SafestartManager m_SafeStartManager;

	override void OnItemAddedListener( IEntity item, notnull BaseInventoryStorageComponent storage )
	{
		super.OnItemAddedListener(item, storage);
		
		// If the item is from an arsenal
		if (MoveItemToStorageSlot_VirtualArsenal()) {
			// Grab item and player information
			COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
			COA_SlottingManager sm = COA_SlottingManager.GetInstance();

			if (!sm || !rplManager)
				return;

			string name = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
			InventoryItemComponent itemIIC = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			UIInfo itemUiInfo = itemIIC.GetUIInfo();
			RplComponent rplComponent = RplComponent.Cast(m_Player.FindComponent(RplComponent));
			if (!rplComponent)
				return;
			COA_SlotData slotData = sm.GetSlotDataFromCharacter(rplComponent.Id());
			
			// Log to admin menu
			rplManager.LogAdminAction(name + "(" + slotData.GetSlotName() + ")" + " took a(n) " + string.Format(itemUiInfo.GetName()) + " from an arsenal", -1, false, COA_EAdminLogLevel.High);
		}
		
	}
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		SCR_ButtonComponent.Cast(GetRootWidget().FindWidget("MiniArsenal").FindHandler(SCR_ButtonComponent)).m_OnClicked.Insert(OpenMiniArsenal);
		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafeStartManager = COA_SafestartManager.GetInstance();
		
		if (!m_Gamemode || !m_SafeStartManager)
			return;
		
		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!playerFaction)
			return;
		
		COA_GearScriptContainer container = m_Gamemode.GetGearScriptSettings(playerFaction.GetFactionKey());
		if (!container)
			return;
		
		if ((!container.m_bEnableMiniArsenal || !m_SafeStartManager.GetSafestartStatus()) && COA_PlayerController.IsGracePeriodOver())
			GetRootWidget().FindWidget("MiniArsenal").SetVisible(false);
			
		
	}
	
	void OpenMiniArsenal()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_MiniArsenal);
	}
}