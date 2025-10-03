/*
* Damage utility class for COALITION games
* Helper functions for working with damage events and weapon tracking
*/

class CRF_DamageUtility
{
	//------------------------------------------------------------------------------------------------
	// Convert damage type to human readable string
	static string GetDamageTypeString(int damageType)
	{
		switch (damageType)
		{
			case EDamageType.TRUE: return "Direct";
			case EDamageType.COLLISION: return "Collision";
			case EDamageType.MELEE: return "Melee";
			case EDamageType.KINETIC: return "Gunshot";
			case EDamageType.FRAGMENTATION: return "Fragmentation";
			case EDamageType.EXPLOSIVE: return "Explosion";
			case EDamageType.INCENDIARY: return "Incendiary";
			case EDamageType.FIRE: return "Fire";
			case EDamageType.REGENERATION: return "Regeneration";
			case EDamageType.BLEEDING: return "Bleeding";
			case EDamageType.HEALING: return "Healing";
			case EDamageType.PROCESSED_FRAGMENTATION: return "Fragmentation";
			default: return "Unknown";
		}
		
		// This should never be reached due to default case above, but added to satisfy compiler
		return "Unknown";
	}
	
	//------------------------------------------------------------------------------------------------
	// Try to get a descriptive name for the weapon that caused the damage
	static string GetWeaponName(SCR_InstigatorContextData instiContext)
	{
		// Check for direct weapon reference
		string weaponName = "Unknown Weapon";
		
		// Try to get direct weapon entity
		IEntity killerEntity = instiContext.GetKillerEntity();
		if (killerEntity)
		{
			// Try to get weapon component first
			BaseWeaponComponent weaponComp = BaseWeaponComponent.Cast(killerEntity.FindComponent(BaseWeaponComponent));
			if (weaponComp)
			{
				return weaponComp.GetUIInfo().GetName();
			}
			
			// Try inventory to get current weapon
			SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(killerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (inventory)
			{
				BaseWeaponComponent currentWeapon = inventory.GetCurrentCharacterWeapon();
				if (currentWeapon)
					return currentWeapon.GetUIInfo().GetName();
			}
			
			// If not a weapon component, try to extract useful info from the entity name
			weaponName = killerEntity.GetPrefabData().GetPrefabName();
			
			// Look for specific keywords to make the name more descriptive
			string lowerName = weaponName;
			lowerName.ToLower(); // Use method instead of direct assignment
			if (lowerName.Contains("grenade") || lowerName.Contains("explosive"))
			{
				return "Explosive: " + weaponName;
			}
			else if (lowerName.Contains("mine"))
			{
				return "Mine: " + weaponName;
			}
			
			return weaponName;
		}
		
		// Since we can't get damage type directly, we'll have to infer it from other context
		// For example, we could check the killer's equipment or the environment
		
		// If we've reached this point, we can only provide a generic name based on the killer
		if (killerEntity)
		{
			string entityName = killerEntity.GetName();
			string prefabName = killerEntity.GetPrefabData().GetPrefabName();
			
			// Check for common patterns
			if (prefabName.Contains("Grenade") || prefabName.Contains("Explosive"))
				return "Explosive Device";
				
			if (prefabName.Contains("Fire") || prefabName.Contains("Flame"))
				return "Fire Source";
				
			if (entityName.Contains("Vehicle"))
				return "Vehicle";
		}
		
		return weaponName;
	}
	
	// These utility methods have been simplified and moved directly to the LoggingManager class
}
