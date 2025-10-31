class CRF_SupplyArsenalComponentClass: ScriptComponentClass
{
}

class CRF_SupplyArsenalComponent: ScriptComponent
{
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
			if (!Replication.FindItem(entityId))
				continue;
			
			entityArray.Insert(RplComponent.Cast(Replication.FindItem(entityId)).GetEntity());
		}
		
		return entityArray;
	}
	
	void UpdateCurrentSupply()
	{
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
		m_aSupplyItems.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
		m_aSupplyCounts.Insert((int)storedResources);
			
		return true;
	}
}