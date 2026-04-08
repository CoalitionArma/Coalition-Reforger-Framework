//------------------------------------------------------------------------------------------------
class CRF_Rallypoint_BuildingCompositionComponentClass : SCR_CampaignBuildingCompositionComponentClass
{
}

//------------------------------------------------------------------------------------------------
class CRF_Rallypoint_BuildingCompositionComponent : SCR_CampaignBuildingCompositionComponent
{
	ref CRF_SpawnPointData m_SpawnPointSettings = new CRF_SpawnPointData();
	
	protected int m_iLocallyStoredId;
	
	//------------------------------------------------------------------------------------------------
	override protected void SetIsCompositionSpawned()
	{
		super.SetIsCompositionSpawned();
		
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		CRF_RallyPoint rallyPoint = CRF_RallyPoint.Cast(GetOwner());
		if (rallyPoint)
			rallyPoint.SetupRallyPoint();
	}
}