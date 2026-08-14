//------------------------------------------------------------------------------------
// Building an SCR_ArsenalItemListConfig at runtime rather than authoring it in a prefab
// hits three traps, all of which show up as an arsenal full of blank tiles rather than
// as an error. Cache Hunt needs to do exactly that - fill each cache's arsenal from the
// defending faction's gearscript - so all three are handled here.
//
//  1. SCR_ArsenalItemStandalone's constructor loads m_ItemResource from the resource
//     name, but a script-constructed instance has no name yet, so the constructor
//     early-returns and m_ItemResource stays null forever. Assigning m_ItemResourceName
//     afterwards does not re-run it. The item then has no Resource to show or spawn.
//
//  2. Attribute defaults ([Attribute("2")] on m_eItemType and m_eItemMode) are applied
//     when a config is deserialised, NOT when a class is constructed with new. A
//     script-built entry therefore has type 0 and mode 0, and SCR_ArsenalItemListConfig
//     filters with `GetItemType() & typeFilter` - zero matches no filter, so the entry
//     is dropped from every query.
//
//  3. SCR_ArsenalItemListConfig caches its filtered results per type in
//     m_mArsenalItemsByType. Refilling m_aArsenalItems without clearing that cache
//     leaves the arsenal serving the results it computed before the swap.
//
// Vanilla reference: scripts/Game/Components/Arsenal/ in the Arma-Reforger-Script-Diff
// repository.
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
modded class SCR_ArsenalItemStandalone
{
	//------------------------------------------------------------------------------------------------
	//! Fills in an entry that was built with new rather than deserialised from a config.
	//!
	//! Sets the fields the constructor and the attribute defaults would normally have
	//! covered - see traps 1 and 2 at the top of this file. Every one of them is required;
	//! an entry missing any of the three renders as an empty arsenal tile.
	//!
	//! \param[in] resource Item prefab this arsenal entry hands out
	//! \param[in] supplyCost Supplies charged per take, 0 for free
	//! \param[in] itemType Arsenal type flag, matched against the component's supported types
	//! \param[in] itemMode Arsenal mode flag, matched against the component's supported modes
	void CRF_Configure(ResourceName resource, int supplyCost, SCR_EArsenalItemType itemType, SCR_EArsenalItemMode itemMode)
	{
		m_ItemResourceName = resource;
		m_iSupplyCost = supplyCost;
		m_eItemType = itemType;
		m_eItemMode = itemMode;

		// Trap 1: the constructor already ran against an empty name, so load it here.
		// m_ItemResource is protected on SCR_ArsenalItem, reachable from this subclass.
		if (!resource.IsEmpty())
			m_ItemResource = Resource.Load(resource);
	}
}

//------------------------------------------------------------------------------------
modded class SCR_ArsenalItemListConfig
{
	//------------------------------------------------------------------------------------------------
	//! Replaces the whole list with one standalone entry per resource.
	//! Building the entries in here keeps every protected access inside its owning class.
	//! \param[in] resources Item prefabs the arsenal should offer
	//! \param[in] supplyCosts Cost per resource, parallel to resources
	//! \param[in] itemType Arsenal type flag applied to every entry
	//! \param[in] itemMode Arsenal mode flag applied to every entry
	void CRF_ReplaceWithStandaloneItems(notnull array<ResourceName> resources, notnull array<int> supplyCosts, SCR_EArsenalItemType itemType, SCR_EArsenalItemMode itemMode)
	{
		if (!m_aArsenalItems)
			m_aArsenalItems = {};

		m_aArsenalItems.Clear();

		foreach (int i, ResourceName resource : resources)
		{
			if (resource.IsEmpty())
				continue;

			int supplyCost = 0;
			if (supplyCosts.IsIndexValid(i))
				supplyCost = supplyCosts[i];

			SCR_ArsenalItemStandalone item = new SCR_ArsenalItemStandalone();
			item.CRF_Configure(resource, supplyCost, itemType, itemMode);

			m_aArsenalItems.Insert(item);
		}

		// Trap 3: GetFilteredArsenalItems() memoises per type filter. Without this the
		// arsenal keeps serving whatever it resolved before the list was swapped.
		m_mArsenalItemsByType.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! \return How many entries the list currently holds
	int CRF_GetItemCount()
	{
		if (!m_aArsenalItems)
			return 0;

		return m_aArsenalItems.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] prefab Item to price
	//! \param[out] supplyCost This list's price for it
	//! \return True when this list carries that item
	bool CRF_GetSupplyCostForPrefab(ResourceName prefab, out int supplyCost)
	{
		if (!m_aArsenalItems)
			return false;

		foreach (SCR_ArsenalItem item : m_aArsenalItems)
		{
			if (!item || item.GetItemResourceName() != prefab)
				continue;

			supplyCost = item.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false);
			return true;
		}

		return false;
	}
}

