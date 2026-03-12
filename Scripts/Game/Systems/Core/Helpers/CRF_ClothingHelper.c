class CRF_ClothingHelper
{	
	//------------------------------------------------------------------------------------------------
	//! Update clothing in a specific slot
	//! \param[in] clothingArray Clothing options
	//! \param[in] slotInt Slot to update
	//! \param[in] role Role identifier
	//! \param[in] deletePreviousItems Whether to delete previous items
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Process previous clothing before replacement
	//! \param[in] previousClothing Previous clothing entity
	//! \param[out] removedItems Array to store removed items
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Spawn new clothing
	//! \param[in] clothingResource Clothing resource to spawn
	//! \param[in] slotInt Slot to place in
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Determine appropriate clothing slots for an item
	//! \param[in] item Item to filter
	//! \param[in] role Role identifier
	//! \param[in] isThrowable Whether item is a throwable
	//! \return Array of appropriate clothing slot IDs
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
		
		bool isExplosive = (CRF_InventoryHelper.IsExplosive(item));
		
		bool isTool = (CRF_InventoryHelper.IsTool(item));

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
		if (isExplosive || isMedical || isTool)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST
			};
		}

		return clothingIDs;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns true if the item's prefab is listed as a clothing piece in the faction gearscript assigned to the local player's faction and role.
	//! Checks both m_DefaultClothing (faction-wide) and m_RolesToSetCustomSettings (role-specific overrides) so custom role uniforms are also covered.
	//! \param[in] item The item the player is trying to remove.
	//! \return True if removal should be blocked.
	static bool IsGearscriptClothingPiece(IEntity item)
	{
		if (!item)
			return false;

		// Resolve the gearscript manager and the local player's faction.
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return false;

		array<CRF_EGearscriptClothing> restrictedClothing = {CRF_EGearscriptClothing.SHIRT, CRF_EGearscriptClothing.PANTS, CRF_EGearscriptClothing.BOOTS};
		TStringArray factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		foreach (string factionKey : factionKeys)
		{
			// Load the gearscript config for this faction.
			ResourceName gearScriptResource = gamemode.GetGearScriptResource(factionKey);
			if (gearScriptResource.IsEmpty())
				return false;
	
			CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(
				BaseContainerTools.CreateInstanceFromContainer(
					BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
			if (!gearConfig)
				return false;
	
			ResourceName itemPrefab = item.GetPrefabData().GetPrefabName();
	
			// --- Check default (faction-wide) clothing ---
			foreach (CRF_Clothing clothing : gearConfig.m_DefaultClothing)
			{
				if (restrictedClothing.Contains(clothing.m_iClothingType) && clothing.m_ClothingPrefabs.Contains(itemPrefab))
					return true;
			}
		}

		return false;
	}
}