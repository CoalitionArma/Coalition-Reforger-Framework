/**
 * CRF_SCR_InventoryStorageManagerComponent
 *
 * Prevents players from removing uniform clothing pieces that were assigned to
 * them by the faction gearscript. Only clothing prefabs explicitly listed in
 * the player's gearscript (default clothing + any custom role clothing) are
 * locked — anything not in the gearscript can still be freely removed.
 *
 * Locking is skipped for:
 *  - The server / authority (so gearscript can still clear and reapply gear)
 *  - Spectator entities
 *  - Items whose prefab is not listed in the faction gearscript clothing arrays
 */
modded class SCR_InventoryStorageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Intercept clothing removal to enforce gearscript uniform locking.
	 *
	 * Only blocks removal when ALL of the following are true:
	 *  1. The local client is the one performing the action (not server authority).
	 *  2. The owner entity is the locally controlled character (not a body being looted).
	 *  3. The owner is not a spectator.
	 *  5. The item's prefab is listed in the faction gearscript clothing config.
	 *
	 * Note: TryRemoveItemFromStorage is proto external and cannot be overridden in script.
	 * TryRemoveItemFromInventory is the scripted entry-point that the inventory UI calls,
	 * and it internally calls TryRemoveItemFromStorage — so this is the correct intercept point.
	 */
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
}
