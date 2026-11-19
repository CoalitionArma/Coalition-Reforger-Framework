//------------------------------------------------------------------------------------------------
//! Tank Battle: no getting out of a mode tank with the get out / jump out keys
modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override bool CanGetOutVehicleScript()
	{
		if (CRF_TankBattleGamemodeComponent.IsLockedInTank(GetOwner()))
			return false;

		return super.CanGetOutVehicleScript();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanJumpOutVehicleScript()
	{
		if (CRF_TankBattleGamemodeComponent.IsLockedInTank(GetOwner()))
			return false;

		return super.CanJumpOutVehicleScript();
	}
}
