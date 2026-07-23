#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Validate Gearscript Resources", description: "Loads every resource referenced by the selected CRF_GearScriptConfig (.conf) - weapons, attachments, magazines, clothing, inventory items, identity, and icons - and reports any that fail to load", shortcut: "", wbModules: { "ResourceManager", "ScriptEditor" })]
/*
	Select one or more CRF_GearScriptConfig .conf files (e.g. Configs/GearScripts/Standard/*) in
	the Resource Browser, then run "Validate Gearscript Resources" from the Workbench Plugins
	menu/panel - same place you already run "Gear Script Config Generator" from.

	Walks every ResourceName referenced by the config - recursing into weapons/attachments/
	magazines, clothing, inventory items, faction identity (and its head/body resources), and
	per-role custom gear - and attempts to load each one the same way CRF_GearscriptManager does
	at runtime, so a broken/renamed/deleted resource is caught here instead of in a live mission.

	Results are printed to the Workbench console.
*/
class CRF_ValidateGearScriptPlugin : ResourceManagerPlugin
{
	protected int m_iCheckedCount;
	protected ref array<string> m_aFailures = {};

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!resourceManager)
			return;

		array<ResourceName> selection = {};
		resourceManager.GetResourceBrowserSelection(selection.Insert, true);

		if (selection.IsEmpty())
		{
			Print("[CRF Gearscript Validator] No resource selected. Select a CRF_GearScriptConfig .conf file in the Resource Browser first.", LogLevel.WARNING);
			return;
		}

		int validatedCount;
		foreach (ResourceName resourceName : selection)
		{
			if (!resourceName.EndsWith(".conf"))
				continue;

			validatedCount++;
			ValidateGearScript(resourceName);
		}

		if (validatedCount == 0)
			Print("[CRF Gearscript Validator] No .conf files found in the selection.", LogLevel.WARNING);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 VALIDATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void ValidateGearScript(ResourceName resourceName)
	{
		m_iCheckedCount = 0;
		m_aFailures.Clear();

		Resource containerResource = BaseContainerTools.LoadContainer(resourceName);
		if (!containerResource)
		{
			Print(string.Format("[CRF Gearscript Validator] Could not load '%1' as a container resource.", resourceName), LogLevel.ERROR);
			return;
		}

		CRF_GearScriptConfig config = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(containerResource.GetResource().ToBaseContainer()));
		if (!config)
		{
			Print(string.Format("[CRF Gearscript Validator] Skipping '%1' - not a CRF_GearScriptConfig.", resourceName), LogLevel.WARNING);
			return;
		}

		// Faction-level resources
		CheckResource(config.m_FactionIcon, "Faction Icon");
		CheckResource(config.m_sLeadershipBinocularsPrefab, "Leadership Binoculars");
		CheckResource(config.m_sAssistantBinocularsPrefab, "Assistant Binoculars");
		CheckIdentity(config.m_FactionIdentity);

		// Weapons
		CheckWeaponArray(config.m_Rifles, "Rifles");
		CheckWeaponArray(config.m_RifleUGLs, "Rifle UGLs");
		CheckWeaponArray(config.m_Carbines, "Carbines");
		CheckWeaponArray(config.m_Pistols, "Pistols");
		CheckSpecWeapon(config.m_AR, "AR");
		CheckSpecWeapon(config.m_MMG, "MMG");
		CheckSpecWeapon(config.m_HMG, "HMG");
		CheckSpecWeapon(config.m_AT, "AT");
		CheckSpecWeapon(config.m_MAT, "MAT");
		CheckSpecWeapon(config.m_HAT, "HAT");
		CheckSpecWeapon(config.m_AA, "AA");
		CheckWeapon(config.m_SNIPER, "Sniper");

		// Clothing
		CheckClothingArray(config.m_DefaultClothing, "Default Clothing");

		// Inventory
		CheckInventoryArray(config.m_DefaultInventoryItems, "Default Inventory Items");
		CheckInventoryArray(config.m_InfantryMedicalItems, "Infantry Medical Items");
		CheckInventoryArray(config.m_MedicMedicalItems, "Medic Medical Items");

		// Per-role custom gear
		if (config.m_RolesToSetCustomSettings)
		{
			foreach (CRF_Role_Custom_Gear roleGear : config.m_RolesToSetCustomSettings)
			{
				if (!roleGear)
					continue;

				string roleLabel = string.Format("Role '%1'", roleGear.m_sRoleName);
				CheckWeaponArray(roleGear.m_PrimaryWeapon, roleLabel + " Primary Weapon");
				CheckWeaponArray(roleGear.m_SecondaryWeapon, roleLabel + " Secondary Weapon");
				CheckWeaponArray(roleGear.m_Pistols, roleLabel + " Pistol");
				CheckClothingArray(roleGear.m_Clothing, roleLabel + " Clothing");
				CheckInventoryArray(roleGear.m_AdditionalInventoryItems, roleLabel + " Additional Inventory Item");
			}
		}

		PrintResults(resourceName);
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the faction identity config and checks the head/body resources of every visual identity
	protected void CheckIdentity(ResourceName identityResource)
	{
		if (!CheckResource(identityResource, "Faction Identity"))
			return;

		Resource containerResource = BaseContainerTools.LoadContainer(identityResource);
		if (!containerResource)
			return;

		CRF_CharacterIdentity identity = CRF_CharacterIdentity.Cast(BaseContainerTools.CreateInstanceFromContainer(containerResource.GetResource().ToBaseContainer()));
		if (!identity || !identity.m_VisualIdentityArray)
			return;

		foreach (CRF_Character_Visual_Identity visual : identity.m_VisualIdentityArray)
		{
			if (!visual)
				continue;

			CheckResource(visual.m_Head, "Identity Head");
			CheckResource(visual.m_Body, "Identity Body");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckWeaponArray(array<ref CRF_Weapon_Class> weapons, string label)
	{
		if (!weapons)
			return;

		foreach (CRF_Weapon_Class weapon : weapons)
			CheckWeapon(weapon, label);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckWeapon(CRF_Weapon_Class weapon, string label)
	{
		if (!weapon)
			return;

		CheckResource(weapon.m_Weapon, label + " Weapon");

		if (weapon.m_Attachments)
			foreach (ResourceName attachment : weapon.m_Attachments)
				CheckResource(attachment, label + " Attachment");

		if (weapon.m_MagazineArray)
			foreach (CRF_Magazine_Class magazine : weapon.m_MagazineArray)
				if (magazine)
					CheckResource(magazine.m_Magazine, label + " Magazine");
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckSpecWeapon(CRF_Spec_Weapon_Class weapon, string label)
	{
		if (!weapon)
			return;

		CheckResource(weapon.m_Weapon, label + " Weapon");

		if (weapon.m_Attachments)
			foreach (ResourceName attachment : weapon.m_Attachments)
				CheckResource(attachment, label + " Attachment");

		if (weapon.m_MagazineArray)
			foreach (CRF_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
				if (magazine)
					CheckResource(magazine.m_Magazine, label + " Magazine");
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckClothingArray(array<ref CRF_Clothing> clothingArray, string label)
	{
		if (!clothingArray)
			return;

		foreach (CRF_Clothing clothing : clothingArray)
		{
			if (!clothing || !clothing.m_ClothingPrefabs)
				continue;

			string slotLabel = string.Format("%1 (%2)", label, typename.EnumToString(CRF_EGearscriptClothing, clothing.m_iClothingType));
			foreach (ResourceName prefab : clothing.m_ClothingPrefabs)
				CheckResource(prefab, slotLabel);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckInventoryArray(array<ref CRF_Inventory_Item> items, string label)
	{
		if (!items)
			return;

		foreach (CRF_Inventory_Item item : items)
		{
			if (!item)
				continue;

			CheckResource(item.m_sItemPrefab, label);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Loads a single resource and records a failure if it doesn't resolve to a usable asset.
	//! \return false if the resource is empty or failed to load, true if it loaded successfully
	protected bool CheckResource(ResourceName resourceName, string label)
	{
		if (resourceName.IsEmpty())
			return false;

		m_iCheckedCount++;

		Resource res = Resource.Load(resourceName);
		if (!res || !res.IsValid())
		{
			m_aFailures.Insert(string.Format("%1: '%2' failed to load", label, resourceName));
			return false;
		}

		// Entity prefabs need to resolve to an actual entity source - the same check
		// CRF_GearscriptManager.AddInventoryItem performs at runtime before spawning gear.
		if (resourceName.EndsWith(".et") && !SCR_BaseContainerTools.FindEntitySource(res))
		{
			m_aFailures.Insert(string.Format("%1: '%2' loaded but has no entity source (won't spawn)", label, resourceName));
			return false;
		}

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 OUTPUT
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void PrintResults(ResourceName resourceName)
	{
		Print("", LogLevel.NORMAL);
		Print("================================================================", LogLevel.NORMAL);
		Print(string.Format("  CRF GEARSCRIPT VALIDATOR - %1", resourceName), LogLevel.NORMAL);
		Print("================================================================", LogLevel.NORMAL);

		if (m_aFailures.IsEmpty())
		{
			Print(string.Format("[OK] All %1 referenced resources loaded successfully.", m_iCheckedCount), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("[X] %1 of %2 referenced resources failed to load:", m_aFailures.Count(), m_iCheckedCount), LogLevel.WARNING);
			foreach (string failure : m_aFailures)
				Print("  [X] " + failure, LogLevel.WARNING);
		}

		Print("================================================================", LogLevel.NORMAL);
		Print("", LogLevel.NORMAL);
	}
}
#endif
