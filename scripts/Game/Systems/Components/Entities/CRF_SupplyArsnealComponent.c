class CRF_SupplyArsenalComponentClass: ScriptComponentClass
{
}

class CRF_SupplyArsenalComponent: ScriptComponent
{
	[Attribute("1")] bool m_bSupplyEnabled;
	
	// REPLICATION FIX: Removed [RplProp()] from arrays
	// Clients only need the total supply count for UI display, not individual items
	// Server maintains full inventory for operations (withdrawals, calculations)
	// Old: ~100-200 bytes per update (2 arrays with 10-20 items each)
	// New: 4 bytes per update (single integer)
	// Reduction: ~95-98% bandwidth savings
	protected ref array<RplId> m_aSupplyItems = {};
	protected ref array<int> m_aSupplyCounts = {};
	
	// Replicate only the total supply count
	[RplProp()]
	protected int m_iTotalSupply = 0;
	
	//------------------------------------------------------------------------------------------------
	// Get current total supply (used by UI)
	int GetCurrentSupply()
	{
		return m_iTotalSupply;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get supply counts array (server-side only - used for calculations)
	array<int> GetSupplyCounts()
	{
		// This method is server-side only
		if (!Replication.IsServer())
		{
			Print("WARNING: GetSupplyCounts() called on client - this is server-side only!", LogLevel.WARNING);
			return {};
		}
		
		return m_aSupplyCounts;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get supply items array (server-side only - used for operations)
	array<RplId> GetSupplyItems()
	{
		// This method is server-side only
		if (!Replication.IsServer())
		{
			Print("WARNING: GetSupplyItems() called on client - this is server-side only!", LogLevel.WARNING);
			return {};
		}
		
		return m_aSupplyItems;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get full entity array (server-side only - used for supply operations)
	array<IEntity> GetEntityArray()
	{
		// This method is server-side only
		if (!Replication.IsServer())
		{
			Print("WARNING: GetEntityArray() called on client - this is server-side only!", LogLevel.WARNING);
			return {};
		}
		
		array<IEntity> entityArray = {};
		foreach (RplId entityId: m_aSupplyItems)
		{
			// Safely get the RplComponent - might be null if item was deleted or streamed out
			RplComponent rplComp = RplComponent.Cast(Replication.FindItem(entityId));
			if (!rplComp)
				continue;
			
			IEntity itemEntity = rplComp.GetEntity();
			if (itemEntity)
				entityArray.Insert(itemEntity);
		}
		
		return entityArray;
	}
	
	//------------------------------------------------------------------------------------------------
	// Update supply inventory (server-side only)
	void UpdateCurrentSupply()
	{
		// Only authority should modify replicated state
		if (!Replication.IsServer())
			return;
			
		// Rebuild inventory arrays
		m_aSupplyItems.Clear();
		m_aSupplyCounts.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), 50, FindSupplyCallback, null);
		
		// Calculate new total
		int newTotal = 0;
		foreach (int count: m_aSupplyCounts)
		{
			newTotal += count;
		}
		
		// Only replicate if total changed (optimization)
		if (m_iTotalSupply != newTotal)
		{
			m_iTotalSupply = newTotal;
			Replication.BumpMe();
		}
	}
	
	bool FindSupplyCallback(IEntity entity)
	{
		if (!entity.FindComponent(SCR_ResourceComponent))
			return true;
		
		SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.FindResourceComponent(entity, false);
		float storedResources = 0;
		
		SCR_ResourceConsumer resConsumer = SCR_ResourceSystemHelper.GetStorageConsumer(resourceComponent);
		if (!resConsumer)
			return true;
		
		storedResources = resConsumer.GetAggregatedResourceValue();
		
		// Safely get RplComponent - entity might not be replicated
		RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		if (!rplComp)
			return true;
			
		m_aSupplyItems.Insert(rplComp.Id());
		m_aSupplyCounts.Insert((int)storedResources);
			
		return true;
	}
}