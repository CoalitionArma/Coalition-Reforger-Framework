class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected CRF_Gamemode m_Gamemode;
	
	// Track entities currently having gear applied to prevent race conditions
	protected ref set<IEntity> m_sEntitiesBeingGeared = new set<IEntity>();
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITILIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
	
		// Only run on in-game post init
		if (!GetGame().InPlayMode())
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
	}	
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load gear script config from resource
	 * @param resourceName Resource to load
	 * @return Loaded config or null if failed
	 */
	CRF_GearScriptConfig LoadGearScriptConfig(ResourceName resourceName)
	{
		return CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load gear script config from resource
	 * @param resourceName Resource to load
	 * @return Loaded config or null if failed
	 */
	CRF_CharacterIdentity LoadIdentityConfig(ResourceName resourceName)
	{
		return CRF_CharacterIdentity.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 APPLYING GEAR METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Set gear for an entity based on its resource name
	 * @param entity The entity to equip
	 * @param resourceNameToScan Resource name containing faction info
	 */
	void SetEntityGear(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!entity)
			return;

		// Prevent multiple simultaneous gearscript operations on same entity (fixes MuzzleInMagComponent crash)
		if (m_sEntitiesBeingGeared.Contains(entity))
		{
			Print(string.Format("CRF GEARSCRIPT: Entity %1 is already being geared, skipping to prevent race condition", entity), LogLevel.WARNING);
			return;
		}

		// Determine faction from resource name
		FactionKey factionKey = CRF_EntityHelper.DetermineFactionKey(entity);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = m_Gamemode.GetGearScriptResource(factionKey);
		CRF_GearScriptContainer gearScriptSettings = m_Gamemode.GetGearScriptSettings(factionKey);

		if (gearScriptResourceName.IsEmpty() || !gearScriptSettings)
			return;

		// Get required components
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!inventory || !inventoryManager)
		{
			Print(string.Format("CRF GEAR SCRIPT ERROR: %1 DOESN'T HAVE REQUIRED COMPONENTS!", entity), LogLevel.ERROR);
			return;
		}

		// Mark entity as being geared
		m_sEntitiesBeingGeared.Insert(entity);

		// Get role and clear entity
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceNameToScan);
		 ClearEntityGear(inventory, inventoryManager);

		//Delay so when we clear gear, the client has enough time to actually clear it before getting new gear. This prevents animation bugs.
		GetGame().GetCallqueue().CallLater(SetEntityGearDelay, 500, false, gearScriptResourceName, entity, role, inventory, inventoryManager, gearScriptSettings);
	}
	
	//------------------------------------------------------------------------------------------------
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
		EntitySpawnParams spawnParams = CRF_EntityHelper.CreateSpawnParams(entity.GetOrigin());
		
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
			CRF_PlayerController pc = CRF_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			CRF_PlayerRplToOwnerManager rplToOwnerManager = CRF_PlayerRplToOwnerManager.GetInstance();
			// Cache groups manager reference - PERFORMANCE OPTIMIZATION
			SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
			
			if (groupsMan)
				groupsMan.TuneFreqDelayWithPresets(playerId, entity);
			
			if (rplToOwnerManager && pc)
			{
				pc.InitializeRadios(entity);
				rplToOwnerManager.InitializeRadioFromServer();
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 IDENTITY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Set identity for an entity
	 * @param entity The entity to equip
	 */
	void SetEntityIdentity(IEntity entity)
	{
		if (!entity)
			return;

		// Determine faction from resource name
		FactionKey factionKey = CRF_EntityHelper.DetermineFactionKey(entity);
		if (factionKey.IsEmpty())
			return;

		// Get gearscript resources
		ResourceName gearScriptResourceName = m_Gamemode.GetGearScriptResource(factionKey);
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GEAR METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
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
				CRF_ClothingHelper.UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, false, spawnParams, inventory, inventoryManager);
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
					CRF_ClothingHelper.UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, true, spawnParams, inventory, inventoryManager);
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
				CRF_Weapon_Class primary = CRF_WeaponHelper.SelectRandomWeapon(customGear.m_PrimaryWeapon);
				if(primary.m_Weapon)
				{
					CRF_WeaponHelper.SpawnWeapon(primary.m_Weapon, primary.m_Attachments, spawnParams, inventory, inventoryManager);
					CRF_WeaponHelper.AddMagazines(primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Secondary weapon
			if (!customGear.m_SecondaryWeapon.IsEmpty())
			{
				CRF_Weapon_Class secondary = CRF_WeaponHelper.SelectRandomWeapon(customGear.m_SecondaryWeapon);
				if(secondary.m_Weapon)
				{
					CRF_WeaponHelper.SpawnWeapon(secondary.m_Weapon, secondary.m_Attachments, spawnParams, inventory, inventoryManager);
					CRF_WeaponHelper.AddMagazines(secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
			
			// Pistol
			if (!customGear.m_Pistols.IsEmpty())
			{
				CRF_Weapon_Class pistol = CRF_WeaponHelper.SelectRandomWeapon(customGear.m_Pistols);
				if(pistol.m_Weapon)
				{
					CRF_WeaponHelper.SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, spawnParams, inventory, inventoryManager);
					CRF_WeaponHelper.AddMagazines(pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
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
						weapon = CRF_WeaponHelper.SelectRandomWeapon(gearConfig.m_Rifles);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.RIFLEUGL:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
					{
						weapon = CRF_WeaponHelper.SelectRandomWeapon(gearConfig.m_RifleUGLs);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.CARBINE:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
					{
						weapon = CRF_WeaponHelper.SelectRandomWeapon(gearConfig.m_Carbines);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case CRF_EGearscriptWeapons.PISTOL:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
					{
						weapon = CRF_WeaponHelper.SelectRandomWeapon(gearConfig.m_Pistols);
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
				CRF_WeaponHelper.SpawnWeapon(weapon.m_Weapon, weapon.m_Attachments, spawnParams, inventory, inventoryManager);
			
			if (specWeapon && specWeapon.m_Weapon)
				CRF_WeaponHelper.SpawnWeapon(specWeapon.m_Weapon, specWeapon.m_Attachments, spawnParams, inventory, inventoryManager);
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
						magazineArray = CRF_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Rifles, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.RIFLEUGL_MAG:
					if(gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
						magazineArray = CRF_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_RifleUGLs, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.CARBINE_MAG:
					if(gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
						magazineArray = CRF_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Carbines, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.PISTOL_MAG:
					if(gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
						magazineArray = CRF_WeaponHelper.FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_Pistols, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.SNIPER_MAG:
					if(gearConfig.m_SNIPER && gearConfig.m_SNIPER.m_MagazineArray)
						magazineArray = gearConfig.m_SNIPER.m_MagazineArray;
					break;

				case CRF_EGearscriptMagazines.AR_MAG:
					if(gearConfig.m_AR && gearConfig.m_AR.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AR.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.MMG_MAG:
					if(gearConfig.m_MMG && gearConfig.m_MMG.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MMG.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AT_MAG:
					if(gearConfig.m_AT && gearConfig.m_AT.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.MAT_MAG:
					if(gearConfig.m_MAT && gearConfig.m_MAT.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MAT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.HAT_MAG:
					if(gearConfig.m_HAT && gearConfig.m_HAT.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HAT.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AA_MAG:
					if(gearConfig.m_AA && gearConfig.m_AA.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AA.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.HMG_MAG:
					if(gearConfig.m_HMG && gearConfig.m_HMG.m_MagazineArray)
						magazineArray = CRF_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HMG.m_MagazineArray, isAssistant);
					break;
			}
			
			if (magazineArray && !magazineArray.IsEmpty())
				CRF_WeaponHelper.AddMagazines(magazineArray, spawnParams, inventory, inventoryManager);
			
			if (selectedWeapon)
				weaponsSelected.RemoveItem(selectedWeapon)
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
				CRF_InventoryHelper.AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
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
						CRF_InventoryHelper.AddInventoryItem(gearScriptSettings.m_rShortRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.LONGRANGE_RADIO:
					if (gearScriptSettings.m_bEnableLeadershipRadios)
						CRF_InventoryHelper.AddInventoryItem(gearScriptSettings.m_rLongRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.RTO_RADIO:
					if (gearScriptSettings.m_bEnableRTORadios)
						CRF_InventoryHelper.AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.LEADERSHIP_BINO:
					if (gearConfig.m_sLeadershipBinocularsPrefab != "")
						CRF_InventoryHelper.AddInventoryItem(gearConfig.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.ASSISTANT_BINO:
					if (gearConfig.m_sAssistantBinocularsPrefab != "")
						CRF_InventoryHelper.AddInventoryItem(gearConfig.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;

				case CRF_EGearscriptItems.MEDIC_ITEMS:
					foreach (CRF_Inventory_Item item : gearConfig.m_MedicMedicalItems)
						CRF_InventoryHelper.AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
					break;
			}
		}
		
		// Default medical items
		foreach (CRF_Inventory_Item item : gearConfig.m_InfantryMedicalItems)
			CRF_InventoryHelper.AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
		
		// Default inventory items
		foreach (CRF_Inventory_Item item : gearConfig.m_DefaultInventoryItems)
			CRF_InventoryHelper.AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_GearscriptManager m_sInstance;
	void CRF_GearscriptManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GearscriptManager GetInstance()
	{
		return m_sInstance;
	}
};