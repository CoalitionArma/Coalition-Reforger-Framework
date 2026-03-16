class CRF_GroupSpawnPointClass : GenericEntityClass
{
}

class CRF_GroupSpawnPoint: GenericEntity
{
	[Attribute("1-1", "auto", "Callsign for the group to spawn at this point", category: "CRF Spawn Point Settings")]
	string m_sCallsignOfGroupToSpawn;
	
	[Attribute(category: "CRF Spawn Point Settings")]
	ref CRF_SpawnPointContainer m_SpawnPointSettings;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Only server should register respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		m_SpawnPointSettings.SetSpawnPointToTemp();
		
		if(CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);
	};
	
	//------------------------------------------------------------------------------------------------
	void ~CRF_GroupSpawnPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(this);
	}
};