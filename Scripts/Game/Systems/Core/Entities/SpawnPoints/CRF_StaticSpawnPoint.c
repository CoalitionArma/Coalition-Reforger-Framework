class CRF_StaticSpawnPointClass : GenericEntityClass
{
}

class CRF_StaticSpawnPoint: GenericEntity
{
	[Attribute("0", "auto", "Is this the default respawn point to be selected", category: "CRF Spawn Point Settings")]
	bool m_bIsDefaultSpawn;
	
	[Attribute(category: "CRF Spawn Point Settings")]
	ref CRF_SpawnPointContainer m_SpawnPointSettings;
	
	protected int m_iLocallyStoredId;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Only server should register respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if(CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);
	};
	
	//------------------------------------------------------------------------------------------------
	int GetLocalSpawnPointId()
	{
		return m_iLocallyStoredId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetLocalSpawnPointId(int spawnPointId)
	{
		m_iLocallyStoredId = spawnPointId;
	}
	
	//------------------------------------------------------------------------------------------------
	void ~CRF_StaticSpawnPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};