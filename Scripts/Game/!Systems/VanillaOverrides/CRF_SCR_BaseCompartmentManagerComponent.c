modded class SCR_BaseCompartmentManagerComponent
{
	[Attribute(category: "CRF_Spawner", params: "et")] ResourceName m_GroupToSpawn;

	//! RplId of the crew group spawned for this vehicle, so the deferred seating pass can re-resolve
	//! it instead of holding a raw entity pointer across the delay.
	protected RplId m_SpawnedCrewGroupId;

	void AddAIToVehicle()
	{
		if (m_GroupToSpawn == "")
			return;

		// Validate the prefab before handing it to the engine, as vanilla does at every spawn site.
		Resource groupResource = Resource.Load(m_GroupToSpawn);
		if (!groupResource || !groupResource.IsValid())
		{
			Print(string.Format("[CRF] Could not load AI group prefab '%1' for vehicle crew.", m_GroupToSpawn), LogLevel.ERROR);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		GetOwner().GetTransform(params.Transform);

		IEntity group = GetGame().SpawnEntityPrefab(groupResource, null, params);
		if (!group)
			return;

		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(group);
		if (!aiGroup)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(group);
			return;
		}

		// Defer by RplId, not by pointer. This call carried a raw SCR_AIGroup across 500ms and
		// AddToCompartment dereferenced it with no null check - and the callback targets a method on
		// THIS component, which lives on the vehicle. Deleting the vehicle anywhere inside that
		// window invoked a method on a freed component holding a freed group.
		RplComponent groupRpl = RplComponent.Cast(aiGroup.FindComponent(RplComponent));
		if (!groupRpl)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(group);
			return;
		}

		m_SpawnedCrewGroupId = groupRpl.Id();
		GetGame().GetCallqueue().CallLater(AddToCompartment, 500, false, m_SpawnedCrewGroupId);
	}

	//------------------------------------------------------------------------------------------------
	//! Cancel the pending crew placement if the vehicle goes away first.
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(AddToCompartment);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	void AddToCompartment(RplId groupId)
	{
		// Re-resolve both ends at use time: the group may have been deleted, and GetOwner() (the
		// vehicle) must still be valid before we start seating anyone in it.
		RplComponent groupRpl = RplComponent.Cast(Replication.FindItem(groupId));
		if (!groupRpl)
			return;

		SCR_AIGroup group = SCR_AIGroup.Cast(groupRpl.GetEntity());
		if (!group)
			return;

		IEntity vehicle = GetOwner();
		if (!vehicle)
			return;

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
				continue;	// was `return` - a single non-character agent aborted the whole loop,
							// after which the cleanup pass below deleted every remaining crew member

			CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
			if (!compartmentAccess)
				continue;

			// Driver first, then turrets, then cargo.
			if (TrySeatAgent(agent, compartmentAccess, vehicle, driverCompartments, compartmentsUsed, agentsAddedToVehicle))
				continue;

			if (TrySeatAgent(agent, compartmentAccess, vehicle, turretCompartments, compartmentsUsed, agentsAddedToVehicle))
				continue;

			TrySeatAgent(agent, compartmentAccess, vehicle, cargoCompartments, compartmentsUsed, agentsAddedToVehicle);
		}

		// Delete crew that could not be seated. Collect first, then delete - the original deleted
		// entities while iterating the group's own agent array.
		array<IEntity> unseatedEntities = {};
		foreach (AIAgent agent: agents)
		{
			if (agentsAddedToVehicle.Contains(agent))
				continue;

			IEntity unseated = agent.GetControlledEntity();
			if (unseated)
				unseatedEntities.Insert(unseated);
		}

		foreach (IEntity unseated : unseatedEntities)
			SCR_EntityHelper.DeleteEntityAndChildren(unseated);

		// If nobody was seated the group is now empty; drop it rather than leaking an empty
		// SCR_AIGroup for the rest of the mission.
		if (agentsAddedToVehicle.IsEmpty())
			SCR_EntityHelper.DeleteEntityAndChildren(group);
	}

	//------------------------------------------------------------------------------------------------
	//! Seat an agent in the first free compartment of the given set.
	//! Returns true only if the agent actually got in - the previous code marked the agent as seated
	//! and consumed the compartment without checking GetInVehicle()'s result, so a refused entry
	//! silently burned a seat and left the agent to be deleted as "unseated".
	protected bool TrySeatAgent(AIAgent agent, notnull CompartmentAccessComponent compartmentAccess, notnull IEntity vehicle, notnull array<BaseCompartmentSlot> compartments, notnull array<BaseCompartmentSlot> compartmentsUsed, notnull array<AIAgent> agentsAddedToVehicle)
	{
		foreach (BaseCompartmentSlot compartment: compartments)
		{
			if (!compartment || compartmentsUsed.Contains(compartment))
				continue;

			if (!compartmentAccess.GetInVehicle(vehicle, compartment, true, -1, ECloseDoorAfterActions.INVALID, true))
				continue;

			compartmentsUsed.Insert(compartment);
			agentsAddedToVehicle.Insert(agent);
			return true;
		}

		return false;
	}
}