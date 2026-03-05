//------------------------------------------------------------------------------------------------
// Serializer for CRF Vehicle Spawner entities
// TODO: Fix entity serialization - ScriptedEntitySerializer may not be the correct approach
// Consider using a state serializer that finds and serializes vehicle spawner entities instead
/*
class CRF_VehicleSpawnerSerializer : ScriptedEntitySerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_VehicleSpawner;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_VehicleSpawner vehicleSpawner = CRF_VehicleSpawner.Cast(instance);
		if (!vehicleSpawner)
			return ESerializeResult.DEFAULT;

		// Save version for future compatibility
		context.WriteValue("version", 1);

		// Save spawner state
		context.WriteValue("isActive", vehicleSpawner.m_bIsActive);
		context.WriteValue("hasSpawned", vehicleSpawner.m_bHasSpawned);
		
		// Save spawn count
		context.WriteValue("spawnCount", vehicleSpawner.m_iSpawnCount);

		// Save spawned vehicle RplId if exists
		if (vehicleSpawner.m_SpawnedVehicle && vehicleSpawner.m_SpawnedVehicle.IsValid())
		{
			context.WriteRplId("spawnedVehicleId", vehicleSpawner.m_SpawnedVehicle);
		}

		IEntity entity = IEntity.Cast(instance);
		if (entity)
			Print(string.Format("[CRF_VehicleSpawnerSerializer] Serialized vehicle spawner: %1", entity.GetName()), LogLevel.VERBOSE);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		CRF_VehicleSpawner vehicleSpawner = CRF_VehicleSpawner.Cast(instance);
		if (!vehicleSpawner)
			return false;

		int version;
		if (!context.ReadValue("version", version))
			return false;

		// Restore spawner state
		bool isActive;
		if (context.ReadValue("isActive", isActive))
			vehicleSpawner.m_bIsActive = isActive;

		bool hasSpawned;
		if (context.ReadValue("hasSpawned", hasSpawned))
			vehicleSpawner.m_bHasSpawned = hasSpawned;

		// Restore spawn count
		int spawnCount;
		if (context.ReadValue("spawnCount", spawnCount))
			vehicleSpawner.m_iSpawnCount = spawnCount;

		// Restore spawned vehicle reference
		RplId spawnedVehicleId;
		if (context.ReadRplId("spawnedVehicleId", spawnedVehicleId))
		{
			vehicleSpawner.m_SpawnedVehicle = spawnedVehicleId;
		}

		IEntity entity = IEntity.Cast(instance);
		if (entity)
			Print(string.Format("[CRF_VehicleSpawnerSerializer] Restored vehicle spawner: %1", entity.GetName()), LogLevel.VERBOSE);
		return true;
	}
}
*/
