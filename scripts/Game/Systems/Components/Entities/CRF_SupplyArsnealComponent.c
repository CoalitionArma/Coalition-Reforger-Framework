class CRF_SupplyArsenalComponentClass: ScriptComponentClass
{
}

class CRF_SupplyArsenalComponent: ScriptComponent
{
	[Attribute("1")] bool m_bSupplyEnabled;
	[RplProp()] ref array<RplId> m_aSupplyItems = {};
	[RplProp()] ref array<int> m_aSupplyCounts = {};
	
	int GetCurrentSupply()
	{
		int totalSupplies = 0;
		foreach (int supply: m_aSupplyCounts)
		{
			totalSupplies += supply;
		}
		return totalSupplies;
	}
	
	array<IEntity> GetEntityArray()
	{
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
	
	void UpdateCurrentSupply()
	{
		// Only authority should modify replicated state
		if (!Replication.IsServer())
			return;
			
		m_aSupplyItems.Clear();
		m_aSupplyCounts.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), 50, FindSupplyCallback, null);
		Replication.BumpMe();
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