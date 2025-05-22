class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected ref RandomGenerator m_RNG = new RandomGenerator();
	
	ref CRF_GearScriptEquipmentConfig m_EquipmentConfig;
	ref CRF_GearScriptWeaponsConfig m_WeaponConfig;
	protected CRF_Gamemode m_Gamemode;

	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get singleton instance of the GearscriptManager
	 * @return Current instance or null if not found
	 */
	static CRF_GearscriptManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		
		return CRF_GearscriptManager.Cast(gameMode.FindComponent(CRF_GearscriptManager));
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on in-game post init
		if (!GetGame().InPlayMode())
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		LoadConfigurations();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load necessary configurations for gearscript
	 */
	protected void LoadConfigurations()
	{
		ResourceName weaponConfigPath = "{AF5B2639B4B12580}Configs/Gearscripts/CRF_Global_Weapons_Config.conf";
		ResourceName equipmentConfigPath = "{DE26DF4B9B934889}Configs/Gearscripts/CRF_Global_Equipment_Config.conf";
		
		m_WeaponConfig = CRF_GearScriptWeaponsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(weaponConfigPath).GetResource().ToBaseContainer()));
		
		m_EquipmentConfig = CRF_GearScriptEquipmentConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(equipmentConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get gearscript resource for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return ResourceName for the gearscript or empty string if not found
	 */
	ResourceName GetGearScriptResource(string factionKey)
	{
		CRF_GearScriptContainer container = GetGearScriptSettings(factionKey);
		if (!container)
		{
			PrintFormat("NO GEARSCRIPT ASSIGNED TO: %1", factionKey, LogLevel.WARNING);
			return "";
		}

		return container.m_rGearScript;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get gearscript container for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return The gearscript container or null if not found
	 */
	CRF_GearScriptContainer GetGearScriptSettings(string factionKey)
	{
		if (!m_Gamemode)
			return null;
			
		CRF_GearScriptContainer gearScriptContainer = null;

		if (factionKey == "BLUFOR")
			gearScriptContainer = m_Gamemode.m_BLUFORGearScriptSettings;
		else if (factionKey == "OPFOR")
			gearScriptContainer = m_Gamemode.m_OPFORGearScriptSettings;
		else if (factionKey == "INDFOR")
			gearScriptContainer = m_Gamemode.m_INDFORGearScriptSettings;
		else if (factionKey == "CIV")
			gearScriptContainer = m_Gamemode.m_CIVILIANGearScriptSettings;

		return gearScriptContainer;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Set gear for an entity based on its resource name
	 * @param entity The entity to equip
	 * @param resourceNameToScan Resource name containing faction info
	 */
	void SetEntityGear(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!CRF_RoleHelper.IsValidGearscriptResource(resourceNameToScan) || !entity)
			return;

		// Determine faction from resource name
		string factionKey = DetermineFactionKey(resourceNameToScan);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = GetGearScriptResource(factionKey);
		CRF_GearScriptContainer gearScriptSettings = GetGearScriptSettings(factionKey);

		if (gearScriptResourceName.IsEmpty() || !gearScriptSettings)
			return;

		// Get required components
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!ValidateComponents(entity, inventory, inventoryManager))
			return;

		// Get role and clear entity
		int role = CRF_RoleHelper.StringToRole(CRF_RoleHelper.PrefabToRole(resourceNameToScan));
		ClearEntityGear(inventory, inventoryManager);

		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		if (!gearConfig)
			return;
		
		// Prepare spawn parameters
		EntitySpawnParams spawnParams = CreateSpawnParams(entity);
		
		// Apply gear
		ApplyClothing(gearConfig, role, spawnParams, inventory, inventoryManager);
		ApplyWeapons(gearConfig, role, factionKey, gearScriptSettings, spawnParams, inventory, inventoryManager);
		ApplyInventoryItems(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		CRF_PlayableCharacter playable = CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter));
		
		if (playable)
			playable.SetGearscriptCompleted();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determine faction key from resource name
	 * @param resourceName Resource name to analyze
	 * @return Faction key or empty string if not found
	 */
	protected string DetermineFactionKey(ResourceName resourceName)
	{
		if (resourceName.Contains("BLUFOR"))
			return "BLUFOR";
		else if (resourceName.Contains("OPFOR"))
			return "OPFOR";
		else if (resourceName.Contains("INDFOR"))
			return "INDFOR";
		else if (resourceName.Contains("CIV"))
			return "CIV";
			
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Validate that entity has all required components
	 * @param entity Entity to validate
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @return True if all components are present
	 */
	protected bool ValidateComponents(IEntity entity, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!inventory || !inventoryManager)
		{
			Print(string.Format("CRF GEAR SCRIPT ERROR: %1 DOESN'T HAVE REQUIRED COMPONENTS!", entity), LogLevel.ERROR);
			return false;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Clear all gear from an entity
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ClearEntityGear(SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		array<IEntity> items = {};
		array<IEntity> itemsRoot = {};
		inventoryManager.GetAllItems(items, inventory);
		inventoryManager.GetAllRootItems(itemsRoot);

		items.InsertAll(itemsRoot);

		foreach (IEntity item : items)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load gear script config from resource
	 * @param resourceName Resource to load
	 * @return Loaded config or null if failed
	 */
	protected CRF_GearScriptConfig LoadGearScriptConfig(ResourceName resourceName)
	{
		return CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Create spawn parameters for entity
	 * @param entity Entity to create parameters for
	 * @return Spawn parameters
	 */
	protected EntitySpawnParams CreateSpawnParams(IEntity entity)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		return spawnParams;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply clothing to entity based on config
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyClothing(CRF_GearScriptConfig gearConfig, int role, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Apply default faction clothing
		if (gearConfig.m_DefaultFactionGear)
		{
			foreach (CRF_Clothing clothing : gearConfig.m_DefaultFactionGear.m_DefaultClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, false, spawnParams, inventory, inventoryManager);
			}
		}
		
		// Apply custom clothing if available
		if (gearConfig.m_CustomFactionGear)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
			{
				if (customGear.m_Role != role)
					continue;
		
				foreach (CRF_Clothing clothing : customGear.m_Clothing)
				{
					UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, true, spawnParams, inventory, inventoryManager);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply weapons to entity based on config
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param factionKey Faction key
	 * @param gearScriptSettings Gearscript settings
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyWeapons(CRF_GearScriptConfig gearConfig, int role, string factionKey, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		bool customWeaponsSet = ApplyCustomWeapons(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Apply default weapons if no custom weapons were set
		if (!customWeaponsSet && gearConfig.m_FactionWeapons)
		{
			ApplyDefaultWeapons(gearConfig, role, spawnParams, inventory, inventoryManager);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply custom weapons based on role
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @return True if custom weapons were applied
	 */
	protected bool ApplyCustomWeapons(CRF_GearScriptConfig gearConfig, int role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig.m_CustomFactionGear)
			return false;
			
		bool customWeaponsSet = false;
		
		foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
		{
			if (customGear.m_Role != role)
				continue;
			
			// Primary weapon
			if (!customGear.m_PrimaryWeapon.IsEmpty())
			{
				CRF_Weapon_Class primary = SelectRandomWeapon(customGear.m_PrimaryWeapon);
				SpawnWeapon(primary.m_Weapon, primary.m_Attachments, primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
				customWeaponsSet = true;
			}
			
			// Secondary weapon
			if (!customGear.m_SecondaryWeapon.IsEmpty())
			{
				CRF_Weapon_Class secondary = SelectRandomWeapon(customGear.m_SecondaryWeapon);
				SpawnWeapon(secondary.m_Weapon, secondary.m_Attachments, secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
				customWeaponsSet = true;
			}
			
			// Pistol
			if (!customGear.m_Pistol.IsEmpty())
			{
				CRF_Weapon_Class pistol = SelectRandomWeapon(customGear.m_Pistol);
				SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
				customWeaponsSet = true;
			}
		}
		
		return customWeaponsSet;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply default weapons based on role
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyDefaultWeapons(CRF_GearScriptConfig gearConfig, int role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig.m_FactionWeapons)
			return;
		
		// Rifle
		if (m_WeaponConfig.m_aRolesThatGetRifles.Contains(role))
		{
			CRF_Weapon_Class rifle = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Rifle);
			SpawnWeapon(rifle.m_Weapon, rifle.m_Attachments, rifle.m_MagazineArray, spawnParams, inventory, inventoryManager);
		}
		
		// Rifle with UGL
		if (m_WeaponConfig.m_aRolesThatGetRifleUGLs.Contains(role))
		{
			CRF_Weapon_Class rifleUGL = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_RifleUGL);
			SpawnWeapon(rifleUGL.m_Weapon, rifleUGL.m_Attachments, rifleUGL.m_MagazineArray, spawnParams, inventory, inventoryManager);
		}
		
		// Carbine
		if (m_WeaponConfig.m_aRolesThatGetCarbines.Contains(role))
		{
			CRF_Weapon_Class carbine = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Carbine);
			SpawnWeapon(carbine.m_Weapon, carbine.m_Attachments, carbine.m_MagazineArray, spawnParams, inventory, inventoryManager);
		}
		
		// Pistol
		if (m_WeaponConfig.m_aRolesThatGetPistols.Contains(role))
		{
			CRF_Weapon_Class pistol = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Pistol);
			SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
		}
		
		// Sniper rifle
		if (m_WeaponConfig.m_aRolesThatGetSnipers.Contains(role) && gearConfig.m_FactionWeapons.m_Sniper)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_Sniper.m_Weapon, 
						gearConfig.m_FactionWeapons.m_Sniper.m_Attachments, 
						gearConfig.m_FactionWeapons.m_Sniper.m_MagazineArray, 
						spawnParams, inventory, inventoryManager);
		}
		
		// Automatic rifle
		if (m_WeaponConfig.m_aRolesThatGetARs.Contains(role) && gearConfig.m_FactionWeapons.m_AR)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_AR.m_Weapon, 
						gearConfig.m_FactionWeapons.m_AR.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AR.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Medium machinegun
		if (m_WeaponConfig.m_aRolesThatGetMMGs.Contains(role) && gearConfig.m_FactionWeapons.m_MMG)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_MMG.m_Weapon, 
						gearConfig.m_FactionWeapons.m_MMG.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Anti-tank
		if (m_WeaponConfig.m_aRolesThatGetAT.Contains(role) && gearConfig.m_FactionWeapons.m_AT)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_AT.m_Weapon, 
						gearConfig.m_FactionWeapons.m_AT.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AT.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Medium anti-tank
		if (m_WeaponConfig.m_aRolesThatGetMAT.Contains(role) && gearConfig.m_FactionWeapons.m_MAT)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_MAT.m_Weapon, 
						gearConfig.m_FactionWeapons.m_MAT.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Heavy anti-tank
		if (m_WeaponConfig.m_aRolesThatGetHAT.Contains(role) && gearConfig.m_FactionWeapons.m_HAT)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_HAT.m_Weapon, 
						gearConfig.m_FactionWeapons.m_HAT.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Anti-air
		if (m_WeaponConfig.m_aRolesThatGetAA.Contains(role) && gearConfig.m_FactionWeapons.m_AA)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_AA.m_Weapon, 
						gearConfig.m_FactionWeapons.m_AA.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AA.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
		
		// Heavy machinegun
		if (m_WeaponConfig.m_aRolesThatGetHMGs.Contains(role) && gearConfig.m_FactionWeapons.m_HMG)
		{
			SpawnWeapon(gearConfig.m_FactionWeapons.m_HMG.m_Weapon, 
						gearConfig.m_FactionWeapons.m_HMG.m_Attachments, 
						ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray), 
						spawnParams, inventory, inventoryManager);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply inventory items based on role and config
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param gearScriptSettings Gearscript settings
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyInventoryItems(CRF_GearScriptConfig gearConfig, int role, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Apply custom gear first
		if (gearConfig.m_CustomFactionGear)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
			{
				if (customGear.m_Role != role)
					continue;
		
				foreach (CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				{
					AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
				}
			}
		}
		
		// Then apply default gear
		if (gearConfig.m_DefaultFactionGear)
		{
			ApplyDefaultInventoryItems(gearConfig, gearScriptSettings, role, spawnParams, inventory, inventoryManager);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply default inventory items
	 * @param gearConfig Gear configuration
	 * @param gearScriptSettings Gearscript settings
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyDefaultInventoryItems(CRF_GearScriptConfig gearConfig, CRF_GearScriptContainer gearScriptSettings, int role,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Leadership radios
		if (gearScriptSettings.m_bEnableLeadershipRadios && m_EquipmentConfig.m_aRolesThatGetLeadershipRadios.Contains(role))
		{
			AddInventoryItem(gearScriptSettings.m_rLeadershipRadiosPrefab, 1, spawnParams, inventory, inventoryManager);
		}

		// GI radios
		if (gearScriptSettings.m_bEnableGIRadios && !m_EquipmentConfig.m_aRolesThatGetLeadershipRadios.Contains(role))
		{
			AddInventoryItem(gearScriptSettings.m_rGIRadiosPrefab, 1, spawnParams, inventory, inventoryManager);
		}

		// RTO radios
		if (gearScriptSettings.m_bEnableRTORadios && m_EquipmentConfig.m_aRolesThatGetRTORadios.Contains(role))
		{
			AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);
		}

		// Leadership binoculars
		if (gearConfig.m_DefaultFactionGear.m_bEnableLeadershipBinoculars && m_EquipmentConfig.m_aRolesThatGetLeadershipBinos.Contains(role))
		{
			AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
		}
		
		// Assistant binoculars
		if (gearConfig.m_DefaultFactionGear.m_bEnableAssistantBinoculars && m_EquipmentConfig.m_aRolesThatGetAssistantBinos.Contains(role))
		{
			AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager, role);
		}

		// Medical items
		if (m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role))
		{
			foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultMedicMedicalItems)
			{
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
			}
		}
		
		// Extra magazines for assistants
		if (m_EquipmentConfig.m_aRolesThatGetAssistantMags.Contains(role))
		{
			AddAssistantMagazines(gearConfig, role, spawnParams, inventory, inventoryManager);
		}

		// Default inventory items
		foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultInventoryItems)
		{
			AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role, gearConfig.m_DefaultFactionGear.m_bEnableMedicFrags);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add assistant magazines based on role
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void AddAssistantMagazines(CRF_GearScriptConfig gearConfig, int role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		array<ref CRF_Spec_Magazine_Class> magazineArray = GetAssistantMagazinesForRole(gearConfig, role);
		if (magazineArray.IsEmpty())
			return;
			
		foreach (ref CRF_Spec_Magazine_Class magazine : magazineArray)
		{
			AddInventoryItem(magazine.m_Magazine, magazine.m_AssistantMagazineCount, spawnParams, inventory, inventoryManager, role, true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get appropriate magazines for assistant roles
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @return Array of magazines appropriate for the role
	 */
	protected array<ref CRF_Spec_Magazine_Class> GetAssistantMagazinesForRole(CRF_GearScriptConfig gearConfig, int role)
	{
		array<ref CRF_Spec_Magazine_Class> magazineArray = {};
		
		if (!gearConfig.m_FactionWeapons)
			return magazineArray;
			
		switch (role)
		{
			case CRF_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN:
				if (gearConfig.m_FactionWeapons.m_AR && gearConfig.m_FactionWeapons.m_AR.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_AR.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_MEDIUM_MACHINEGUN:
				if (gearConfig.m_FactionWeapons.m_MMG && gearConfig.m_FactionWeapons.m_MMG.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_HEAVY_MACHINEGUN:
				if (gearConfig.m_FactionWeapons.m_HMG && gearConfig.m_FactionWeapons.m_HMG.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_MEDIUM_ANTITANK:
				if (gearConfig.m_FactionWeapons.m_MAT && gearConfig.m_FactionWeapons.m_MAT.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_HEAVY_ANTITANK:
				if (gearConfig.m_FactionWeapons.m_HAT && gearConfig.m_FactionWeapons.m_HAT.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_ANTI_AIR:
				if (gearConfig.m_FactionWeapons.m_AA && gearConfig.m_FactionWeapons.m_AA.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_AA.m_MagazineArray;
				break;
				
			case CRF_EGearRole.ASSISTANT_RIFLEMAN_ANTITANK:
				if (gearConfig.m_FactionWeapons.m_AT && gearConfig.m_FactionWeapons.m_AT.m_Weapon)
					magazineArray = gearConfig.m_FactionWeapons.m_AT.m_MagazineArray;
				break;
		}
		
		return magazineArray;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Select a random weapon from an array
	 * @param weaponArray Array of weapon options
	 * @return Randomly selected weapon or null if array is empty
	 */
	protected CRF_Weapon_Class SelectRandomWeapon(array<ref CRF_Weapon_Class> weaponArray)
	{
		if (!weaponArray || weaponArray.IsEmpty())
			return null;

		return weaponArray.GetRandomElement();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Convert specialized magazine array to standard magazine array
	 * @param specMagazineArray Array of specialized magazines
	 * @return Converted magazine array
	 */
	array<ref CRF_Magazine_Class> ConvertSpecMagArrayIntoMagArray(array<ref CRF_Spec_Magazine_Class> specMagazineArray)
	{
		array<ref CRF_Magazine_Class> tempArray = {};
		
		if (!specMagazineArray)
			return tempArray;
			
		foreach (CRF_Spec_Magazine_Class specMagazine : specMagazineArray)
		{
			if (!specMagazine)
				continue;
				
			ref CRF_Magazine_Class tempMag = new CRF_Magazine_Class();
			tempMag.m_Magazine = specMagazine.m_Magazine;
			tempMag.m_MagazineCount = specMagazine.m_MagazineCount;
			
			tempArray.Insert(tempMag);
		}
		
		return tempArray;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawn a weapon and its attachments/magazines
	 * @param weaponResource Weapon resource to spawn
	 * @param attachmentResources Attachments to add
	 * @param magazineArray Magazines to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void SpawnWeapon(ResourceName weaponResource, array<ResourceName> attachmentResources, array<ref CRF_Magazine_Class> magazineArray, 
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(weaponResource.IsEmpty())
			return;
		
		bool successfulSpawn = inventoryManager.TrySpawnPrefabToStorage(weaponResource, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);

		if (!successfulSpawn)
		{
			LogWeaponError(weaponResource, inventoryManager.GetOwner());
			return;
		}

		// Add magazines
		if (magazineArray != null)
		{
			foreach (CRF_Magazine_Class magazine : magazineArray)
			{
				if (magazine != null)
				{
					AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount, spawnParams, inventory, inventoryManager);
				}
			}
		}
		
		// Add attachments after a delay to ensure weapon is fully initialized
		GetGame().GetCallqueue().CallLater(AddAttachments, 1000, false, weaponResource, attachmentResources, spawnParams, inventoryManager);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Log weapon error
	 * @param weaponResource Weapon resource that failed
	 * @param entity Entity that the weapon was being added to
	 */
	protected void LogWeaponError(ResourceName weaponResource, IEntity entity)
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT WEAPON INTO ENTITY: %1", entity.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print("CRF GEAR SCRIPT ERROR: INVALID WEAPON ITEM!", LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add attachments to a weapon
	 * @param weaponResource Weapon resource
	 * @param attachmentResources Attachments to add
	 * @param spawnParams Spawn parameters
	 * @param inventoryManager Inventory manager component
	 */
	protected void AddAttachments(ResourceName weaponResource, array<ResourceName> attachmentResources, 
		EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!inventoryManager || !attachmentResources || attachmentResources.IsEmpty())
			return;
			
		ChimeraCharacter character = ChimeraCharacter.Cast(inventoryManager.GetOwner());
		if (!character)
			return;
			
		BaseWeaponManagerComponent weaponManager = character.GetCharacterController().GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		// Find the weapon
		IEntity weapon = FindWeaponByResource(weaponManager, weaponResource);
		if (!weapon)
			return;

		array<AttachmentSlotComponent> attachmentSlotArray = {};
		BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetAttachments(attachmentSlotArray);

		// Add each attachment
		foreach (ResourceName attachment : attachmentResources)
		{
			AddAttachmentToWeapon(attachment, weapon, attachmentSlotArray, spawnParams, inventoryManager);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Find a weapon by its resource name
	 * @param weaponManager Weapon manager component
	 * @param weaponResource Weapon resource to find
	 * @return Found weapon entity or null
	 */
	protected IEntity FindWeaponByResource(BaseWeaponManagerComponent weaponManager, ResourceName weaponResource)
	{
		array<IEntity> outWeapons = {};
		weaponManager.GetWeaponsList(outWeapons);

		foreach (IEntity weaponToCheck : outWeapons)
		{
			if (weaponToCheck.GetPrefabData().GetPrefabName() == weaponResource)
				return weaponToCheck;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add a single attachment to a weapon
	 * @param attachmentResource Attachment resource to add
	 * @param weapon Weapon to add attachment to
	 * @param attachmentSlots Available attachment slots
	 * @param spawnParams Spawn parameters
	 * @param inventoryManager Inventory manager component
	 */
	protected void AddAttachmentToWeapon(ResourceName attachmentResource, IEntity weapon, array<AttachmentSlotComponent> attachmentSlots, 
		EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		AttachmentSlotComponent verifyAttachmentSlot = null;
		IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachmentResource), GetGame().GetWorld(), spawnParams);
		inventoryManager.TryInsertItem(attachmentSpawned, EStoragePurpose.PURPOSE_ATTACHMENT_PROXY);

		foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
		{
			if (attachmentSlot.CanSetAttachment(attachmentSpawned))
			{
				IEntity attachedEntity = attachmentSlot.GetAttachedEntity();
				if (attachedEntity != null && attachedEntity != attachmentSpawned)
				{
					delete attachedEntity;
				}

				attachmentSlot.SetAttachment(attachmentSpawned);
				verifyAttachmentSlot = attachmentSlot;
				break;
			}
		}

		if (verifyAttachmentSlot == null)
		{
			LogAttachmentError(attachmentResource, weapon);
			delete attachmentSpawned;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Log attachment error
	 * @param attachmentResource Attachment resource that failed
	 * @param weapon Weapon entity it was being attached to
	 */
	protected void LogAttachmentError(ResourceName attachmentResource, IEntity weapon)
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ATTACHMENT: %1", attachmentResource), LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", weapon.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print("CRF GEAR SCRIPT ERROR: INVALID ATTACHMENT ITEM FOR WEAPON!", LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Update clothing in a specific slot
	 * @param clothingArray Clothing options
	 * @param slotInt Slot to update
	 * @param role Role identifier
	 * @param deletePreviousItems Whether to delete previous items
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, int role, bool deletePreviousItems, 
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (clothingArray.IsEmpty() || slotInt == -1)
			return;

		array<IEntity> removedItems = {};
		IEntity previousClothing = inventory.Get(slotInt);
		
		// Get random clothing from the array
		int randomIndex = m_RNG.RandInt(0, clothingArray.Count() - 1);
		ResourceName clothing = clothingArray[randomIndex];

		// Process previous clothing and its contents
		if (previousClothing != null)
		{
			ProcessPreviousClothing(previousClothing, removedItems, inventory, inventoryManager);
		}

		// Add new clothing if exists
		if (!clothing.IsEmpty())
		{
			SpawnClothing(clothing, slotInt, spawnParams, inventory, inventoryManager);
		}

		// Handle previously removed items
		HandleRemovedItems(removedItems, deletePreviousItems, inventory, inventoryManager, role);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Process previous clothing before replacement
	 * @param previousClothing Previous clothing entity
	 * @param removedItems Array to store removed items
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ProcessPreviousClothing(IEntity previousClothing, out array<IEntity> removedItems, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
		if (oldStorage)
		{
			array<IEntity> outItems = {};
			oldStorage.GetAll(outItems);
			
			foreach (IEntity item : outItems)
			{
				if (!item || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
					continue;
					
				if (SCR_EquipmentStorageComponent.Cast(item.FindComponent(SCR_EquipmentStorageComponent)) != null)
					continue;
					
				if (SCR_UniversalInventoryStorageComponent.Cast(item.FindComponent(SCR_UniversalInventoryStorageComponent)) != null)
					continue;
					
				if (BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent)) != null)
					continue;

				inventoryManager.TryRemoveItemFromStorage(item, oldStorage);
				removedItems.Insert(item);
			}
		}

		inventoryManager.TryRemoveItemFromStorage(previousClothing, inventory);
		SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawn new clothing
	 * @param clothingResource Clothing resource to spawn
	 * @param slotInt Slot to place in
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void SpawnClothing(ResourceName clothingResource, int slotInt, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothingResource), GetGame().GetWorld(), spawnParams);
		inventoryManager.TryReplaceItem(resourceSpawned, inventory, slotInt);

		if (!inventoryManager.Contains(resourceSpawned))
		{
			LogClothingError(clothingResource, inventoryManager.GetOwner());
			SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Log clothing error
	 * @param clothingResource Clothing resource that failed
	 * @param entity Entity that the clothing was being added to
	 */
	protected void LogClothingError(ResourceName clothingResource, IEntity entity)
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT CLOTHING: %1", clothingResource), LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", entity.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print("CRF GEAR SCRIPT ERROR: INVALID CLOTHING ITEM!", LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Handle items removed from previous clothing
	 * @param removedItems Items that were removed
	 * @param deletePreviousItems Whether to delete previous items
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @param role Role identifier
	 */
	protected void HandleRemovedItems(array<IEntity> removedItems, bool deletePreviousItems, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, int role)
	{
		foreach (IEntity oldItem : removedItems)
		{
			if (!deletePreviousItems)
			{
				InsertInventoryItem(oldItem, inventory, inventoryManager, role);
			}
			else 
			{
				SCR_EntityHelper.DeleteEntityAndChildren(oldItem);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add inventory item
	 * @param item Item resource to add
	 * @param itemAmount Number of items to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @param role Role identifier
	 * @param enableMedicFrags Whether to enable frags for medics
	 * @param isAssistant Whether item is for assistant role
	 */
	protected void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		int role = 0, bool enableMedicFrags = false, bool isAssistant = false)
	{
		if (item.IsEmpty() || itemAmount <= 0)
			return;

		for (int i = 1; i <= itemAmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);

			if (!resourceSpawned)
				continue;

			bool isThrowable = IsThrowableWeapon(resourceSpawned);

			// Skip frags for medics if disabled
			if (!enableMedicFrags && m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role) && 
				(isThrowable && WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
				continue;
			}
			
			// Special handling for throwables
			if (isThrowable)
			{
				bool spawned = inventoryManager.TrySpawnPrefabToStorage(item, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);
				if (!spawned)
					InsertInventoryItem(resourceSpawned, inventory, inventoryManager, role, isAssistant, isThrowable);
				else 
					SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
				
				continue;
			}

			// Try to equip attachable equipment
			if (inventoryManager.CanInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT))
			{
				BaseInventoryStorageComponent storageComp = inventoryManager.FindStorageForItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
				inventoryManager.EquipAny(storageComp, resourceSpawned, -1);
				continue;
			}

			// Regular inventory insertion
			InsertInventoryItem(resourceSpawned, inventory, inventoryManager, role, isAssistant, isThrowable);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Check if an entity is a throwable weapon
	 * @param entity Entity to check
	 * @return True if entity is a throwable weapon
	 */
	protected bool IsThrowableWeapon(IEntity entity)
	{
		WeaponComponent weaponComp = WeaponComponent.Cast(entity.FindComponent(WeaponComponent));
		return weaponComp && WEAPON_TYPES_THROWABLE.Contains(weaponComp.GetWeaponType());
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Insert an inventory item into appropriate storage
	 * @param item Item to insert
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @param role Role identifier
	 * @param isAssistant Whether item is for assistant role
	 * @param isThrowable Whether item is a throwable
	 */
	protected void InsertInventoryItem(IEntity item, SCR_CharacterInventoryStorageComponent inventory, 
		SCR_InventoryStorageManagerComponent inventoryManager, int role = 0, bool isAssistant = false, bool isThrowable = false)
	{
		if (!item)
			return;

		TIntArray clothingIDs = FilterItemToClothing(item, role, isAssistant, isThrowable);

		// Try inserting into appropriate clothing first
		bool inserted = TryInsertIntoSpecificClothing(item, clothingIDs, inventory, inventoryManager);

		// If not inserted in specific clothing, try general insertion
		if (!inserted)
			inventoryManager.TryInsertItem(item);

		// Log error and clean up if insertion failed
		if (!inventoryManager.Contains(item) || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
		{
			LogInventoryItemError(item, inventoryManager.GetOwner());
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Try to insert item into specific clothing slots
	 * @param item Item to insert
	 * @param clothingIDs Clothing slots to try
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @return True if insertion succeeded
	 */
	protected bool TryInsertIntoSpecificClothing(IEntity item, TIntArray clothingIDs, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		foreach (int clothingID : clothingIDs)
		{
			IEntity clothing = inventory.Get(clothingID);

			if (!clothing || inventoryManager.Contains(item))
				continue;

			BaseInventoryStorageComponent clothingStorage = BaseInventoryStorageComponent.Cast(clothing.FindComponent(BaseInventoryStorageComponent));

			if (!clothingStorage)
				continue;

			bool successfulInsert = inventoryManager.TryInsertItemInStorage(item, clothingStorage);

			if (!successfulInsert)
				inventoryManager.InsertItemCRF(item, clothingStorage, null, null, false);
				
			if (inventoryManager.Contains(item))
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Log inventory item error
	 * @param item Item that failed to insert
	 * @param entity Entity that the item was being added to
	 */
	protected void LogInventoryItemError(IEntity item, IEntity entity)
	{
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ITEM: %1", item.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", entity.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
		Print(" ", LogLevel.ERROR);
		Print("CRF GEAR SCRIPT ERROR: NOT ENOUGH SPACE IN INVENTORY/INVALID INVENTORY ITEM!", LogLevel.ERROR);
		Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determine appropriate clothing slots for an item
	 * @param item Item to filter
	 * @param role Role identifier
	 * @param isAssistant Whether item is for assistant role
	 * @param isThrowable Whether item is a throwable
	 * @return Array of appropriate clothing slot IDs
	 */
	TIntArray FilterItemToClothing(IEntity item, int role = 0, bool isAssistant = false, bool isThrowable = false)
	{
		array<int> clothingIDs = {};

		// Determine item type
		bool isMagazine = MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || 
						  InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent));
		
		bool isPistolAmmo = InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && 
							InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO;
		
		bool isMedical = m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role) && 
						SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
		
		bool isRadio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		
		bool isAssistantMagazine = isAssistant && MagazineComponent.Cast(item.FindComponent(MagazineComponent));
		
		bool isExplosive = IsExplosiveOrTool(item);

		// Magazines and throwables go in vest, armor, backpack primarily
		if (isMagazine)
		{
			clothingIDs = {
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.SHIRT
			};
		}
		// Non-magazines go in shirt, pants, vest primarily
		else
		{
			clothingIDs = {
				CRF_EClothingType.SHIRT, 
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};
		}

		// Medical items go in backpack, vest primarily
		if (isMedical)
		{
			clothingIDs = {
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
			};
		}

		// Pistol ammo and throwables go in pants, vest primarily
		if (isPistolAmmo || isThrowable)
		{
			clothingIDs = {
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};
		}

		// Radios go in pants, shirt, vest primarily
		if (isRadio)
		{
			clothingIDs = {
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.SHIRT, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};
		}

		// Assistant mags go in backpack, vest primarily
		if (isAssistantMagazine)
		{
			clothingIDs = {
				CRF_EClothingType.BACKPACK,
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
			};
		}

		// Explosives and tools go in backpack, vest primarily
		if (isExplosive)
		{
			clothingIDs = {
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
			};
		}

		return clothingIDs;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Check if an item is explosive or a special tool
	 * @param item Item to check
	 * @return True if item is explosive or tool
	 */
	protected bool IsExplosiveOrTool(IEntity item)
	{
		return SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent)) || 
			   SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent)) ||
			   SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent)) ||
			   SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent)) ||
			   SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
	}
};