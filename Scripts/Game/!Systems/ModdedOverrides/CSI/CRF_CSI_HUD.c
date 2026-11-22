modded class CSI_HUD
{
	//------------------------------------------------------------------------------------------------
	//! Hides the CSI HUD while spectating and skips its update meanwhile.
	//! Hidden through Show(), not the root widget, so CSI shows it again once spectating ends: CSI only
	//! calls Show() when its shown state (m_bShown) differs from what it wants.
	//! \param[in] owner The entity that owns this component
	//! \param[in] timeSlice Time elapsed since last update
	protected override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		if (SCR_PlayerController.GetLocalMainEntity() && COA_EntityHelper.IsSpectator())
		{
			if (m_bShown)
				Show(false);

			return;
		}

		super.DisplayUpdate(owner, timeSlice);
	}
}
