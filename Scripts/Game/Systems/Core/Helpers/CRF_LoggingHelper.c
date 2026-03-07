class CRF_LoggingHelper
{
	//------------------------------------------------------------------------------------------------
	//! Log item error
	//! \param[in] item Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(IEntity item, IEntity entity, string itemType = "ITEM")
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF ERROR: UNABLE TO INSERT %1: %2", itemType, item.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(string.Format("CRF ERROR: INTO ENTITY: %1", entity.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print(string.Format("CRF ERROR: NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType), LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Alt Log item error
	//! \param[in] itemResource Item that failed to insert
	//! \param[in] entity Entity that the item was being added to
	//! \param[in] itemType type of item to display (default is "ITEM")
	static void LogItemError(ResourceName itemResource, IEntity entity, string itemType = "ITEM")
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF ERROR: UNABLE TO INSERT %1: %2", itemType, itemResource), LogLevel.ERROR);
		Print(string.Format("CRF ERROR: INTO ENTITY: %1", entity.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print(string.Format("CRF ERROR: NOT ENOUGH SPACE IN ENTITY/INVALID %1!", itemType), LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}
}