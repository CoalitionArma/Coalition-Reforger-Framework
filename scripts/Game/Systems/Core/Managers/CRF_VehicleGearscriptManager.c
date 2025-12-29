modded class CRF_GearscriptManager
{
	protected SCR_EntityCatalogManagerComponent m_CatalogManager; // PERFORMANCE OPTIMIZATION
	ref array<IEntity> m_VehiclesInQueue = {};
	
	// Resource cache to avoid repeated Resource.Load() calls - PERFORMANCE OPTIMIZATION
	protected ref map<ResourceName, ref Resource> m_mResourceCache = new map<ResourceName, ref Resource>();
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Get cached resource to avoid repeated Resource.Load() calls
	 * @param resourceName Resource to load/retrieve from cache
	 * @return Cached or newly loaded resource
	 */
	protected Resource GetCachedResource(ResourceName resourceName)
	{
		if (m_mResourceCache.Contains(resourceName))
			return m_mResourceCache.Get(resourceName);
		
		Resource res = Resource.Load(resourceName);
		if (res)
			m_mResourceCache.Set(resourceName, res);
		
		return res;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
	super.OnPostInit(owner);

	// Only run on in-game post init
	if (!GetGame().InPlayMode())
		return;
	
	m_Gamemode = CRF_Gamemode.GetInstance();
	m_CatalogManager = SCR_EntityCatalogManagerComponent.GetInstance(); // Cache catalog manager - PERFORMANCE OPTIMIZATION
	#ifdef WORKBENCH
	#else
	if (!System.IsConsoleApp())
		return;
	#endif
	SetEventMask(owner, EntityEvent.FRAME);
	}	
	
	array<int> GetSupplyValuesForItems(array<ResourceName> items)
	{
		// Pre-allocate array capacity - PERFORMANCE OPTIMIZATION
		array<int> itemSupply = new array<int>();
		itemSupply.Reserve(items.Count());
		
		foreach(ResourceName item: items)
		{
			itemSupply.Insert(0);
		}
		
		array<Faction> factions = new array<Faction>();
		FactionManager factionManager = GetGame().GetFactionManager();
		
		if (!factionManager)
			return itemSupply;
		
		factionManager.GetFactionsList(factions);
		
		// Pre-allocate catalogs array - PERFORMANCE OPTIMIZATION
		array<ref SCR_EntityCatalog> itemCatalogs = new array<ref SCR_EntityCatalog>();
		itemCatalogs.Reserve(factions.Count());
		
		// Use cached catalog manager - PERFORMANCE OPTIMIZATION
		if (!m_CatalogManager)
			m_CatalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		
		if (!m_CatalogManager)
			return itemSupply;
	
		foreach (Faction faction: factions)
		{
			SCR_EntityCatalog catalog = m_CatalogManager.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, faction.GetFactionKey(), false);
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
			// Pre-allocate array capacity - PERFORMANCE OPTIMIZATION
			array<IEntity> vehiclesToRemove = new array<IEntity>();
			vehiclesToRemove.Reserve(m_VehiclesInQueue.Count());
			
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
		
		// Cache GetGame() reference - PERFORMANCE OPTIMIZATION
		ArmaReforgerScripted game = GetGame();
		
		array<AIAgent> agents = {};
		game.GetAIWorld().GetAIAgents(agents);	
		foreach (AIAgent agent: agents)
		{
			IEntity aiPlayer = agent.GetControlledEntity();
			if (!aiPlayer)
				continue;
			
			if (!ChimeraCharacter.Cast(aiPlayer))
				continue;
			
			// Cache component lookup - PERFORMANCE OPTIMIZATION
			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent));
			if (!factionComp)
				continue;
			
			if (!closestPlayer)
			{
				int distance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
				if (distance > 200)
					continue;
				
				closestPlayerDistance = distance;
				closestPlayer = aiPlayer;
				factionKey = factionComp.GetAffiliatedFactionKey();
				continue;
			}
			
			float playerDistance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
			if (playerDistance > closestPlayerDistance || playerDistance > 200)
				continue;
			
			closestPlayer = aiPlayer;
			closestPlayerDistance = playerDistance;
			factionKey = factionComp.GetAffiliatedFactionKey();
		}		//There's no players
			if (!closestPlayer)
				return false;
			
		Vehicle.Cast(vehicle).m_sFactionKey = factionKey;
		// Cache manager reference - PERFORMANCE OPTIMIZATION
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		if (gearscriptManager)
		{
			game.GetCallqueue().CallLater(
				gearscriptManager.SetVehicleGear, 500, false,
				vehicle, Vehicle.Cast(vehicle).m_sFactionKey
			);
		}
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
	
	//------------------------------------------------------------------------------------------------
	// Get vehicle resupply cost from map
	int GetTruckResupplyCost(ResourceName resource)
	{
		return m_mVehicleSupplyCosts.Get(resource);
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
	// Cache GetGame() reference - PERFORMANCE OPTIMIZATION
	ChimeraGame game = GetGame();
	
	//Lets find a faction, if there is none start looking for one in the loop.
	Faction faction = SCR_FactionManager.Cast(game.GetFactionManager()).GetFactionByKey(factionKey);
	if (!faction)
	{	
		float closestPlayerDistance;
		IEntity closestPlayer;
		factionKey = "";
		array<AIAgent> agents = {};
		
		game.GetAIWorld().GetAIAgents(agents);		foreach (AIAgent agent: agents)
		{
			IEntity aiPlayer = agent.GetControlledEntity();
			if (!aiPlayer)
				continue;
			
			if (!ChimeraCharacter.Cast(aiPlayer))
				continue;
			
			// Cache component lookup - PERFORMANCE OPTIMIZATION
			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(aiPlayer.FindComponent(FactionAffiliationComponent));
			
			if (!closestPlayer)
			{
				closestPlayerDistance = vector.Distance(vehicle.GetOrigin(), aiPlayer.GetOrigin());
				if (closestPlayerDistance > 200)
					continue;
				closestPlayer = aiPlayer;
				if (factionComp)
				{
					factionKey = factionComp.GetAffiliatedFactionKey();
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
			if (factionComp)
				{
					factionKey = factionComp.GetAffiliatedFactionKey();
				}
				else
					factionKey = "CIV";
		}			//There's no players
			if (!closestPlayer)
			{
				m_VehiclesInQueue.Insert(vehicle);
				return;
			}

			

		
		faction = game.GetFactionManager().GetFactionByKey(factionKey);
		Vehicle.Cast(vehicle).m_sFactionKey = faction.GetFactionKey();
	}		ref CRF_GearScriptContainer gsContainer = GetGearScriptSettings(faction.GetFactionKey());
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
		if (Vehicle.Cast(truck).m_bShouldAddAmmo)
		{
			suppliesNeeded += ApplyTruckLoadout(truck, invManager, gsContainer, faction.GetFactionKey(), isSupply);
			array<ResourceName> heGLsToAdd = {};
			heGLsToAdd.Reserve(8);
			array<ResourceName> glsToAdd = {};
			glsToAdd.Reserve(8);
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
					// Pre-allocate based on weapons and typical magazine counts
					int estimatedMagazines = weapons.Count() * 2; // Estimate 2 magazine types per weapon
					magazinesToAdd.Reserve(estimatedMagazines);
					magazineCounts.Reserve(estimatedMagazines);
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
					// Pre-allocate for magazine arrays
					if (weapon.m_MagazineArray)
					{
						magazinesToAdd.Reserve(weapon.m_MagazineArray.Count());
						magazineCounts.Reserve(weapon.m_MagazineArray.Count());
					}
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
			// Pre-allocate based on default inventory items
			if (gearSriptConfig.m_DefaultInventoryItems)
			{
				int itemCount = gearSriptConfig.m_DefaultInventoryItems.Count();
				grenadesToAdd.Reserve(itemCount / 2); // Estimate half might be grenades
				smokesToAdd.Reserve(itemCount / 2); // Estimate half might be smokes
			}
			foreach (CRF_Inventory_Item item: gearSriptConfig.m_DefaultInventoryItems)
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
		
		// Server: Add vehicle to catalog and replicate to clients
		if (!m_mVehicleSupplyCosts.Contains(truck.GetPrefabData().GetPrefabName()))
		{
			if (!Replication.IsServer())
				return;
				
			m_mVehicleSupplyCosts.Set(truck.GetPrefabData().GetPrefabName(), suppliesNeeded);
			
			// Send only this vehicle's data to clients via broadcast manager
			CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
			if (broadcastManager)
				broadcastManager.AddVehicleSupplyCost(truck.GetPrefabData().GetPrefabName(), suppliesNeeded);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Client-side: Add vehicle cost entry (called by RPC handler in broadcast manager)
	void AddVehicleCostClient(ResourceName vehicleResource, int supplyCost)
	{
		m_mVehicleSupplyCosts.Set(vehicleResource, supplyCost);
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
	
	//------------------------------------------------------------------------------------------------
	// Check if vehicle supply cost has been calculated
	bool HasSupplyBeenCalculated(ResourceName resource)
	{
		return m_mVehicleSupplyCosts.Contains(resource);
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
		Resource itemLoaded = GetCachedResource(item);
		if (!itemLoaded)
			return;
		
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
		Resource weaponLoaded = GetCachedResource(weapon);
		if (!weaponLoaded)
			return false;
		
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
		Resource magazine = GetCachedResource(resource);
		if (!magazine)
			return 0;
		
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

		switch(index)
		{
			case 0:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Rifles)
				weapons.Insert(weapon);
			break;
			
			case 1:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_RifleUGLs)
				weapons.Insert(weapon);
			break;
			
			case 2:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Carbines)
				weapons.Insert(weapon);
			break;
			
			case 3:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Pistols)
				weapons.Insert(weapon);
			break;
				
			case 11:
			weapons.Insert(gearSriptConfig.m_SNIPER);
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
		
		switch (index)
		{
			case 4:
			weapon = gearSriptConfig.m_AR;
			break;
			
			case 5:
			weapon = gearSriptConfig.m_MMG;
			break;
			
			case 6:
			weapon = gearSriptConfig.m_HMG;
			break;
			
			case 7:
			weapon = gearSriptConfig.m_AT;
			break;
			
			case 8:
			weapon = gearSriptConfig.m_MAT;
			break;
			
			case 9:
			weapon = gearSriptConfig.m_HAT;
			break;
			
			case 10:
			weapon = gearSriptConfig.m_AA;
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
		Resource glLoaded = GetCachedResource(glToCheck);
		if (!glLoaded)
			return false;
		
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
		Resource magazine = GetCachedResource(magazineToCheck);
		if (!magazine)
			return false;
		
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
		
		Resource weaponLoaded = GetCachedResource(weaponToCheck.m_Weapon);
		if (!weaponLoaded)
			return false;
		
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
		Resource magazine = GetCachedResource(magazineToCheck);
		if (!magazine)
			return false;
		
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
			Resource weaponLoaded = GetCachedResource(weapon.m_Weapon);
			if (!weaponLoaded)
				return false;
			
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
			if (!spawner.m_bShouldAddAmmo)
				vehicle.m_bShouldAddAmmo = false;
		}
	}
}