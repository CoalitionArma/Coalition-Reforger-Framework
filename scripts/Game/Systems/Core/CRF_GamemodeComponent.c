class CRF_GamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeComponent : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Globally Used Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------

	protected ref RandomGenerator m_RNG = new RandomGenerator();

	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GamemodeComponent.Cast(gameMode.FindComponent(CRF_GamemodeComponent));
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
		
		m_WeaponConfig = CRF_GearScriptWeaponsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{AF5B2639B4B12580}Configs/Gearscripts/CRF_Global_Weapons_Config.conf").GetResource().ToBaseContainer()));
		m_EquipmentConfig = CRF_GearScriptEquipmentConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{DE26DF4B9B934889}Configs/Gearscripts/CRF_Global_Equipment_Config.conf").GetResource().ToBaseContainer()));

		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		GetGame().GetCallqueue().CallLater(AddMsgAction, 0, false);

		#ifdef WORKBENCH
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(UpdatePlayerGearScriptsArray, m_RNG.RandInt(10000, 20000), true);

			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			GetGame().GetCallqueue().CallLater(UpdatePlayerGearScriptsArray, m_RNG.RandInt(10000, 20000), true);

			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void WaitTillGameStart()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_GamemodeState.GAME)
			return;

		m_bSafeStartEnabled = !CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		GetGame().GetCallqueue().Remove(WaitTillGameStart);
		GetGame().GetCallqueue().CallLater(ToggleSafeStartServer, 1000, false, CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled);
	}

	void OnGamemodeStateChanged()
	{}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// GearScripts Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------

	const int HEADGEAR = 0;
	const int SHIRT = 1;
	const int ARMOREDVEST = 2;
	const int PANTS = 3;
	const int BOOTS = 4;
	const int BACKPACK = 5;
	const int VEST = 6;
	const int HANDWEAR = 7;
	const int HEAD = 8;
	const int EYES = 9;
	const int EARS = 10;
	const int FACE = 11;
	const int NECK = 12;
	const int EXTRA1 = 13;
	const int EXTRA2 = 14;
	const int WAIST = 15;
	const int EXTRA3 = 16;
	const int EXTRA4 = 17;
	
	ref CRF_GearScriptEquipmentConfig m_EquipmentConfig;
	ref CRF_GearScriptWeaponsConfig m_WeaponConfig;

	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};

	//------------------------------------------------------------------------------------------------

	// A array we use primarily for replication of m_mAllPlayerGearScriptsMap to clients.
	[RplProp(onRplName: "UpdateLocalPlayerGearScriptsMap")]
	protected ref array<string> m_aAllPlayerGearScriptsArray = {};

	// A hashmap that is modified only on each client by a .BumpMe from the authority.
	protected ref map<string, string> m_mAllPlayerGearScriptsMap = new map<string, string>();

	//------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);

		if (RplSession.Mode() == RplMode.Client)
			return;

		if (entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			return;

		
		GetGame().GetCallqueue().CallLater(CheckWorldValid, 150, false, entity);
	}

	//------------------------------------------------------------------------------------------------
	void CheckWorldValid(IEntity entity)
	{
		if (!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(CheckWorldValid, 500, false, entity);
			return;
		}

		GetGame().GetCallqueue().CallLater(SetupAddGearToEntity, m_RNG.RandInt(250, 500), false, entity, entity.GetPrefabData().GetPrefabName());
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
			case "BLUFOR" : {gearScriptContainer = CRF_Gamemode.GetInstance().m_BLUFORGearScriptSettings; break; }
			case "OPFOR" : {gearScriptContainer = CRF_Gamemode.GetInstance().m_OPFORGearScriptSettings; break; }
			case "INDFOR" : {gearScriptContainer = CRF_Gamemode.GetInstance().m_INDFORGearScriptSettings; break; }
			case "CIV" : {gearScriptContainer = CRF_Gamemode.GetInstance().m_CIVILIANGearScriptSettings; break; }
		}

		return gearScriptContainer;
	}

	//------------------------------------------------------------------------------------------------
	// Functions to replicate and store values to each clients m_mAllPlayerGearScriptsMap
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override protected void OnGameEnd()
	{
		super.OnGameEnd();

		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;

		GetGame().GetCallqueue().Remove(UpdatePlayerGearScriptsArray);
	}

	//------------------------------------------------------------------------------------------------
	string ReturnPlayerGearScriptsMapValue(int playerID, string key)
	{
		// Get the players key
		key = string.Format("%1%2", playerID, key);
		return m_mAllPlayerGearScriptsMap.Get(key);
	}

	//------------------------------------------------------------------------------------------------
	void SetPlayerGearScriptsMapValue(string value, int playerID, string key)
	{
		// Get the players key
		key = string.Format("%1%2", playerID, key);
		m_mAllPlayerGearScriptsMap.Set(key, value);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerGearScriptsArray()
	{
		// Create a temp array so we arent broadcasting for each change to m_aAllPlayerGearScriptsArray.
		protected ref array<string> tempPlayerArray = {};

		// Fill tempPlayerArray with all keys and values in m_mAllPlayerGearScriptsMap.
		for (int i = 0; i < m_mAllPlayerGearScriptsMap.Count(); i++)
		{
			string key = m_mAllPlayerGearScriptsMap.GetKey(i);
			string value = m_mAllPlayerGearScriptsMap.Get(key);

			tempPlayerArray.Insert(string.Format("%1~%2", key, value));
		};

		// Replicate m_aAllPlayerGearScriptsArray to all clients.
		m_aAllPlayerGearScriptsArray = tempPlayerArray;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateLocalPlayerGearScriptsMap()
	{
		// Fill m_mAllPlayerGearScriptsMap with all keys and values from authorities m_mAllPlayerGearScriptsMap.
		foreach (string playerKeyAndValueToSplit : m_aAllPlayerGearScriptsArray)
		{
			array<string> playerKeyAndValueArray = {};
			playerKeyAndValueToSplit.Split("~", playerKeyAndValueArray, false);
			m_mAllPlayerGearScriptsMap.Set(playerKeyAndValueArray[0], playerKeyAndValueArray[1]);
		};
	}

	//------------------------------------------------------------------------------------------------
	// Functions to for Gear Script
	//------------------------------------------------------------------------------------------------
	void SetupAddGearToEntity(IEntity entity, ResourceName resourceNameToScan)
	{
		if (!resourceNameToScan.Contains("CRF_GS_") || !entity)
			return;

		string factionKey;

		switch (true)
		{
			case(resourceNameToScan.Contains("BLUFOR")) 	: {factionKey = "BLUFOR"; break; }
			case(resourceNameToScan.Contains("OPFOR")) 		: {factionKey = "OPFOR"; break; }
			case(resourceNameToScan.Contains("INDFOR")) 	: {factionKey = "INDFOR"; break; }
			case(resourceNameToScan.Contains("CIV")) 		: {factionKey = "CIV"; break; }
		}

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
		array<string> value = {};
		resourceNameToScan.Split("_", value, true);

		string role = "_" + value[3] + "_" + value[4];

		role.Split(".", value, true);
		role = value[0];

		// CLEAR CHARACTER
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
		GetGame().GetCallqueue().CallLater(AddGearToEntity, m_RNG.RandInt(100, 250), false, entity, role, gearScriptResourceName, gearScriptSettings, inventory, inventoryManager);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddGearToEntity(IEntity entity, string role, ResourceName gearScriptResourceName, CRF_GearScriptContainer gearScriptSettings, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
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
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, spawnParams, inventory, inventoryManager);
			}
		}
		
		bool customWeaponsSet = false;
		
		// CUSTOM WEAPONS
		if (gearConfig.m_CustomFactionGear)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
			{
				if (customGear.m_sRole != role)
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
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetRifles, role)) 	
				{
				CRF_Weapon_Class rifle = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Rifle);
				SpawnWeapon(rifle.m_Weapon, rifle.m_Attachments, rifle.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetRifleUGLs, role))
			{
				CRF_Weapon_Class rifleUGL = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_RifleUGL);
				SpawnWeapon(rifleUGL.m_Weapon, rifleUGL.m_Attachments, rifleUGL.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetCarbines, role)) 	
			{
				CRF_Weapon_Class carbine = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Carbine);
				SpawnWeapon(carbine.m_Weapon, carbine.m_Attachments, carbine.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if(CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetPistols, role))
			{
				CRF_Weapon_Class pistol = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Pistol);
				SpawnWeapon(pistol.m_Weapon, pistol.m_Attachments, pistol.m_MagazineArray, spawnParams, inventory, inventoryManager);
			};
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetSnipers, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_Sniper.m_Weapon, gearConfig.m_FactionWeapons.m_Sniper.m_Attachments, gearConfig.m_FactionWeapons.m_Sniper.m_MagazineArray, spawnParams, inventory, inventoryManager);
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetARs, role))	
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AR.m_Weapon, gearConfig.m_FactionWeapons.m_AR.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AR.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetMMGs, role)) 
				SpawnWeapon(gearConfig.m_FactionWeapons.m_MMG.m_Weapon, gearConfig.m_FactionWeapons.m_MMG.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray), spawnParams, inventory, inventoryManager);	
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetAT, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AT.m_Weapon, gearConfig.m_FactionWeapons.m_AT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AT.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetMAT, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_MAT.m_Weapon, gearConfig.m_FactionWeapons.m_MAT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray), spawnParams, inventory, inventoryManager);
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetHAT, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_HAT.m_Weapon, gearConfig.m_FactionWeapons.m_HAT.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray), spawnParams, inventory, inventoryManager);	
			
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetAA, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_AA.m_Weapon, gearConfig.m_FactionWeapons.m_AA.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AA.m_MagazineArray), spawnParams, inventory, inventoryManager);
				
			if (CheckIfRoleInArray(m_WeaponConfig.m_aRolesThatGetHMGs, role))
				SpawnWeapon(gearConfig.m_FactionWeapons.m_HMG.m_Weapon, gearConfig.m_FactionWeapons.m_HMG.m_Attachments, ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray), spawnParams, inventory, inventoryManager);
		} else
			Print(string.Format("CRF GEAR SCRIPT ERROR: NO WEAPONS SET: %1", gearScriptResourceName), LogLevel.ERROR);

		// CUSTOM GEAR
		if (gearConfig.m_CustomFactionGear)
		{
			foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
			{
				if (customGear.m_sRole != role)
					continue;
		
				foreach (CRF_Clothing clothing : customGear.m_Clothing)
				{
					UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_iClothingType, role, spawnParams, inventory, inventoryManager);
				}
		
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
			if (gearScriptSettings.m_bEnableLeadershipRadios && CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetLeadershipRadios, role))
				AddInventoryItem(gearScriptSettings.m_rLeadershipRadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give GI Radios
			if (gearScriptSettings.m_bEnableGIRadios && !CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetLeadershipRadios, role))
				AddInventoryItem(gearScriptSettings.m_rGIRadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give RTO Radios
			if (gearScriptSettings.m_bEnableRTORadios && CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetRTORadios, role))
				AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1, spawnParams, inventory, inventoryManager);

			//Who we give Leadership Binos
			if (gearConfig.m_DefaultFactionGear.m_bEnableLeadershipBinoculars && CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetLeadershipBinos, role))
				AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
			
			//Who we give Assistant Binos
			if (gearConfig.m_DefaultFactionGear.m_bEnableAssistantBinoculars && CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetAssistantBinos, role))
				AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager, role);

			//Who we give extra medical items
			if (CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetMedicalItems, role))
			{
				foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultMedicMedicalItems)
				{
					AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
				}
			}
			
			//Extra magazines
			if (CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetAssistantMags, role))
			{
				array<ref CRF_Spec_Magazine_Class> magazineArray = {};

				switch (role)
				{
					case "_AAR_P" 	: {if(!gearConfig.m_FactionWeapons.m_AR 	|| !gearConfig.m_FactionWeapons.m_AR.m_Weapon) 		{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AR.m_MagazineArray; 	break;}
					case "_AMMG_P" 	: {if(!gearConfig.m_FactionWeapons.m_MMG 	|| !gearConfig.m_FactionWeapons.m_MMG.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray; 	break;}
					case "_AHMG_P" 	: {if(!gearConfig.m_FactionWeapons.m_HMG 	|| !gearConfig.m_FactionWeapons.m_HMG.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray; 	break;}
					case "_AMAT_P" 	: {if(!gearConfig.m_FactionWeapons.m_MAT 	|| !gearConfig.m_FactionWeapons.m_MAT.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray;	break;}
					case "_AHAT_P" 	: {if(!gearConfig.m_FactionWeapons.m_HAT 	|| !gearConfig.m_FactionWeapons.m_HAT.m_Weapon) 	{return;};  magazineArray = gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray; 	break;}
					case "_AAA_P" 	: {if(!gearConfig.m_FactionWeapons.m_AA 	|| !gearConfig.m_FactionWeapons.m_AA.m_Weapon) 		{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AA.m_MagazineArray; 	break;}
					case "_AAT_P" 	: {if(!gearConfig.m_FactionWeapons.m_AT 	|| !gearConfig.m_FactionWeapons.m_AT.m_Weapon) 		{return;};  magazineArray = gearConfig.m_FactionWeapons.m_AT.m_MagazineArray; 	break;}
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
		} else
			Print(string.Format("CRF GEAR SCRIPT ERROR: NO DEFAULT GEAR SET: %1", gearScriptResourceName), LogLevel.ERROR);
	}
	
	//------------------------------------------------------------------------------------------------
	bool CheckIfRoleInArray(array<ref CRF_Role> arrayToCheck, string role)
	{
		foreach(ref CRF_Role roleContiner : arrayToCheck)
		{
			if(roleContiner.m_sRole == role)
				return true;
		}
		return false;
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
		foreach (CRF_Spec_Magazine_Class specMagazine : specMagazineArray)
		{
			ref CRF_Magazine_Class tempMag = specMagazine;
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

		foreach (CRF_Magazine_Class magazine : magazineArray)
		{
			AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount, spawnParams, inventory, inventoryManager);
		}
		
		GetGame().GetCallqueue().CallLater(AddAttachments, 1000, false, weaponResource, attatchementResources, spawnParams, inventoryManager);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddAttachments(ResourceName weaponResource, array<ResourceName> attatchementResources, EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		BaseWeaponManagerComponent weaponManager = ChimeraCharacter.Cast(inventoryManager.GetOwner()).GetCharacterController().GetWeaponManagerComponent();

		if (!weaponManager || !attatchementResources || attatchementResources.IsEmpty())
			return;

		IEntity weapon;
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

		if (!weapon)
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
					if (attachmentSlot.GetAttachedEntity() != attachmentSpawned)
						delete attachmentSlot.GetAttachedEntity();

					attachmentSlot.SetAttachment(attachmentSpawned);
					verifyAttachmentSlot = attachmentSlot;
					break;
				};
			}

			if (!verifyAttachmentSlot)
			{
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ATTACHMENT: %1", attachment), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", weapon.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print(" ", LogLevel.ERROR);
				Print("CRF GEAR SCRIPT ERROR: INVALID ATTACHMENT ITEM FOR WEAPON!", LogLevel.ERROR);
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				delete attachmentSpawned;
			};
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, int slotInt, string role, EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (clothingArray.IsEmpty() || slotInt == -1)
			return;

		array<IEntity> removedItems = {};
		IEntity previousClothing = inventory.Get(slotInt);
		ResourceName clothing = clothingArray.GetRandomElement();

		if (previousClothing != null)
		{
			BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
			if (oldStorage)
			{
				array<IEntity> outItems = {};
				oldStorage.GetAll(outItems);
				foreach (IEntity item : outItems)
				{
					if (!item || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)) || SCR_EquipmentStorageComponent.Cast(item.FindComponent(SCR_EquipmentStorageComponent)) || SCR_UniversalInventoryStorageComponent.Cast(item.FindComponent(SCR_UniversalInventoryStorageComponent)) || BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent)))
						continue;

					inventoryManager.TryRemoveItemFromStorage(item, oldStorage);
					removedItems.Insert(item);
				}
			};

			inventoryManager.TryRemoveItemFromStorage(previousClothing, inventory);
			SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
		};

		if (!clothing.IsEmpty())
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothing), GetGame().GetWorld(), spawnParams);
			inventoryManager.TryReplaceItem(resourceSpawned, inventory, slotInt);

			if (!inventoryManager.Contains(resourceSpawned))
			{
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT CLOTHING: %1", clothing), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", inventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print(" ", LogLevel.ERROR);
				Print("CRF GEAR SCRIPT ERROR: INVALID CLOTHING ITEM!", LogLevel.ERROR);
				Print("--------------------------------------------------------------------------------", LogLevel.ERROR);
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			};
		}

		foreach (IEntity oldItem : removedItems)
		{
			InsertInventoryItem(oldItem, inventory, inventoryManager, role);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddInventoryItem(ResourceName item, int itemAmmount, EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, string role = "", bool enableMedicFrags = false, bool isAssistant = false)
	{
		if (item.IsEmpty() || itemAmmount <= 0)
			return;

		for (int i = 1; i <= itemAmmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), spawnParams);

			if (!resourceSpawned)
				continue;

			bool isThrowable = (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)) && WEAPON_TYPES_THROWABLE.Contains(WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType()));

			if (!enableMedicFrags && CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetMedicalItems, role) && (isThrowable && WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE))
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
	protected void InsertInventoryItem(IEntity item, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, string role = "", bool isAssistant = false, bool isThrowable = false)
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
	TIntArray FilterItemToClothing(IEntity item, string role = "", bool isAssistant = false, bool isThrowable = false)
	{
		array<int> clothingIDs = {};

		// Any magazine
		if (MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)))
			clothingIDs = {VEST, ARMOREDVEST, BACKPACK, PANTS, SHIRT};
		else // Any Non-magazine
			clothingIDs = {SHIRT, PANTS, VEST, ARMOREDVEST, BACKPACK};

		// Any medical item
		if (CheckIfRoleInArray(m_EquipmentConfig.m_aRolesThatGetMedicalItems, role) && SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent)))
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};

		// Any pistol ammo
		if ((InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO) || isThrowable)
			clothingIDs = {PANTS, VEST, ARMOREDVEST, BACKPACK};

		// Any radio
		if (BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent)))
			clothingIDs = {PANTS, SHIRT, VEST, ARMOREDVEST, BACKPACK};

		// Any Assistant Mags
		if (isAssistant && MagazineComponent.Cast(item.FindComponent(MagazineComponent)))
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};

		// Check if item is explosives related
		SCR_DetonatorGadgetComponent detonator = SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent));
		SCR_ExplosiveChargeComponent explosives = SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent));
		SCR_MineWeaponComponent mine = SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent));
		SCR_RepairSupportStationComponent engTool = SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent));
		SCR_HealSupportStationComponent medTool = SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
		if (detonator || explosives || mine || engTool || medTool)
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};

		return clothingIDs;
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Admin Menu Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------

	private Widget m_wSavedHintWidget;

	//------------------------------------------------------------------------------------------------
	// Admin Messaging
	//------------------------------------------------------------------------------------------------
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage_Callback);
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage_Callback);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
		CRF_ClientComponent.GetInstance().SendAdminMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
		if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
			return;

		CRF_ClientComponent.GetInstance().ReplyAdminMessage(data, true);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data)
	{
		Rpc(RpcAsk_SendAdminMessage, data);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_SendAdminMessage(string data)
	{
		if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerID, bool logAction)
	{
		Rpc(RpcAsk_ReplyAdminMessage, data, playerID, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_ReplyAdminMessage(string data, int playerID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("Reply to %1: %2", GetGame().GetPlayerManager().GetPlayerName(playerID), data), playerID, false);

		if (GetGame().GetPlayerController().GetPlayerId() != playerID)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;

		chatComponent.ShowMessage(string.Format("Admin: %1", data));
	}

	//------------------------------------------------------------------------------------------------
	// Respawn
	//------------------------------------------------------------------------------------------------
	void SpawnOnGroupServer(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		CRF_Gamemode.GetInstance().RespawnPlayer(playerId, spawnLocation, groupID);

		if (logAction)
			LogAdminAction(string.Format("%1 was respawned to %2", GetGame().GetPlayerManager().GetPlayerName(playerId), SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID).m_faction), playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	void SendGroupIDToPlayer(int requestedId, int requesterID)
	{
		CRF_Gamemode gm = CRF_Gamemode.GetInstance();

		if (gm.m_aSlots.Find(requestedId) == -1)
			return;

		RplId groupID = gm.m_aActivePlayerGroupsIDs.Get(gm.m_aGroupRplIDs.Find(gm.m_aPlayerGroupIDs.Get(gm.m_aSlots.Find(requestedId))));
		SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
		if (playerGroup)
		{
			Rpc(RpcDo_SendGroupIDToPlayer, requesterID, playerGroup.GetGroupID());
			RpcDo_SendGroupIDToPlayer(requesterID, playerGroup.GetGroupID());
		};
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendGroupIDToPlayer(int requesterID, int groupId)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != requesterID || groupId == -1)
			return;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);

		if (adminMenu)
			adminMenu.UpdateSpawnGroup(groupId);
	}

	//------------------------------------------------------------------------------------------------
	// Gear
	//------------------------------------------------------------------------------------------------
	void SetPlayerGear(int playerID, ResourceName prefab, bool logAction)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		GetGame().GetCallqueue().CallLater(SetupAddGearToEntity, m_RNG.RandInt(250, 1000), false, entity, prefab);
		SetPlayerGearScriptsMapValue(prefab, playerID, "GSR"); // GSR = Gear Script Resource

		if (logAction)
			LogAdminAction(string.Format("%1's gear was set to %2", GetGame().GetPlayerManager().GetPlayerName(playerID), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	void AddItem(int playerID, string prefab, bool logAction)
	{
		if (playerID == 0 || prefab.IsEmpty())
			return;

		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
			delete resourceSpawned;

		if (logAction)
			LogAdminAction(string.Format("%2 was added to %1's inventory", GetGame().GetPlayerManager().GetPlayerName(playerID), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	// Teleport
	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerID1, int playerID2, bool logAction)
	{
		Rpc(Rpc_TeleportPlayers, playerID1, playerID2, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_TeleportPlayers(int playerID1, int playerID2, bool logAction)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != playerID1)
			return;

		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerID1, teleportLocation);

		if (logAction)
			LogAdminAction(string.Format("%1 was teleported to %2", GetGame().GetPlayerManager().GetPlayerName(playerID1), GetGame().GetPlayerManager().GetPlayerName(playerID2)), playerID1, true);
	}

	//------------------------------------------------------------------------------------------------
	// Hints
	//------------------------------------------------------------------------------------------------
	void SendHintAll(string data)
	{
		Rpc(Rpc_SendHintAll, data);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintAll(string data)
	{
		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintPlayer(string data, int playerID)
	{
		Rpc(Rpc_SendHintPlayer, data, playerID);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintPlayer(string data, int playerID)
	{
		if (playerID == 0 || SCR_PlayerController.GetLocalPlayerId() != playerID)
			return;

		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintFaction(string data, string factionKey)
	{
		Rpc(Rpc_SendHintFaction, data, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintFaction(string data, string factionKey)
	{
		if (factionKey.IsEmpty() && !SCR_FactionManager.SGetLocalPlayerFaction() && (SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction()).GetFactionKey() != factionKey))
			return;

		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminHint(string data)
	{
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");

		if (!widget)
			return;

		if (m_wSavedHintWidget)
			delete m_wSavedHintWidget;

		m_wSavedHintWidget = widget;

		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		hint.ShowHint(data, 8000);
	}

	//------------------------------------------------------------------------------------------------
	// Heal
	//------------------------------------------------------------------------------------------------
	void HealPlayer(int playerID, bool logAction)
	{
		IEntity PlayerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(PlayerEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
			LogAdminAction(string.Format("%1's was healed", GetGame().GetPlayerManager().GetPlayerName(playerID)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	void HealPlayerVehicle(int playerID, bool logAction)
	{
		IEntity PlayerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		IEntity VehicleEntity = SCR_CompartmentAccessComponent.GetVehicleIn(PlayerEntity);
		if (!VehicleEntity)
			return;

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(VehicleEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
			LogAdminAction(string.Format("%1's vehicle was repaired", GetGame().GetPlayerManager().GetPlayerName(playerID)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	// Log Admin Actions
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	void SendAdminMessage(string data)
	void LogAdminAction(string data, int playerID, bool sendToPlayer)
	{
		Rpc(RpcAsk_LogAdminAction, data, playerID, sendToPlayer);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_LogAdminAction(string data, int playerID, bool sendToPlayer)
	{
		if (sendToPlayer)
		{
			if (GetGame().GetPlayerController().GetPlayerId() != playerID && (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator()))
				return;
		} else {
			if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
				return;
		}

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Safestart Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------

	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_bSafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();

	[RplProp()]
	protected string m_sServerWorldTime;

	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = {};

	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;

	[RplProp()]
	protected bool m_bKillRedundantUnitsBool;

	protected int m_iTimeSafeStartBegan;
	protected int m_iTimeMissionEnds;
	protected int m_iSafeStartTimeRemaining;

	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	protected bool m_bCivReady = false;

	protected bool m_bAdminForcedReady = false;

	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity, bool> m_mEntitiesWithEHsMap = new map<IEntity, bool>();

	protected SCR_PopUpNotification m_PopUpNotification = null;

	bool m_bHUDVisible = true;
	CRF_LoggingServerComponent m_Logging;

	//------------------------------------------------------------------------------------------------
	// Ready Up functions
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	void UpdateHUDVisible()
	{
		m_bHUDVisible = !m_bHUDVisible;
	};

	//------------------------------------------------------------------------------------------------
	TStringArray GetWhosReady() {
		return m_aFactionsStatusArray;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayedFactions()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		SCR_SortedArray<SCR_Faction> outFaction = new SCR_SortedArray<SCR_Faction>();
		factionManager.GetSortedFactionsList(outFaction);

		if (!outFaction || outFaction.IsEmpty())
			return;

		array<SCR_Faction> outArray = {};
		outFaction.ToArray(outArray);

		m_aPlayedFactionsArray.Clear();
		string bluforString = "#Coal_SS_No_Faction";
		string opforString = "#Coal_SS_No_Faction";
		string indforString = "#Coal_SS_No_Faction";
		string civString = "#Coal_SS_No_Faction";

		foreach (SCR_Faction faction : outArray)
		{
			if (faction.GetPlayerCount() == 0 || (faction.GetFactionKey() != "BLUFOR" && faction.GetFactionKey() != "OPFOR" && faction.GetFactionKey() != "INDFOR" && faction.GetFactionKey() != "CIV"))
				continue;

			m_aPlayedFactionsArray.Insert(faction);

			switch (true) {
				case(!m_bBluforReady && faction.GetFactionKey() == "BLUFOR") : {bluforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bBluforReady && faction.GetFactionKey() == "BLUFOR") : {bluforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bOpforReady && faction.GetFactionKey() == "OPFOR") : {opforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bOpforReady && faction.GetFactionKey() == "OPFOR") : {opforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bIndforReady && faction.GetFactionKey() == "INDFOR") : {indforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bIndforReady && faction.GetFactionKey() == "INDFOR") : {indforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bCivReady && faction.GetFactionKey() == "CIV") : {civString = "#Coal_SS_Faction_Not_Ready"; 			break; };
				case(m_bCivReady && faction.GetFactionKey() == "CIV") : {civString = "#Coal_SS_Faction_Ready"; 			break; };
			};
		};

		m_aFactionsStatusArray = {bluforString, opforString, indforString, civString};
		m_iPlayedFactionsCount = 0;

		foreach (string factionString : m_aFactionsStatusArray)
		{
			if (factionString == "#Coal_SS_No_Faction")
				continue;

			m_iPlayedFactionsCount = m_iPlayedFactionsCount + 1;
		}

		Replication.BumpMe();
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(FactionKey setReady, string playerName, bool adminForced) {
		if (!GetSafestartStatus())
			return;

		// If it's an admin-forced action
		if (adminForced)
		{
			if (m_bAdminForcedReady)
			{
				m_bBluforReady = false;
				m_bOpforReady = false;
				m_bIndforReady = false;
				m_bCivReady = false;
				m_bAdminForcedReady = false;

				m_sMessageContent = string.Format("An Admin (%1) Has Force Unreadied All Sides!", playerName);
				Replication.BumpMe();
				ShowMessage();
				return;
			};

			m_bBluforReady = true;
			m_bOpforReady = true;
			m_bIndforReady = true;
			m_bCivReady = true;
			m_bAdminForcedReady = true;

			m_sMessageContent = string.Format("An Admin (%1) Has Force Readied All Sides!", playerName);
			Replication.BumpMe();
			ShowMessage();
			return;
		}

		if (m_bAdminForcedReady)
			return;

		switch (setReady)
		{
			case("BLUFOR") : {
				m_bBluforReady = !m_bBluforReady;
				if (m_bBluforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Blufor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Blufor - %1", playerName);
				break;
			};
			case("OPFOR") : {
				m_bOpforReady = !m_bOpforReady;
				if (m_bOpforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Opfor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Opfor - %1", playerName);
				break;
			};
			case("INDFOR") : {
				m_bIndforReady = !m_bIndforReady;
				if (m_bIndforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Indfor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Indfor - %1", playerName);
				break;
			};
			case("CIV") : {
				m_bCivReady = !m_bCivReady;
				if (m_bCivReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Civ - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Civ - %1", playerName);
				break;
			};
		};
		Replication.BumpMe();
		ShowMessage();
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCountDown()
	{
		int factionsReadyCount = 0;
		foreach (string factionCheckReadyString : m_aFactionsStatusArray)
		{
			if (factionCheckReadyString != "#Coal_SS_Faction_Ready")
				continue;
			factionsReadyCount = factionsReadyCount + 1;
		};

		if (factionsReadyCount == 0 && m_iPlayedFactionsCount == 0 || factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining == 35)
			return;

		if (factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining != 35)
		{
			m_sMessageContent = "#Coal_SS_Countdown_Cancelled";
			Replication.BumpMe();
			m_iSafeStartTimeRemaining = 35;
			return;
		};

		if (factionsReadyCount == m_iPlayedFactionsCount)
		{
			m_iSafeStartTimeRemaining = m_iSafeStartTimeRemaining - 5;
			m_sMessageContent = string.Format("#Coal_SS_Countdown_Started %1 Seconds!", m_iSafeStartTimeRemaining);
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_sMessageContent = "#Coal_SS_Game_Live";
			};
		};
		Replication.BumpMe();
		ShowMessage();
	};

	//------------------------------------------------------------------------------------------------
	// SafeStart functions
	//------------------------------------------------------------------------------------------------
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}

	//------------------------------------------------------------------------------------------------
	bool GetSafestartStatus()
	{
		return m_bSafeStartEnabled;
	}

	//------------------------------------------------------------------------------------------------
	void OnSafeStartChange()
	{
		m_OnSafeStartChange.Invoke(m_bSafeStartEnabled);
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void ToggleSafeStartServer(bool status)
	{
		if (status)
		{ // Turn on safestart
			if (m_bSafeStartEnabled)
				return;

			m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_bSafeStartEnabled = true;
			m_iSafeStartTimeRemaining = 35;

			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			GetGame().GetCallqueue().Remove(CheckPlayersAlive);

			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 1000, true);
			GetGame().GetCallqueue().CallLater(ActivateSafeStartEHs, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdatePlayedFactions, 1000, true);

			Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		} else { // Turn off safestart
			if (!m_bSafeStartEnabled)
				return;

			UpdatePlayedFactions();

			DeleteEmptySlots();
			m_bKillRedundantUnitsBool = true;
			m_bAdminForcedReady = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			m_bCivReady = false;

			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(UpdateServerWorldTime);
			GetGame().GetCallqueue().Remove(ActivateSafeStartEHs);
			GetGame().GetCallqueue().Remove(UpdatePlayedFactions);

			GetGame().GetCallqueue().CallLater(CheckPlayersAlive, 5000, true);

			if (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes * 60000);
				GetGame().GetCallqueue().CallLater(UpdateMissionEndTimer, 1000, true);
			} else {
				m_sServerWorldTime = "N/A";
			};

			Replication.BumpMe();//Broadcast change

			DeactivateSafeStartEHs();

			// Send notification message
			if (m_Logging)
				m_Logging.GameStarted();

			// Use CallLater to delay the call for the removal of EHs so the changes so m_bSafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 1500);

			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 12500);

			GetGame().GetCallqueue().CallLater(DelayChangeSafeStartDisabled, 250);

			// Update logging component since game is now live
			CRF_MDB_LoggingServerComponent logCom = CRF_MDB_LoggingServerComponent.GetInstance();
			if (logCom)
			{
				logCom.m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
				SCR_FactionManager scrFM = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				logCom.m_iBluforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("BLUFOR"));
				logCom.m_iOpforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("OPFOR"));
				logCom.m_iIndforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("INDFOR"));
				logCom.m_iCivCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("CIV"));
			};
		}
	};

	//------------------------------------------------------------------------------------------------
	void DelayChangeSafeStartDisabled() {
		m_bSafeStartEnabled = false;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		Replication.BumpMe();
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		};

		Replication.BumpMe();
	};

	// Why are these two methods done this way? It should just be one wtf
	//------------------------------------------------------------------------------------------------
	void DeleteEmptySlots()
	{
		if (CRF_Gamemode.GetInstance().m_bDeleteJIPSlots)
			if (m_bSafeStartEnabled)
				GetGame().GetCallqueue().CallLater(DeleteEmptySlotsSlowly, 125, true);
	}

	//------------------------------------------------------------------------------------------------
	void DeleteEmptySlotsSlowly()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode.m_bDeleteJIPSlots && !gamemode.m_bRespawnEnabled)
		{
			for (int i = 0; i < gamemode.m_aEntitySlots.Count(); i++)
			{
				if (gamemode.m_aSlots.Get(i) == 0 || gamemode.m_aSlots.Get(i) == -1)
				{
					//Print("Removing Entity");
					gamemode.RemovePlayableEntity(gamemode.m_aEntitySlots.Get(i));
					return;
				}
			}
			GetGame().GetCallqueue().Remove(DeleteEmptySlotsSlowly);
		}

	}

	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;

		m_PopUpNotification = SCR_PopUpNotification.GetInstance();

		m_sStoredMessageContent = m_sMessageContent;

		if (m_sMessageContent == "All Blufor Players Have Been Eliminated!" || m_sMessageContent == "All Opfor Players Have Been Eliminated!" || m_sMessageContent == "All Indfor Players Have Been Eliminated!" || m_sMessageContent == "All Civilian Players Have Been Eliminated!")
		{
			m_PopUpNotification.PopupMsg(m_sMessageContent, 20);
			return;
		};

		if (m_sMessageContent == "#Coal_SS_Game_Live")
			m_PopUpNotification.PopupMsg(m_sMessageContent, 8, "#Coal_SS_SafeStart_Started_Subtext");
		else
			m_PopUpNotification.PopupMsg(m_sMessageContent, 2.5, "#Coal_SS_Countdown_Started_Subtext");
	};

	//------------------------------------------------------------------------------------------------
	void CheckPlayersAlive()
	{
		foreach (SCR_Faction faction : m_aPlayedFactionsArray)
		{
			switch (true) {
				case(faction.GetFactionKey() == "BLUFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[0] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Blufor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "OPFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[1] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Opfor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "INDFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[2] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Indfor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "CIV" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[3] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Civilian Players Have Been Eliminated!"; break; };
			};
		};

		Replication.BumpMe();
		ShowMessage();
	}

	//------------------------------------------------------------------------------------------------
	// SafeStart EHs
	//------------------------------------------------------------------------------------------------
	protected void ActivateSafeStartEHs()
	{	
		auto aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
		{
			array<AIAgent> aiAgents = {};
			aiWorld.GetAIAgents(aiAgents);
			foreach (AIAgent agent : aiAgents)
			{	
				IEntity controlledEntity = agent.GetControlledEntity();
				if (!controlledEntity)
					continue;
				
				SetSafeStartEHs(controlledEntity);
			}
		}
		
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(outPlayers);

		foreach (int playerID : outPlayers)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			if (!controlledEntity)
				continue;

			SetSafeStartEHs(controlledEntity);
		};
	}

	//------------------------------------------------------------------------------------------------
	protected void DeactivateSafeStartEHs()
	{
		for (int i = 0; i < m_mEntitiesWithEHsMap.Count(); i++)
		{
			IEntity controlledEntity = m_mEntitiesWithEHsMap.GetKey(i);

			if (!controlledEntity)
				continue;

			SCR_CharacterDamageManagerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent)).EnableDamageHandling(true);

			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp)
				continue;

			charComp.SetSafety(false, false);

			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler)
				continue;

			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);

			m_mEntitiesWithEHsMap.Set(controlledEntity, false);
		};
	};
	
	//------------------------------------------------------------------------------------------------
	protected void SetSafeStartEHs(IEntity controlledEntity)
	{
		SCR_CharacterDamageManagerComponent damageHandler = SCR_CharacterDamageManagerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageHandler)
			damageHandler.EnableDamageHandling(false);

		EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));

		bool alreadyHasEventHandlers = m_mEntitiesWithEHsMap.Get(controlledEntity);

		if (!alreadyHasEventHandlers && charComp && eventHandler) {
			charComp.SetSafety(true, true);
			eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			m_mEntitiesWithEHsMap.Set(controlledEntity, true);
		};
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		// Get projectile and delete it
		delete entity;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}
	
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Moderator Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	
	override void OnPlayerAuditSuccess(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (!playerIdentity || playerIdentity == "")
			return;
		
		if (CRF_ModeratorConfig.IsModerator(playerIdentity))
			GetGame().GetCallqueue().CallLater(SetPlayerModerator, 10000, false, playerId);
	};
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetPlayerManager().GivePlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	};
}
