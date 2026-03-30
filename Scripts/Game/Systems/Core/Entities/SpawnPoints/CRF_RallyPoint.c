class CRF_RallyPointClass : GenericEntityClass
{
}

class CRF_RallyPoint: GenericEntity
{	
	ref CRF_SpawnPointData m_SpawnPointSettings = new CRF_SpawnPointData();
	
	protected int m_iLocallyStoredId;

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

		SCR_AIGroup group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;
		
		Faction faction = slottingManager.GetPlayerSlotFaction(playerId);
		
		string groupCustomName = group.GetCustomNameWithOriginal();
		
		m_SpawnPointSettings.SetSpawnPointName(groupCustomName + " RP");
		m_SpawnPointSettings.SetSpawnPointFaction(factions.Find(faction.GetFactionKey()));
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
	void ~CRF_RallyPoint()
	{
		// Only server should unregister respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(m_iLocallyStoredId);
	}
};