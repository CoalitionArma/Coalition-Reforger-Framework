modded class SCR_InventoryMenuUI
{
	//To disable picking up weapons in Gun Game
	override void OnMenuOpen()
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
		{
			Close();
			return;
		}
		super.OnMenuOpen();
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void OpenAsNewContainer()
	{
		
		m_pSelectedSlotUI = m_pFocusedSlotUI;
		auto storage = m_pFocusedSlotUI.GetAsStorage();
		
		if (storage)
		{
			//Checks if Leader only arsenal is enabled, and if it is prevents non leaders from opening an arsenal
			CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
			if (slottingMan)
			{
				SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
				if (storage.GetOwner().FindComponent(SCR_ArsenalComponent) && gamemode.IsLeadersArsenalEnabled())
				{
					
					int playerId = SCR_PlayerController.GetLocalPlayerId();
					CRF_SlotDataContainer slotData = slottingMan.GetPlayerSlotData(playerId);
					CRF_ESlotType slotType = slotData.GetSlotType();
					if (slotType != CRF_ESlotType.TEAM_LEADER && slotType != CRF_ESlotType.SQUAD_LEADER && slotType != CRF_ESlotType.MEDIC)
						return;
				}
			}
			OpenStorageAsContainer(storage);
			OpenLinkedStorages(storage);
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_HOTKEY_OPEN);
		}
	}
	//------------------------------------------------------------------------------------------------
	//!
	override protected void Action_UnfoldItem()
	{
		m_pSelectedSlotUI = m_pFocusedSlotUI;
		auto storage = m_pFocusedSlotUI.GetAsStorage();
		
		if (storage)
		{
			//Checks if Leader only arsenal is enabled, and if it is prevents non leaders from opening an arsenal
			CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
			if (slottingMan)
			{
				SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
				if (storage.GetOwner().FindComponent(SCR_ArsenalComponent) && gamemode.IsLeadersArsenalEnabled())
				{
					
					int playerId = SCR_PlayerController.GetLocalPlayerId();
					CRF_SlotDataContainer slotData = slottingMan.GetPlayerSlotData(playerId);
					CRF_ESlotType slotType = slotData.GetSlotType();
					if (slotType != CRF_ESlotType.TEAM_LEADER && slotType != CRF_ESlotType.SQUAD_LEADER && slotType != CRF_ESlotType.MEDIC)
						return;
				}
			}
		}
 		SimpleFSM( EMenuAction.ACTION_UNFOLD );
	}
}