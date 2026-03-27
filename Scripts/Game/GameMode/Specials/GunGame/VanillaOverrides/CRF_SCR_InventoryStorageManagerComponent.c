modded class SCR_InventoryStorageManagerComponent : ScriptedInventoryStorageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//For the GunGame Gamemode to prevent players from picking up weapons
	override void EquipItem(EquipedWeaponStorageComponent weaponStorage, IEntity weapon)
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
			return;
		
		super.EquipItem(weaponStorage, weapon);
	}

	//------------------------------------------------------------------------------------------------
	override void InsertItem( IEntity pItem, BaseInventoryStorageComponent pStorageTo = null, BaseInventoryStorageComponent pStorageFrom = null, SCR_InvCallBack cb = null  )
	{
		if (GetGame().GetGameMode().FindComponent(CRF_GunGame))
			return;
		
		super.InsertItem(pItem, pStorageTo, pStorageFrom, cb);
	}
}