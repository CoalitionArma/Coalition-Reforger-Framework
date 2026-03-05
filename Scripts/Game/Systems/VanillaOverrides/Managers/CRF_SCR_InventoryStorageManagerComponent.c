modded class SCR_InventoryStorageManagerComponent : ScriptedInventoryStorageManagerComponent
{
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	/**
	 * Callback triggered when an item is added to storage
	 * This is executed locally after the server completes an Insert/Move operation
	 * Contains special handling for radio equipment based on faction
	 * 
	 * @param storageOwner The storage component that owns the item
	 * @param item The item entity that was added
	 */
	override protected void OnItemAdded(BaseInventoryStorageComponent storageOwner, IEntity item)
	{		
		// Call the parent implementation first
		super.OnItemAdded(storageOwner, item);
		
		// If no gamemode instance exists, exit
		if (!CRF_Gamemode.GetInstance())
			return;
		
		// Skip radio validation if espionage is allowed and not in client mode
		if (CRF_Gamemode.GetInstance() && RplSession.Mode() != RplMode.Client && CRF_Gamemode.GetInstance().m_bAllowEspionage)
			return;

		// Check if the item is a radio
		BaseRadioComponent radioComp = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		if (!radioComp)
			return;
		
		// Get the player entity that owns this storage
		IEntity player = storageOwner.GetOwner().GetRootParent().GetRootParent().GetRootParent().GetRootParent();
		if (!player)
			return;
		
		// Get the faction affiliation component from the player
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(player.FindComponent(FactionAffiliationComponent));
		if (!facComp)
			return;
		
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			// Delete radio if its encryption key doesn't match the player's faction
			switch(true)
			{
				// BLUFOR players must use "chickenNuggets" encryption
				case(facComp.GetAffiliatedFactionKey() == "BLUFOR" && radioComp.GetEncryptionKey() != "chickenNuggets"): 
				{
					SCR_EntityHelper.DeleteEntityAndChildren(item); 
					break;
				}
				
				// OPFOR players must use "coldBorscht" encryption
				case(facComp.GetAffiliatedFactionKey() == "OPFOR" && radioComp.GetEncryptionKey() != "coldBorscht"):  
				{
					SCR_EntityHelper.DeleteEntityAndChildren(item); 
					break;
				}
				
				// INDFOR players must use "candleSauce" encryption
				case(facComp.GetAffiliatedFactionKey() == "INDFOR" && radioComp.GetEncryptionKey() != "candleSauce"):  
				{
					SCR_EntityHelper.DeleteEntityAndChildren(item); 
					break;
				}
			};
		}
		
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	/**
	 * Attempts to insert an item into storage with various fallback options
	 * Custom implementation for the framework with additional storage handling
	 * 
	 * @param pItem The item to insert
	 * @param pStorageTo Target storage (if null, best storage will be determined)
	 * @param pStorageFrom Source storage (null if from ground)
	 * @param cb Callback for operation completion
	 * @param playSound Whether to play sound effects
	 */
	void InsertItemCRF(IEntity pItem, BaseInventoryStorageComponent pStorageTo = null, BaseInventoryStorageComponent pStorageFrom = null, SCR_InvCallBack cb = null, bool playSound = true)
	{
		// Early exit checks
		if (!pItem || !IsAnimationReady() || IsInventoryLocked())
			return;
		
		SetInventoryLocked(true);

		bool canInsert = true;
		
		// Case 1: No storage selected - find the best fitting storage
		if (!pStorageTo)
		{
			string soundEvent = SCR_SoundEvent.SOUND_EQUIP;
			
			// Try inserting into weapon proxy storage
			if (!TryInsertItem(pItem, EStoragePurpose.PURPOSE_WEAPON_PROXY, cb))
			{
				// Try inserting into equipment attachment storage
				if (!TryInsertItem(pItem, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT, cb))
				{
					// Try inserting into deposit storage
					if (!TryInsertItem(pItem, EStoragePurpose.PURPOSE_DEPOSIT, cb))
					{
						// Try finding any storage that fits the item
						if (!TryMoveItemToStorage(pItem, FindStorageForItem(pItem, EStoragePurpose.PURPOSE_ANY), -1, cb))
						{
							// Last resort: try moving to the main storage
							canInsert = TryMoveItemToStorage(pItem, m_Storage, -1, cb);
						}
						else
						{
							// Item was picked up from vicinity
							soundEvent = SCR_SoundEvent.SOUND_PICK_UP;
						}
						
						if(playSound)
							SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_HOTKEY_CONFIRM);
					}
					else
					{
						soundEvent = SCR_SoundEvent.SOUND_PICK_UP;
					}
				}
			}
			
			// Play appropriate sound effect if insertion was successful
			if (canInsert && playSound)
				PlayItemSound(pItem, soundEvent);
		}
		// Case 2: Specific storage target provided
		else
		{
			// Special case for main storage: try to replace an item
			if (pStorageTo == m_Storage)
			{
				canInsert = TryReplaceItem(pStorageTo, pItem, 0, cb);
				if (canInsert)
				{
					SetInventoryLocked(false);
					return;
				}
			}
			
			// Find a valid storage to insert item in - first try equipment attachment
			BaseInventoryStorageComponent validStorage = FindStorageForInsert(pItem, pStorageTo, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
			
			// If not found, try any purpose
			if (!validStorage)
			{
				validStorage = FindStorageForInsert(pItem, pStorageTo, EStoragePurpose.PURPOSE_ANY);
			}
			
			// Use the valid storage if found
			if (validStorage)
			{
				pStorageTo = validStorage;
			}
			// Otherwise check linked storages
			else 
			{
				// Try to find valid storage in linked storages
				SCR_UniversalInventoryStorageComponent universalStorage = SCR_UniversalInventoryStorageComponent.Cast(pStorageTo);
				if (universalStorage)
				{
					array<BaseInventoryStorageComponent> linkedStorages = {};
					universalStorage.GetLinkedStorages(linkedStorages);
					
					// Check each linked storage
					foreach(BaseInventoryStorageComponent linkedStorage : linkedStorages)
					{
						// Use first valid linked storage found
						if (FindStorageForInsert(pItem, linkedStorage, EStoragePurpose.PURPOSE_ANY))
						{
							pStorageTo = linkedStorage;
							break;
						}
					}
				}
			}

			// MODDED SECTION: Special handling for cloth components
			int targetSlot = -1;
			BaseInventoryStorageComponent originalStorageTo = pStorageTo;
			
			// Check if the item is a clothing component
			BaseLoadoutClothComponent clothComponent = BaseLoadoutClothComponent.Cast(pItem.FindComponent(BaseLoadoutClothComponent));
			if (clothComponent)
			{
				// If it has an area type, try to find a cloth node storage
				if (clothComponent.GetAreaType())
				{
					BaseInventoryStorageComponent clothNodeStorage = BaseInventoryStorageComponent.Cast(pStorageTo.FindComponent(RHS_ClothNodeStorageComponent));
					if (clothNodeStorage)
					{
						pStorageTo = clothNodeStorage;
					}
				}
			}

			// If we're using a cloth node storage, find the appropriate slot
			if (RHS_ClothNodeStorageComponent.Cast(pStorageTo))
			{
				RHS_ClothNodeStorageComponent clothNodeStorage = RHS_ClothNodeStorageComponent.Cast(pStorageTo);
				InventoryStorageSlot targetInventoryStorageSlot = clothNodeStorage.GetEmptySlotForItem(pItem);
				
				if (targetInventoryStorageSlot)
				{
					targetSlot = targetInventoryStorageSlot.GetID();
				}
			}

			// Fallback to original storage if no valid slot found
			if (targetSlot < 0)
			{
				pStorageTo = originalStorageTo;
			}

			// Perform the actual item insertion
			if (!pStorageFrom)
			{
				// Moving from ground to storage
				canInsert = TryInsertItemInStorage(pItem, pStorageTo, targetSlot, cb);
			}
			else
			{
				// Moving between storages
				canInsert = TryMoveItemToStorage(pItem, pStorageTo, targetSlot, cb);
			}
			// END OF MODDED SECTION
		}

		// Handle sound effects based on insertion success
		if (!canInsert)
		{
			if(playSound)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_DROP_ERROR);
		}
		else
		{
			if(playSound)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_CONTAINER_DIFR_DROP);
		}

		// Play the pickup animation if successful and item was taken from ground
		if (m_CharacterController && canInsert && !pStorageFrom && playSound)
		{
			m_CharacterController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
		}

		SetInventoryLocked(false);
	}

	
	//For the GunGame Gamemode to prevent players from picking up weapons
	override void EquipItem(EquipedWeaponStorageComponent weaponStorage, IEntity weapon)
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
			return;
		
		super.EquipItem(weaponStorage, weapon);
	}
	override void InsertItem( IEntity pItem, BaseInventoryStorageComponent pStorageTo = null, BaseInventoryStorageComponent pStorageFrom = null, SCR_InvCallBack cb = null  )
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
			return;
		
		super.InsertItem(pItem, pStorageTo, pStorageFrom, cb);
	}
}