class CRF_InventoryHelper
{
	const static ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//------------------------------------------------------------------------------------------------
	//! Check if a prefab is a throwable weapon by inspecting its weapon type
	//! \param[in] entitySource Prefab source
	//! \return True if prefab is a throwable (grenade)
	static bool IsThrowableFromSource(IEntitySource entitySource)
	{
		if (!entitySource)
			return false;
			
		IEntityComponentSource componentSource = SCR_BaseContainerTools.FindComponentSource(entitySource, "WeaponComponent");
		if (!componentSource)
			return false;
			
		int weaponType;
		if (componentSource.Get("WeaponType", weaponType))
		{
			return WEAPON_TYPES_THROWABLE.Contains(weaponType);
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Determine appropriate clothing slots for an item
	//! \param[in] itemSource Item to filter
	//! \param[in] role Role identifier
	//! \return Array of appropriate clothing slot IDs
	static TIntArray FilterItemToClothing(IEntitySource itemSource, CRF_EGearRole role = 0)
	{
		array<int> clothingIDs = {};
		
		// Determine item type
		bool isThrowable = CRF_InventoryHelper.IsThrowableFromSource(itemSource);
		
		bool isGL = !isThrowable && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ShellSoundComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "ShellMoveComponent"));
		
		bool isMagazine = !isGL && !isThrowable && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "MagazineComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "InventoryMagazineComponent"));
		
		bool isMedical = !isThrowable && !isMagazine && 
			CRF_GearscriptManager.GetRolesConfig().FindRoleConfig(role).m_aItems.Contains(CRF_EGearscriptItems.MEDIC_ITEMS) && 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ConsumableItemComponent");
		
		bool isRadio = !isGL && !isThrowable && !isMagazine && !isMedical && 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "BaseRadioComponent");
		
		bool isExplosive = !isGL && !isThrowable && !isMagazine && !isMedical && !isRadio && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_DetonatorGadgetComponent") || 
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_ExplosiveChargeComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_MineWeaponComponent"));
		
		bool isTool = !isGL && !isThrowable && !isMagazine && !isMedical && !isRadio && !isExplosive && 
			(SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_RepairSupportStationComponent") ||
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_HealSupportStationComponent"));
		
		bool isBino = !isGL && !isThrowable && !isMagazine && !isMedical && !isRadio && !isExplosive && !isTool &&
			SCR_BaseContainerTools.FindComponentSource(itemSource, "SCR_BinocularsComponent");
		
		switch (true)
		{
			//--------------------------------------------------------------------------
			// Magazine/Explosives/Medical/Tool go in backpack, vest, armor, primarily
			case (isMagazine || isExplosive || isMedical || isTool) : {
				clothingIDs = {
					CRF_EGearscriptClothing.BACKPACK,
					CRF_EGearscriptClothing.VEST, 
					CRF_EGearscriptClothing.ARMOREDVEST,
					CRF_EGearscriptClothing.PANTS, 
					CRF_EGearscriptClothing.SHIRT
				}; break;
			}
			//--------------------------------------------------------------------------
			// Throwables go in pants, vest primarily
			case isThrowable : {
				clothingIDs = {
					CRF_EGearscriptClothing.PANTS, 
					CRF_EGearscriptClothing.VEST, 
					CRF_EGearscriptClothing.ARMOREDVEST, 
					CRF_EGearscriptClothing.BACKPACK
				}; break;
			}
			//--------------------------------------------------------------------------
			// GLs go in vest, pants, backpack, primarily
			case (isGL) : {
				clothingIDs = {
					CRF_EGearscriptClothing.VEST,
					CRF_EGearscriptClothing.PANTS, 
					CRF_EGearscriptClothing.BACKPACK,
					CRF_EGearscriptClothing.ARMOREDVEST, 
					CRF_EGearscriptClothing.SHIRT
				}; break;
			}
			//--------------------------------------------------------------------------
			// Radios go in pants, shirt, vest primarily
			case (isRadio || isBino) : {
				clothingIDs = {
					CRF_EGearscriptClothing.SHIRT, 
					CRF_EGearscriptClothing.PANTS, 
					CRF_EGearscriptClothing.VEST, 
					CRF_EGearscriptClothing.ARMOREDVEST, 
					CRF_EGearscriptClothing.BACKPACK
				}; break;
			}
			//--------------------------------------------------------------------------
			// Non-sorted items go in shirt, pants, vest primarily
			default : {
				clothingIDs = {
					CRF_EGearscriptClothing.SHIRT, 
					CRF_EGearscriptClothing.PANTS, 
					CRF_EGearscriptClothing.VEST, 
					CRF_EGearscriptClothing.ARMOREDVEST, 
					CRF_EGearscriptClothing.BACKPACK
				};
			}
		}

		return clothingIDs;
	}
}