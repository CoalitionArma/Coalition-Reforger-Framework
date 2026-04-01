class CRF_RallyPointClass : GenericEntityClass
{
}

class CRF_RallyPoint: GenericEntity
{	
	ref CRF_SpawnPointData m_SpawnPointSettings = new CRF_SpawnPointData();
	
	protected int m_iLocallyStoredId;
	
	protected SCR_AIGroup m_group;
	protected Faction m_faction;

	//------------------------------------------------------------------------------------------------
	void SetupRallyPoint()
	{
		array<string> factions = new array<string>();
		SCR_Enum.GetEnumNames(CRF_EFactions, factions);
		
		SCR_CampaignBuildingCompositionComponent compositionComponent = SCR_CampaignBuildingCompositionComponent.Cast(this.FindComponent(SCR_CampaignBuildingCompositionComponent));
		if (!compositionComponent)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		int playerId = compositionComponent.GetBuilderId();
		if (playerId == -1)
			return;

		m_group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!m_group)
			return;
		
		m_faction = slottingManager.GetPlayerSlotFaction(playerId);
		if (!m_faction)
			return;
		
		string groupCustomName = m_group.GetCustomNameWithOriginal();
		
		RemovePreviousRallyPoint();
		
		m_SpawnPointSettings.SetSpawnPointName(groupCustomName + " RP");
		m_SpawnPointSettings.SetSpawnPointFaction(factions.Find(m_faction.GetFactionKey()));
		m_SpawnPointSettings.SetRestrictedToGroup(groupCustomName);
		m_SpawnPointSettings.SetSpawnPointActive(true);
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().RegisterRespawnPoint(m_SpawnPointSettings, this);
	}
	
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
	private void RemovePreviousRallyPoint()
	{
		array<CRF_SpawnPointData> factionRespawnPoints = CRF_RespawnManager.GetInstance().GetFactionSpawnpoints(m_faction.GetFactionKey(), m_group);
		
		foreach(CRF_SpawnPointData spawnPointData : factionRespawnPoints)
		{ 
			IEntity entity = CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
			if (!entity)
				continue;
			
			CRF_RallyPoint rally = CRF_RallyPoint.Cast(entity);
			if (!rally)
				continue;
			
			if (rally.m_group == m_group)			
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ~CRF_RallyPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};