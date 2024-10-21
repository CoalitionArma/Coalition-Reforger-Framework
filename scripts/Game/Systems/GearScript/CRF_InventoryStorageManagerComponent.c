modded class SCR_InventoryStorageManagerComponent : ScriptedInventoryStorageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//! try insert the item into the storage (not slot)
	void InsertItemCRF(IEntity pItem, BaseInventoryStorageComponent pStorageTo = null, BaseInventoryStorageComponent pStorageFrom = null, SCR_InvCallBack cb = null, bool playSound = true)
	{
		if (!pItem || !IsAnimationReady() || IsInventoryLocked())
			return;
		
		SetInventoryLocked(true);

		bool canInsert = true;
		if ( !pStorageTo ) // no storage selected, put it into best fitting storage
		{
			string soundEvent = SCR_SoundEvent.SOUND_EQUIP;
			//TryInsertItem( pItem, EStoragePurpose.PURPOSE_DEPOSIT);	// works for the owned storages ( not for the vicinity storages )
			if ( !TryInsertItem( pItem, EStoragePurpose.PURPOSE_WEAPON_PROXY, cb ) )
			{
				if ( !TryInsertItem( pItem, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT, cb ) )
				{
					if ( !TryInsertItem( pItem, EStoragePurpose.PURPOSE_DEPOSIT, cb ) )
					{
						if ( !TryMoveItemToStorage( pItem, FindStorageForItem( pItem, EStoragePurpose.PURPOSE_ANY ), -1, cb ) )
							canInsert = TryMoveItemToStorage(pItem, m_Storage, -1, cb); 				// clothes from storage in vicinity
						else
							soundEvent = SCR_SoundEvent.SOUND_PICK_UP;	// play pick up sound for everything else
						
						if(playSound)
							SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_HOTKEY_CONFIRM);
					}
					else
						soundEvent = SCR_SoundEvent.SOUND_PICK_UP;
				}
			}
			
			if (canInsert && playSound)
				PlayItemSound(pItem, soundEvent);
		}
		else
		{
			if (pStorageTo == m_Storage)
			{
				canInsert = TryReplaceItem( pStorageTo, pItem, 0, cb );
				if (canInsert)
				{
					SetInventoryLocked(false);
					return;
				}
			}
			
			//~ Find a valid storage to insert item in
			BaseInventoryStorageComponent validStorage = FindStorageForInsert(pItem, pStorageTo, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
			
			if (!validStorage)
				validStorage = FindStorageForInsert(pItem, pStorageTo, EStoragePurpose.PURPOSE_ANY);
			
			if (validStorage)
			{
				pStorageTo = validStorage;
			}
			//~ Check if item can be inserted in linked storages
			else 
			{
				//~ Find valid storage in linked storages
				SCR_UniversalInventoryStorageComponent universalStorage = SCR_UniversalInventoryStorageComponent.Cast(pStorageTo);
				if (universalStorage)
				{
					array<BaseInventoryStorageComponent> linkedStorages = {};
					universalStorage.GetLinkedStorages(linkedStorages);
					
					foreach(BaseInventoryStorageComponent linkedStorage : linkedStorages)
					{
						//~ Valid linked storage found
						if (FindStorageForInsert(pItem, linkedStorage, EStoragePurpose.PURPOSE_ANY))
						{
							pStorageTo = linkedStorage;
							break;
						}
					}
				}
			}

//MODDED START
			int targetSlot = -1;
			BaseInventoryStorageComponent pTmpStorageTo = pStorageTo;
			BaseLoadoutClothComponent blcComp = BaseLoadoutClothComponent.Cast(pItem.FindComponent(BaseLoadoutClothComponent));
			if (blcComp)
			{
				if (blcComp.GetAreaType())
				{
					pStorageTo = BaseInventoryStorageComponent.Cast(pStorageTo.FindComponent(RHS_ClothNodeStorageComponent));
					if (!pStorageTo)
						pStorageTo = pTmpStorageTo;
				}
			}

			if (RHS_ClothNodeStorageComponent.Cast(pStorageTo))
			{
				InventoryStorageSlot targetInventoryStorageSlot = RHS_ClothNodeStorageComponent.Cast(pStorageTo).GetEmptySlotForItem(pItem);
				if (targetInventoryStorageSlot)
					targetSlot = targetInventoryStorageSlot.GetID();
			}

			if (targetSlot < 0)
				pStorageTo = pTmpStorageTo;

			if (!pStorageFrom)
				canInsert = TryInsertItemInStorage(pItem, pStorageTo, targetSlot, cb);	// if we move item from ground to opened storage
			else
				canInsert = TryMoveItemToStorage(pItem, pStorageTo, targetSlot, cb);	// if we move item between storages
//MODDED END
		}

		if (!canInsert)
			if(playSound)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_DROP_ERROR);
		else
			if(playSound)
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_CONTAINER_DIFR_DROP);

		if (m_CharacterController && canInsert && !pStorageFrom && playSound)
			m_CharacterController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);

		SetInventoryLocked(false);
	}
}