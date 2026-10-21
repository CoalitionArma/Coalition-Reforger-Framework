//------------------------------------------------------------------------------------------------
//! Hides the magazine icon on the infantry weapon HUD so players can't read how full the magazine is
//! - they check it with Ctrl+R (CRF_MagazineCheckComponent). Hiding it in the layout override
//! (UI/layouts/HUD/WeaponInfo/WeaponInfo.layout) isn't enough: vanilla turns m_MagazineIndicator back
//! on when the HUD is drawn and whenever the magazine changes. The spare magazine count beside it
//! (m_MagCount) stays. Vehicle weapon HUDs (SCR_WeaponInfoVehicle) keep their indicator, since
//! turret ammo can't be checked by hand.
modded class SCR_WeaponInfo
{
	//------------------------------------------------------------------------------------------------
	override protected void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);
		CRF_HideMagazineIndicator();
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateMagazineIndicator_Textures(SCR_WeaponState state)
	{
		super.UpdateMagazineIndicator_Textures(state);
		CRF_HideMagazineIndicator();
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_HideMagazineIndicator()
	{
		if (SCR_WeaponInfoVehicle.Cast(this))
			return;

		if (m_Widgets && m_Widgets.m_wMagazineIndicator)
			m_Widgets.m_wMagazineIndicator.SetVisible(false);
	}
}
