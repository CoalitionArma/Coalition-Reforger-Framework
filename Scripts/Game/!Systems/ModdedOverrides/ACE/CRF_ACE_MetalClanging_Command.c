modded class ACE_MetalClangingCommand
{
	//------------------------------------------------------------------------------------------------
	//! CRF doesn't want the "bell ringing" metal-clanging gesture available to players - disabled for
	//! everyone by always failing the availability check, so it never shows up in the radial menu.
	override bool CanBePerformed(notnull SCR_ChimeraCharacter user)
	{
		return false;
	}
}
