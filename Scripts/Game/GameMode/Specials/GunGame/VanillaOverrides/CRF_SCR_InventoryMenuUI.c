modded class SCR_InventoryMenuUI
{
	//To disable picking up weapons in Gun Game
	override void OnMenuOpen()
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
		{
			Close();
			return;
		}
		super.OnMenuOpen();
	}
}