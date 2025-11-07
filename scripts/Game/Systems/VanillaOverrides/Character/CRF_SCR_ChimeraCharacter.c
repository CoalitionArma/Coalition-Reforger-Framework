modded class SCR_ChimeraCharacter
{
	void SelectPrimaryWeapon()
	{
		Rpc(RpcDo_SelectPrimaryWeapon);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SelectPrimaryWeapon()
	{
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (!entity)
			return;
		
		if (!ChimeraCharacter.Cast(entity))
			return;
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(entity).GetWeaponManager());
		if (!weaponMan)
			return;
		
		CharacterControllerComponent charController = CharacterControllerComponent.Cast(ChimeraCharacter.Cast(entity).GetCharacterController());
		if (!charController)
			return;
		
		array<WeaponSlotComponent> outSlots = {};
		weaponMan.GetWeaponsSlots(outSlots);
		WeaponSlotComponent weapon;
		foreach (WeaponSlotComponent outSlot: outSlots)
		{
			if (!outSlot.GetWeaponEntity())
				continue;
			
			if (outSlot.GetWeaponEntity().FindComponent(GrenadeMoveComponent))
				continue;
			
			weapon = outSlot;
			break;
		}
		
		if (!weapon)
			return;

		Print("Selecting Weapon");
		charController.SelectWeapon(weapon);
	}
}