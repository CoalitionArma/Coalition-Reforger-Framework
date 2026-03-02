class CRF_GearscriptCharacterClass : SCR_ChimeraCharacterClass
{
}

class CRF_GearscriptCharacter : SCR_ChimeraCharacter
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		
		if (!GetGame().InPlayMode() || !gearscriptManager)
			return;
		
		// Schedule gearscript identity setup with appropriate delay
		GetGame().GetCallqueue().Call(
			gearscriptManager.SetEntityIdentity, 
			owner
		);
	
		// Apply gearscript if not on client
		if (RplSession.Mode() != RplMode.Client)
			// Schedule gear setup with appropriate delay
			GetGame().GetCallqueue().Call(
				gearscriptManager.SetEntityGear, 
				owner, 
				owner.GetPrefabData().GetPrefabName()
			);
	}
	
	//------------------------------------------------------------------------------------------------
	void SelectPrimaryWeapon()
	{
		Rpc(RpcDo_SelectPrimaryWeapon);
	}
	
	//------------------------------------------------------------------------------------------------
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

		charController.SelectWeapon(weapon);
	}
}