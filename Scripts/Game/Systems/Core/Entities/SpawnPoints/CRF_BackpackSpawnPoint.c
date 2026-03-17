class CRF_BackpackSpawnPointClass : GenericEntityClass
{
}

class CRF_BackpackSpawnPoint: GenericEntity
{	
	[Attribute(category: "CRF Spawn Point Settings")]
	ref CRF_SpawnPointData m_SpawnPointSettings;
};