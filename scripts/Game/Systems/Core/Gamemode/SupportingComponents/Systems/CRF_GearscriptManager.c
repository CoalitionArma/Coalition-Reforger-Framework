class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected ref RandomGenerator m_RNG = new RandomGenerator();
	
	ref CRF_GearScriptEquipmentConfig m_EquipmentConfig;
	ref CRF_GearScriptWeaponsConfig m_WeaponConfig;
	protected CRF_Gamemode m_Gamemode;

	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//------------------------------------------------------------------------------------------------
	static CRF_GearscriptManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GearscriptManager.Cast(gameMode.FindComponent(CRF_GearscriptManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on in-game post init
		// Is the the right way to do this? WHO KNOWS !
		if (!GetGame().InPlayMode())
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_WeaponConfig = CRF_GearScriptWeaponsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{AF5B2639B4B12580}Configs/Gearscripts/CRF_Global_Weapons_Config.conf").GetResource().ToBaseContainer()));
		m_EquipmentConfig = CRF_GearScriptEquipmentConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{DE26DF4B9B934889}Configs/Gearscripts/CRF_Global_Equipment_Config.conf").GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetGearScriptResource(string factionKey)
	{
		if (!GetGearScriptSettings(factionKey))
		{
			PrintFormat("NO GEARSCRIPT ASSIGNED TO: %1", factionKey, LogLevel.WARNING);
			return "";
		};

		return GetGearScriptSettings(factionKey).m_rGearScript;
	}

	//------------------------------------------------------------------------------------------------
	CRF_GearScriptContainer GetGearScriptSettings(string factionKey)
	{
		CRF_GearScriptContainer gearScriptContainer;

		switch (factionKey)
		{
			case "BLUFOR" : {gearScriptContainer = m_Gamemode.m_BLUFORGearScriptSettings; break; }
			case "OPFOR" : {gearScriptContainer = m_Gamemode.m_OPFORGearScriptSettings; break; }
			case "INDFOR" : {gearScriptContainer = m_Gamemode.m_INDFORGearScriptSettings; break; }
			case "CIV" : {gearScriptContainer = m_Gamemode.m_CIVILIANGearScriptSettings; break; }
		}

		return gearScriptContainer;
	}

	//------------------------------------------------------------------------------------------------
	void SetupAddGearToEntity(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!CRF_RoleHelper.IsValidGearscriptResource(resourceNameToScan) || !entity)
			return;

		string factionKey;

		switch (true)
		{
			case(resourceNameToScan.Contains("BLUFOR")): 
				factionKey = "BLUFOR"; 
				break;
			case(resourceNameToScan.Contains("OPFOR")): 
				factionKey = "OPFOR"; 
				break;
			case(resourceNameToScan.Contains("INDFOR")): 
				factionKey = "INDFOR"; 
				break;
			case(resourceNameToScan.Contains("CIV")): 
				factionKey = "CIV"; 
				break;
		}
		
		if(factionKey.IsEmpty())
			return;

		ResourceName gearScriptResourceName = GetGearScriptResource(factionKey);
		CRF_GearScriptContainer gearScriptSettings = GetGearScriptSettings(factionKey);

		if (gearScriptResourceName.IsEmpty() || !gearScriptSettings)
			return;

		// GET COMPONENTS
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!inventory || !inventoryManager)
		{
			Print(string.Format("CRF GEAR SCRIPT ERROR: %1 DOESN'T HAVE COMPONENTS WE NEED!", entity), LogLevel.ERROR);
			return;
		}

		// GET ROLE
		int role = CRF_RoleHelper.StringToRole(CRF_RoleHelper.PrefabToRole(resourceNameToScan));

		// CLEAR ENTITY
		array<IEntity> items = {};
		array<IEntity> itemsRoot = {};
		inventoryManager.GetAllItems(items, inventory);
		inventoryManager.GetAllRootItems(itemsRoot);

		items.InsertAll(itemsRoot);

		foreach (IEntity item : items)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}

		// ADD CLOTHING/WEAPONS/ITEMS
		GetGame().GetCallqueue().CallLater(AddGearToEntity, m_RNG.RandInt(125, 250), false, entity, role, gearScriptResourceName, gearScriptSettings, inventory, inventoryManager);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddGearToEntity(IEntity entity, int role, ResourceName gearScriptResourceName, CRF_GearScriptContainer gearScriptSettings, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();

		// CLOTHING
		if (gearConfig.m_DefaultFactionGear)
		{
			foreach (CRF_Clothing clothing : gearConfig.m_DefaultFactionGear.m_DefaultClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, false, spawnParams, inventory, inventoryManager);
			}
		}
		
		// CUSTOM CLOTHING
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
		
		bool customWeaponsSet = false;
		
		// CUSTOM WEAPONS
		if (gearConfig.m_CustomFactionGear)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
			{
				if (customGear.m_Role != role)
					continue;
				
				if (!customGear.m_PrimaryWeapon.IsEmpty())
				{
					CRF_Weapon_Class primary = SelectRandomWeapon(customGear.m_PrimaryWeapon);
					SpawnWeapon(primary.m_Weapon, primary.m_Attachments, primary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				}
				
				if (!customGear.m_SecondaryWeapon.IsEmpty())
				{
					CRF_Weapon_Class secondary = SelectRandomWeapon(customGear.m_SecondaryWeapon);
					SpawnWeapon(secondary.m_Weapon, secondary.m_Attachments, secondary.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				}
				
				if (!customGear.m_Pistol.IsEmpty())
				{
					CRF_Weapon_Class pistol = SelectRandomWeapon(customGear.m_Pistol);
					SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
					customWeaponsSet = true;
				}
			}
		}

		// DEFAULT WEAPONS
		if (gearConfig.m_FactionWeapons && !customWeaponsSet)
		{	
			if (m_WeaponConfig.m_aRolesThatGetRifles.Contains(role))
			{
				CRF_Weapon_Class rifle = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Rifle);
				SpawnWeapon(rifle.m_Weapon, rifle.m_Attachments, rifle.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (m_WeaponConfig.m_aRolesThatGetRifleUGLs.Contains(role))
			{
				CRF_Weapon_Class rifleUGL = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_RifleUGL);
				SpawnWeapon(rifleUGL.m_Weapon, rifleUGL.m_Attachments, rifleUGL.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (m_WeaponConfig.m_aRolesThatGetCarbines.Contains(role)) 	
			{
				CRF_Weapon_Class carbine = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Carbine);
				SpawnWeapon(carbine.m_Weapon, carbine.m_Attachments, carbine.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if(m_WeaponConfig.m_aRolesThatGetPistols.Contains(role))
			{
				CRF_Weapon_Class pistol = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Pistol);
				SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (m_WeaponConfig.m_aRolesThatGetSnipers.Contains(role) && gearConfig.m_FactionWeapons.m_Sniper)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_Sniper.m_Weapon, gearConfig.m_FactionWeapons.m_Sniper.m_Attachments, gearConfig.m_FactionWeapons.m_Sniper.m_MagazineArray, spawnParams, inventory, inventoryManager);
			
			if (m_WeaponConfig.m_aRolesThatGetARs.Contains(role) && gearConfig.m_FactionWeapons.m_AR)	
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AR.m_Weapon, gearConfig.m_FactionWeapons.m_AR.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AR.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (m_WeaponConfig.m_aRolesThatGetMMGs.Contains(role) && gearConfig.m_FactionWeapons.m_MMG) 
				SpawnWeapon(gearConfig.m_FactionWeapons.m_MMG.m_Weapon, gearConfig.m_FactionWeapons.m_MMG.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray), spawnParams, inventory, inventoryManager);	
			
			if (m_WeaponConfig.m_aRolesThatGetAT.Contains(role) && gearConfig.m_FactionWeapons.m_AT)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AT.m_Weapon, gearConfig.m_FactionWeapons.m_AT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AT.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (m_WeaponConfig.m_aRolesThatGetMAT.Contains(role) && gearConfig.m_FactionWeapons.m_MAT)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_MAT.m_Weapon, gearConfig.m_FactionWeapons.m_MAT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (m_WeaponConfig.m_aRolesThatGetHAT.Contains(role) && gearConfig.m_FactionWeapons.m_HAT)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_HAT.m_Weapon, gearConfig.m_FactionWeapons.m_HAT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray), spawnParams, inventory, inventoryManager);	
			
			if (m_WeaponConfig.m_aRolesThatGetAA.Contains(role) && gearConfig.m_FactionWeapons.m_AA)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AA.m_Weapon, gearConfig.m_FactionWeapons.m_AA.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AA.m_MagazineArray), spawnParams, inventory, inventoryManager);
				
			if (m_WeaponConfig.m_aRolesThatGetHMGs.Contains(role) && gearConfig.m_FactionWeapons.m_HMG)
				SpawnWeapon(gearConfig.m_FactionWeapons.m_HMG.m_Weapon, gearConfig.m_FactionWeapons.m_HMG.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray), spawnParams, inventory, inventoryManager);
		}
		
		// CUSTOM GEAR
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
		
		// DEFAULT GEAR
		if (gearConfig.m_DefaultFactionGear)
		{	
			//Who we give Leadership Radios
			if (gearScriptSettings.m_bEnableLeadershipRadios && m_EquipmentConfig.m_aRolesThatGetLeadershipRadios.Contains(role))
				AddInventoryItem(gearScriptSettings.m_rLeadershipRadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give GI Radios
			if (gearScriptSettings.m_bEnableGIRadios && !m_EquipmentConfig.m_aRolesThatGetLeadershipRadios.Contains(role))
				AddInventoryItem(gearScriptSettings.m_rGIRadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give RTO Radios
			if (gearScriptSettings.m_bEnableRTORadios && m_EquipmentConfig.m_aRolesThatGetRTORadios.Contains(role))
				AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give Leadership Binos
			if (gearConfig.m_DefaultFactionGear.m_bEnableLeadershipBinoculars && m_EquipmentConfig.m_aRolesThatGetLeadershipBinos.Contains(role))
				AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
			
			//Who we give Assistant Binos
			if (gearConfig.m_DefaultFactionGear.m_bEnableAssistantBinoculars && m_EquipmentConfig.m_aRolesThatGetAssistantBinos.Contains(role))
				AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager, role);

			//Who we give extra medical items
			if (m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role))
			{
				foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultMedicMedicalItems)
				{
					AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
				}
			}
			
			//Extra magazines
			if (m_EquipmentConfig.m_aRolesThatGetAssistantMags.Contains(role))
			{
				array<ref CRF_Spec_Magazine_Class> magazineArray = {};

				switch (role)
				{
					case CRF_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN	: {if(!gearConfig.m_FactionWeapons.m_AR 	|| !gearConfig.m_FactionWeapons.m_AR.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AR.m_MagazineArray; 	break;}
					case CRF_EGearRole.ASSISTANT_MEDIUM_MACHINEGUN		: {if(!gearConfig.m_FactionWeapons.m_MMG 	|| !gearConfig.m_FactionWeapons.m_MMG.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray; 	break;}
					case CRF_EGearRole.ASSISTANT_HEAVY_MACHINEGUN 		: {if(!gearConfig.m_FactionWeapons.m_HMG 	|| !gearConfig.m_FactionWeapons.m_HMG.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray; 	break;}
					case CRF_EGearRole.ASSISTANT_MEDIUM_ANTITANK 		: {if(!gearConfig.m_FactionWeapons.m_MAT 	|| !gearConfig.m_FactionWeapons.m_MAT.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray;	break;}
					case CRF_EGearRole.ASSISTANT_HEAVY_ANTITANK 		: {if(!gearConfig.m_FactionWeapons.m_HAT 	|| !gearConfig.m_FactionWeapons.m_HAT.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray; 	break;}
					case CRF_EGearRole.ASSISTANT_ANTI_AIR 			: {if(!gearConfig.m_FactionWeapons.m_AA 	|| !gearConfig.m_FactionWeapons.m_AA.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AA.m_MagazineArray; 	break;}
					case CRF_EGearRole.ASSISTANT_RIFLEMAN_ANTITANK 	: {if(!gearConfig.m_FactionWeapons.m_AT 	|| !gearConfig.m_FactionWeapons.m_AT.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AT.m_MagazineArray; 	break;}
				}

				foreach (ref CRF_Spec_Magazine_Class magazine : magazineArray)
				{
					AddInventoryItem(magazine.m_Magazine, magazine.m_AssistantMagazineCount, spawnParams, inventory, inventoryManager, role, true);
				}
			}

			//What everyone gets
			foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultInventoryItems)
			{
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role, gearConfig.m_DefaultFactionGear.m_bEnableMedicFrags);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected CRF_Weapon_Class SelectRandomWeapon(array<ref CRF_Weapon_Class> weaponArray)
	{
		if (!weaponArray || weaponArray.IsEmpty())
			return null;

		CRF_Weapon_Class weaponToSpawnContainer = weaponArray.GetRandomElement();

		if (!weaponToSpawnContainer)
			return null;

		return weaponToSpawnContainer;
	}
	
	//------------------------------------------------------------------------------------------------
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
	protected void SpawnWeapon(ResourceName weaponResource, array<ResourceName> attatchementResources, array<ref CRF_Magazine_Class> magazineArray, EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(weaponResource.IsEmpty())
			return;
		
		bool successfulSpawn = inventoryManager.TrySpawnPrefabToStorage(weaponResource, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);

		if (!successfulSpawn)
		{
			Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT WEAPON INTO ENTITY: %1", inventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print(" ", LogLevel.ERROR);
			Print("CRF GEAR SCRIPT ERROR: INVALID WEAPON ITEM!", LogLevel.ERROR);
			Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
			return;
		};

		if (magazineArray != null)
		{
			for (int i = 0; i < magazineArray.Count(); i++)
			{
				CRF_Magazine_Class magazine = magazineArray[i];
				if (magazine != null)
				{
					AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount, spawnParams, inventory, inventoryManager);
				}
			}
		}
		
		GetGame().GetCallqueue().CallLater(AddAttachments, 1000, false, weaponResource, attatchementResources, spawnParams, inventoryManager);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddAttachments(ResourceName weaponResource, array<ResourceName> attatchementResources, EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!inventoryManager || !attatchementResources || attatchementResources.IsEmpty())
			return;
			
		ChimeraCharacter character = ChimeraCharacter.Cast(inventoryManager.GetOwner());
		if (character == null)
			return;
			
		BaseWeaponManagerComponent weaponManager = character.GetCharacterController().GetWeaponManagerComponent();
		if (weaponManager == null)
			return;

		IEntity weapon = null;
		array<IEntity> outWeapons = {};
		weaponManager.GetWeaponsList(outWeapons);

		foreach (IEntity weaponToCheck : outWeapons)
		{
			if (weaponToCheck.GetPrefabData().GetPrefabName() == weaponResource)
			{
				weapon = weaponToCheck;
				break;
			}
		}

		if (weapon == null)
			return;

		array<AttachmentSlotComponent> attatchmentSlotArray = {};
		BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetAttachments(attatchmentSlotArray);

		foreach (ResourceName attachment : attatchementResources)
		{
			AttachmentSlotComponent verifyAttachmentSlot = null;
			IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachment), GetGame().GetWorld(), spawnParams);
			inventoryManager.TryInsertItem(attachmentSpawned, EStoragePurpose.PURPOSE_ATTACHMENT_PROXY);

			foreach (AttachmentSlotComponent attachmentSlot : attatchmentSlotArray)
			{
				if (attachmentSlot.CanSetAttachment(attachmentSpawned))
				{
					IEntity attachedEntity = attachmentSlot.GetAttachedEntity();
					if (attachedEntity != null)
					{
						if (attachedEntity != attachmentSpawned)
						{
							delete attachedEntity;
						}
					}

					attachmentSlot.SetAttachment(attachmentSpawned);
					verifyAttachmentSlot = attachmentSlot;
					break;
				}
			}

			if (verifyAttachmentSlot == null)
			{
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ATTACHMENT: %1", attachment), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", weapon.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print(" ", LogLevel.ERROR);
				Print("CRF GEAR SCRIPT ERROR: INVALID ATTACHMENT ITEM FOR WEAPON!", LogLevel.ERROR);
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				delete attachmentSpawned;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, int role, bool deletePreviousItems, EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
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
			BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
			if (oldStorage)
			{
				array<IEntity> outItems = {};
				oldStorage.GetAll(outItems);
				
				int itemCount = outItems.Count();
				for (int i = 0; i < itemCount; i++)
				{
					IEntity item = outItems[i];
					if (item == null)
						continue;
						
					if (InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)) == null)
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

		// Add new clothing if exists
		if (clothing != "")
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothing), GetGame().GetWorld(), spawnParams);
			inventoryManager.TryReplaceItem(resourceSpawned, inventory, slotInt);

			if (inventoryManager.Contains(resourceSpawned) == false)
			{
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT CLOTHING: %1", clothing), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", inventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print(" ", LogLevel.ERROR);
				Print("CRF GEAR SCRIPT ERROR: INVALID CLOTHING ITEM!", LogLevel.ERROR);
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			}
		}

		// Handle previously removed items
		int removedItemCount = removedItems.Count();
		for (int j = 0; j < removedItemCount; j++)
		{
			IEntity oldItem = removedItems[j];
			if (deletePreviousItems == false)
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
	protected void AddInventoryItem(ResourceName item, int itemAmmount, EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, int role = 0, bool enableMedicFrags = false, bool isAssistant = false)
	{
		if (item.IsEmpty() || itemAmmount <= 0)
			return;

		for (int i = 1; i <= itemAmmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);

			if (!resourceSpawned)
				continue;

			bool isThrowable = (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)) && WEAPON_TYPES_THROWABLE.Contains(WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType()));

			if (!enableMedicFrags && m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role) && (isThrowable && WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
				continue;
			};

			if (inventoryManager.CanInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT))
			{
				BaseInventoryStorageComponent storageComp = inventoryManager.FindStorageForItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
				inventoryManager.EquipAny(storageComp, resourceSpawned, -1);
				continue;
			};

			InsertInventoryItem(resourceSpawned, inventory, inventoryManager, role, isAssistant, isThrowable);

			if (isThrowable)
			{
				CharacterGrenadeSlotComponent grenadeSlot = CharacterGrenadeSlotComponent.Cast(inventoryManager.GetOwner().FindComponent(CharacterGrenadeSlotComponent));

				if (grenadeSlot && !grenadeSlot.GetWeaponEntity())
				{
					if (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE)
					{
						grenadeSlot.SetWeapon(resourceSpawned);
					} else {
						if (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_SMOKEGRENADE && !grenadeSlot.GetWeaponEntity())
							grenadeSlot.SetWeapon(resourceSpawned);
					};
				};
			};
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void InsertInventoryItem(IEntity item, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, int role = 0, bool isAssistant = false, bool isThrowable = false)
	{
		if (!item)
			return;

		TIntArray clothingIDs = FilterItemToClothing(item, role, isAssistant, isThrowable);

		// Try and insert into select clothing at first
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
		};

		// if we cant do select clothing, just slap it in wherever
		if (!inventoryManager.Contains(item))
			inventoryManager.TryInsertItem(item);

		if (!inventoryManager.Contains(item) || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
		{
			Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ITEM: %1", item.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", inventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print(" ", LogLevel.ERROR);
			Print("CRF GEAR SCRIPT ERROR: NOT ENOUGH SPACE IN INVENTORY/INVALID INVENTORY ITEM!", LogLevel.ERROR);
			Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		};
	}

	//------------------------------------------------------------------------------------------------
	TIntArray FilterItemToClothing(IEntity item, int role = 0, bool isAssistant = false, bool isThrowable = false)
	{
		array<int> clothingIDs = {};

		// Any magazine
		if (MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)))
			clothingIDs = {
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.SHIRT
			};
		else // Any Non-magazine
			clothingIDs = {
				CRF_EClothingType.SHIRT, 
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};

		// Any medical item
		if (m_EquipmentConfig.m_aRolesThatGetMedicalItems.Contains(role) && SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent)))
			clothingIDs = {
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
			};

		// Any pistol ammo
		if ((InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO) || isThrowable)
			clothingIDs = {
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};

		// Any radio
		if (BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent)))
			clothingIDs = {
				CRF_EClothingType.PANTS, 
				CRF_EClothingType.SHIRT, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST, 
				CRF_EClothingType.BACKPACK
			};

		// Any Assistant Mags
		if (isAssistant && MagazineComponent.Cast(item.FindComponent(MagazineComponent)))
			clothingIDs = {
				CRF_EClothingType.BACKPACK,
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
		};

		// Check if item is explosives related
		SCR_DetonatorGadgetComponent detonator = SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent));
		SCR_ExplosiveChargeComponent explosives = SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent));
		SCR_MineWeaponComponent mine = SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent));
		SCR_RepairSupportStationComponent engTool = SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent));
		SCR_HealSupportStationComponent medTool = SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
		if (detonator || explosives || mine || engTool || medTool)
			clothingIDs = {
				CRF_EClothingType.BACKPACK, 
				CRF_EClothingType.VEST, 
				CRF_EClothingType.ARMOREDVEST
		};

		return clothingIDs;
	}
};