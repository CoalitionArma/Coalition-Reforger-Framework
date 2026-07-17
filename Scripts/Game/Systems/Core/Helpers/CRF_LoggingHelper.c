class CRF_LoggingHelper
{
	//------------------------------------------------------------------------------------------------
	//! Log item error
	//! \param[in] item Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(IEntity item, IEntity entity, string itemType = "ITEM", CRF_EGearRole role = CRF_EGearRole.UNARMED)
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (facComp)
		{
			string error = string.Format("[%3 %4 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(item.GetPrefabData().GetPrefabName()), facComp.GetAffiliatedFaction().GetFactionKey(), role);
		
			Debug.Error(error);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Alt Log item error
	//! \param[in] itemResource ResourceName of the item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(ResourceName itemResource, IEntity entity, string itemType = "ITEM", CRF_EGearRole role = CRF_EGearRole.UNARMED)
	{	
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (facComp)
		{
			string error = string.Format("[%3 %4 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(itemResource), facComp.GetAffiliatedFaction().GetFactionKey(), role);
		
			Debug.Error(error);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sanitize Resource Name of its path and extension
	//! \param[in] resName Resource name to sanitize and strip (kinky)
	//! \return sanatized string in uppercase
	static string SanitizeResourceName(ResourceName resName)
	{
		resName = FilePath.StripPath(resName);
		resName = FilePath.StripExtension(resName);
		resName.ToUpper();

		return resName;
	}
}
