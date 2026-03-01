class CRF_ClothingHelper
{	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Update clothing in a specific slot
	 * @param clothingArray Clothing options
	 * @param slotInt Slot to update
	 * @param role Role identifier
	 * @param deletePreviousItems Whether to delete previous items
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	static void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, CRF_EGearRole role, bool deletePreviousItems, 
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (clothingArray.IsEmpty() || slotInt == -1)
			return;
		
		array<IEntity> removedItems = {};
		IEntity previousClothing = inventory.Get(slotInt);
		
		// Get random clothing from the array
		ResourceName clothing = clothingArray.GetRandomElement();

		// Process previous clothing and its contents
		if (previousClothing != null)
			ProcessPreviousClothing(previousClothing, removedItems, inventory, inventoryManager);

		// Add new clothing if exists
		if (!clothing.IsEmpty())
			SpawnClothing(clothing, slotInt, spawnParams, inventory, inventoryManager);

		// Handle previously removed items
		foreach (IEntity oldItem : removedItems)
		{
			if (!deletePreviousItems)
				CRF_InventoryHelper.InsertInventoryItem(oldItem, inventory, inventoryManager, role);
			else 
				SCR_EntityHelper.DeleteEntityAndChildren(oldItem);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Process previous clothing before replacement
	 * @param previousClothing Previous clothing entity
	 * @param removedItems Array to store removed items
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	static void ProcessPreviousClothing(IEntity previousClothing, out array<IEntity> removedItems, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
		if (oldStorage)
		{
			array<IEntity> outItems = {};
			oldStorage.GetAll(outItems);
			
			foreach (IEntity item : outItems)
			{
				if (!item || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
					continue;
					
				if (SCR_EquipmentStorageComponent.Cast(item.FindComponent(SCR_EquipmentStorageComponent)) != null)
					continue;
					
				if (SCR_UniversalInventoryStorageComponent.Cast(item.FindComponent(SCR_UniversalInventoryStorageComponent)) != null)
					continue;
					
				if (BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent)) != null)
					continue;

				inventoryManager.TryRemoveItemFromStorage(item, oldStorage);
				removedItems.Insert(item);
			}
		}

		inventoryManager.TryRemoveItemFromStorage(previousClothing, inventory);
		SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawn new clothing
	 * @param clothingResource Clothing resource to spawn
	 * @param slotInt Slot to place in
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	static void SpawnClothing(ResourceName clothingResource, int slotInt, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothingResource), GetGame().GetWorld(), spawnParams);
		inventoryManager.TryReplaceItem(resourceSpawned, inventory, slotInt);

		if (!inventoryManager.Contains(resourceSpawned))
		{
			CRF_LoggingHelper.LogItemError(resourceSpawned, inventoryManager.GetOwner(), "CLOTHING");
			SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determine appropriate clothing slots for an item
	 * @param item Item to filter
	 * @param role Role identifier
	 * @param isThrowable Whether item is a throwable
	 * @return Array of appropriate clothing slot IDs
	 */
	static TIntArray FilterItemToClothing(IEntity item, CRF_EGearRole role = 0, bool isThrowable = false)
	{
		array<int> clothingIDs = {};

		// Determine item type
		bool isMagazine = MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || 
						  InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent));
		
		bool isPistolAmmo = InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && 
							InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO;
		
		bool isMedical = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role).m_aItems.Contains(CRF_EGearscriptItems.MEDIC_ITEMS) && 
						SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
		
		bool isRadio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		
		bool isExplosive = CRF_InventoryHelper.IsExplosiveOrTool(item);

		// Magazines and throwables go in backpack, vest, armor, primarily
		if (isMagazine)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST,
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.SHIRT
			};
		}
		// Non-magazines go in shirt, pants, vest primarily
		else
		{
			clothingIDs = {
				CRF_EGearscriptClothing.SHIRT, 
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Pistol ammo and throwables go in pants, vest primarily
		if (isPistolAmmo || isThrowable)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Radios go in pants, shirt, vest primarily
		if (isRadio)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.SHIRT, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Explosives/Medical items go in backpack, vest primarily
		if (isExplosive || isMedical)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST
			};
		}

		return clothingIDs;
	}
}