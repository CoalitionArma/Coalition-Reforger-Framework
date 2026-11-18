//------------------------------------------------------------------------------------------------
//! Tank Battle camera:
//! - the camera switch key does nothing while the local player is in their tank (it stays in third
//!   person; CRF_TankBattleGamemodeComponent also puts it back there every frame)
//! - the driver, who has the main gun attached to their seat (BaseCompartmentSlot.AttachTurret), gets
//!   the turret cameras: third person follows the gun, right click goes to its sight. Vanilla only
//!   counts a seat with its own turret controller as a turret seat (CheckIsInTurret); its turret
//!   cameras already handle an attached turret (CharacterCamera3rdPersonTurret, CharacterCameraADSVehicle).
modded class SCR_CharacterCameraHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnCameraSwitchPressed()
	{
		if (CRF_TankBattleGamemodeComponent.IsThirdPersonLocked(GetOwner()))
		{
			if (!IsInThirdPerson())
				SetThirdPerson(true);

			return;
		}

		super.OnCameraSwitchPressed();
	}

	//------------------------------------------------------------------------------------------------
	override bool CheckIsInTurret(out bool isInAds)
	{
		if (super.CheckIsInTurret(isInAds))
			return true;

		// (vanilla's m_OwnerCharacter is private: a modded class can't read it)
		ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
		if (!character || !CRF_TankBattleGamemodeComponent.IsInTankBattleTank(character))
			return false;

		CompartmentAccessComponent compartmentAccess = character.GetCompartmentAccessComponent();
		if (!compartmentAccess || !compartmentAccess.IsInCompartment() || compartmentAccess.IsGettingOut())
			return false;

		BaseCompartmentSlot compartment = compartmentAccess.GetCompartment();
		if (!compartment || compartment.GetType() != ECompartmentType.PILOT)
			return false;

		TurretControllerComponent turretController = compartment.GetAttachedTurret();
		if (!turretController || !turretController.GetTurretComponent())
			return false;

		isInAds = turretController.IsWeaponADS();
		return true;
	}
}
