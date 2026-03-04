modded class SCR_CharacterInventoryStorageComponent
{
	override BaseWeaponComponent GetCurrentCharacterWeapon()
	{
		BaseWeaponComponent turretWeapon = GetCurrentTurretWeapon();
		if (turretWeapon)
			return turretWeapon;

		ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return null;

		CharacterControllerComponent controller = character.GetCharacterController();	
		if (!controller)
			return null;

		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if (!weaponManager)
			return null;

		return weaponManager.GetCurrentWeapon();
	}
}