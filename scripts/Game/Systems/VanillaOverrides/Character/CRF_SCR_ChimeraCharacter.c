modded class SCR_ChimeraCharacter
{
	// Used in the event pole process to know if we should delete this character if a respawn is requested.
	// Only tracked on the auth
	bool m_bEventPoleCharacter = false;
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
		
		BaseWeaponManagerComponent weaponMan = ChimeraCharacter.Cast(entity).GetWeaponManager();
		if (!weaponMan)
			return;
		
		CharacterControllerComponent charController = ChimeraCharacter.Cast(entity).GetCharacterController();
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