//------------------------------------------------------------------------------------------------
modded class SCR_ItemPlacementComponent : ScriptComponent
{	
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
