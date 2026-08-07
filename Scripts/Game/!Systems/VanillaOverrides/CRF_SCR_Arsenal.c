//------------------------------------------------------------------------------------
// The vanilla arsenal classes keep their item list behind protected members with no
// script accessors, so a gamemode cannot build an arsenal at runtime. Cache Hunt needs
// exactly that: the defending faction's ammunition, pulled from their assigned
// gearscript, dropped into each cache's arsenal.
//
// A modded class can reach its own class's protected members, so every accessor below
// does its work from inside the class that owns the field. The member names are taken
// straight from authored prefab data (see CRF_CacheHunt_Cache.et), not guessed.
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
modded class SCR_ArsenalItemStandalone
{
	//------------------------------------------------------------------------------------------------
	//! Fills in this entry's resource and cost.
	//! \param[in] resource Item prefab this arsenal entry hands out
	//! \param[in] supplyCost Supplies charged per take, 0 for free
	void CRF_Configure(ResourceName resource, int supplyCost)
	{
		m_ItemResourceName = resource;
		m_iSupplyCost = supplyCost;
	}
}

//------------------------------------------------------------------------------------
modded class SCR_ArsenalItemListConfig
{
	//------------------------------------------------------------------------------------------------
	//! Replaces the whole list with one standalone entry per resource.
	//! Building the entries in here keeps every protected access inside its owning class.
	//! \param[in] resources Item prefabs the arsenal should offer
	//! \param[in] supplyCost Supplies charged per take, 0 for free
	void CRF_ReplaceWithStandaloneItems(notnull array<ResourceName> resources, int supplyCost)
	{
		if (!m_aArsenalItems)
			m_aArsenalItems = {};

		m_aArsenalItems.Clear();

		foreach (ResourceName resource : resources)
		{
			if (resource.IsEmpty())
				continue;

			SCR_ArsenalItemStandalone item = new SCR_ArsenalItemStandalone();
			item.CRF_Configure(resource, supplyCost);

			m_aArsenalItems.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return How many entries the list currently holds
	int CRF_GetItemCount()
	{
		if (!m_aArsenalItems)
			return 0;

		return m_aArsenalItems.Count();
	}
}

//------------------------------------------------------------------------------------
modded class SCR_ArsenalComponent
{
	//------------------------------------------------------------------------------------------------
	//! \return The component's overwrite item list, or null when the prefab authored none
	SCR_ArsenalItemListConfig CRF_GetOverwriteArsenalConfig()
	{
		return m_OverwriteArsenalConfig;
	}

	//------------------------------------------------------------------------------------------------
	//! Swaps in a different overwrite item list.
	//! \param[in] config List the arsenal should serve from now on
	void CRF_SetOverwriteArsenalConfig(SCR_ArsenalItemListConfig config)
	{
		m_OverwriteArsenalConfig = config;
	}
}
