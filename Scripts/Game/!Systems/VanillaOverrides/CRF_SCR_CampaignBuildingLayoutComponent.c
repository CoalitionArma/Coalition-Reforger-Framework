//------------------------------------------------------------------------------------------------
//! Scales up the required building value for ACE trench compositions so digging one takes more
//! E-tool swings: 2x during safestart, 4x once it ends, by default (see
//! CRF_SCR_CampaignBuildingGadgetToolComponent.CRF_GetTrenchDigSlowdownMultiplier for the tunables).
//! This directly changes the "Build X% [built Y%]" UI, unlike scaling swing timing would.
//!
//! ACE_Trenches_BuildingCompositionComponent lives on the root parent entity, not on this
//! component's own owner (which only carries the outline/layout, spawned as a child - see vanilla's
//! SCR_CampaignBuildingCompositionComponent.SpawnCompositionLayout, which parents the layout entity
//! under the composition root, and LockCompositionInteraction elsewhere in the vanilla class, which
//! uses this exact same GetOwner().GetRootParent() lookup to find the composition component).
modded class SCR_CampaignBuildingLayoutComponent
{
	//------------------------------------------------------------------------------------------------
	override int GetBuildingValue(int prefabID)
	{
		int baseValue = super.GetBuildingValue(prefabID);

		IEntity rootEntity = GetOwner().GetRootParent();
		if (!rootEntity || !rootEntity.FindComponent(ACE_Trenches_BuildingCompositionComponent))
			return baseValue;

		float slowdown = SCR_CampaignBuildingGadgetToolComponent.CRF_GetTrenchDigSlowdownMultiplier();
		if (slowdown <= 0)
			return baseValue;

		return (int)Math.Round(baseValue * slowdown);
	}
}
