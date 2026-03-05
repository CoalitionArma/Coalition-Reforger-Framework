modded class SCR_EquipWeaponHolsterAction
{
	//To disable picking up weapons in Gun Game
	override bool CanBeShownScript(IEntity user)
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
			return false;
		
		return super.CanBeShownScript(user);
	}
}