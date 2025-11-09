class CRF_GearscriptManagerClass : ScriptComponentClass {}

class CRF_GearscriptManager : ScriptComponent
{
	protected CRF_Gamemode m_Gamemode;

	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	ref array<IEntity> m_VehiclesInQueue = {};
	
	[RplProp()] ref array<ResourceName> m_aVehicleResourceName = {};
	[RplProp()] ref array<int> m_aVehicleSupplyCost = {};
	
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
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		SetEventMask(owner, EntityEvent.FRAME);
	}	
	
	array<int> GetSupplyValuesForItems(array<ResourceName> items)
	{
		array<int> itemSupply = {};
		foreach(ResourceName item: items)
		{
			itemSupply.Insert(0);
		}
		
		array<Faction> factions = {};
		FactionManager factionManager = GetGame().GetFactionManager();
		
		if (!factionManager)
			return itemSupply;
		
		factionManager.GetFactionsList(factions);
		
		array<ref SCR_EntityCatalog> itemCatalogs = {};
		SCR_EntityCatalogManagerComponent catalogMan = SCR_EntityCatalogManagerComponent.GetInstance();
		foreach (Faction faction: factions)
		{
			SCR_EntityCatalog catalog = catalogMan.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, faction.GetFactionKey(), false);
			itemCatalogs.Insert(catalog);
		}
		
		foreach (SCR_EntityCatalog catalog: itemCatalogs)
		{
			for (int i = 0; i < itemSupply.Count(); i++)
			{
				SCR_EntityCatalogEntry entry = catalog.GetEntryWithPrefab(items.Get(i));
				if (!entry)
					continue;
				
				SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
				itemSupply.Set(i, data.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false));
			}
		}
		
		return itemSupply;
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_fUpdateBuffer >= 5)
		{
			array<IEntity> vehiclesToRemove = {};
			foreach (IEntity vehicle: m_VehiclesInQueue)
			{
				if (!vehicle)
				{
					vehiclesToRemove.Insert(vehicle);
					continue;
				}
				if(FindFactionByClosestPlayer(vehicle))
					vehiclesToRemove.Insert(vehicle);
			}
			
			foreach (IEntity vehicle: vehiclesToRemove)
			{
				m_VehiclesInQueue.RemoveItem(vehicle);
			}
			
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		super.EOnFrame(owner, timeSlice);
	}
	
	bool FindFactionByClosestPlayer(IEntity vehicle)
	{	
		float closestPlayerDistance;
		IEntity closestPlayer;
		string factionKey = "";
		
		array<AIAgent> agents = {};
		GetGame().GetAIWorld().GetAIAgents(agents);
		
		foreach (AIAgent agent: agents)
		{
			IEntity aiPlayer = agent.GetControlledEntity();
			if (!aiPlayer)
				continue;
			
			if (!ChimeraCharacter.Cast(aiPlayer))
				continue;
			
			if (!aiPlayer.FindComponent(FactionAffiliationComponent))
				continue;
			
			if (!closestPlayer)
			{
				int distance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
				if (distance > 200)
					continue;
				
				closestPlayerDistance = distance;
				closestPlayer = aiPlayer;
				factionKey = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent)).GetAffiliatedFactionKey();
				continue;
			}
			
			float playerDistance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
			if (playerDistance > closestPlayerDistance || playerDistance > 200)
				continue;
			
			closestPlayer = aiPlayer;
			closestPlayerDistance = playerDistance;
			factionKey = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent)).GetAffiliatedFactionKey();
		}
		
		//There's no players
		if (!closestPlayer)
			return false;
		
		Vehicle.Cast(vehicle).m_sFactionKey = factionKey;
		GetGame().GetCallqueue().CallLater(
					CRF_GearscriptManager.GetInstance().SetVehicleGear, 500, false,
					vehicle, Vehicle.Cast(vehicle).m_sFactionKey
				);
		return true;
	}
	
	int GetSuppliesInTruck(IEntity truck)
	{
		SCR_VehicleInventoryStorageManagerComponent invManager = SCR_VehicleInventoryStorageManagerComponent.Cast(truck.FindComponent(SCR_VehicleInventoryStorageManagerComponent));
		if (!invManager)
			return 0;
		
		array<IEntity> items = {};
		invManager.GetItems(items);
		array<ResourceName> itemToScan = {};
		array<int> amountOfItem = {};
		foreach (IEntity item: items)
		{
			string prefab = item.GetPrefabData().GetPrefabName();
			if (itemToScan.Contains(prefab))
			{
				int index = itemToScan.Find(prefab);
				amountOfItem.Set(index, amountOfItem.Get(index) + 1);
				continue;
			}
			itemToScan.Insert(prefab);
			amountOfItem.Insert(1);
		}
		
		array<int> supplies = GetSupplyValuesForItems(itemToScan);
		
		int suppliesNeeded = 0;
		for (int i = 0; i < supplies.Count(); i++)
		{
			suppliesNeeded += supplies[i] * amountOfItem[i];
		}
		
		SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(truck.FindComponent(SCR_BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> turrets = {};
		array<IEntity> weapons = {};
		compartmentMan.GetCompartmentsOfType(turrets, ECompartmentType.TURRET);
		foreach (BaseCompartmentSlot turret: turrets)
		{
			TurretControllerComponent turretController = TurretControllerComponent.Cast(turret.GetController());
			if (!turretController)
				continue;
			
			array<IEntity> weaponsToAdd = {};
			BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
			if (weaponManager)
				weaponManager.GetWeaponsList(weaponsToAdd);
		
			foreach (IEntity weapon: weaponsToAdd)
			{
				weapons.Insert(weapon);
			}
		}
		
		foreach (IEntity weapon: weapons)
		{
			if (!weapon.FindComponent(WeaponComponent))
				continue;
			
			WeaponComponent weaponComp = WeaponComponent.Cast(weapon.FindComponent(WeaponComponent));
			EWeaponType type = weaponComp.GetWeaponType();
			
			array<BaseMuzzleComponent> muzzles = {};
			weaponComp.GetMuzzlesList(muzzles);
			array<ResourceName> magazinesToAdd = {};
			array<int> magazineCount = {};
			foreach (BaseMuzzleComponent muzzle: muzzles)
			{
				BaseMagazineComponent mag = muzzle.GetMagazine();
				if (!mag)
					continue;
				
				if (type == EWeaponType.WT_AUTOCANNON)
					suppliesNeeded += mag.GetAmmoCount();
			}
		}
		return suppliesNeeded;
	}
	
	int GetTruckResupplyCost(ResourceName resource)
	{
		int index = m_aVehicleResourceName.Find(resource);
		if (index == -1)
			return 0;
		
		return m_aVehicleSupplyCost.Get(index);
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
		
		CRF_Gamemode gm = CRF_Gamemode.GetInstance();
		switch (factionKey)
		{
			case "BLUFOR": return gm.m_rBLUFORCurrentGearScript; break;
			case "OPFOR": return gm.m_rOPFORCurrentGearScript; break;
			case "INDFOR": return gm.m_rINDFORCurrentGearScript; break;
			case "CIV": return gm.m_rCIVILIANCurrentGearScript; break;
		}

		return gm.m_rCIVILIANCurrentGearScript;
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
			return;
		
		// Load gearscript config
		CRF_GearScriptConfig gearConfig = LoadGearScriptConfig(gearScriptResourceName);
		if (!gearConfig)
			return;
		
		// Prepare spawn parameters
		EntitySpawnParams spawnParams = CreateSpawnParams(entity);
		
		// Apply gear
		ApplyClothing(gearConfig, role, spawnParams, inventory, inventoryManager);
		GetGame().GetCallqueue().CallLater(ApplyWeapons, 375, false, gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		GetGame().GetCallqueue().CallLater(ApplyInventoryItems, 250, false, gearConfig, role, gearScriptSettings, spawnParams, inventory, inventoryManager);
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
			GetGame().GetCallqueue().CallLater(groupsMan.TuneFreqDelayWithPresets, 500, false, playerId, entity);
			GetGame().GetCallqueue().CallLater(pc.InitializeRadios, 500, false, entity);
			pc.InitializeRadioFromServer();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Set gear for a vehicle entity based on its faction key
	 *
	 * Attempts to find the correct faction for the vehicle. If no faction is provided,
	 * it searches for the closest player within 200m to determine the faction. Then,
	 * it applies the appropriate gear loadout to the vehicle, checking if it's a supply truck.
	 *
	 * @param vehicle The vehicle entity to equip with gear
	 * @param factionKey The key identifying the faction to use for gear configuration
	 */
	void SetVehicleGear(IEntity vehicle, string factionKey)
	{
		//Lets find a faction, if there is none start looking for one in the loop.
		Faction faction = SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetFactionByKey(factionKey);
		if (!faction)
		{	
			float closestPlayerDistance;
			IEntity closestPlayer;
			factionKey = "";
			array<AIAgent> agents = {};
			
			GetGame().GetAIWorld().GetAIAgents(agents);
			
			foreach (AIAgent agent: agents)
			{
				IEntity aiPlayer = agent.GetControlledEntity();
				if (!aiPlayer)
					continue;
				
				if (!ChimeraCharacter.Cast(aiPlayer))
					continue;
				
				if (!closestPlayer)
				{
					closestPlayerDistance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
					if (closestPlayerDistance > 200)
						continue;
					closestPlayer = aiPlayer;
					if (aiPlayer.FindComponent(FactionAffiliationComponent))
					{
						factionKey = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent)).GetAffiliatedFactionKey();
					}
					else
						factionKey = "CIV";
					continue;
				}
				
				float playerDistance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
				if (playerDistance > closestPlayerDistance || playerDistance > 200)
					continue;
				
				closestPlayer = aiPlayer;
				closestPlayerDistance = playerDistance;
				if (aiPlayer.FindComponent(FactionAffiliationComponent))
					{
						factionKey = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent)).GetAffiliatedFactionKey();
					}
					else
						factionKey = "CIV";
			}
			
			//There's no players
			if (!closestPlayer)
			{
				m_VehiclesInQueue.Insert(vehicle);
				return;
			}

			
			faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
			Vehicle.Cast(vehicle).m_sFactionKey = faction.GetFactionKey();
		}
		
		ref CRF_GearScriptContainer gsContainer = GetGearScriptSettings(faction.GetFactionKey());
		if (gsContainer.m_aSupplyTrucks.Contains(vehicle.GetPrefabData().GetPrefabName()))
			SetTruckGear(vehicle, faction, gsContainer, true);
		else
			SetTruckGear(vehicle, faction, gsContainer, false);
		
	}
	
	bool IsSupplyTruck(IEntity truck, string factionKey)
	{
		ref CRF_GearScriptContainer gsContainer = GetGearScriptSettings(factionKey);
		return gsContainer.m_aSupplyTrucks.Contains(truck.GetPrefabData().GetPrefabName());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Configures a truck’s inventory and equipment loadout
	 *
	 * Clears existing inventory, applies the configured loadout, and spawns weapons,
	 * magazines, grenades, smoke, and additional faction-specific items. Handles both
	 * supply trucks and regular vehicles.
	 *
	 * @param truck The truck entity to configure
	 * @param faction The faction object used to determine loadout
	 * @param gsContainer The gear script container holding loadout data
	 * @param isSupply Whether the truck is a supply truck (true) or a regular vehicle (false)
	 */
	void SetTruckGear(IEntity truck, Faction faction, CRF_GearScriptContainer gsContainer, bool isSupply)
	{
		ref CRF_GearScriptConfig gearSriptConfig = LoadGearScriptConfig(gsContainer.m_rGearScript);
		ref CRF_VehicleGearscriptConfig vehicleGearScriptConfig = LoadVehicleGearScriptConfig(gsContainer.m_rVehicleGearscriptValues);
		SCR_VehicleInventoryStorageManagerComponent invManager = SCR_VehicleInventoryStorageManagerComponent.Cast(truck.FindComponent(SCR_VehicleInventoryStorageManagerComponent));
		if (!invManager)
			return;
		
		int suppliesNeeded = 0;
		ClearTruckGear(truck, invManager);
		suppliesNeeded += ApplyTruckLoadout(truck, invManager, gsContainer, faction.GetFactionKey(), isSupply);
		array<ResourceName> heGLsToAdd = {};
		array<ResourceName> glsToAdd = {};
		for (int i = 0; i <= 11; i++)
		{
			//Regular Weapons
			if (i < 4 || i == 11)
			{
				int bulletForWeapon = GetBulletCountForWeapon(truck, i, vehicleGearScriptConfig, gsContainer);
				array<ResourceName> magazinesToAdd = {};
				array<int> magazineCounts = {};
				array<ref CRF_Weapon_Class> weapons = GetWeaponsByIndex(i, gearSriptConfig);
				if (weapons.Count() == 0)
					continue;
				foreach (CRF_Weapon_Class weapon: weapons)
				{
					if (!weapon)
						continue;
					
					if (!weapon.m_MagazineArray)
						continue;
					foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
					{
						if (!IsRegularMagazine(weapons, magazine.m_Magazine) && i == 1)
						{
							if (IsGLHE(magazine.m_Magazine))
								heGLsToAdd.Insert(magazine.m_Magazine);
							else
								glsToAdd.Insert(magazine.m_Magazine);
							continue;
						}
						
						int magazineCount = GetMagazineCount(magazine.m_Magazine);
						if (magazineCount <= 0)
							continue;
						magazinesToAdd.Insert(magazine.m_Magazine);
						magazineCounts.Insert(magazineCount);
					}
				}				
				if (magazinesToAdd.Count() == 0)
					continue;
				
				suppliesNeeded += SpawnMagazinesToVehicle(bulletForWeapon, magazineCounts, magazinesToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
			}
			//Spec Weapons
			else
			{
				int bulletForWeapon = GetBulletCountForWeapon(truck, i, vehicleGearScriptConfig, gsContainer);
				array<ResourceName> magazinesToAdd = {};
				array<int> magazineCounts = {};
				CRF_Spec_Weapon_Class weapon = GetSpecWeaponByIndex(i, gearSriptConfig);
				if (!weapon)
					continue;
				bool isDisposable = IsWeaponDisposable(weapon.m_Weapon);
				if (isDisposable)
				{
					magazinesToAdd.Insert(weapon.m_Weapon);
					suppliesNeeded += SpawnItemsToVehicle(bulletForWeapon, magazinesToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
				}
				else
				{
					foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
					{
						if (!IsSpecRegularMagazine(weapon, magazine.m_Magazine))
							continue;
						
						int magazineCount = GetMagazineCount(magazine.m_Magazine);
						if (magazineCount <= 0)
							continue;
						magazinesToAdd.Insert(magazine.m_Magazine);
						magazineCounts.Insert(magazineCount);
					}
					if (magazinesToAdd.Count() == 0)
						continue;
					
					suppliesNeeded += SpawnMagazinesToVehicle(bulletForWeapon, magazineCounts, magazinesToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
				}
			}
		}
				
		array<ResourceName> grenadesToAdd = {};
		array<ResourceName> smokesToAdd = {};
		foreach (CRF_Inventory_Item item: gearSriptConfig.m_DefaultFactionGear.m_DefaultInventoryItems)
		{
			bool isGrenade;
			bool isSmoke;
			IsItemGrenade(item.m_sItemPrefab, isGrenade, isSmoke);
			if (isGrenade)
			{
				if (isSmoke)
					smokesToAdd.Insert(item.m_sItemPrefab);
				else
					grenadesToAdd.Insert(item.m_sItemPrefab);
			}
		}
		
		if (grenadesToAdd.Count() > 0)
		{
			int grenades = GetBulletCountForWeapon(truck, 12, vehicleGearScriptConfig, gsContainer);
			suppliesNeeded += SpawnItemsToVehicle(grenades, grenadesToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
		}
		
		if (smokesToAdd.Count() > 0)
		{
			int grenades = GetBulletCountForWeapon(truck, 13, vehicleGearScriptConfig, gsContainer);
			suppliesNeeded += SpawnItemsToVehicle(grenades, smokesToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
		}
		
		//Add misc items
		if (heGLsToAdd.Count() > 0)
		{
			int glsToSpawn = GetBulletCountForWeapon(truck, 14, vehicleGearScriptConfig, gsContainer);
			suppliesNeeded += SpawnItemsToVehicle(glsToSpawn, heGLsToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
		}
		
		if (glsToAdd.Count() > 0)
		{
			int glsToSpawn = GetBulletCountForWeapon(truck, 15, vehicleGearScriptConfig, gsContainer);
			suppliesNeeded += SpawnItemsToVehicle(glsToSpawn, glsToAdd, invManager, faction.GetFactionKey(), isSupply, isSupply, truck.GetPrefabData().GetPrefabName());
		}
		
		array<ref CRF_VehicleGearScriptAdditionalItem> additionalItems = {};
		if (Vehicle.Cast(truck).m_aAdditionalVehicleItems.Count() > 0)
			additionalItems = Vehicle.Cast(truck).m_aAdditionalVehicleItems;
		else
			additionalItems = gsContainer.m_aAdditionalVehicleItems;
		foreach (CRF_VehicleGearScriptAdditionalItem item: additionalItems)
		{
			array<ResourceName> holder = {item.m_Prefab};
			if (isSupply)
				suppliesNeeded += SpawnItemsToVehicle(item.m_iAmountOfItemSupplyTruck, holder, invManager, faction.GetFactionKey(), isSupply, true, truck.GetPrefabData().GetPrefabName());
			else
				suppliesNeeded += SpawnItemsToVehicle(item.m_iAmountOfItemRegularVehicle, holder, invManager, faction.GetFactionKey(), isSupply, true, truck.GetPrefabData().GetPrefabName());
		}
		
		
		if (!m_aVehicleResourceName.Contains(truck.GetPrefabData().GetPrefabName()))
		{
			m_aVehicleResourceName.Insert(truck.GetPrefabData().GetPrefabName());
			m_aVehicleSupplyCost.Insert(suppliesNeeded);
			Replication.BumpMe();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Removes all existing items from a truck’s inventory
	 *
	 * Iterates through the truck’s inventory storage manager and deletes all entities found.
	 *
	 * @param truck The truck whose inventory will be cleared
	 * @param invManager The truck’s inventory storage manager component
	 */
	void ClearTruckGear(IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager)
	{
		array<IEntity> items = {};
		invManager.GetItems(items);
		foreach (IEntity item: items)
		{
			if (!item)
				continue;
			
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Applies a predefined loadout to a truck, including turret weapons and ammo
	 *
	 * Loads the correct vehicle loadout (either overridden or default), then spawns
	 * ammunition into the truck’s storage.
	 *
	 * @param truck The truck entity to configure
	 * @param invManager The truck’s inventory storage manager component
	 * @param gsContainer The gear script container holding vehicle loadout data
	 */
	int ApplyTruckLoadout(IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager, CRF_GearScriptContainer gsContainer, string factionKey, bool isSupply)
	{
		ref CRF_VehicleGearScriptLoadout vehLoadout;
		int suppliesNeeded = 0;
		bool calculateSupplies = HasSupplyBeenCalculated(truck.GetPrefabData().GetPrefabName());
		if (Vehicle.Cast(truck).m_OverridedVehicleLoadout)
			vehLoadout = Vehicle.Cast(truck).m_OverridedVehicleLoadout;
		else
			vehLoadout = gsContainer.m_VehicleLoadout;
		SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(truck.FindComponent(SCR_BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> turrets = {};
		array<IEntity> weapons = {};
		compartmentMan.GetCompartmentsOfType(turrets, ECompartmentType.TURRET);
		foreach (BaseCompartmentSlot turret: turrets)
		{
			TurretControllerComponent turretController = TurretControllerComponent.Cast(turret.GetController());
			if (!turretController)
				continue;
			
			array<IEntity> weaponsToAdd = {};
			BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
			if (weaponManager)
				weaponManager.GetWeaponsList(weaponsToAdd);
		
			foreach (IEntity weapon: weaponsToAdd)
			{
				weapons.Insert(weapon);
			}
		}
		
		foreach (IEntity weapon: weapons)
		{
			if (!weapon.FindComponent(WeaponComponent))
				continue;
			
			int bulletsToAdd = 0;
			WeaponComponent weaponComp = WeaponComponent.Cast(weapon.FindComponent(WeaponComponent));
			EWeaponType type = weaponComp.GetWeaponType();
			if (type == EWeaponType.WT_AUTOCANNON)
				bulletsToAdd = vehLoadout.m_iAmountofAutoCannonAmmo;
			else
				bulletsToAdd = vehLoadout.m_iAmountofMachineGunAmmo;
			
			array<BaseMuzzleComponent> muzzles = {};
			weaponComp.GetMuzzlesList(muzzles);
			array<ResourceName> magazinesToAdd = {};
			array<int> magazineCount = {};
			foreach (BaseMuzzleComponent muzzle: muzzles)
			{
				BaseMagazineComponent mag = muzzle.GetMagazine();
				if (!mag)
					continue;
				
				if (type == EWeaponType.WT_AUTOCANNON)
				{
					if (!calculateSupplies)
						suppliesNeeded += mag.GetMaxAmmoCount();
					if (mag.GetMaxAmmoCount() < bulletsToAdd)
						PrintFormat("[CRF_GEARSCRIPT ERROR] Magazine: %1 does not have the proper max ammo set for the gearscript! Current: %2 | Needs: %3", WidgetManager.Translate(mag.GetUIInfo().GetName()), mag.GetMaxAmmoCount(), bulletsToAdd);
					mag.SetAmmoCount(bulletsToAdd);
					continue;
				}
				magazinesToAdd.Insert(mag.GetOwner().GetPrefabData().GetPrefabName());
				magazineCount.Insert(mag.GetMaxAmmoCount());
			}
			
			if (magazinesToAdd.Count() == 0)
				continue;
			
			suppliesNeeded += SpawnMagazinesToVehicle(bulletsToAdd, magazineCount, magazinesToAdd, invManager, factionKey, isSupply, true, truck.GetPrefabData().GetPrefabName());
		}
		return suppliesNeeded;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawns magazines into a vehicle’s inventory
	 *
	 * Loops until the requested amount of ammunition is added, distributing across
	 * multiple magazine types.
	 *
	 * @param amountToSpawn Total number of bullets to distribute
	 * @param magazineCounts Array of magazine capacities
	 * @param magazinesToAdd Array of magazine resource names
	 * @param invManager Vehicle’s inventory storage manager component
	 * @param isSupply Whether this is a supply vehicle (full load) or not (reduced load)
	 */
	int SpawnMagazinesToVehicle(int amountToSpawn, array<int> magazineCounts, array<ResourceName> magazinesToAdd, SCR_VehicleInventoryStorageManagerComponent invManager, string factionKey, bool isSupply, bool divide, string truckResource)
	{
		int suppliesNeeded = 0;
		int catch = 0;
		if (!divide)
			amountToSpawn /= 4;
		array<int> magazinesAdded = {};
		for (int i = 0; i < magazinesToAdd.Count(); i++)
		{
			magazinesAdded.Insert(0);
		}
		while (amountToSpawn > 0 && catch < 200)
		{
			for (int i = 0; i < magazinesToAdd.Count(); i++)
			{
				invManager.TrySpawnPrefabToStorage(magazinesToAdd[i]);
				amountToSpawn -= magazineCounts[i];
				magazinesAdded.Set(i, magazinesAdded.Get(i) + 1);
					
			}
			catch++;
		}
		
		if (!HasSupplyBeenCalculated(truckResource))
		{
			array<int> supplies = GetSupplyValuesForItems(magazinesToAdd);
			for (int i = 0; i < supplies.Count(); i++)
			{
				suppliesNeeded += supplies[i] * magazinesAdded[i];
			}
		}
		
		return suppliesNeeded;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawns items (e.g., grenades, disposable launchers) into a vehicle’s inventory
	 *
	 * Iterates through provided items and spawns them until the requested amount is added.
	 *
	 * @param amountToSpawn Number of items to spawn
	 * @param itemsToSpawn Array of item resource names
	 * @param invManager Vehicle’s inventory storage manager component
	 * @param isSupply Whether this is a supply vehicle (full load) or not (reduced load)
	 */
	int SpawnItemsToVehicle(int amountToSpawn, array<ResourceName> itemsToSpawn, SCR_VehicleInventoryStorageManagerComponent invManager, string factionKey, bool isSupply, bool divide, string truckResource)
	{
		int suppliesNeeded = 0;
		int catch = 0;
		if (!divide)
			amountToSpawn /= 4;
		
		array<int> itemsAdded = {};
		for (int i = 0; i < itemsToSpawn.Count(); i++)
		{
			itemsAdded.Insert(0);
		}
		while (amountToSpawn > 0 && catch < 1000)
		{
			for (int i = 0; i < itemsToSpawn.Count(); i++)
			{
				invManager.TrySpawnPrefabToStorage(itemsToSpawn.Get(i));
				itemsAdded.Set(i, itemsAdded.Get(i) + 1);
				amountToSpawn--;
			}
			catch++;
		}
		
		if (!HasSupplyBeenCalculated(truckResource))
		{
			array<int> supplies = GetSupplyValuesForItems(itemsToSpawn);
			for (int i = 0; i < supplies.Count(); i++)
			{
				suppliesNeeded += supplies[i] * itemsAdded[i];
			}
		}
		
		return suppliesNeeded;
	}
	
	bool HasSupplyBeenCalculated(ResourceName resource)
	{
		return m_aVehicleResourceName.Contains(resource);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determines if an item is a grenade or smoke grenade
	 *
	 * Inspects the item’s components to identify if it is a grenade, and whether it is smoke.
	 *
	 * @param item The resource name of the item to check
	 * @param isGrenade Outputs true if the item is a grenade
	 * @param isSmoke Outputs true if the item is a smoke grenade
	 */
	void IsItemGrenade(ResourceName item, out bool isGrenade = false, out bool isSmoke = false)
	{
		Resource itemLoaded = Resource.Load(item);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(itemLoaded);
		if (!entitySource)
			return;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(componentSource.GetClassName().ToType().IsInherited(GrenadeMoveComponent))
				isGrenade = true;
			
			if (componentSource.GetClassName().ToType().IsInherited(WeaponComponent))
			{
				EWeaponType type;
				componentSource.Get("WeaponType", type);
				if (type == EWeaponType.WT_SMOKEGRENADE)
					isSmoke = true;
			}
		}
		return;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Checks if a weapon is disposable
	 *
	 * Loads the weapon prefab and inspects components to determine if it is marked as disposable.
	 *
	 * @param weapon The resource name of the weapon to check
	 * @return true if the weapon is disposable, false otherwise
	 */
	bool IsWeaponDisposable(ResourceName weapon)
	{
		Resource weaponLoaded = Resource.Load(weapon);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(weaponLoaded);
		if (!entitySource)
			return false;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(!componentSource.GetClassName().ToType().IsInherited(WeaponComponent))
		        continue;
			
            BaseContainerList attachmentComponents = componentSource.GetObjectArray("components");
			for (int i = 0; i < attachmentComponents.Count(); i++)
			{
				IEntityComponentSource attachmentComponent = attachmentComponents.Get(i);
				if (!attachmentComponent.GetClassName().ToType().IsInherited(SCR_MuzzleInMagComponent))
					continue;
				
				bool disposable = false;
				attachmentComponent.Get("Disposable", disposable);
				return disposable;
			}
	    }
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Gets the maximum ammo count of a magazine resource
	 *
	 * Loads the magazine prefab and extracts its MaxAmmo property from the MagazineComponent.
	 *
	 * @param resource The magazine resource name
	 * @return The maximum number of bullets in the magazine, or 0 if not found
	 */
	int GetMagazineCount(ResourceName resource)
	{
		Resource magazine = Resource.Load(resource);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(magazine);
		if (!entitySource)
			return 0;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(componentSource.GetClassName().ToType().IsInherited(MagazineComponent))
	        {
	            int maxAmmo = 0;
				componentSource.Get("MaxAmmo", maxAmmo);
				return maxAmmo;
	        }
	    }
		return 0;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Retrieves a list of standard weapons by index from the gear script config
	 *
	 * Uses an index to map to weapon categories such as rifles, carbines, pistols, etc.
	 *
	 * @param index The weapon category index
	 * @param gearSriptConfig The gear script configuration to use
	 * @return Array of weapon class references
	 */
	array<ref CRF_Weapon_Class> GetWeaponsByIndex(int index, CRF_GearScriptConfig gearSriptConfig)
	{
		array<ref CRF_Weapon_Class> weapons = {};
		CRF_Weapons weaponConfig = gearSriptConfig.m_FactionWeapons;
		switch(index)
		{
			case 0:
			foreach (CRF_Weapon_Class weapon: weaponConfig.m_Rifle)
				weapons.Insert(weapon);
			break;
			
			case 1:
			foreach (CRF_Weapon_Class weapon: weaponConfig.m_RifleUGL)
				weapons.Insert(weapon);
			break;
			
			case 2:
			foreach (CRF_Weapon_Class weapon: weaponConfig.m_Carbine)
				weapons.Insert(weapon);
			break;
			
			case 3:
			foreach (CRF_Weapon_Class weapon: weaponConfig.m_Pistol)
				weapons.Insert(weapon);
			break;
				
			case 11:
			weapons.Insert(weaponConfig.m_Sniper);
			break;
		}
		
		return weapons;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Retrieves a specific special weapon by index
	 *
	 * Uses an index to fetch special weapons such as ARs, MMGs, HMGs, AT, MAT, HAT, and AA.
	 *
	 * @param index The weapon type index
	 * @param gearSriptConfig The gear script configuration to use
	 * @return A special weapon class reference
	 */
	CRF_Spec_Weapon_Class GetSpecWeaponByIndex(int index, CRF_GearScriptConfig gearSriptConfig)
	{
		CRF_Spec_Weapon_Class weapon;
		CRF_Weapons weaponConfig = gearSriptConfig.m_FactionWeapons;
		
		switch (index)
		{
			case 4:
			weapon = weaponConfig.m_AR;
			break;
			
			case 5:
			weapon = weaponConfig.m_MMG;
			break;
			
			case 6:
			weapon = weaponConfig.m_HMG;
			break;
			
			case 7:
			weapon = weaponConfig.m_AT;
			break;
			
			case 8:
			weapon = weaponConfig.m_MAT;
			break;
			
			case 9:
			weapon = weaponConfig.m_HAT;
			break;
			
			case 10:
			weapon = weaponConfig.m_AA;
			break;
		}
		
		return weapon;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Checks if a grenade launcher round is a high-explosive (HE) type
	 *
	 * Loads the grenade launcher resource and inspects its components for a collision component(Only explosives have this enabled).
	 *
	 * @param glToCheck Resource name of the grenade launcher round
	 * @return true if the round is HE, false otherwise
	 */
	bool IsGLHE(ResourceName glToCheck)
	{
		Resource glLoaded = Resource.Load(glToCheck);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(glLoaded);
		if (!entitySource)
			return false;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(!componentSource.GetClassName().ToType().IsInherited(CollisionTriggerComponent))
				continue;
			
			bool enabled = false;
			componentSource.Get("Enabled", enabled);
			return enabled;
					
	    }
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Checks if a magazine belongs to a given special weapon
	 *
	 * Compares magazine wells between a weapon and magazine to determine compatibility.
	 *
	 * @param weaponToCheck The special weapon to check against
	 * @param magazineToCheck The magazine resource name to check
	 * @return true if the magazine is valid for the weapon, false otherwise
	 */
	bool IsSpecRegularMagazine(CRF_Spec_Weapon_Class weaponToCheck, ResourceName magazineToCheck)
	{
		BaseMagazineWell magazineWell;
		Resource magazine = Resource.Load(magazineToCheck);
		IEntitySource magazineEntitySource = SCR_BaseContainerTools.FindEntitySource(magazine);
		if (!magazineEntitySource)
			return false;
		
		for(int nComponent, componentCount = magazineEntitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = magazineEntitySource.GetComponent(nComponent);
	        if(componentSource.GetClassName().ToType().IsInherited(MagazineComponent))
				componentSource.Get("MagazineWell", magazineWell);
	    }
		
		if (!magazineWell)
			return false;
		
		Resource weaponLoaded = Resource.Load(weaponToCheck.m_Weapon);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(weaponLoaded);
		if (!entitySource)
			return false;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(!componentSource.GetClassName().ToType().IsInherited(WeaponComponent))
		        continue;
			
            BaseContainerList attachmentComponents = componentSource.GetObjectArray("components");
			for (int i = 0; i < attachmentComponents.Count(); i++)
			{
				IEntityComponentSource attachmentComponent = attachmentComponents.Get(i);
				if (!attachmentComponent.GetClassName().ToType().IsInherited(MuzzleComponent) && !attachmentComponent.GetClassName().ToType().IsInherited(SCR_MuzzleInMagComponent))
					continue;
				
				BaseMagazineWell weaponMagazineWell;
				attachmentComponent.Get("MagazineWell", weaponMagazineWell);
				if (magazineWell.Type() == weaponMagazineWell.Type())
					return true;
				else
					return false;
			}
	    }
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Checks if a magazine is compatible with a set of weapons
	 *
	 * Compares magazine wells between a magazine and each weapon in the array.
	 *
	 * @param weaponsToCheck Array of weapons to check against
	 * @param magazineToCheck The magazine resource name to check
	 * @return true if the magazine is compatible, false otherwise
	 */
	bool IsRegularMagazine(array<ref CRF_Weapon_Class> weaponsToCheck, ResourceName magazineToCheck)
	{
		BaseMagazineWell magazineWell;
		Resource magazine = Resource.Load(magazineToCheck);
		IEntitySource magazineEntitySource = SCR_BaseContainerTools.FindEntitySource(magazine);
		if (!magazineEntitySource)
			return false;
		
		for(int nComponent, componentCount = magazineEntitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = magazineEntitySource.GetComponent(nComponent);
	        if(componentSource.GetClassName().ToType().IsInherited(MagazineComponent))
				componentSource.Get("MagazineWell", magazineWell);
	    }
		
		foreach (CRF_Weapon_Class weapon: weaponsToCheck)
		{
			Resource weaponLoaded = Resource.Load(weapon.m_Weapon);
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(weaponLoaded);
			if (!entitySource)
				return false;
			
			for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if(!componentSource.GetClassName().ToType().IsInherited(WeaponComponent))
			        continue;
				
	            BaseContainerList attachmentComponents = componentSource.GetObjectArray("components");
				for (int i = 0; i < attachmentComponents.Count(); i++)
				{
					IEntityComponentSource attachmentComponent = attachmentComponents.Get(i);
					if (!attachmentComponent.GetClassName().ToType().IsInherited(MuzzleComponent))
						continue;
					
					BaseMagazineWell weaponMagazineWell;
					attachmentComponent.Get("MagazineWell", weaponMagazineWell);
					if (magazineWell.Type() == weaponMagazineWell.Type())
						return true;
					else
						return false;
				}
		    }
		}
		return false;
	}
	
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Gets the number of bullets allocated to a weapon type for a vehicle
	 *
	 * Looks up overrides or defaults in the vehicle gear script configuration,
	 * based on a weapon type index.
	 *
	 * @param vehicle The vehicle entity
	 * @param index The weapon type index
	 * @param vehicleGearScript The vehicle gear script configuration
	 * @param gearContainer The gear script container holding overrides
	 * @return The number of bullets to allocate
	 */
	int GetBulletCountForWeapon(IEntity vehicle, int index, CRF_VehicleGearscriptConfig vehicleGearScript, CRF_GearScriptContainer gearContainer)
	{
		array<ref CRF_VehicleGearscriptOverride> gearOverides = {};
		if (Vehicle.Cast(vehicle).m_aVehicleGearscriptOverrides.Count() > 0)
			gearOverides = Vehicle.Cast(vehicle).m_aVehicleGearscriptOverrides;
		else
			gearOverides = gearContainer.m_aVehicleGearscriptOverrides;
		foreach (CRF_VehicleGearscriptOverride vehicleOverride: gearOverides)
		{
			if (vehicleOverride.m_VehicleAmmoType == index)
				return vehicleOverride.m_iAmountOfBullets;
		}
		//There's definitely a better way to do this
		//At least it's fast
		switch(index)
		{
			case 0: return vehicleGearScript.m_iAmountOfBulletsRifles; 		break;
			case 1: return vehicleGearScript.m_iAmountOfBulletsRifleUGLs; 	break;
			case 2: return vehicleGearScript.m_iAmountOfBulletsCarbines; 	break;
			case 3: return vehicleGearScript.m_iAmountOfBulletsPistols; 	break;
			case 4: return vehicleGearScript.m_iAmountOfBulletsAR; 			break;
			case 5: return vehicleGearScript.m_iAmountOfBulletsMMG; 		break;
			case 6: return vehicleGearScript.m_iAmountOfBulletsHMG; 		break;
			case 7: return vehicleGearScript.m_iAmountOfDisposables; 		break;
			case 8: return vehicleGearScript.m_iAmountOfRocketsAT; 			break;
			case 9: return vehicleGearScript.m_iAmountOfRocketsMAT;			break;
			case 10: return vehicleGearScript.m_iAmountOfRocketsAA; 		break;
			case 11: return vehicleGearScript.m_iAmountOfBulletsSniper;		break;
			case 12: return vehicleGearScript.m_iAmountOfGrenades;			break;
			case 13: return vehicleGearScript.m_iAmountOfSmokeGrenades;		break;
			case 14: return vehicleGearScript.m_iAmountOfHEGLs;				break;
			case 15: return vehicleGearScript.m_iAmountOfSmokeGLs;			break;
		}
		
		return 0;
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
		inventoryManager.GetAllRootItems(itemsRoot);

		items.InsertAll(itemsRoot);

		foreach (IEntity item : items)
		{
			if(item)
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
		
		if (!gearConfig || !gearConfig.m_CustomFactionGear)
			return string.Empty;
		
		foreach (ref CRF_Role_Custom_Gear customGear : gearConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
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
	 * @param gearScriptSettings Gearscript settings
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	protected void ApplyWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, CRF_GearScriptContainer gearScriptSettings,
		EntitySpawnParams spawnParams, SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(!inventory || !inventoryManager)
			return;
		
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
	protected bool ApplyCustomWeapons(CRF_GearScriptConfig gearConfig, CRF_EGearRole role, EntitySpawnParams spawnParams,
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
			if (!customGear.m_Pistol.IsEmpty())
			{
				CRF_Weapon_Class pistol = SelectRandomWeapon(customGear.m_Pistol);
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
		if (!gearConfig.m_FactionWeapons)
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
					if(gearConfig.m_FactionWeapons.m_Rifle && !gearConfig.m_FactionWeapons.m_Rifle.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Rifle);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.RIFLEUGL:
					if(gearConfig.m_FactionWeapons.m_RifleUGL && !gearConfig.m_FactionWeapons.m_RifleUGL.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_RifleUGL);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;
				
				case CRF_EGearscriptWeapons.CARBINE:
					if(gearConfig.m_FactionWeapons.m_Carbine && !gearConfig.m_FactionWeapons.m_Carbine.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Carbine);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case CRF_EGearscriptWeapons.PISTOL:
					if(gearConfig.m_FactionWeapons.m_Pistol && !gearConfig.m_FactionWeapons.m_Pistol.IsEmpty())
					{
						weapon = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Pistol);
						weaponsSelected.Insert(weapon); // Need to store the weapon we selected for magazines
					};
					break;

				case CRF_EGearscriptWeapons.SNIPER:
					if(gearConfig.m_FactionWeapons.m_Sniper)
						weapon = gearConfig.m_FactionWeapons.m_Sniper;
					break;

				case CRF_EGearscriptWeapons.AR:
					if(gearConfig.m_FactionWeapons.m_AR)
						specWeapon = gearConfig.m_FactionWeapons.m_AR;
					break;

				case CRF_EGearscriptWeapons.MMG:
					if(gearConfig.m_FactionWeapons.m_MMG)
						specWeapon = gearConfig.m_FactionWeapons.m_MMG;
					break;

				case CRF_EGearscriptWeapons.AT:
					if(gearConfig.m_FactionWeapons.m_AT)
						specWeapon = gearConfig.m_FactionWeapons.m_AT;
					break;
	
				case CRF_EGearscriptWeapons.MAT:
					if(gearConfig.m_FactionWeapons.m_MAT)
						specWeapon = gearConfig.m_FactionWeapons.m_MAT;
					break;
	
				case CRF_EGearscriptWeapons.HAT:
					if(gearConfig.m_FactionWeapons.m_HAT)
						specWeapon = gearConfig.m_FactionWeapons.m_HAT;
					break;

				case CRF_EGearscriptWeapons.AA:
					if(gearConfig.m_FactionWeapons.m_AA)
						specWeapon = gearConfig.m_FactionWeapons.m_AA;
					break;

				case CRF_EGearscriptWeapons.HMG:
					if(gearConfig.m_FactionWeapons.m_HMG)
						specWeapon = gearConfig.m_FactionWeapons.m_HMG;
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
		if (!gearConfig.m_FactionWeapons)
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
					if(gearConfig.m_FactionWeapons.m_Rifle && !gearConfig.m_FactionWeapons.m_Rifle.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_FactionWeapons.m_Rifle, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.RIFLEUGL_MAG:
					if(gearConfig.m_FactionWeapons.m_RifleUGL && !gearConfig.m_FactionWeapons.m_RifleUGL.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_FactionWeapons.m_RifleUGL, selectedWeapon);
					break;
				
				case CRF_EGearscriptMagazines.CARBINE_MAG:
					if(gearConfig.m_FactionWeapons.m_Carbine && !gearConfig.m_FactionWeapons.m_Carbine.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_FactionWeapons.m_Carbine, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.PISTOL_MAG:
					if(gearConfig.m_FactionWeapons.m_Pistol && !gearConfig.m_FactionWeapons.m_Pistol.IsEmpty())
						magazineArray = FindMagArrayForWeaponsSelected(weaponsSelected, gearConfig.m_FactionWeapons.m_Pistol, selectedWeapon);
					break;

				case CRF_EGearscriptMagazines.SNIPER_MAG:
					if(gearConfig.m_FactionWeapons.m_Sniper && gearConfig.m_FactionWeapons.m_Sniper.m_MagazineArray)
						magazineArray = gearConfig.m_FactionWeapons.m_Sniper.m_MagazineArray;
					break;

				case CRF_EGearscriptMagazines.AR_MAG:
					if(gearConfig.m_FactionWeapons.m_AR && gearConfig.m_FactionWeapons.m_AR.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AR.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.MMG_MAG:
					if(gearConfig.m_FactionWeapons.m_MMG && gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AT_MAG:
					if(gearConfig.m_FactionWeapons.m_AT && gearConfig.m_FactionWeapons.m_AT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.MAT_MAG:
					if(gearConfig.m_FactionWeapons.m_MAT && gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray, isAssistant);
					break;
	
				case CRF_EGearscriptMagazines.HAT_MAG:
					if(gearConfig.m_FactionWeapons.m_HAT && gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.AA_MAG:
					if(gearConfig.m_FactionWeapons.m_AA && gearConfig.m_FactionWeapons.m_AA.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_AA.m_MagazineArray, isAssistant);
					break;

				case CRF_EGearscriptMagazines.HMG_MAG:
					if(gearConfig.m_FactionWeapons.m_HMG && gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray)
						magazineArray = ConvertSpecMagArrayIntoMagArray(gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray, isAssistant);
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
		if(!inventory || !inventoryManager)
			return;
		
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
						if (gearConfig.m_DefaultFactionGear.m_bEnableLeadershipBinoculars)
							AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
						break;
					
					case CRF_EGearscriptItems.ASSISTANT_BINO:
						if (gearConfig.m_DefaultFactionGear.m_bEnableAssistantBinoculars)
							AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sAssistantBinocularsPrefab, 1, spawnParams, inventory, inventoryManager);
						break;
	
					case CRF_EGearscriptItems.MEDIC_ITEMS:
						foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultMedicMedicalItems)
							AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role);
						break;
				}
			}
			
			// Default inventory items
			foreach (CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultInventoryItems)
			{
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, spawnParams, inventory, inventoryManager, role, gearConfig.m_DefaultFactionGear.m_bEnableMedicFrags);
			}
		}
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
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(entity).GetWeaponManager());
		if (!weaponMan)
			return;
		
		CharacterControllerComponent charController = CharacterControllerComponent.Cast(ChimeraCharacter.Cast(entity).GetCharacterController());
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
		IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachmentResource), GetGame().GetWorld(), spawnParams);
		BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
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
	 * @param enableMedicFrags Whether to enable frags for medics
	 * @param isAssistant Whether item is for assistant role
	 */
	protected void AddInventoryItem(ResourceName item, int itemAmount, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager, 
		CRF_EGearRole role = 0, bool enableMedicFrags = false, bool isAssistant = false)
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
			if (!enableMedicFrags && CRF_GamemodeManager.RolesConfig().FindRoleConfig(role).m_aItems.Contains(CRF_EGearscriptItems.MEDIC_ITEMS) && 
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
				inventoryManager.TryInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
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
	/*
	Public method to insert an item into a storage and keep the same storage it would usually be assigned to
	*/
	void InsertInventoryItemPublic(IEntity item, SCR_CharacterInventoryStorageComponent inventory, 
		SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role = 0, bool isAssistant = false, bool isThrowable = false)
	{
		InsertInventoryItem(item, inventory, inventoryManager, role, isAssistant, isThrowable);
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
		SCR_InventoryStorageManagerComponent inventoryManager, CRF_EGearRole role = 0, bool isAssistant = false, bool isThrowable = false)
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
	TIntArray FilterItemToClothing(IEntity item, CRF_EGearRole role = 0, bool isAssistant = false, bool isThrowable = false)
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
		
		bool isAssistantMagazine = isAssistant && MagazineComponent.Cast(item.FindComponent(MagazineComponent));
		
		bool isExplosive = IsExplosiveOrTool(item);

		// Magazines and throwables go in vest, armor, backpack primarily
		if (isMagazine)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST, 
				CRF_EGearscriptClothing.BACKPACK, 
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

		// Medical items go in backpack, vest primarily
		if (isMedical)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK, 
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST
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

		// Assistant mags go in backpack, vest primarily
		if (isAssistantMagazine)
		{
			clothingIDs = {
				CRF_EGearscriptClothing.BACKPACK,
				CRF_EGearscriptClothing.VEST, 
				CRF_EGearscriptClothing.ARMOREDVEST
			};
		}

		// Explosives and tools go in backpack, vest primarily
		if (isExplosive)
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
	
	void SpawnVehicle(CRF_VehicleSpawner spawner)
	{
		if (!spawner.m_sFactionKey)
		{
			Debug.Error("No Faction Key set on " + spawner.m_rVehicle + " spawner");
			return;
		}
		//Do not spawn the vehicle if the faction doesn't have the tickets
		//Handles subtracting tickets from kills that are on a timer. This means tickets are subtracted WHEN the vehicle is spawned
		if (spawner.m_bWaitingToRespawn && !spawner.m_bShouldRespawnOnSideRespawn)
		{
			if (spawner.m_RespawnManager.GetFactionTickets(spawner.m_sFactionKey) != 0 && spawner.m_RespawnManager.GetFactionTickets(spawner.m_sFactionKey) < spawner.m_iTicketsPerRespawn)
				return;
		
			if (spawner.m_RespawnManager.TicketsRemaining(spawner.m_sFactionKey))
				spawner.m_RespawnManager.SubtractTicket(spawner.m_sFactionKey, spawner.m_iTicketsPerRespawn);
		}
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		spawner.GetTransform(params.Transform);
		IEntity vehicle = GetGame().SpawnEntityPrefab(Resource.Load(spawner.m_rVehicle), GetGame().GetWorld(), params);
		GetGame().GetCallqueue().CallLater(SetVehicle, 1000, false, vehicle, spawner);
	}
	
	void SetVehicle(IEntity vehicleEntity, CRF_VehicleSpawner spawner)
	{
		spawner.m_eVehicle = vehicleEntity;
		Vehicle vehicle = Vehicle.Cast(spawner.m_eVehicle);
		if (vehicle)
		{
			vehicle.m_iVehicleSpawnerIndex = spawner.m_iVehicleSpawnerIndex;
			vehicle.m_sFactionKey = spawner.m_sFactionKey;
			if (spawner.m_OverridedVehicleLoadout)
				vehicle.m_OverridedVehicleLoadout = spawner.m_OverridedVehicleLoadout;
			if (spawner.m_aVehicleGearscriptOverrides.Count() > 0)
				vehicle.m_aVehicleGearscriptOverrides = spawner.m_aVehicleGearscriptOverrides;
			if (spawner.m_aAdditionalVehicleItems.Count() > 0)
				vehicle.m_aAdditionalVehicleItems = spawner.m_aAdditionalVehicleItems;
		}
	}
};