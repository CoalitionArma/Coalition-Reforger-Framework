//------------------------------------------------------------------------------------------------
modded class SCR_ItemPlacementComponent : ScriptComponent
{	
	//------------------------------------------------------------------------------------------------
	//! Only allow squad leaders to place rallypoints
	override void ValidatePlacement(vector up, IEntity tracedEntity, BaseWorld world, IEntity character, out ENotification cantPlaceReason)
	{
		super.ValidatePlacement(up, tracedEntity, world, character, cantPlaceReason);
		if (cantPlaceReason > 0)
			return;
		
		SCR_CampaignBuildingGadgetToolComponent gadgetComponent = SCR_CampaignBuildingGadgetToolComponent.Cast(m_PlacedItem.FindComponent(SCR_CampaignBuildingGadgetToolComponent));
		if (!gadgetComponent)
			return super.StartPlaceItem();

		if (!CRF_RoleHelper.IsSquadLeaderRole(character) && CRF_Gamemode.GetInstance().m_bRallyPointsEnabled && gadgetComponent.ACE_Trenches_GetCurrentVariantID() == 5) // This might change?
			cantPlaceReason = "Not squadleader";
	}
}