//------------------------------------------------------------------------------------
//! The arsenal UI prices a slot purely from the assigned faction's ITEM entity catalog,
//! and returns a bare 0 when the item is not in it. Items that only exist in an arsenal's
//! overwrite list - which is every magazine a Cache Hunt cache serves - are therefore shown
//! as free no matter what their arsenal entry says.
//!
//! Note the PURCHASE path does not work this way: SCR_ResourcePlayerControllerInventoryComponent
//! falls back to the overwrite entry's cost when there is no catalog entry. So without this
//! the displayed price and the charged price disagree. This aligns the display with what the
//! player is actually charged.
modded class SCR_ArsenalInventorySlotUI
{
	//------------------------------------------------------------------------------------------------
	override float GetTotalResources()
	{
		float cost = super.GetTotalResources();

		// -1 means the arsenal does not use supplies at all; anything positive came from the
		// catalog. Only a zero is worth a second look.
		if (cost != 0)
			return cost;

		SCR_ArsenalItemListConfig itemList = CRF_GetOverwriteListForSlot();
		if (!itemList)
			return cost;

		ResourceName prefab = CRF_GetSlotItemPrefab();
		if (prefab.IsEmpty())
			return cost;

		int overwriteCost;
		if (!itemList.CRF_GetSupplyCostForPrefab(prefab, overwriteCost) || overwriteCost <= 0)
			return cost;

		m_fSupplyCost = overwriteCost;

		SCR_ResourceComponent resourceComponent = GetArsenalResourceComponent();
		if (resourceComponent)
		{
			SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			if (consumer)
				m_fSupplyCost = m_fSupplyCost * consumer.GetBuyMultiplier();
		}

		return m_fSupplyCost;
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_ArsenalItemListConfig CRF_GetOverwriteListForSlot()
	{
		if (!GetStorageUI() || !GetStorageUI().GetCurrentNavigationStorage())
			return null;

		IEntity storageEntity = GetStorageUI().GetCurrentNavigationStorage().GetOwner();
		if (!storageEntity)
			return null;

		SCR_ArsenalComponent arsenal = SCR_ArsenalComponent.Cast(storageEntity.FindComponent(SCR_ArsenalComponent));
		if (!arsenal)
			return null;

		return arsenal.GetOverwriteArsenalConfig();
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName CRF_GetSlotItemPrefab()
	{
		if (!m_pItem || !m_pItem.GetOwner() || !m_pItem.GetOwner().GetPrefabData())
			return string.Empty;

		return m_pItem.GetOwner().GetPrefabData().GetPrefabName();
	}
}

//------------------------------------------------------------------------------------
modded class SCR_ArsenalComponent
{
	//------------------------------------------------------------------------------------------------
	//! Swaps in a different overwrite item list. Vanilla exposes GetOverwriteArsenalConfig()
	//! already, but has no setter, and a prefab that authored no list needs one creating.
	//! \param[in] config List the arsenal should serve from now on
	void CRF_SetOverwriteArsenalConfig(SCR_ArsenalItemListConfig config)
	{
		m_OverwriteArsenalConfig = config;
	}
}
