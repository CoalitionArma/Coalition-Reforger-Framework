modded class SCR_OpenStorageAction : SCR_InventoryAction
{
	#ifndef DISABLE_INVENTORY
	//------------------------------------------------------------------------------------------------
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		//Checks if Leader only arsenal is enabled, and if it is prevents non leaders from opening an arsenal
		CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
		if (slottingMan)
		{
			SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (pOwnerEntity.FindComponent(SCR_ArsenalComponent) && gamemode.IsLeadersArsenalEnabled())
			{
				
				int playerId = SCR_PlayerController.GetLocalPlayerId();
				CRF_SlotDataContainer slotData = slottingMan.GetPlayerSlotData(playerId);
				CRF_ESlotType slotType = slotData.GetSlotType();
				if (slotType != CRF_ESlotType.TEAM_LEADER && slotType != CRF_ESlotType.SQUAD_LEADER && slotType != CRF_ESlotType.MEDIC)
					return;
			}
		}
		
		super.PerformActionInternal(manager, pOwnerEntity, pUserEntity);
	}
	
	#endif
	
	
}

modded class SCR_OpenVehicleStorageAction : SCR_InventoryAction
{
	#ifndef DISABLE_INVENTORY

	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		//Checks if Leader only arsenal is enabled, and if it is prevents non leaders from opening an arsenal
		CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
		if (slottingMan)
		{
			SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (pOwnerEntity.FindComponent(SCR_ArsenalComponent) && gamemode.IsLeadersArsenalEnabled())
			{
				
				int playerId = SCR_PlayerController.GetLocalPlayerId();
				CRF_SlotDataContainer slotData = slottingMan.GetPlayerSlotData(playerId);
				CRF_ESlotType slotType = slotData.GetSlotType();
				if (slotType != CRF_ESlotType.TEAM_LEADER && slotType != CRF_ESlotType.SQUAD_LEADER && slotType != CRF_ESlotType.MEDIC)
					return;
			}
		}
		
		super.PerformActionInternal(manager, pOwnerEntity, pUserEntity);
	}
	#endif
}