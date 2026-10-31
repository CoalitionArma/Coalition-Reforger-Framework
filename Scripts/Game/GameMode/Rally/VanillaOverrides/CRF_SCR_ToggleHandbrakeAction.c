//------------------------------------------------------------------------------------------------
//! Rally stage: a car held at the start (CRF_RallyStageGamemodeComponent) can't have its parking brake
//! let off. The hold re-applies it anyway; this stops the action working at all, on every machine,
//! instead of the car lurching for a tick before the hold catches it.
modded class SCR_ToggleHandbrakeAction
{
	protected const string HELD_AT_START = "Held until your start time";

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!super.CanBePerformedScript(user))
			return false;

		if (CRF_RallyStageGamemodeComponent.IsCarHeld(GetOwner().GetRootParent()))
		{
			SetCannotPerformReason(HELD_AT_START);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Also refused here: the action runs where it is performed and on the server, and the held list
	//! may have reached one of them a moment before the other
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (CRF_RallyStageGamemodeComponent.IsCarHeld(GetOwner().GetRootParent()))
			return;

		super.PerformAction(pOwnerEntity, pUserEntity);
	}
}
