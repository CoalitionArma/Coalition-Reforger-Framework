class CRF_BackpackSpawnPointClass : GenericEntityClass
{
}

class CRF_BackpackSpawnPoint: GenericEntity
{
	[Attribute("0", "auto", "Is this the default respawn point to be selected", category: "CRF SpawnPoint Settings")]
	bool m_bIsDefaultSpawn;
	
	[Attribute(category: "CRF SpawnPoint Settings")]
	ref CRF_SpawnPointData m_SpawnPointSettings;
};