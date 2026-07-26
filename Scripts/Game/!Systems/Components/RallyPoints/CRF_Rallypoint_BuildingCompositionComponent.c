//------------------------------------------------------------------------------------------------
class COA_RallyPoint_BuildingCompositionComponentClass : SCR_CampaignBuildingCompositionComponentClass
{
}

//------------------------------------------------------------------------------------------------
class COA_RallyPoint_BuildingCompositionComponent : SCR_CampaignBuildingCompositionComponent
{
	ref COA_SpawnPointData m_SpawnPointSettings = new COA_SpawnPointData();
	
	protected int m_iLocallyStoredId;
	
	//------------------------------------------------------------------------------------------------
	override protected void SetIsCompositionSpawned()
	{
		super.SetIsCompositionSpawned();
		
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		COA_RallyPoint rallyPoint = COA_RallyPoint.Cast(GetOwner());
		if (rallyPoint)
			rallyPoint.SetupRallyPoint();
	}
}