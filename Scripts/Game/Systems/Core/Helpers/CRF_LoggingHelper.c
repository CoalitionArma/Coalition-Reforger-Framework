class CRF_LoggingHelper
{
	//------------------------------------------------------------------------------------------------
	//! Log item error
	//! \param[in] item Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(IEntity item, IEntity entity, string itemType = "ITEM")
	{
		Print(string.Format("[CRF GEARSCRIPT ERROR] : UNABLE TO INSERT %1: %2 \n INTO ENTITY: %3 \n\n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, item.GetPrefabData().GetPrefabName(), entity.GetPrefabData().GetPrefabName()), LogLevel.FATAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Alt Log item error
	//! \param[in] itemResource Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(ResourceName itemResource, IEntity entity, string itemType = "ITEM")
	{
		Print(string.Format("[CRF GEARSCRIPT ERROR] : UNABLE TO INSERT %1: %2 \n INTO ENTITY: %3 \n\n NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType, itemResource, entity.GetPrefabData().GetPrefabName()), LogLevel.FATAL);
	}
}