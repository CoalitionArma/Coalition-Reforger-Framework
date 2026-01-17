class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected CRF_Gamemode m_Gamemode;

	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	protected ref map<ResourceName, int> m_mVehicleSupplyCosts = new map<ResourceName, int>;
	
	// Track entities currently having gear applied to prevent race conditions
	protected ref set<IEntity> m_sEntitiesBeingGeared = new set<IEntity>();
	
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
	/**
	 * @brief Get gearscript resource for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return ResourceName for the gearscript or empty string if not found
	 */
	ResourceName GetGearScriptResource(FactionKey factionKey)
	{
		CRF_GearScriptContainer container = GetGearScriptSettings(factionKey);
		if (!container)
		{
			PrintFormat("NO GEARSCRIPT ASSIGNED TO: %1", factionKey, LogLevel.WARNING);
			return "";
		}
		
		// Use cached reference - PERFORMANCE OPTIMIZATION
		if (!m_Gamemode)
			return "";
		
		switch (factionKey)
		{
			case "BLUFOR": return m_Gamemode.m_rBLUFORCurrentGearScript; break;
			case "OPFOR": return m_Gamemode.m_rOPFORCurrentGearScript; break;
			case "INDFOR": return m_Gamemode.m_rINDFORCurrentGearScript; break;
			case "CIV": return m_Gamemode.m_rCIVILIANCurrentGearScript; break;
		}

		return m_Gamemode.m_rCIVILIANCurrentGearScript;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get gearscript container for a faction
	 * @param factionKey Faction identifier (BLUFOR, OPFOR, etc.)
	 * @return The gearscript container or null if not found
	 */
	CRF_GearScriptContainer GetGearScriptSettings(FactionKey factionKey)
	{
		if (!m_Gamemode)
			return null;
			
		CRF_GearScriptContainer gearScriptContainer = null;

		switch (factionKey)
		{
			case "BLUFOR":
				gearScriptContainer = m_Gamemode.m_BLUFORGearScriptSettings;
				break;
			
			case "OPFOR":
				gearScriptContainer = m_Gamemode.m_OPFORGearScriptSettings;
				break;
			
			case "INDFOR":
				gearScriptContainer = m_Gamemode.m_INDFORGearScriptSettings;
				break;
			
			case "CIV":
				gearScriptContainer = m_Gamemode.m_CIVILIANGearScriptSettings;
				break;
		}
		
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

		// Prevent multiple simultaneous gearscript operations on same entity (fixes MuzzleInMagComponent crash)
		if (m_sEntitiesBeingGeared.Contains(entity))
		{
			Print(string.Format("CRF GEARSCRIPT: Entity %1 is already being geared, skipping to prevent race condition", entity), LogLevel.WARNING);
			return;
		}

		// Determine faction from resource name
		FactionKey factionKey = DetermineFactionKey(resourceNameToScan);
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

		// Mark entity as being geared
		m_sEntitiesBeingGeared.Insert(entity);

		// Get role and clear entity
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceNameToScan);
		ClearEntityGear(inventory, inventoryManager);

		//Delay so when we clear gear, the client has enough time to actually clear it before getting new gear. This prevents animation bugs.
		GetGame().GetCallqueue().CallLater(SetEntityGearDelay, 500, false, gearScriptResourceName, entity, role, inventory, inventoryManager, gearScriptSettings);
	}
	
	void SetEntityGearDelay(string gearScriptResourceName, IEntity entity, CRF_EGearRole role, SCR_CharacterInventoryStorageComponent inventory,
	SCR_InventoryStorageManagerComponent inventoryManager, CRF_GearScriptContainer gearScriptSettings)
	{
		// If entity was deleted or snapped up by the slotting manager
		if(!entity)
		{
			// Clean up tracking set
			m_sEntitiesBeingGeared.RemoveItem(entity);
			return;
		}
		
		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		if (!gearConfig)
		{
			m_sEntitiesBeingGeared.RemoveItem(entity);
			return;
		}
		
		// Prepare spawn parameters
		EntitySpawnParams spawnParams = CreateSpawnParams(entity);
		
		// Apply gear - OPTIMIZED: Consolidate CallLater calls to reduce scheduling overhead
		ApplyClothing(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Use single consolidated callback instead of multiple separate ones
		GetGame().GetCallqueue().CallLater(ApplyGearConsolidated, 500, false, gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager, entity);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Consolidated gear application callback - PERFORMANCE OPTIMIZATION
	 * Applies weapons and inventory items in a single callback to reduce CallQueue overhead
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param gearScriptSettings Gearscript settings
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @param entity Entity being equipped
	 */
	protected void ApplyGearConsolidated(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, IEntity entity)
	{
		if (!inventory || !inventoryManager || !entity)
		{
			// Clean up tracking set if entity is invalid
			if (entity)
				m_sEntitiesBeingGeared.RemoveItem(entity);
			return;
		}
		
		// Apply weapons (originally 375ms delay, now immediate in this consolidated callback at 500ms)
		ApplyWeapons(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		// Apply inventory items (originally 250ms delay, now immediate in this consolidated callback at 500ms)
		ApplyInventoryItems(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		// Initialize radios for player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			// Cache groups manager reference - PERFORMANCE OPTIMIZATION
			SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
			
			if (groupsMan)
				groupsMan.TuneFreqDelayWithPresets(playerId, entity);
			if (pc)
			{
				pc.InitializeRadios(entity);
				pc.InitializeRadioFromServer();
			}
		}
		
		// CRITICAL: Mark entity as fully geared after ALL operations complete (including weapon attachment delays)
		// Wait for attachment delay (1000ms from SpawnWeapon) + safety margin
		GetGame().GetCallqueue().CallLater(FinishGearingEntity, 1200, false, entity);
	}	
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Mark entity as finished being geared, allowing future gearscript operations
	 * @param entity Entity that finished being geared
	 */
	protected void FinishGearingEntity(IEntity entity)
	{
		if (entity && m_sEntitiesBeingGeared.Contains(entity))
			m_sEntitiesBeingGeared.RemoveItem(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Set identity for an entity
	 * @param entity The entity to equip
	 */
	void SetEntityIdentity(IEntity entity)
	{
		if (!entity)
			return;
		
		ResourceName resourceNameToScan = entity.GetPrefabData().GetPrefabName();
		
		if (!CRF_RoleHelper.IsValidGearscriptResource(resourceNameToScan) || !entity)
			return;

		// Determine faction from resource name
		FactionKey factionKey = DetermineFactionKey(resourceNameToScan);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = GetGearScriptResource(factionKey);
		if (gearScriptResourceName.IsEmpty())
			return;

		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		if (!gearConfig)
			return;
		
		// Apply gear
		SetIdentity(gearConfig, entity)
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determine faction key from resource name
	 * @param resourceName Resource name to analyze
	 * @return Faction key or empty string if not found
	 */
	protected FactionKey DetermineFactionKey(ResourceName resourceName)
	{
		switch (true)
		{
			case resourceName.Contains("BLUFOR"):
				return "BLUFOR";
			case resourceName.Contains("OPFOR"):
				return "OPFOR";
			case resourceName.Contains("INDFOR"):
				return "INDFOR";
			case resourceName.Contains("CIV"):
				return "CIV";
		};
			
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
		inventoryManager.GetItems(itemsRoot);

		items.InsertAll(itemsRoot);

		// Separate weapons from other items to delete weapons first
		// This prevents MuzzleInMagComponent crashes when projectiles are deleted before weapon detaches them
		array<IEntity> weapons = {};
		array<IEntity> otherItems = {};
		
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
				
			// Check if item is a weapon
			if (item.FindComponent(WeaponComponent))
				weapons.Insert(item);
			else
				otherItems.Insert(item);
		}
		
		// Delete weapons FIRST - this allows them to properly detach projectiles from MuzzleInMagComponent
		foreach (IEntity weapon : weapons)
		{
			if (weapon)
				SCR_EntityHelper.DeleteEntityAndChildren(weapon);
		}
		
		// Small delay before deleting other items to ensure weapon cleanup is complete
		// This prevents race conditions with MuzzleInMagComponent projectile attachment
		if (!otherItems.IsEmpty())
			GetGame().GetCallqueue().CallLater(DeleteRemainingItems, 50, false, otherItems);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Delete remaining non-weapon items after weapon cleanup
	 * @param items Array of items to delete
	 */
	protected void DeleteRemainingItems(array<IEntity> items)
	{
		foreach (IEntity item : items)
		{
			if (item)
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
	 * @brief Load vehicle gear script config from resource
	 * @param resourceName Resource to load
	 * @return Loaded config or null if failed
	 */
	protected CRF_VehicleGearscriptConfig LoadVehicleGearScriptConfig(ResourceName resourceName)
	{
		return CRF_VehicleGearscriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load gear script config from resource
	 * @param resourceName Resource to load
	 * @return Loaded config or null if failed
	 */
	protected CRF_CharacterIdentity LoadIdentityConfig(ResourceName resourceName)
	{
		return CRF_CharacterIdentity.Cast(BaseContainerTools.CreateInstanceFromContainer(
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
	 * @brief Apply custom weapons based on role
	 * @param faction Faction to pull GS
	 * @param role Role identifier
	 */
	string GetCustomRoleName(FactionKey factionKey, CRF_EGearRole role)
	{
		// Get gearscript resources
		ResourceName gearScriptResourceName = GetGearScriptResource(factionKey);

		if (gearScriptResourceName.IsEmpty())
			return string.Empty;

		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		
		if (!gearConfig)
			return string.Empty;
		
		foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != role)
				continue;
			
			return customGear.m_sRoleName;
		}
		
		return string.Empty;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply clothing to entity based on config
	 * @param gearConfig Gear configuration
	 * @param entity Entity to apply randomized head/body to from the identity config of the gear config
	 */
    protected void SetIdentity(CRF_GearScriptConfig gearConfig, IEntity entity)
    {
		CRF_Character_Visual_Identity gsVisIdentity;
		CRF_Character_Sound_Identity gsSndIdentity;
        SCR_CharacterIdentityComponent identityComp = SCR_CharacterIdentityComponent.Cast(entity.FindComponent(SCR_CharacterIdentityComponent));
		
		if (!identityComp)
			return;
		
		// Get both sound and visual identities from the identity comp
        VisualIdentity visIdentity = identityComp.GetIdentity().GetVisualIdentity();
		SoundIdentity sndIdentity = identityComp.GetIdentity().GetSoundIdentity();
		
		if (!visIdentity || !sndIdentity)
			return;
		
		CRF_CharacterIdentity gsCharIdentity = LoadIdentityConfig(gearConfig.m_FactionIdentity);
		
		if (gsCharIdentity)
		{
			if (!gsCharIdentity.m_VisualIdentityArray.IsEmpty())
				gsVisIdentity = gsCharIdentity.m_VisualIdentityArray.GetRandomElement();
			
			if (!gsCharIdentity.m_SoundIdentityArray.IsEmpty())
				gsSndIdentity = gsCharIdentity.m_SoundIdentityArray.GetRandomElement();
			
			if (gsVisIdentity)
			{
	        	visIdentity.SetHead(gsVisIdentity.m_Head);
	        	visIdentity.SetBody(gsVisIdentity.m_Body);
			};
			
			if (gsSndIdentity)
			{
	        	sndIdentity.SetVoiceID(gsSndIdentity.m_VoiceID);
				sndIdentity.SetPitch(gsSndIdentity.m_VoicePitch);
			};
			
			// Commit all changes to the identity comp
	        identityComp.CommitChanges();
		};
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
	protected void ApplyClothing(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Apply default faction clothing
		if (gearConfig)
		{
			foreach (CRF_Clothing clothing : gearConfig.m_DefaultClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, false, spawnParams, inventory, inventoryManager);
			}
		}
		
		// Apply custom clothing if available
		if (gearConfig)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
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
	 * @param gearScriptSettings Gearscript settings
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(!inventory || !inventoryManager || !gearConfig)
			return;
		
		bool customWeaponsSet = ApplyCustomWeapons(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Apply default weapons if no custom weapons were set
		if (!customWeaponsSet)
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
	protected bool ApplyCustomWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return false;
			
		bool customWeaponsSet = false;
		
		foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != role)
				continue;
			
			// Primary weapon
			if (!customGear.m_PrimaryWeapon.IsEmpty())
			{
				CRF_Weapon_Class primary = SelectRandomWeapon(customGear.m_PrimaryWeapon);
				if(primary.m_Weapon)
				{
					SpawnWeapon(primary.m_Weapon, primary.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Secondary weapon
			if (!customGear.m_SecondaryWeapon.IsEmpty())
			{
				CRF_Weapon_Class secondary = SelectRandomWeapon(customGear.m_SecondaryWeapon);
				if(secondary.m_Weapon)
				{
					SpawnWeapon(secondary.m_Weapon, secondary.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Pistol
			if (!customGear.m_Pistols.IsEmpty())
			{
				CRF_Weapon_Class pistol = SelectRandomWeapon(customGear.m_Pistols);
				if(pistol.m_Weapon)
				{
					SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, spawnParams, inventory, inventoryManager);
					AddMagazines(pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
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
	protected void ApplyDefaultWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		CRF_RoleConfig rolesConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);
		array<CRF_Weapon_Class> weaponsSelected = {};
		
		foreach (CRF_EGearscriptWeapons weaponType : rolesConfig.m_aWeapons)
		{
			CRF_Weapon_Class weapon;
			CRF_Spec_Weapon_Class specWeapon;
			
			switch (weaponType)
			{
				case CRF_EGearscriptWeapons.RIFLE:
					if(gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_Rifles);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.RIFLEUGL:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_RifleUGLs);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.CARBINE:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_Carbines);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case CRF_EGearscriptWeapons.PISTOL:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_Pistols);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case CRF_EGearscriptWeapons.SNIPER:
					if(gearConfig.m_SNIPER)
						weapon = gearConfig.m_SNIPER;
					break;

				case CRF_EGearscriptWeapons.AR:
					if(gearConfig.m_AR)
						specWeapon = gearConfig.m_AR;
					break;

				case CRF_EGearscriptWeapons.MMG:
					if(gearConfig.m_MMG)
						specWeapon = gearConfig.m_MMG;
					break;

				case CRF_EGearscriptWeapons.AT:
					if(gearConfig.m_AT)
						specWeapon = gearConfig.m_AT;
					break;
	
				case CRF_EGearscriptWeapons.MAT:
					if(gearConfig.m_MAT)
						specWeapon = gearConfig.m_MAT;
					break;
	
				case CRF_EGearscriptWeapons.HAT:
					if(gearConfig.m_HAT)
						specWeapon = gearConfig.m_HAT;
					break;

				case CRF_EGearscriptWeapons.AA:
					if(gearConfig.m_AA)
						specWeapon = gearConfig.m_AA;
					break;

				case CRF_EGearscriptWeapons.HMG:
					if(gearConfig.m_HMG)
						specWeapon = gearConfig.m_HMG;
					break;
			}
			
			if (weapon && weapon.m_Weapon)
				SpawnWeapon(weapon.m_Weapon, weapon.m_Attachments, spawnParams, inventory, inventoryManager);
			
			if (specWeapon && specWeapon.m_Weapon)
				SpawnWeapon(specWeapon.m_Weapon, specWeapon.m_Attachments, spawnParams, inventory, inventoryManager);
		}
		
		ApplyDefaultMagazines(weaponsSelected, gearConfig, role, spawnParams, inventory, inventoryManager);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Apply default magazines based on role
	 * @param gearConfig Gear configuration
	 * @param role Role identifier
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyDefaultMagazines(array<CRF_Weapon_Class> weaponsSelected, CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		CRF_RoleConfig rolesConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);
		bool isAssistant = (rolesConfig.m_SlottingType == CRF_ESlotType.ASSISTANT || rolesConfig.m_SlottingType == CRF_ESlotType.SPECIALTY_ASSISTANT);
		
		foreach (CRF_EGearscriptMagazines roleMags : rolesConfig.m_aMagazines)
		{
			array<ref CRF_Magazine_Class> magazineArray;
			CRF_Weapon_Class selectedWeapon;
			
			switch (roleMags)
			{
				case CRF_EGearscriptMagazines.RIFLE_MAG:
					if(gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Rifles, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.RIFLEUGL_MAG:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_RifleUGLs, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.CARBINE_MAG:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Carbines, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.PISTOL_MAG:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Pistols, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.SNIPER_MAG:
					if(gearConfig.m_SNIPER && gearConfig.m_SNIPER.m_MagazineArray)
						magazineArray = gearConfig.m_SNIPER.m_MagazineArray;
					break;

				case CRF_EGearscriptMagazines.AR_MAG:
					if(gearConfig.m_AR && gearConfig.m_AR.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_AR.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.MMG_MAG:
					if(gearConfig.m_MMG && gearConfig.m_MMG.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_MMG.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AT_MAG:
					if(gearConfig.m_AT && gearConfig.m_AT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_AT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.MAT_MAG:
					if(gearConfig.m_MAT && gearConfig.m_MAT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_MAT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.HAT_MAG:
					if(gearConfig.m_HAT && gearConfig.m_HAT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_HAT.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AA_MAG:
					if(gearConfig.m_AA && gearConfig.m_AA.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_AA.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.HMG_MAG:
					if(gearConfig.m_HMG && gearConfig.m_HMG.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_HMG.m_MagazineArray, isAssistant);
					break;
			}
			
			if (magazineArray && !magazineArray.IsEmpty())
				AddMagazines(magazineArray, spawnParams, inventory, inventoryManager);
			
			if (selectedWeapon)
				weaponsSelected.RemoveItem(selectedWeapon)
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Find the appropriate magazine array for the weapon type based off the slected weapons in ApplyDefaultWeapons
	 * @param weaponsSelected Weapons we selected when initilizing the role
	 * @param weaponType the weapon array we are comparing it to 
	 */
	protected array<ref CRF_Magazine_Class> FindMagArrayForWeaponsSelected(array<CRF_Weapon_Class> weaponsSelected, array<ref CRF_Weapon_Class> weaponType, out CRF_Weapon_Class selectedWeapon)
	{	
		foreach (CRF_Weapon_Class weaponSelected : weaponsSelected)
		{
			if (weaponType.Contains(weaponSelected))
			{
				foreach (CRF_Weapon_Class weaponToCompare : weaponType)
				{
					if (weaponToCompare == weaponSelected)
						return weaponSelected.m_MagazineArray;
					
					selectedWeapon = weaponSelected;
				}
			};
		}
		
		return new array<ref CRF_Magazine_Class>; 
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
	protected void ApplyInventoryItems(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(!inventory || !inventoryManager || !gearConfig)
			return;
		
		foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_RolesToSetCustomSettings)
		{
			if (customGear.m_Role != role)
				continue;
	
			foreach (CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
			}
		}
		
		// Then apply default gear
		CRF_RoleConfig rolesConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);
		
		foreach (CRF_EGearscriptItems roleItem : rolesConfig.m_aItems)
		{
			switch (roleItem)
			{
				case CRF_EGearscriptItems.SHORTRANGE_RADIO:
					if (gearScriptSettings.m_bEnableGIRadios)
						AddInventoryItem(gearScriptSettings.m_rShortRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.LONGRANGE_RADIO:
					if (gearScriptSettings.m_bEnableLeadershipRadios)
						AddInventoryItem(gearScriptSettings.m_rLongRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.RTO_RADIO:
					if (gearScriptSettings.m_bEnableRTORadios)
						AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.LEADERSHIP_BINO:
					if (gearConfig.m_bEnableLeadershipBinoculars)
						AddInventoryItem(gearConfig.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.ASSISTANT_BINO:
					if (gearConfig.m_bEnableAssistantBinoculars)
						AddInventoryItem(gearConfig.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;

				case CRF_EGearscriptItems.MEDIC_ITEMS:
					foreach (CRF_Inventory_Item item : gearConfig.m_MedicMedicalItems)
						AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
					break;
			}
		}
		
		// Default medical items
		foreach (CRF_Inventory_Item item : gearConfig.m_InfantryMedicalItems)
			AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
		
		// Default inventory items
		foreach (CRF_Inventory_Item item : gearConfig.m_DefaultInventoryItems)
			AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
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
	array<ref CRF_Magazine_Class> ConvertSpecMagArrayIntoMagArray(array<ref CRF_Spec_Magazine_Class> specMagazineArray, bool isAssistant)
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
			
			if (isAssistant)
				tempMag.m_MagazineCount = specMagazine.m_AssistantMagazineCount;
			else
				tempMag.m_MagazineCount = specMagazine.m_MagazineCount;
			
			tempArray.Insert(tempMag);
		}
		
		return tempArray;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawn a weapon and its attachments
	 * @param weaponResource Weapon resource to spawn
	 * @param attachmentResources Attachments to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void SpawnWeapon(ResourceName weaponResource, array<ResourceName> attachmentResources, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(weaponResource.IsEmpty())
			return;
		
		bool successfulSpawn = inventoryManager.TrySpawnPrefabToStorage(weaponResource, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);

		if (!successfulSpawn)
		{
			LogWeaponError(weaponResource, inventoryManager.GetOwner());
			return;
		}
		
		// Add attachments after a delay to ensure weapon is fully initialized
		GetGame().GetCallqueue().CallLater(AddAttachments, 1000, false, weaponResource, attachmentResources, spawnParams, inventoryManager);
		GetGame().GetCallqueue().CallLater(SelectWeapon, 500, false, inventory.GetOwner()); 
	}
	
	void SelectWeapon(IEntity entity)
	{
		if (!ChimeraCharacter.Cast(entity))
			return;
		
		BaseWeaponManagerComponent weaponMan = ChimeraCharacter.Cast(entity).GetWeaponManager();
		if (!weaponMan)
			return;
		
		CharacterControllerComponent charController = ChimeraCharacter.Cast(entity).GetCharacterController();
		if (!charController)
			return;
		
		array<WeaponSlotComponent> outSlots = {};
		weaponMan.GetWeaponsSlots(outSlots);
		WeaponSlotComponent weapon;
		foreach (WeaponSlotComponent outSlot: outSlots)
		{
			if (!outSlot.GetWeaponEntity())
				continue;
			
			if (outSlot.GetWeaponEntity().FindComponent(GrenadeMoveComponent))
				continue;
			
			weapon = outSlot;
			break;
		}
		
		if (!weapon)
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			SCR_ChimeraCharacter.Cast(entity).SelectPrimaryWeapon();
		else
			charController.SelectWeapon(weapon);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief add a weapons magazines
	 * @param magazineArray Magazines to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void AddMagazines(array<ref CRF_Magazine_Class> magazineArray, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
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
		if (!attachmentResources || attachmentResources.IsEmpty())
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
		
		if (!Resource.Load(attachmentResource).IsValid())
			return;
		
		IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachmentResource), GetGame().GetWorld(), spawnParams);
		BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
		if (!weaponStorageComp)
			return;

		IEntity oldSight = weaponStorageComp.FindSuitableSlotForItem(attachmentSpawned).GetAttachedEntity();
		
		foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
		{
			if (attachmentSlot.CanSetAttachment(attachmentSpawned))
			{
				if (oldSight)
				delete oldSight;
			
				inventoryManager.TryInsertItemInStorage(attachmentSpawned, weaponStorageComp);
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
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, CRF_EGearRole role, bool deletePreviousItems, 
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (clothingArray.IsEmpty() || slotInt == -1)
			return;
		
		array<IEntity> removedItems = {};
		IEntity previousClothing = inventory.Get(slotInt);
		
		// Get random clothing from the array
		ResourceName clothing = clothingArray.GetRandomElement();

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
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role)
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
	 */
	protected void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		CRF_EGearRole role = 0)
	{
		if (item.IsEmpty() || itemAmount <= 0)
			return;

		for (int i = 1; i <= itemAmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);

			if (!resourceSpawned)
				continue;

			bool isThrowable = IsThrowableWeapon(resourceSpawned);
			
			// Special handling for throwables
			if (isThrowable)
			{
				// Delete the pre-spawned entity first to avoid duplicate projectile attachment issues
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
				
				// Now spawn directly to storage - this prevents MuzzleInMagComponent conflicts
				bool spawned = inventoryManager.TrySpawnPrefabToStorage(item, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);
				if (!spawned)
				{
					// If direct spawn failed, try spawning again and inserting manually
					resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);
					if (resourceSpawned)
						InsertInventoryItem(resourceSpawned, inventory, inventoryManager, role, isThrowable);
				}
				
				continue;
			}

			// Try to equip attachable equipment
			if (inventoryManager.CanInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT))
			{
				inventoryManager.TryInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
				continue;
			}

			// Regular inventory insertion
			InsertInventoryItem(resourceSpawned, inventory, inventoryManager, role, isThrowable);
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
	/*
	Public method to insert an item into a storage and keep the same storage it would usually be assigned to
	*/
	void InsertInventoryItemPublic(IEntity item, SCR_CharacterInventoryStorageComponent inventory, 
		SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role = 0, bool isThrowable = false)
	{
		InsertInventoryItem(item, inventory, inventoryManager, role, isThrowable);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Insert an inventory item into appropriate storage
	 * @param item Item to insert
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 * @param role Role identifier
	 * @param isThrowable Whether item is a throwable
	 */
	protected void InsertInventoryItem(IEntity item, SCR_CharacterInventoryStorageComponent inventory, 
		SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role = 0, bool isThrowable = false)
	{
		if (!item)
			return;

		TIntArray clothingIDs = FilterItemToClothing(item, role, isThrowable);

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
	 * @param isThrowable Whether item is a throwable
	 * @return Array of appropriate clothing slot IDs
	 */
	TIntArray FilterItemToClothing(IEntity item, CRF_EGearRole role = 0, bool isThrowable = false)
	{
		array<int> clothingIDs = {};

		// Determine item type
		bool isMagazine = MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || 
						  InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent));
		
		bool isPistolAmmo = InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && 
							InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO;
		
		bool isMedical = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role).m_aItems.Contains(CRF_EGearscriptItems.MEDIC_ITEMS) && 
						SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
		
		bool isRadio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
		
		bool isExplosive = IsExplosiveOrTool(item);

		// Magazines and throwables go in backpack, vest, armor, primarily
		if (isMagazine)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST,
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.SHIRT
			};
		}
		// Non-magazines go in shirt, pants, vest primarily
		else
		{
			clothingIDs = {
				CRF_EGearscriptClothing.SHIRT, 
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Pistol ammo and throwables go in pants, vest primarily
		if (isPistolAmmo || isThrowable)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Radios go in pants, shirt, vest primarily
		if (isRadio)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.PANTS, 
				CRF_EGearscriptClothing.SHIRT, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK
			};
		}

		// Explosives/Medical items go in backpack, vest primarily
		if (isExplosive || isMedical)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST
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
	
	//------------------------------------------------------------------------------------------------
	// RplSave: Serialize vehicle supply costs for JIP sync
	override bool RplSave(ScriptBitWriter writer)
	{
		// Write the count of vehicle supply cost entries
		int count = m_mVehicleSupplyCosts.Count();
		writer.WriteInt(count);
		
		// Write each vehicle resource and its supply cost
		foreach (ResourceName vehicleResource, int supplyCost : m_mVehicleSupplyCosts)
		{
			writer.WriteResourceName(vehicleResource);
			writer.WriteInt(supplyCost);
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// RplLoad: Deserialize vehicle supply costs for JIP sync
	override bool RplLoad(ScriptBitReader reader)
	{
		// Read the count of vehicle supply cost entries
		int count;
		reader.ReadInt(count);
		
		// Clear existing data (for JIP clients)
		m_mVehicleSupplyCosts.Clear();
		
		// Read each vehicle resource and its supply cost
		for (int i = 0; i < count; i++)
		{
			ResourceName vehicleResource;
			int supplyCost;
			
			reader.ReadResourceName(vehicleResource);
			reader.ReadInt(supplyCost);
			
			m_mVehicleSupplyCosts.Set(vehicleResource, supplyCost);
		}
		
		return true;
	}
};