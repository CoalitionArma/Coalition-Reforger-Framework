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
	//! Check if a prefab is a throwable weapon by inspecting its weapon type
	//! \param[in] prefab Prefab resource name
	//! \return True if prefab is a throwable (grenade)
	static bool IsThrowableFromPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;
			
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
			
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return false;
			
		IEntityComponentSource componentSource = SCR_BaseContainerTools.FindComponentSource(entitySource, "WeaponComponent");
		if (!componentSource)
			return false;
			
		int weaponType;
		if (componentSource.Get("WeaponType", weaponType))
		{
			return WEAPON_TYPES_THROWABLE.Contains(weaponType);
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if a prefab is a weapon (non-throwable) by inspecting its components
	//! \param[in] prefab Prefab resource name
	//! \return True if prefab is a weapon that should go in weapon slots
	static bool IsWeaponFromPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;
			
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
			
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return false;
			
		IEntityComponentSource weaponSource = SCR_BaseContainerTools.FindComponentSource(entitySource, "WeaponComponent");
		if (!weaponSource)
			return false;
			
		// Exclude throwables - they go in gadget slots instead
		int weaponType;
		if (weaponSource.Get("WeaponType", weaponType))
		{
			return !WEAPON_TYPES_THROWABLE.Contains(weaponType);
		}
		
		return true; // Has WeaponComponent but no specific type
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if a prefab is a gadget or tool by inspecting its components
	//! \param[in] prefab Prefab resource name
	//! \return True if prefab is a gadget/tool that should go in gadget slots
	static bool IsGadgetFromPrefab(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return false;
			
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;
			
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return false;
			
		// Check for various gadget-related components
		if (SCR_BaseContainerTools.FindComponentSource(entitySource, "SCR_GadgetComponent"))
			return true;
		
		if (SCR_BaseContainerTools.FindComponentSource(entitySource, "SCR_ConsumableItemComponent"))
			return true;
			
		if (SCR_BaseContainerTools.FindComponentSource(entitySource, "SCR_RepairSupportStationComponent"))
			return true;
			
		if (SCR_BaseContainerTools.FindComponentSource(entitySource, "SCR_HealSupportStationComponent"))
			return true;
		
		return false;
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

		// Determine item type to use appropriate storage priority
		bool isThrowable = IsThrowableFromPrefab(item);
		bool isWeapon = !isThrowable && IsWeaponFromPrefab(item);
		bool isGadget = !isThrowable && !isWeapon && IsGadgetFromPrefab(item);
		
		for (int i = 1; i <= itemAmount; i++)
		{
			bool spawned = false;
			IEntity spawnedEntity;
			
			// Try type-specific storage first for weapons and gadgets
			if (isWeapon)
			{
				// Weapons: try weapon slots first
				spawned = inventoryManager.TrySpawnPrefabToStorage(
					item, 
					null,
					-1,
					EStoragePurpose.PURPOSE_WEAPON_PROXY
				);
			}
			else if (isThrowable || isGadget)
			{
				// Throwables and gadgets: try gadget slots first
				spawned = inventoryManager.TrySpawnPrefabToStorage(
					item, 
					null,
					-1,
					EStoragePurpose.PURPOSE_GADGET_PROXY
				);
			}
			
			// If not spawned in specific slots, spawn item and use FilterItemToClothing for distribution
			if (!spawned)
			{
				spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);
				
				if (spawnedEntity)
				{
					// Use FilterItemToClothing to intelligently distribute to appropriate clothing
					TIntArray clothingIDs = CRF_ClothingHelper.FilterItemToClothing(spawnedEntity, role, isThrowable);
					
					// Try inserting into appropriate clothing
					bool inserted = TryInsertIntoSpecificClothing(spawnedEntity, clothingIDs, inventory, inventoryManager);
					
					// If still not inserted, try general insertion
					if (!inserted)
						inventoryManager.TryInsertItem(spawnedEntity);
					
					// Check if insertion succeeded
					if (!inventoryManager.Contains(spawnedEntity))
					{
						CRF_LoggingHelper.LogItemError(item, inventoryManager.GetOwner(), "ITEM");
						SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
					}
				}
				else
				{
					CRF_LoggingHelper.LogItemError(item, inventoryManager.GetOwner(), "ITEM");
				}
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