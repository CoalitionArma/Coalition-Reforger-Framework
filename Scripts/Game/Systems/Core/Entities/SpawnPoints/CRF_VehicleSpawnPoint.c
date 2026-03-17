class CRF_VehicleSpawnPointClass : VehicleClass
{
}

class CRF_VehicleSpawnPoint: Vehicle
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
	void ~CRF_VehicleSpawnPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};