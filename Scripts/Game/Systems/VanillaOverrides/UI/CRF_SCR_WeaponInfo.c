modded class SCR_WeaponInfo
{
    override protected void DisplayOnResumed()
    {
        super.DisplayOnResumed();
        
        Show(m_WeaponState && m_WeaponState.m_Weapon, UIConstants.FADE_RATE_SLOW); // Pausing hides this display thus we should show it again when it is resumed
    }
}