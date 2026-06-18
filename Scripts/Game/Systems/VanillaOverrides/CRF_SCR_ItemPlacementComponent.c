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

	//------------------------------------------------------------------------------------------------
	//! Re-enables movement and weapon controls after placement ends.
	//! Vanilla locks both in OnPlacingEnded but never restores them on the client — the only
	//! vanilla path that restores them is UpdateControls() firing when a menu opens then closes,
	//! which is why opening/closing inventory unfreezes the player.
	override protected void OnPlacingEnded(IEntity item, bool successful, ItemUseParameters animParams)
	{
		super.OnPlacingEnded(item, successful, animParams);

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return;

		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!charController)
			return;

		charController.SetDisableMovementControls(false);
		charController.SetDisableWeaponControls(false);
	}
}
