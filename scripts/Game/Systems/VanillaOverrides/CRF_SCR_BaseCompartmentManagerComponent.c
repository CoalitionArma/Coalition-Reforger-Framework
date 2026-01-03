modded class SCR_BaseCompartmentManagerComponent
{
	[Attribute(category: "CRF_Spawner", params: "et")] ResourceName m_GroupToSpawn;
	
	void AddAIToVehicle()
	{
		if (m_GroupToSpawn == "")
			return;
		
		EntitySpawnParams params = new EntitySpawnParams();
		GetOwner().GetTransform(params.Transform);
		
		IEntity group = GetGame().SpawnEntityPrefab(Resource.Load(m_GroupToSpawn), null, params);
		if (!SCR_AIGroup.Cast(group))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(group);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(AddToCompartment, 500, false, SCR_AIGroup.Cast(group));
	}
	
	void AddToCompartment(SCR_AIGroup group)
	{
		array<BaseCompartmentSlot> cargoCompartments = {};
		array<BaseCompartmentSlot> driverCompartments = {};
		array<BaseCompartmentSlot> turretCompartments = {};
		GetCompartmentsOfType(cargoCompartments, ECompartmentType.CARGO);
		GetCompartmentsOfType(driverCompartments, ECompartmentType.PILOT);
		GetCompartmentsOfType(turretCompartments, ECompartmentType.TURRET);
		
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		array<AIAgent> agentsAddedToVehicle = {};
		array<BaseCompartmentSlot> compartmentsUsed = {};
		foreach (AIAgent agent: agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character)
				return;
			
			CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
			if (!compartmentAccess)
				continue;
			
			foreach (BaseCompartmentSlot compartment: driverCompartments)
			{
				if (compartmentsUsed.Contains(compartment))
					continue;
				
				compartmentsUsed.Insert(compartment);
				compartmentAccess.GetInVehicle(GetOwner(), compartment, true, -1, ECloseDoorAfterActions.INVALID, true);
				agentsAddedToVehicle.Insert(agent);
				break;
			}
			
			if (agentsAddedToVehicle.Contains(agent))
				continue;
			
			foreach (BaseCompartmentSlot compartment: turretCompartments)
			{
				if (compartmentsUsed.Contains(compartment))
					continue;

				compartmentsUsed.Insert(compartment);
				compartmentAccess.GetInVehicle(GetOwner(), compartment, true, -1, ECloseDoorAfterActions.INVALID, true);
				agentsAddedToVehicle.Insert(agent);
				break;
			}
			
			if (agentsAddedToVehicle.Contains(agent))
				continue;
			
			foreach (BaseCompartmentSlot compartment: cargoCompartments)
			{
				if (compartmentsUsed.Contains(compartment))
					continue;
				
				compartmentsUsed.Insert(compartment);
				compartmentAccess.GetInVehicle(GetOwner(), compartment, true, -1, ECloseDoorAfterActions.INVALID, true);
				agentsAddedToVehicle.Insert(agent);
				break;
			}
		}
		
		foreach (AIAgent agent: agents)
		{
			if (agentsAddedToVehicle.Contains(agent))
				continue;
			SCR_EntityHelper.DeleteEntityAndChildren(agent.GetControlledEntity());
		}
	}
}