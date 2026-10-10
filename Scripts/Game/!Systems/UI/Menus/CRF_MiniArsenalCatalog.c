//------------------------------------------------------------------------------------------------
//! What the mini arsenal offers a player: clothing per slot and weapons per category, read from the
//! faction gear script and the player's role (role overrides replace the defaults for a slot or
//! weapon category, so players can't pick a default item that doesn't fit their role).
//!
//! Shared by the menu (CRF_MiniArsenal, to list options) and the server (the mini arsenal RPCs in
//! CRF_COA_PlayerRplToAuthorityManager, to reject anything that isn't on this list), so what the
//! player sees and what the server accepts can never drift apart.
class CRF_MiniArsenalCatalog
{
	// Category indices: 0-17 are COA_EGearscriptClothing slots (and the character storage slot ids),
	// 18+ are weapon categories
	static const int CATEGORY_RIFLE = 18;
	static const int CATEGORY_RIFLE_UGL = 19;
	static const int CATEGORY_CARBINE = 20;
	static const int CATEGORY_PISTOL = 22;
	static const int CATEGORY_PRIMARY = 23;	// role override primary weapons
	static const int CLOTHING_SLOT_COUNT = 18;

	// Requests arriving faster than this are dropped by the server
	static const float REQUEST_COOLDOWN_MS = 900;

	//------------------------------------------------------------------------------------------------
	static bool IsWeaponCategory(int category)
	{
		return category >= CATEGORY_RIFLE;
	}

