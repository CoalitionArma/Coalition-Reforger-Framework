class CRF_LoggingHelper
{
	//------------------------------------------------------------------------------------------------
	//! Log item error
	//! \param[in] item Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(IEntity item, IEntity entity, string itemType = "ITEM")
	{
		string error = string.Format("[%3 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(item.GetPrefabData().GetPrefabName()), SanitizeResourceName(entity.GetPrefabData().GetPrefabName()));
		
		Debug.Error(error);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Alt Log item error
	//! \param[in] itemResource Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(ResourceName itemResource, IEntity entity, string itemType = "ITEM")
	{
		string error = string.Format("[%3 GEARSCRIPT ERROR] \n\n UNABLE TO INSERT %1 %2 \n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, SanitizeResourceName(itemResource), SanitizeResourceName(entity.GetPrefabData().GetPrefabName()));
		
		Debug.Error(error);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sanitize Resource Name of its path and extension
	//! \param[in] resName Resource name to sanitize and strip (kinky)
	static string SanitizeResourceName(ResourceName resName)
	{
		resName = FilePath.StripPath(resName);
		resName = FilePath.StripExtension(resName);
		resName.ToUpper();
		
		return resName;
	}
}