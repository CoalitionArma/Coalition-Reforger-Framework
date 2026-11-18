//------------------------------------------------------------------------------------------------
// Tank Battle: no getting out of a mode tank, or into another of its seats, from the action menu
//------------------------------------------------------------------------------------------------
modded class SCR_GetOutAction : SCR_CompartmentUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (CRF_TankBattleGamemodeComponent.IsLockedInTank(user))
			return false;

		return super.CanBePerformedScript(user);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_JumpOutAction : SCR_CompartmentUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (CRF_TankBattleGamemodeComponent.IsLockedInTank(user))
			return false;

		return super.CanBePerformedScript(user);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_SwitchSeatAction : SCR_GetInUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (CRF_TankBattleGamemodeComponent.IsLockedInTank(user))
			return false;

		return super.CanBePerformedScript(user);
	}
}