	//------------------------------------------------------------------------------------------------
	//! The parsed gear script for a faction (cached by the gearscript manager when it exists)
	static COA_GearScriptConfig GetConfig(FactionKey factionKey)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || factionKey.IsEmpty())
			return null;

		ResourceName resource = gamemode.GetGearScriptResource(factionKey);
		if (resource.IsEmpty())
			return null;

		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (gearscriptManager)
			return gearscriptManager.LoadGearScriptConfig(resource);

		Resource loaded = BaseContainerTools.LoadContainer(resource);
		if (!loaded || !loaded.IsValid())
			return null;

		return COA_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(loaded.GetResource().ToBaseContainer()));
	}

	//------------------------------------------------------------------------------------------------
	static COA_Role_Custom_Gear FindRoleGear(COA_GearScriptConfig config, COA_EGearRole role)
	{
		if (!config || !config.m_RolesToSetCustomSettings)
			return null;

		foreach (COA_Role_Custom_Gear gear : config.m_RolesToSetCustomSettings)
		{
			if (gear && gear.m_Role == role)
				return gear;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Clothing slots that have at least one option, in slot order
	static void GetClothingSlots(COA_GearScriptConfig config, COA_EGearRole role, notnull array<int> outSlots)
	{
		outSlots.Clear();
		array<ResourceName> options = {};
		for (int slot = 0; slot < CLOTHING_SLOT_COUNT; slot++)
		{
			GetClothingOptions(config, role, slot, options);
			if (!options.IsEmpty())
				outSlots.Insert(slot);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Selectable items for one clothing slot. If the role overrides the slot, only the override counts.
	static void GetClothingOptions(COA_GearScriptConfig config, COA_EGearRole role, int slot, notnull array<ResourceName> outItems)
	{
		outItems.Clear();
		if (!config)
			return;

		COA_Role_Custom_Gear roleGear = FindRoleGear(config, role);
		if (roleGear && roleGear.m_Clothing)
		{
			foreach (COA_Clothing clothing : roleGear.m_Clothing)
			{
				if (clothing && clothing.m_iClothingType == slot)
					AddUnique(clothing.m_ClothingPrefabs, outItems);
			}
		}

		if (!outItems.IsEmpty() || !config.m_DefaultClothing)
			return;

		foreach (COA_Clothing clothing : config.m_DefaultClothing)
		{
			if (clothing && clothing.m_iClothingType == slot)
				AddUnique(clothing.m_ClothingPrefabs, outItems);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon categories for the role, each with its weapon list. Role override primaries/pistols
	//! replace the default lists, like clothing overrides do.
	static void GetWeaponCategories(COA_GearScriptConfig config, COA_EGearRole role, notnull array<int> outCategories)
	{
		outCategories.Clear();
		if (!config)
			return;

		COA_Role_Custom_Gear roleGear = FindRoleGear(config, role);
		bool primaryOverride = roleGear && roleGear.m_PrimaryWeapon && HasWeapons(roleGear.m_PrimaryWeapon);
		bool pistolOverride = roleGear && roleGear.m_Pistols && HasWeapons(roleGear.m_Pistols);

		if (primaryOverride)
			outCategories.Insert(CATEGORY_PRIMARY);

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		COA_RoleConfig roleConfig;
		if (rolesConfig)
			roleConfig = rolesConfig.FindRoleConfig(role);

		if (roleConfig && roleConfig.m_aWeapons)
		{
			foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
			{
				int category = -1;
				switch (weaponType)
				{
					case COA_EGearscriptWeapons.RIFLE: category = CATEGORY_RIFLE; break;
					case COA_EGearscriptWeapons.RIFLEUGL: category = CATEGORY_RIFLE_UGL; break;
					case COA_EGearscriptWeapons.CARBINE: category = CATEGORY_CARBINE; break;
					case COA_EGearscriptWeapons.PISTOL: category = CATEGORY_PISTOL; break;
				}

				if (category < 0 || outCategories.Contains(category))
					continue;

				// A role with its own primary weapons doesn't also get the default long guns
				if (primaryOverride && category != CATEGORY_PISTOL)
					continue;

				array<COA_Weapon_Class> weapons = {};
				GetWeapons(config, role, category, weapons);
				if (!weapons.IsEmpty())
					outCategories.Insert(category);
			}
		}

		if (pistolOverride && !outCategories.Contains(CATEGORY_PISTOL))
			outCategories.Insert(CATEGORY_PISTOL);
	}

	//------------------------------------------------------------------------------------------------
	static void GetWeapons(COA_GearScriptConfig config, COA_EGearRole role, int category, notnull array<COA_Weapon_Class> outWeapons)
	{
		outWeapons.Clear();
		if (!config)
			return;

		COA_Role_Custom_Gear roleGear = FindRoleGear(config, role);
		array<ref COA_Weapon_Class> source;
		switch (category)
		{
			case 23:
				if (roleGear)
					source = roleGear.m_PrimaryWeapon;
				break;
			case 18: source = config.m_Rifles; break;
			case 19: source = config.m_RifleUGLs; break;
			case 20: source = config.m_Carbines; break;
			case 22:
				if (roleGear && roleGear.m_Pistols && HasWeapons(roleGear.m_Pistols))
					source = roleGear.m_Pistols;
				else
					source = config.m_Pistols;
				break;
		}

		if (!source)
			return;

		foreach (COA_Weapon_Class weapon : source)
		{
			if (weapon && !weapon.m_Weapon.IsEmpty())
				outWeapons.Insert(weapon);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the gear script entry for a weapon the role may take from the arsenal, or null
	static COA_Weapon_Class FindAllowedWeapon(COA_GearScriptConfig config, COA_EGearRole role, ResourceName weaponResource, out bool isPistol)
	{
		isPistol = false;
		array<int> categories = {};
		GetWeaponCategories(config, role, categories);

		array<COA_Weapon_Class> weapons = {};
		foreach (int category : categories)
		{
			GetWeapons(config, role, category, weapons);
			foreach (COA_Weapon_Class weapon : weapons)
			{
				if (weapon.m_Weapon == weaponResource)
				{
					isPistol = category == CATEGORY_PISTOL;
					return weapon;
				}
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: may this clothing item go into this slot for the role
	static bool IsClothingAllowed(COA_GearScriptConfig config, COA_EGearRole role, int slot, ResourceName item)
	{
		if (slot < 0 || slot >= CLOTHING_SLOT_COUNT || item.IsEmpty())
			return false;

		array<ResourceName> options = {};
		GetClothingOptions(config, role, slot, options);
		return options.Contains(item);
	}

	//------------------------------------------------------------------------------------------------
	static string GetCategoryName(int category)
	{
		switch (category)
		{
			case 0: return "Headgear";
			case 1: return "Shirt";
			case 2: return "Body Armor";
			case 3: return "Pants";
			case 4: return "Boots";
			case 5: return "Backpack";
			case 6: return "Vest";
			case 7: return "Gloves";
			case 8: return "Head";
			case 9: return "Eyewear";
			case 10: return "Ears";
			case 11: return "Face";
			case 12: return "Neck";
			case 13: return "Extra";
			case 14: return "Extra";
			case 15: return "Belt";
			case 16: return "Extra";
			case 17: return "Extra";
			case 18: return "Rifle";
			case 19: return "Rifle UGL";
			case 20: return "Carbine";
			case 22: return "Pistol";
			case 23: return "Primary";
		}

		return "Gear";
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetCategoryIcon(int category)
	{
		switch (category)
		{
			case 0: return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds";
			case 1: return "{92245C15E122EDB2}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Jackets.edds";
			case 2: return "{2DCA69EEB8628C06}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Vests.edds";
			case 3: return "{2CD4D1D2199CE475}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Trousers.edds";
			case 4: return "{AC6095E11A1E4144}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Footware.edds";
			case 5: return "{769B709DF200BF84}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Backpacks.edds";
			case 6: return "{2DCA69EEB8628C06}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Vests.edds";
			case 7: return "{A785C2D354BC7382}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Handwear.edds";
			case 8: return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds";
			case 9: return "{077B2AB761904B92}UI/textures/inventory/glasses.edds";
			case 10: return "{FA22678B4CA2F5F6}UI/textures/inventory/headphones.edds";
			case 11: return "{168CCC01315CE526}UI/textures/inventory/mask.edds";
			case 12: return "{44CA33D6376563EE}UI/textures/inventory/neck.edds";
			case 15: return "{67FBAF5873940829}UI/textures/inventory/belt.edds";
			case 22: return "{2EEBBBCA36DD775F}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Pistols.edds";
			case 18:
			case 19:
			case 20:
			case 23:
				return "{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds";
		}

		return "{140A80B0B3DCF2B5}UI/textures/inventory/extra.edds";
	}

	//------------------------------------------------------------------------------------------------
	//! Display name from a prefab's InventoryItemComponent, without spawning it
	static string GetDisplayName(ResourceName prefab)
	{
		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
			return "";

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loaded);
		if (!entitySource)
			return "";

		for (int i, count = entitySource.GetComponentCount(); i < count; i++)
		{
			IEntityComponentSource componentSource = entitySource.GetComponent(i);
			if (!componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
				continue;

			BaseContainer attributes = componentSource.GetObject("Attributes");
			if (!attributes)
				continue;

			BaseContainer displayName = attributes.GetObject("ItemDisplayName");
			if (!displayName)
				continue;

			string name;
			displayName.Get("Name", name);
			return name;
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddUnique(array<ResourceName> from, notnull array<ResourceName> into)
	{
		if (!from)
			return;

		foreach (ResourceName item : from)
		{
			if (!item.IsEmpty() && !into.Contains(item))
				into.Insert(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasWeapons(array<ref COA_Weapon_Class> weapons)
	{
		foreach (COA_Weapon_Class weapon : weapons)
		{
			if (weapon && !weapon.m_Weapon.IsEmpty())
				return true;
		}

		return false;
	}
}
