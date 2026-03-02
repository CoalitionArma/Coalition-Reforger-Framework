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
 *  - During safestart (players can still adjust gear freely before the mission begins)
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
	 *  4. Safestart has ended.
	 *  5. The item's prefab is listed in the faction gearscript clothing config.
	 */
	override bool TryRemoveItemFromStorage(IEntity item, BaseInventoryStorageComponent storage, bool updateQuickbar = false)
	{
		// Authority (server) always bypasses the lock so gearscript can re-equip freely.
		if (Replication.IsServer())
			return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);

		// Only apply the lock when the owner of this manager is the locally controlled character.
		IEntity ownerEntity = GetOwner();
		if (!ownerEntity)
			return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);

		// Skip if the owner is not the local player's controlled entity (e.g. looting a body).
		IEntity localEntity = SCR_PlayerController.GetLocalMainEntity();
		if (ownerEntity != localEntity)
			return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);

		// Skip locking for spectator entities.
		if (CRF_GamemodeManager.IsSpectator(ownerEntity))
			return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);

		// Skip locking during safestart — players can still adjust gear then.
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart || safestart.GetSafestartStatus())
			return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);

		// Block removal if the item is a gearscript-assigned clothing piece.
		if (IsGearscriptClothingPiece(item, ownerEntity))
		{
			Print("[CRF] Uniform lock: blocked removal of gearscript clothing " + item.GetPrefabData().GetPrefabName(), LogLevel.DEBUG);
			return false;
		}

		return super.TryRemoveItemFromStorage(item, storage, updateQuickbar);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Returns true if the item's prefab is listed as a clothing piece in the
	 *        faction gearscript assigned to the local player's faction and role.
	 *
	 * Checks both m_DefaultClothing (faction-wide) and m_RolesToSetCustomSettings
	 * (role-specific overrides) so custom role uniforms are also covered.
	 *
	 * @param item      The item the player is trying to remove.
	 * @param character The character entity that owns this inventory.
	 * @return True if removal should be blocked.
	 */
	protected bool IsGearscriptClothingPiece(IEntity item, IEntity character)
	{
		if (!item || !character)
			return false;

		// Resolve the gearscript manager and the local player's faction.
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		if (!gearscriptManager)
			return false;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(localPlayerId);
		if (!playerFaction)
			return false;

		// Load the gearscript config for this faction.
		ResourceName gearScriptResource = gearscriptManager.GetGearScriptResource(playerFaction.GetFactionKey());
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
			if (clothing.m_ClothingPrefabs.Contains(itemPrefab))
				return true;
		}

		// --- Check custom role clothing ---
		// Determine the local player's role directly from their slot data.
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		CRF_SlotDataContainer slotData = slottingManager.GetPlayerSlotData(localPlayerId);
		if (!slotData)
			return false;

		// m_SlotRole is directly stored on the container — no resource round-trip needed.
		CRF_EGearRole playerRole = slotData.m_SlotRole;

		foreach (CRF_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != playerRole)
				continue;

			foreach (CRF_Clothing clothing : customGear.m_Clothing)
			{
				if (clothing.m_ClothingPrefabs.Contains(itemPrefab))
					return true;
			}
		}

		return false;
	}
}
