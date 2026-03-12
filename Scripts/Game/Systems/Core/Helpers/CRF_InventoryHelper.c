class CRF_InventoryHelper
{
	const static ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//------------------------------------------------------------------------------------------------
	//! Check if an item is a explosive
	//! \param[in] item Item to check
	//! \return True if item is explosive or tool
	static bool IsExplosive(IEntity item)
	{
		return SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent)) || 
			   SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent)) ||
			   SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if an item is a special tool
	//! \param[in] item Item to check
	//! \return True if item is explosive or tool
	static bool IsTool(IEntity item)
	{
		return SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent)) ||
			   SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if an entity is a throwable weapon
	//! \param[in] entity Entity to check
	//! \return True if entity is a throwable weapon
	static bool IsThrowable(IEntity entity)
	{
		WeaponComponent weaponComp = WeaponComponent.Cast(entity.FindComponent(WeaponComponent));
		return weaponComp && WEAPON_TYPES_THROWABLE.Contains(weaponComp.GetWeaponType());
	}	
	
	//------------------------------------------------------------------------------------------------
	//! Add inventory item
	//! \param[in] item Item resource to add
	//! \param[in] itemAmount Number of items to add
	//! \param[in] spawnParams Spawn parameters (unused - kept for compatibility)
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \param[in] role Role identifier
	static void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		CRF_EGearRole role = 0)
	{
		if (item.IsEmpty() || itemAmount <= 0)
			return;

		for (int i = 1; i <= itemAmount; i++)
		{
			// Use TrySpawnPrefabToStorage for proper replication and attachment slot readiness
			// This method handles spawning AND insertion atomically, ensuring all components are ready
			bool spawned = inventoryManager.TrySpawnPrefabToStorage(
				item, 
				null,  // Let the system find the best storage automatically
				-1,    // Auto-select slot
				EStoragePurpose.PURPOSE_ANY  // Allow any storage type
			);
			
			if (!spawned)
			{
				CRF_LoggingHelper.LogItemError(null, inventoryManager.GetOwner(), "Failed to spawn: " + item);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Insert an inventory item into appropriate storage
	//! \param[in] item Item to insert
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \param[in] role Role identifier
	static void InsertInventoryItem(IEntity item, SCR_CharacterInventoryStorageComponent inventory, 
		SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role = 0)
	{
		if (!item)
			return;

		TIntArray clothingIDs = CRF_ClothingHelper.FilterItemToClothing(item, role, IsThrowable(item));

		// Try inserting into appropriate clothing first
		bool inserted = TryInsertIntoSpecificClothing(item, clothingIDs, inventory, inventoryManager);

		// If not inserted in specific clothing, try general insertion
		if (!inserted)
			inventoryManager.TryInsertItem(item);

		// Log error and clean up if insertion failed
		if (!inventoryManager.Contains(item) || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
		{
			CRF_LoggingHelper.LogItemError(item, inventoryManager.GetOwner());
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Try to insert item into specific clothing slots
	//! \param[in] item Item to insert
	//! \param[in] clothingIDs Clothing slots to try
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \return true if insertion succeeded
	static bool TryInsertIntoSpecificClothing(IEntity item, TIntArray clothingIDs, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		foreach (int clothingID : clothingIDs)
		{
			IEntity clothing = inventory.Get(clothingID);

			if (!clothing || inventoryManager.Contains(item))
				continue;

			BaseInventoryStorageComponent clothingStorage = BaseInventoryStorageComponent.Cast(clothing.FindComponent(BaseInventoryStorageComponent));

			if (!clothingStorage)
				continue;

			bool successfulInsert = inventoryManager.TryInsertItemInStorage(item, clothingStorage);

			if (!successfulInsert)
				inventoryManager.InsertItemCRF(item, clothingStorage, null, null, false);
				
			if (inventoryManager.Contains(item))
				return true;
		}
		
		return false;
	}
}