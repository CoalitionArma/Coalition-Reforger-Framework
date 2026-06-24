class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected ref CRF_ResourceCache m_ResourceCache;
	static ref CRF_RolesConfig m_RolesConfig;
	
	protected CRF_Gamemode m_Gamemode;
	
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
		
		LoadRoleConfig();
		m_ResourceCache = new CRF_ResourceCache;
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		// 10APR26 - Pat, this has started causing crashing as of today. No idea why. Commenting out for now.
		// #ifdef WORKBENCH
		// 	GetGame().GetCallqueue().CallLater(DEBUG_SpawnAllRoleCharacters, 250, false);
		// #endif
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_RolesConfig GetRolesConfig()
	{
		return m_RolesConfig;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load necessary configurations for gearscript
	protected void LoadRoleConfig()
	{
		ResourceName rolesConfigPath;
		if (!CVON_VONGameModeComponent.GetInstance())
			  rolesConfigPath = "{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf";
		else
			rolesConfigPath = "{F04F02DBFC65553E}Configs/Gearscripts/Additional Configs/CRF_CVON_Global_Roles_Config.conf";
		
		m_RolesConfig = CRF_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load gear script config from resource
	//! \param[in] resourceName Resource to load
	//! \return Loaded config or null if failed
	CRF_GearScriptConfig LoadGearScriptConfig(ResourceName resourceName)
	{
		return CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load gear script config from resource
	//! \param[in] resourceName Resource to load
	//! \return Loaded config or null if failed
	CRF_CharacterIdentity LoadIdentityConfig(ResourceName resourceName)
	{
		return CRF_CharacterIdentity.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 APPLYING GEAR METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Set gear for an entity based on its resource name
	//! \param[in] entity The entity to equip
	//! \param[in] resourceNameToScan Resource name containing faction info
	void SetEntityGear(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!entity)
			return;

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
			string errorMsg = string.Format("Entity %1 is missing required inventory components (SCR_CharacterInventoryStorageComponent or SCR_InventoryStorageManagerComponent)", entity);
			
			// Use MissionValidatorManager in Workbench, fallback to Print in game
			#ifdef WORKBENCH
			CRF_MissionValidatorManager validator = CRF_MissionValidatorManager.GetInstance();
			if (validator)
				validator.AddCriticalError("[GEARSCRIPT] " + errorMsg);
			else
				Print("[CRF GEARSCRIPT ERROR] " + errorMsg, LogLevel.ERROR);
			#else
			Print("[CRF GEARSCRIPT ERROR] " + errorMsg, LogLevel.ERROR);
			#endif
			
			return;
		}

		// Get role and clear entity
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(resourceNameToScan);
		 ClearEntityGear(inventory, inventoryManager);

		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		
		// Prepare spawn parameters
		EntitySpawnParams spawnParams = CRF_EntityHelper.CreateSpawnParams(entity.GetOrigin());
		
		// Apply gear
		ApplyClothing(gearConfig, role, spawnParams, inventory, inventoryManager);
		
		// Apply weapons
		ApplyWeapons(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		// Apply inventory items
		ApplyInventoryItems(gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		
		// Initialize radios for player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			CRF_PlayerController pc = CRF_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			CRF_PlayerRplToOwnerManager rplToOwnerManager = CRF_PlayerRplToOwnerManager.GetInstance();
			// Cache groups manager reference - PERFORMANCE OPTIMIZATION
			SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
			
			// Rebuild the radio list after replacing the player's gear. Tuning before
			// this can access stale entities left behind by ClearEntityGear().
			if (pc)
				pc.InitializeRadios(entity);

			if (groupsMan)
				groupsMan.TuneFreqDelayWithPresets(playerId, entity);
			
			if (rplToOwnerManager && pc)
			{
				rplToOwnerManager.InitializeRadioFromServer();
			}
		}
	}	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 IDENTITY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------	
	//! Set identity for an entity
	//! \param[in] entity The entity to equip
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
		
		SCR_CharacterIdentityComponent identityComp = SCR_CharacterIdentityComponent.Cast(entity.FindComponent(SCR_CharacterIdentityComponent));
		if (!identityComp)
			return;
		
		// Get both sound and visual identities from the identity identityComp
		VisualIdentity visIdentity = identityComp.GetIdentity().GetVisualIdentity();
		SoundIdentity sndIdentity = identityComp.GetIdentity().GetSoundIdentity();
		if (!visIdentity || !sndIdentity)
			return;
			
		CRF_CharacterIdentity gsCharIdentity = LoadIdentityConfig(gearConfig.m_FactionIdentity);
		
		if (gsCharIdentity)
		{
			CRF_Character_Visual_Identity gsVisIdentity;
			CRF_Character_Sound_Identity gsSndIdentity;
			
			if (!gsCharIdentity.m_VisualIdentityArray.IsEmpty())
				gsVisIdentity = gsCharIdentity.m_VisualIdentityArray.GetRandomElement();			if (!gsCharIdentity.m_SoundIdentityArray.IsEmpty())
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
	//! Apply clothing to entity based on config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Apply weapons to entity based on config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] gearScriptSettings Gearscript settings
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Apply custom weapons based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \return True if custom weapons were applied
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
					AddMagazines(primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
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
					AddMagazines(secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
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
					AddMagazines(pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				};
			}
		}
		
		return customWeaponsSet;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Apply default weapons based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyDefaultWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		CRF_RoleConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
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
	//! Apply default magazines based on role
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	protected void ApplyDefaultMagazines(array<CRF_Weapon_Class> weaponsSelected, CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!gearConfig)
			return;
		
		CRF_RoleConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
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
				AddMagazines(magazineArray, spawnParams, inventory, inventoryManager);
			
			if (selectedWeapon)
				weaponsSelected.RemoveItem(selectedWeapon)
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add a weapons magazines
	//! \param[in] magazineArray Magazines to add
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
	//! Apply inventory items based on role and config
	//! \param[in] gearConfig Gear configuration
	//! \param[in] role Role identifier
	//! \param[in] gearScriptSettings Gearscript settings
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
		CRF_RoleConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
		
		foreach (CRF_EGearscriptItems roleItem : rolesConfig.m_aItems)
		{
			switch (roleItem)
			{
				case CRF_EGearscriptItems.SHORTRANGE_RADIO:
					if (gearScriptSettings.m_bEnableGIRadios)
						AddInventoryItem(gearScriptSettings.m_rShortRangeRadioPrefab, 1, spawnParams, inventory, inventoryManager);
					else if (gearScriptSettings.m_bEnableLeadershipRadios && (rolesConfig.m_SlottingType == CRF_ESlotType.TEAM_LEADER || rolesConfig.m_SlottingType == CRF_ESlotType.SQUAD_LEADER || rolesConfig.m_SlottingType == CRF_ESlotType.SPECIALTY || rolesConfig.m_SlottingType == CRF_ESlotType.SPECIALTY_ASSISTANT))
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
					if (gearConfig.m_sLeadershipBinocularsPrefab != "")
						AddInventoryItem(gearConfig.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
					break;
				
				case CRF_EGearscriptItems.ASSISTANT_BINO:
					if (gearConfig.m_sAssistantBinocularsPrefab != "")
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
	//! Add inventory item
	//! \param[in] item Item resource to add
	//! \param[in] itemAmount Number of items to add
	//! \param[in] spawnParams Spawn parameters (unused - kept for compatibility)
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
	//! \param[in] role Role identifier
	void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		CRF_EGearRole role = 0)
	{
		if (item.IsEmpty())
			return;
			
		Resource resource = m_ResourceCache.GetCachedResource(item);
		if (!resource || !resource.IsValid())
			return;
			
		IEntitySource itemSource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!itemSource)
			return;
		
		// Determine item type to use appropriate storage priority
		TIntArray clothingIDs = CRF_InventoryHelper.FilterItemToClothing(itemSource, role);
		
		for (int i = 1; i <= itemAmount; i++)
		{
			bool spawned = false;

			foreach (int clothingID : clothingIDs)
			{
				IEntity clothing = inventory.Get(clothingID);
				if (clothing)
				{
					BaseInventoryStorageComponent clothingStorage = BaseInventoryStorageComponent.Cast(clothing.FindComponent(BaseInventoryStorageComponent));
					if (clothingStorage)
					{
						if (inventoryManager.CanInsertResourceInStorage(item, clothingStorage))
							spawned = inventoryManager.TrySpawnPrefabToStorage(item, clothingStorage);
						
						if (!spawned && clothingID == CRF_EGearscriptClothing.VEST) // unable to insert directly into some vests storage comp, so we just let the item/inventoryManager decide (99% of the time it's a vest)
							spawned = inventoryManager.TrySpawnPrefabToStorage(item);
						
						if (spawned)
							break;
					};
				};
			}
			
			if (!spawned) // One final effort is all that remains
				spawned = inventoryManager.TrySpawnPrefabToStorage(item);
			
			if (!spawned)
				CRF_LoggingHelper.LogItemError(item, inventoryManager.GetOwner());
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all gear from an entity
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
			GetGame().GetCallqueue().Call(DeleteRemainingItems, otherItems);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Delete remaining non-weapon items after weapon cleanup
	//! \param[in] items Array of items to delete
	protected void DeleteRemainingItems(array<IEntity> items)
	{
		foreach (IEntity item : items)
		{
			if (item)
				SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 DEBUGGING METHODS (ONLY CALLED IN WORKBENCH)
//=============================================================================================================================================================================================================================================================================================================================================================
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	protected void DEBUG_SpawnAllRoleCharacters()
	{
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		
		// Setup Faction/Role Arrays
		array<FactionKey> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		array<ref CRF_RoleConfig> roleArray = m_RolesConfig.GetRoleConfigArray();
		RandomGenerator rng = new RandomGenerator;
		
		foreach (FactionKey factionKey : factionKeys)
		{
			CRF_SpawnPointData initialSpawnData = respawnManager.FindInitalFactionSpawnpoint(factionKey);
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
			
			if (initialSpawnData && faction)
			{
				IEntity spawnPointEnt = CRF_EntityHelper.GetEntityFromRplId(initialSpawnData.GetSpawnPointEntity());
				if (spawnPointEnt)
				{
					EntitySpawnParams spawnParams = new EntitySpawnParams();
					spawnParams.TransformMode = ETransformMode.WORLD;
					spawnPointEnt.GetWorldTransform(spawnParams.Transform);
					
					foreach (ref CRF_RoleConfig roleConfig : roleArray)
						GetGame().GetCallqueue().CallLater(DEBUG_SpawnThenDeleteCharacter, rng.RandInt(500, 12000), false, spawnParams, roleConfig.m_RoleResource, faction);
				};
			};
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DEBUG_SpawnThenDeleteCharacter(EntitySpawnParams spawnParams, ResourceName characterResource, Faction faction)
	{
		CRF_PlayerCharacter playerCharacter = CRF_PlayerCharacter.Cast(
			GetGame().SpawnEntityPrefab(m_ResourceCache.GetCachedResource(characterResource), GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return;
		
		// Update character faction
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		facComp.SetAffiliatedFaction(faction);
		
		GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 250, false, playerCharacter);
	}
	#endif
	
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
