//------------------------------------------------------------------------------------------------
//! CRF_ReplicationHelpers
//! Utility class providing safe wrappers for common replication operations
//! These helpers prevent null pointer dereferences and enforce best practices
//------------------------------------------------------------------------------------------------
class CRF_ReplicationHelpers
{
	//------------------------------------------------------------------------------------------------
	//! Safely get RplId from an entity
	//! \param entity The entity to get RplId from
	//! \param[out] outRplId The resulting RplId (only valid if function returns true)
	//! \return True if entity has RplComponent and RplId was retrieved, false otherwise
	static bool GetRplId(IEntity entity, out RplId outRplId)
	{
		if (!entity)
			return false;
			
		RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rplComp)
			return false;
			
		outRplId = rplComp.Id();
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Safely get entity from RplId
	//! \param rplId The RplId to look up
	//! \return The entity if found and valid, null otherwise
	static IEntity GetEntityFromRplId(RplId rplId)
	{
		if (!rplId.IsValid())
			return null;
			
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplId));
		if (!rplComp)
			return null;
			
		return rplComp.GetEntity();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Safely get RplComponent from entity
	//! \param entity The entity to get RplComponent from
	//! \return The RplComponent if found, null otherwise
	static RplComponent GetRplComponent(IEntity entity)
	{
		if (!entity)
			return null;
			
		return RplComponent.Cast(entity.FindComponent(RplComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if entity is authority (server-side)
	//! \param entity The entity to check
	//! \return True if entity is authority, false if proxy or invalid
	static bool IsAuthority(IEntity entity)
	{
		RplComponent rplComp = GetRplComponent(entity);
		if (!rplComp)
			return Replication.IsServer(); // Fallback for non-replicated entities
			
		return rplComp.IsProxy() == false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if entity is proxy (client-side)
	//! \param entity The entity to check  
	//! \return True if entity is proxy, false if authority or invalid
	static bool IsProxy(IEntity entity)
	{
		RplComponent rplComp = GetRplComponent(entity);
		if (!rplComp)
			return !Replication.IsServer(); // Fallback for non-replicated entities
			
		return rplComp.IsProxy();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Safely call Replication.BumpMe() only if on server
	//! This prevents clients from attempting to replicate changes
	//! \return True if BumpMe was called, false if skipped (not on server)
	static bool SafeBumpMe()
	{
		if (!Replication.IsServer())
			return false;
			
		Replication.BumpMe();
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get multiple entities from array of RplIds
	//! Skips invalid or non-existent entities
	//! \param rplIds Array of RplIds to look up
	//! \param[out] outEntities Array to fill with found entities
	//! \return Number of entities successfully retrieved
	static int GetEntitiesFromRplIds(notnull array<RplId> rplIds, out notnull array<IEntity> outEntities)
	{
		outEntities.Clear();
		
		foreach (RplId rplId : rplIds)
		{
			IEntity entity = GetEntityFromRplId(rplId);
			if (entity)
				outEntities.Insert(entity);
		}
		
		return outEntities.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get multiple RplIds from array of entities
	//! Skips entities without RplComponent
	//! \param entities Array of entities to get RplIds from
	//! \param[out] outRplIds Array to fill with found RplIds
	//! \return Number of RplIds successfully retrieved
	static int GetRplIdsFromEntities(notnull array<IEntity> entities, out notnull array<RplId> outRplIds)
	{
		outRplIds.Clear();
		
		foreach (IEntity entity : entities)
		{
			RplId rplId;
			if (GetRplId(entity, rplId))
				outRplIds.Insert(rplId);
		}
		
		return outRplIds.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Validate that an entity has proper replication setup
	//! Useful for debugging and prefab validation
	//! \param entity The entity to validate
	//! \param printWarnings Whether to print warnings for validation failures
	//! \return True if entity has valid RplComponent, false otherwise
	static bool ValidateReplicationSetup(IEntity entity, bool printWarnings = true)
	{
		if (!entity)
		{
			if (printWarnings)
				Print("[CRF_ReplicationHelpers] WARNING: Null entity passed to ValidateReplicationSetup", LogLevel.WARNING);
			return false;
		}
		
		RplComponent rplComp = GetRplComponent(entity);
		if (!rplComp)
		{
			if (printWarnings)
			{
				string entityName = entity.GetName();
				Print(string.Format("[CRF_ReplicationHelpers] WARNING: Entity '%1' has no RplComponent but may require replication", entityName), LogLevel.WARNING);
			}
			return false;
		}
		
		RplId rplId = rplComp.Id();
		if (!rplId.IsValid())
		{
			if (printWarnings)
				Print(string.Format("[CRF_ReplicationHelpers] WARNING: Entity '%1' has RplComponent but invalid RplId", entity.GetName()), LogLevel.WARNING);
			return false;
		}
		
		return true;
	}
}
