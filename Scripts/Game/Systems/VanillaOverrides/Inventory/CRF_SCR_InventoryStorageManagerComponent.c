modded class SCR_InventoryStorageManagerComponent : ScriptedInventoryStorageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Intercept clothing removal to enforce gearscript uniform locking.
	//! Only blocks removal when ALL of the following are true:
	//!  1. The local client is the one performing the action (not server authority).
	//!  2. The owner entity is the locally controlled character (not a body being looted).
	//!  3. The owner is not a spectator.
	//!  5. The item's prefab is listed in the faction gearscript clothing config.
	//! Note: TryRemoveItemFromStorage is proto external and cannot be overridden in script.
	//! TryRemoveItemFromInventory is the scripted entry-point that the inventory UI calls,
	//! and it internally calls TryRemoveItemFromStorage — so this is the correct intercept point.
	//! 
	//! \param[in] item Item to check
	override bool CanMoveItem(IEntity item)
	{
		if (CRF_Gamemode.GetInstance().m_bEnableClothesSwapping)
			return super.CanMoveItem(item);
		
		// Block removal if the item is a gearscript-assigned clothing piece.
		if (CRF_ClothingHelper.IsGearscriptClothingPiece(item))
		{
			Print("[CRF] Uniform lock: blocked removal of gearscript clothing " + item.GetPrefabData().GetPrefabName(), LogLevel.DEBUG);
			return false;
		}

		return super.CanMoveItem(item);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Try to equip the item into the slot (cloth)
	//! \param[in] pOwnerEntity
	override void EquipCloth( IEntity pOwnerEntity )
	{
		if (CRF_Gamemode.GetInstance().m_bEnableClothesSwapping)
			return super.EquipCloth(pOwnerEntity);
		
		// Block removal if the item is a gearscript-assigned clothing piece.
		if (CRF_ClothingHelper.IsGearscriptClothingPiece(pOwnerEntity))
		{
			Print("[CRF] Uniform lock: blocked swapping of gearscript clothing " + pOwnerEntity.GetPrefabData().GetPrefabName(), LogLevel.DEBUG);
			return;
		}
		
		super.EquipCloth(pOwnerEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Callback triggered when an item is added to storage
	//! This is executed locally after the server completes an Insert/Move operation
	//! Contains special handling for radio equipment based on faction
	//! 
	//! \param[in] storageOwner The storage component that owns the item
	//! \param[in] item The item entity that was added
	override protected void OnItemAdded(BaseInventoryStorageComponent storageOwner, IEntity item)
	{		
		// Call the parent implementation first
		super.OnItemAdded(storageOwner, item);
		
		// Skip radio validation if espionage is allowed and not in client mode
		if (CRF_Gamemode.GetInstance() && RplSession.Mode() != RplMode.Client && CRF_Gamemode.GetInstance().m_bMissionAllowsEspionage)
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
}