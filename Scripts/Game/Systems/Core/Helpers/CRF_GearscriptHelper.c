class CRF_GearscriptHelper
{
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Check if an item is explosive or a special tool
	 * @param item Item to check
	 * @return True if item is explosive or tool
	 */
	static bool IsExplosiveOrTool(IEntity item)
	{
		return SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent)) || 
			   SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent)) ||
			   SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent)) ||
			   SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent)) ||
			   SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
	}
}