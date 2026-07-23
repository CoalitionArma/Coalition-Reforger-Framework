class CRF_InitializationHelper
{
	//------------------------------------------------------------------------------------------------
	//! Sets up radio frequencies based on player group
	//! Configures both group and platoon frequencies
	static void SetupRadioFrequency()
	{
		// Get player's entity
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		if (!entity || CRF_EntityHelper.IsSpectator(entity))
			return;

		// Find radio in inventory
		array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return;

		inventoryManager.GetItems(items);
		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Get radio components
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (!radio)
			return;

		BaseTransceiver grpTsv = radio.GetTransceiver(0);
		if (!grpTsv)
			return;

		// Get player's group
		SCR_GroupsManagerComponent m_GroupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!m_GroupManager)
			return;

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		// Set frequency based on group
		if (group)
		{
			grpTsv.SetFrequency(group.GetRadioFrequency());
		}

		// Set up Voice over Network component
		SCR_VONController vc = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		if (!vc || !von)
			return;

		von.SetTransmitRadio(grpTsv);

		// Set up platoon radio if available
		BaseTransceiver pltTsv = radio.GetTransceiver(1);
		if (pltTsv)
			von.SetTransmitRadio(pltTsv);

		vc.PublicResetVON();
		vc.SetVONComponent(von);
	}
}
