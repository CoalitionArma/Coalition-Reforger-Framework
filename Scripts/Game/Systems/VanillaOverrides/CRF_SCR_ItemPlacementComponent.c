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
			return;
		
		if (gadgetComponent.ACE_Trenches_GetCurrentVariantID() != 10042001)
			return;

		if (!CRF_RoleHelper.IsSquadLeaderRole(character))
			cantPlaceReason = ENotification.PLACEABLE_ITEM_CANT_PLACE_GENERIC;
		
		if (!CRF_Gamemode.GetInstance().m_bRallyPointsEnabled)
			cantPlaceReason = ENotification.PLACEABLE_ITEM_CANT_PLACE_GENERIC;
	}
}
