class CRF_VehicleGearscriptManagerClass : ScriptComponentClass
{
}

class CRF_VehicleGearscriptManager : ScriptComponent
{
	protected ref map<ResourceName, int> m_mVehicleSupplyCosts = new map<ResourceName, int>;
	protected SCR_EntityCatalogManagerComponent m_CatalogManager; // PERFORMANCE OPTIMIZATION
	protected ref array<Vehicle> m_aSpawnedVehicles = new array<Vehicle>;
	protected ref array<IEntity> m_VehiclesInQueue = new array<IEntity>;

	// Shared resource caching helper (also used by COA_GearscriptManager) instead of a second
	// hand-rolled map<ResourceName, Resource> - PERFORMANCE OPTIMIZATION
	protected ref COA_ResourceCache m_ResourceCache = new COA_ResourceCache();

	// These are pure facts about a static prefab resource (does it disable, how much ammo does its
	// magazine hold, is its GL round HE) that never change mid-mission, so memoize them instead of
	// re-walking the prefab's component sources on every vehicle spawn/refit - PERFORMANCE OPTIMIZATION
	protected ref map<ResourceName, bool> m_mIsWeaponDisposableCache = new map<ResourceName, bool>();
	protected ref map<ResourceName, int> m_mMagazineCountCache = new map<ResourceName, int>();
	protected ref map<ResourceName, bool> m_mIsGLHECache = new map<ResourceName, bool>();

//=============================================================================================================================================================================================================================================================================================================================================================
//	 VEHICLE SPAWN SCHEDULING
//=============================================================================================================================================================================================================================================================================================================================================================

	// Vehicle spawners used to spawn themselves synchronously inside their own EOnInit, and each held
	// EntityEvent.FRAME for the whole mission just to decrement a respawn timer. On a map like Eden
	// Prejoin that is 42 vehicles created in a single frame during world streaming, followed by 42
	// synchronised gearscript applications 2.5s later (each of which spawns a full inventory), plus
	// 42 permanent per-entity frame callbacks.
	//
	// Vanilla never does this: SCR_AmbientVehicleSpawnPointComponent only REGISTERS itself in
	// OnPostInit, and SCR_AmbientVehicleSystem spawns exactly one spawnpoint per update tick,
	// round-robin, from a single server-side system. This mirrors that: spawners register here, and
	// this manager drains a queue a few at a time so the work is spread across frames instead of
	// landing in one.

	[Attribute("2", desc: "Maximum vehicles spawned per queue drain. Keep low - this exists to stop every spawner firing in the same frame.", params: "1 16 1", category: "CRF Vehicle Spawning")]
	protected int m_iMaxSpawnsPerTick;

	[Attribute("0.25", desc: "Seconds between vehicle spawn queue drains.", params: "0.05 5 0.05", category: "CRF Vehicle Spawning")]
	protected float m_fSpawnQueuePeriod;

	[Attribute("5", desc: "Radius in metres that must be clear before a vehicle spawns. Matches vanilla SCR_AmbientVehicleSpawnPointComponent.SPAWNING_RADIUS.", params: "1 30 0.5", category: "CRF Vehicle Spawning")]
	protected float m_fSpawnClearanceRadius;

	[Attribute("15", desc: "Seconds to wait before retrying a spawner whose spawn point was blocked.", params: "1 120 1", category: "CRF Vehicle Spawning")]
	protected float m_fBlockedSpawnRetryDelay;

	//! Every registered spawner. Used to drive respawn timers centrally.
	protected ref array<COA_VehicleSpawner> m_aRegisteredSpawners = new array<COA_VehicleSpawner>();

	//! Spawners waiting for their turn to spawn (initial spawn or an elapsed respawn timer).
	protected ref array<COA_VehicleSpawner> m_aPendingSpawns = new array<COA_VehicleSpawner>();

	//! Scratch buffer for spawners that turned out to be dead during a drain, so the queue is never
	//! mutated while it is being indexed.
	protected ref array<COA_VehicleSpawner> m_aStaleSpawners = new array<COA_VehicleSpawner>();

	protected float m_fSpawnQueueTick = 0;

	//------------------------------------------------------------------------------------------------
	//! Clamped accessors. These are prefab attributes, so a bad value on one game mode prefab would
	//! otherwise silently break vehicle spawning for an entire mission - a batch size of 0 means the
	//! queue never drains and no vehicle ever spawns.
	protected int GetMaxSpawnsPerTick()
	{
		return Math.Max(1, m_iMaxSpawnsPerTick);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetSpawnQueuePeriod()
	{
		return Math.Max(0.05, m_fSpawnQueuePeriod);
	}

	//------------------------------------------------------------------------------------------------
	//! Called by COA_VehicleSpawner.EOnInit instead of spawning inline. Queues the initial spawn.
	void RegisterSpawner(COA_VehicleSpawner spawner)
	{
		if (!spawner)
			return;

		if (!m_aRegisteredSpawners.Contains(spawner))
			m_aRegisteredSpawners.Insert(spawner);

		QueueSpawn(spawner);
	}

	//------------------------------------------------------------------------------------------------
	void UnregisterSpawner(COA_VehicleSpawner spawner)
	{
		m_aRegisteredSpawners.RemoveItem(spawner);
		m_aPendingSpawns.RemoveItem(spawner);
	}

	//------------------------------------------------------------------------------------------------
	//! Enqueue a spawner. Safe to call repeatedly - a spawner is only ever queued once.
	void QueueSpawn(COA_VehicleSpawner spawner)
	{
		if (!spawner || m_aPendingSpawns.Contains(spawner))
			return;

		m_aPendingSpawns.Insert(spawner);
	}

	//------------------------------------------------------------------------------------------------
	//! Central respawn-timer tick. This replaces the per-spawner EOnFrame: one component ticks for
	//! all spawners rather than every spawner ticking for itself.
	//! StartRespawnTimer() on the spawner still just sets m_fTimer/m_bWaitingToRespawn, so the
	//! damage-manager path that calls it is unchanged.
	protected void UpdateSpawnerTimers(float timeSlice)
	{
		m_aStaleSpawners.Clear();

		foreach (COA_VehicleSpawner spawner : m_aRegisteredSpawners)
		{
			if (!spawner)
			{
				m_aStaleSpawners.Insert(spawner);
				continue;
			}

			if (spawner.m_fTimer > 0)
				spawner.m_fTimer -= timeSlice;

			// The flag is deliberately NOT cleared here - SpawnVehicle() reads it to decide whether
			// this is a respawn that should cost tickets, and clears it itself once the spawn is
			// committed. QueueSpawn() dedupes, so re-queueing on subsequent ticks is harmless.
			if (spawner.m_bWaitingToRespawn && spawner.m_fTimer <= 0)
				QueueSpawn(spawner);
		}

		foreach (COA_VehicleSpawner stale : m_aStaleSpawners)
			m_aRegisteredSpawners.RemoveItem(stale);
	}

	//------------------------------------------------------------------------------------------------
	//! Drain up to m_iMaxSpawnsPerTick spawners from the queue.
	protected void DrainSpawnQueue()
	{
		int spawned = 0;
		int maxSpawns = GetMaxSpawnsPerTick();

		while (spawned < maxSpawns && !m_aPendingSpawns.IsEmpty())
		{
			COA_VehicleSpawner spawner = m_aPendingSpawns[0];
			m_aPendingSpawns.Remove(0);

			if (!spawner)
				continue;

			// A blocked spawn point is not a failure - requeue it on a delay and move on, so a
			// vehicle parked on a spawn pad just postpones that one spawner instead of losing it.
			if (!SpawnVehicle(spawner))
			{
				spawner.m_fTimer = m_fBlockedSpawnRetryDelay;
				spawner.m_bWaitingToRespawn = true;
			}

			spawned++;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
	
		// Only run on in-game post init
		if (!GetGame().InPlayMode())
			return;
		
		m_CatalogManager = SCR_EntityCatalogManagerComponent.GetInstance(); // Cache catalog manager - PERFORMANCE OPTIMIZATION
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		SetEventMask(owner, EntityEvent.FRAME);
	}	
	
	//------------------------------------------------------------------------------------------------
	array<Vehicle> GetSpawnedVehicleArray()
	{
		return m_aSpawnedVehicles;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddVehicleToSpawnedArray(Vehicle vehicle)
	{
		if (m_aSpawnedVehicles.Contains(vehicle))
			return;
		
		m_aSpawnedVehicles.Insert(vehicle);
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveVehicleFromSpawnedArray(Vehicle vehicle)
	{
		m_aSpawnedVehicles.RemoveItem(vehicle);

		// Also drop it from the faction-resolution queue. This was previously missed, so a vehicle
		// deleted while still queued left a dangling pointer that EOnFrame dereferenced every 5s
		// via FindFactionByClosestPlayer() for the rest of the mission. RemoveItem() is a no-op if
		// the vehicle was never queued, and both calls are pointer comparisons - no dereference.
		m_VehiclesInQueue.RemoveItem(vehicle);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load vehicle gear script config from resource
	//! \param[in] resourceName Resource to load
	//! \return Loaded config or null if failed
	protected CRF_VehicleGearscriptConfig LoadVehicleGearScriptConfig(ResourceName resourceName)
	{
		return CRF_VehicleGearscriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(resourceName).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get cached resource to avoid repeated Resource.Load() calls
	//! \param[in] resourceName Resource to load/retrieve from cache
	//! \return Cached or newly loaded resource
	protected Resource GetCachedResource(ResourceName resourceName)
	{
		return m_ResourceCache.GetCachedResource(resourceName);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Gets Supply values for the inputed items
	//! \param[in] items the array of resource names you want the supply counts for
	//! \return an array of ints representing the supply values in the same order as the items put in
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
	
	//------------------------------------------------------------------------------------------------
	//! Frame logic for vehicles getting the closest available faction to fill their inventory with that factiosn gear
	float m_fUpdateBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Central spawner tick. This single frame callback now serves every vehicle spawner in the
		// mission, replacing one EntityEvent.FRAME per spawner. Both the timer update and the queue
		// drain run on the same throttle, so respawn timers cost one pass every m_fSpawnQueuePeriod
		// rather than a pass every frame.
		m_fSpawnQueueTick += timeSlice;
		if (m_fSpawnQueueTick >= GetSpawnQueuePeriod())
		{
			float spawnerDelta = m_fSpawnQueueTick;
			m_fSpawnQueueTick = 0;

			UpdateSpawnerTimers(spawnerDelta);

			if (!m_aPendingSpawns.IsEmpty())
				DrainSpawnQueue();
		}

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
	
	//------------------------------------------------------------------------------------------------
	//! Used in EOnFrame to check local entities around a vehicle to get the closest faction entity near it
	//! \param[in] vehicle the vehicle we are checking for
	//! \return bool if it was succesful or not
	bool FindFactionByClosestPlayer(IEntity vehicle)
	{	
		float closestPlayerDistance;
		IEntity closestPlayer;
		string factionKey = "";
		
		// Cache GetGame() reference - PERFORMANCE OPTIMIZATION
		ArmaReforgerScripted game = GetGame();
		if (!game)
			return false;
		
		AIWorld aiWorld = game.GetAIWorld();
		
		if (!aiWorld)
			return false;
		
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);	
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

		// Resolved through replication rather than a raw entity pointer, which the call queue would
		// otherwise hold for the full 500ms delay - if the vehicle is destroyed in that window
		// (e.g. ambushed right after spawning), SetVehicleGear's `if (!vehicle)` guard does not
		// detect an entity the engine has already deleted. See SetVehicleGearById().
		RplComponent vehicleRpl = RplComponent.Cast(vehicle.FindComponent(RplComponent));
		if (!vehicleRpl)
			return true;

		game.GetCallqueue().CallLater(
			SetVehicleGearById, 500, false,
			vehicleRpl.Id(), Vehicle.Cast(vehicle).m_sFactionKey
		);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Gets the total supply value of all items combined in the vehilce
	//! \param[in] truck the truck you want to check
	//! \return the total supply value of the items in the truck
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
	//! Deferred entry point for SetVehicleGear, keyed on RplId instead of an entity pointer.
	//! Callers that schedule gear application through the call queue must use this: the queue holds
	//! its arguments for the whole delay, and the `if (!vehicle)` guard in SetVehicleGear does NOT
	//! detect an entity the engine has already deleted - it only catches a pointer that was never
	//! set. Resolving through replication at call time is what actually makes this safe.
	//! \param[in] vehicleId RplId of the vehicle to equip
	//! \param[in] factionKey The key identifying the faction to use for gear configuration
	void SetVehicleGearById(RplId vehicleId, string factionKey)
	{
		RplComponent vehicleRpl = RplComponent.Cast(Replication.FindItem(vehicleId));
		if (!vehicleRpl)
			return;

		IEntity vehicle = vehicleRpl.GetEntity();
		if (!vehicle)
			return;

		SetVehicleGear(vehicle, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Set gear for a vehicle entity based on its faction key
	//!
	//! Attempts to find the correct faction for the vehicle. If no faction is provided,
	//! it searches for the closest player within 200m to determine the faction. Then,
	//! it applies the appropriate gear loadout to the vehicle, checking if it's a supply truck.
	//!
	//! \param[in] vehicle The vehicle entity to equip with gear
	//! \param[in] factionKey The key identifying the faction to use for gear configuration
	void SetVehicleGear(IEntity vehicle, string factionKey)
	{
		// Check if vehicle still exists (prevents crash when vehicle is deleted during queued operation)
		if (!vehicle)
			return;
		
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
		}		
		
		// Respect the per-side enable/disable toggle on the gamemode prefab.
		if (!COA_Gamemode.GetInstance().IsVehicleGearscriptEnabled(faction.GetFactionKey()))
			return;

		ref COA_GearScriptContainer gsContainer = COA_Gamemode.GetInstance().GetGearScriptSettings(faction.GetFactionKey());
		if (gsContainer.m_aSupplyTrucks.Contains(vehicle.GetPrefabData().GetPrefabName()))
			SetTruckGear(vehicle, faction, gsContainer, true);
		else
			SetTruckGear(vehicle, faction, gsContainer, false);
			
	}
	
	//------------------------------------------------------------------------------------------------
	//! Checks to see if the inputed vehicle is a supply truck
	//! \param[in] truck the truck we are checking
	//! \param[in] factionKey what faction we check to see if this truck is in their supply truck array
	//! \return an array of ints representing the supply values in the same order as the items put in
	bool IsSupplyTruck(IEntity truck, string factionKey)
	{
		ref COA_GearScriptContainer gsContainer = COA_Gamemode.GetInstance().GetGearScriptSettings(factionKey);
		return gsContainer.m_aSupplyTrucks.Contains(truck.GetPrefabData().GetPrefabName());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Configures a truck’s inventory and equipment loadout
	//!
	//! Clears existing inventory, applies the configured loadout, and spawns weapons,
	//! magazines, grenades, smoke, and additional faction-specific items. Handles both
	//! supply trucks and regular vehicles.
	//!
	//! \param[in] truck The truck entity to configure
	//! \param[in] faction The faction object used to determine loadout
	//! \param[in] gsContainer The gear script container holding loadout data
	//! \param[in] isSupply Whether the truck is a supply truck (true) or a regular vehicle (false)
	void SetTruckGear(IEntity truck, Faction faction, COA_GearScriptContainer gsContainer, bool isSupply)
	{
		// Check if truck still exists (prevents crash when vehicle is deleted)
		if (!truck)
			return;
		
		ref COA_GearScriptConfig gearSriptConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(gsContainer.m_rGearScript);
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
					array<ref COA_Weapon_Class> weapons = GetWeaponsByIndex(i, gearSriptConfig);
					if (weapons.Count() == 0)
						continue;
					// Pre-allocate based on weapons and typical magazine counts
					int estimatedMagazines = weapons.Count() * 2; // Estimate 2 magazine types per weapon
					magazinesToAdd.Reserve(estimatedMagazines);
					magazineCounts.Reserve(estimatedMagazines);
					foreach (COA_Weapon_Class weapon: weapons)
					{
						if (!weapon)
							continue;
						
						if (!weapon.m_MagazineArray)
							continue;
						foreach (COA_Magazine_Class magazine: weapon.m_MagazineArray)
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
					COA_Spec_Weapon_Class weapon = GetSpecWeaponByIndex(i, gearSriptConfig);
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
						foreach (COA_Magazine_Class magazine: weapon.m_MagazineArray)
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
			foreach (COA_Inventory_Item item: gearSriptConfig.m_DefaultInventoryItems)
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
			COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
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
	//! Removes all existing items from a truck’s inventory
	//!
	//! Iterates through the truck’s inventory storage manager and deletes all entities found.
	//!
	//! \param[in] truck The truck whose inventory will be cleared
	//! \param[in] invManager The truck’s inventory storage manager component
	void ClearTruckGear(IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager)
	{
		// Collect all items owned by turret weapon storages so they are not deleted.
		// Turret ammo is defined by the vehicle prefab and must not be touched by gearscript.
		set<IEntity> turretItems = new set<IEntity>();
		// Track prefab names of loaded turret ammo so extra copies in cargo are preserved.
		// CanInsertResourceInStorage returns false when the weapon storage is already full (magazine loaded),
		// causing cargo ammo for the turret to be incorrectly deleted. Tracking by prefab name fixes this.
		array<ResourceName> turretAmmoPrefabs = {};
		SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(truck.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compartmentMan)
		{
			array<BaseCompartmentSlot> turrets = {};
			compartmentMan.GetCompartmentsOfType(turrets, ECompartmentType.TURRET);
			foreach (BaseCompartmentSlot turret : turrets)
			{
				TurretControllerComponent turretController = TurretControllerComponent.Cast(turret.GetController());
				if (!turretController)
					continue;
				BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
				if (!weaponManager)
					continue;
				array<IEntity> turretWeapons = {};
				weaponManager.GetWeaponsList(turretWeapons);
				foreach (IEntity weapon : turretWeapons)
				{
					BaseInventoryStorageComponent weaponStorage = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
					if (!weaponStorage)
						continue;
					array<IEntity> weaponItems = {};
					weaponStorage.GetAll(weaponItems);
					foreach (IEntity item : weaponItems)
					{
						turretItems.Insert(item);
						ResourceName prefabName = item.GetPrefabData().GetPrefabName();
						if (!turretAmmoPrefabs.Contains(prefabName))
							turretAmmoPrefabs.Insert(prefabName);
					}
				}
			}
		}

		array<IEntity> items = {};
		invManager.GetItems(items);
		foreach (IEntity item: items)
		{
			if (!item)
				continue;
			if (turretItems.Contains(item))
				continue;
			if (turretAmmoPrefabs.Contains(item.GetPrefabData().GetPrefabName()))
				continue;
			if (CanStoreResourceInTurretWeaponStorage(item.GetPrefabData().GetPrefabName(), truck, invManager))
				continue;
			
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Applies a predefined loadout to a truck, including turret weapons and ammo
	//!
	//! Loads the correct vehicle loadout (either overridden or default), then spawns
	//! ammunition into the truck’s storage.
	//!
	//! \param[in] truck The truck entity to configure
	//! \param[in] invManager The truck’s inventory storage manager component
	//! \param[in] gsContainer The gear script container holding vehicle loadout data
	int ApplyTruckLoadout(IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager, COA_GearScriptContainer gsContainer, string factionKey, bool isSupply)
	{
		ref CRF_VehicleGearScriptLoadout vehLoadout;
		int suppliesNeeded = 0;
		bool calculateSupplies = HasSupplyBeenCalculated(truck.GetPrefabData().GetPrefabName());
		if (Vehicle.Cast(truck).m_OverridedVehicleLoadout)
			vehLoadout = Vehicle.Cast(truck).m_OverridedVehicleLoadout;
		else
			vehLoadout = gsContainer.m_VehicleLoadout;
		// Turret ammo is intentionally left untouched — it is defined by the vehicle prefab.
		// ClearTruckGear already skips turret-owned items, and we do not re-fill them here.
		if (vehLoadout && !vehLoadout.m_rRepairKitPrefab.IsEmpty())
		{
			for (int i = 0; i < vehLoadout.m_iAmountOfRepairKits; i++)
			{
				invManager.TrySpawnPrefabToStorage(vehLoadout.m_rRepairKitPrefab);
			}
		}
		return suppliesNeeded;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Spawns magazines into a vehicle’s inventory
	//!
	//! Loops until the requested amount of ammunition is added, distributing across
	//! multiple magazine types.
	//!
	//! \param[in] amountToSpawn Total number of bullets to distribute
	//! \param[in] magazineCounts Array of magazine capacities
	//! \param[in] magazinesToAdd Array of magazine resource names
	//! \param[in] invManager Vehicle’s inventory storage manager component
	//! \param[in] isSupply Whether this is a supply vehicle (full load) or not (reduced load)
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
				if (TrySpawnPrefabToVehicleOrTurretStorage(magazinesToAdd[i], invManager))
				{
					amountToSpawn -= magazineCounts[i];
					magazinesAdded.Set(i, magazinesAdded.Get(i) + 1);
				}
					
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
	//! Spawns items (e.g., grenades, disposable launchers) into a vehicle’s inventory
	//!
	//! Iterates through provided items and spawns them until the requested amount is added.
	//!
	//! \param[in] amountToSpawn Number of items to spawn
	//! \param[in] itemsToSpawn Array of item resource names
	//! \param[in] invManager Vehicle’s inventory storage manager component
	//! \param[in] isSupply Whether this is a supply vehicle (full load) or not (reduced load)
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
				if (TrySpawnPrefabToVehicleOrTurretStorage(itemsToSpawn.Get(i), invManager))
				{
					itemsAdded.Set(i, itemsAdded.Get(i) + 1);
					amountToSpawn--;
				}
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
	//! Spawns an item into normal vehicle cargo, falling back to turret weapon storage for large weapon magazines.
	protected bool TrySpawnPrefabToVehicleOrTurretStorage(ResourceName prefab, SCR_VehicleInventoryStorageManagerComponent invManager)
	{
		if (!invManager)
			return false;

		if (invManager.TrySpawnPrefabToStorage(prefab))
			return true;

		return TrySpawnPrefabToTurretWeaponStorage(prefab, invManager.GetOwner(), invManager);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns true if a prefab can be stored in one of the vehicle's turret weapon storages.
	protected bool CanStoreResourceInTurretWeaponStorage(ResourceName prefab, IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager)
	{
		if (prefab.IsEmpty() || !truck || !invManager)
			return false;

		array<BaseInventoryStorageComponent> turretWeaponStorages = {};
		GetTurretWeaponStorages(truck, turretWeaponStorages);

		foreach (BaseInventoryStorageComponent storage : turretWeaponStorages)
		{
			if (invManager.CanInsertResourceInStorage(prefab, storage))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Attempts to place an item directly into a compatible turret weapon storage.
	protected bool TrySpawnPrefabToTurretWeaponStorage(ResourceName prefab, IEntity truck, SCR_VehicleInventoryStorageManagerComponent invManager)
	{
		if (prefab.IsEmpty() || !truck || !invManager)
			return false;

		array<BaseInventoryStorageComponent> turretWeaponStorages = {};
		GetTurretWeaponStorages(truck, turretWeaponStorages);

		foreach (BaseInventoryStorageComponent storage : turretWeaponStorages)
		{
			if (!invManager.CanInsertResourceInStorage(prefab, storage))
				continue;

			if (invManager.TrySpawnPrefabToStorage(prefab, storage))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Collects inventory storages owned by weapons mounted in turret compartments.
	protected void GetTurretWeaponStorages(IEntity truck, out notnull array<BaseInventoryStorageComponent> turretWeaponStorages)
	{
		if (!truck)
			return;

		SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(truck.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartmentMan)
			return;

		array<BaseCompartmentSlot> turrets = {};
		compartmentMan.GetCompartmentsOfType(turrets, ECompartmentType.TURRET);
		foreach (BaseCompartmentSlot turret : turrets)
		{
			TurretControllerComponent turretController = TurretControllerComponent.Cast(turret.GetController());
			if (!turretController)
				continue;

			BaseWeaponManagerComponent weaponManager = turretController.GetWeaponManager();
			if (!weaponManager)
				continue;

			array<IEntity> turretWeapons = {};
			weaponManager.GetWeaponsList(turretWeapons);
			foreach (IEntity weapon : turretWeapons)
			{
				BaseInventoryStorageComponent weaponStorage = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
				if (weaponStorage)
					turretWeaponStorages.Insert(weaponStorage);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Check if vehicle supply cost has been calculated
	bool HasSupplyBeenCalculated(ResourceName resource)
	{
		return m_mVehicleSupplyCosts.Contains(resource);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Gets the number of bullets allocated to a weapon type for a vehicle
	//!
	//! Looks up overrides or defaults in the vehicle gear script configuration,
	//! based on a weapon type index.
	//!
	//! \param[in] vehicle The vehicle entity
	//! \param[in] index The weapon type index
	//! \param[in] vehicleGearScript The vehicle gear script configuration
	//! \param[in] gearContainer The gear script container holding overrides
	//! \return The number of bullets to allocate
	int GetBulletCountForWeapon(IEntity vehicle, int index, CRF_VehicleGearscriptConfig vehicleGearScript, COA_GearScriptContainer gearContainer)
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
	//! Determines if an item is a grenade or smoke grenade
	//!
	//! Inspects the item’s components to identify if it is a grenade, and whether it is smoke.
	//!
	//! \param[in] item The resource name of the item to check
	//! \param[in] isGrenade Outputs true if the item is a grenade
	//! \param[in] isSmoke Outputs true if the item is a smoke grenade
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
	//! Checks if a weapon is disposable
	//!
	//! Loads the weapon prefab and inspects components to determine if it is marked as disposable.
	//!
	//! \param[in] weapon The resource name of the weapon to check
	//! \return true if the weapon is disposable, false otherwise
	bool IsWeaponDisposable(ResourceName weapon)
	{
		if (m_mIsWeaponDisposableCache.Contains(weapon))
			return m_mIsWeaponDisposableCache.Get(weapon);

		bool isDisposable = ComputeIsWeaponDisposable(weapon);
		m_mIsWeaponDisposableCache.Set(weapon, isDisposable);
		return isDisposable;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ComputeIsWeaponDisposable(ResourceName weapon)
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
	//! Gets the maximum ammo count of a magazine resource
	//!
	//! Loads the magazine prefab and extracts its MaxAmmo property from the MagazineComponent.
	//!
	//! \param[in] resource The magazine resource name
	//! \return The maximum number of bullets in the magazine, or 0 if not found
	int GetMagazineCount(ResourceName resource)
	{
		if (m_mMagazineCountCache.Contains(resource))
			return m_mMagazineCountCache.Get(resource);

		int maxAmmo = ComputeMagazineCount(resource);
		m_mMagazineCountCache.Set(resource, maxAmmo);
		return maxAmmo;
	}

	//------------------------------------------------------------------------------------------------
	protected int ComputeMagazineCount(ResourceName resource)
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
	//! Retrieves a list of standard weapons by index from the gear script config
	//!
	//! Uses an index to map to weapon categories such as rifles, carbines, pistols, etc.
	//!
	//! \param[in] index The weapon category index
	//! \param[in] gearSriptConfig The gear script configuration to use
	//! \return Array of weapon class references
	array<ref COA_Weapon_Class> GetWeaponsByIndex(int index, COA_GearScriptConfig gearSriptConfig)
	{
		array<ref COA_Weapon_Class> weapons = {};

		switch(index)
		{
			case 0:
			foreach (COA_Weapon_Class weapon: gearSriptConfig.m_Rifles)
				weapons.Insert(weapon);
			break;
			
			case 1:
			foreach (COA_Weapon_Class weapon: gearSriptConfig.m_RifleUGLs)
				weapons.Insert(weapon);
			break;
			
			case 2:
			foreach (COA_Weapon_Class weapon: gearSriptConfig.m_Carbines)
				weapons.Insert(weapon);
			break;
			
			case 3:
			foreach (COA_Weapon_Class weapon: gearSriptConfig.m_Pistols)
				weapons.Insert(weapon);
			break;
				
			case 11:
			weapons.Insert(gearSriptConfig.m_SNIPER);
			break;
		}
		
		return weapons;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Retrieves a specific special weapon by index
	//!
	//! Uses an index to fetch special weapons such as ARs, MMGs, HMGs, AT, MAT, HAT, and AA.
	//!
	//! \param[in] index The weapon type index
	//! \param[in] gearSriptConfig The gear script configuration to use
	//! \return A special weapon class reference
	COA_Spec_Weapon_Class GetSpecWeaponByIndex(int index, COA_GearScriptConfig gearSriptConfig)
	{
		COA_Spec_Weapon_Class weapon;
		
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
	//! Checks if a grenade launcher round is a high-explosive (HE) type
	//!
	//! Loads the grenade launcher resource and inspects its components for a collision component(Only explosives have this enabled).
	//!
	//! \param[in] glToCheck Resource name of the grenade launcher round
	//! \return true if the round is HE, false otherwise
	bool IsGLHE(ResourceName glToCheck)
	{
		if (m_mIsGLHECache.Contains(glToCheck))
			return m_mIsGLHECache.Get(glToCheck);

		bool isHE = ComputeIsGLHE(glToCheck);
		m_mIsGLHECache.Set(glToCheck, isHE);
		return isHE;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ComputeIsGLHE(ResourceName glToCheck)
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
	//! Checks if a magazine belongs to a given special weapon
	//!
	//! Compares magazine wells between a weapon and magazine to determine compatibility.
	//!
	//! \param[in] weaponToCheck The special weapon to check against
	//! \param[in] magazineToCheck The magazine resource name to check
	//! \return true if the magazine is valid for the weapon, false otherwise
	bool IsSpecRegularMagazine(COA_Spec_Weapon_Class weaponToCheck, ResourceName magazineToCheck)
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
	//! Checks if a magazine is compatible with a set of weapons
	//!
	//! Compares magazine wells between a magazine and each weapon in the array.
	//!
	//! \param[in] weaponsToCheck Array of weapons to check against
	//! \param[in] magazineToCheck The magazine resource name to check
	//! \return true if the magazine is compatible, false otherwise
	bool IsRegularMagazine(array<ref COA_Weapon_Class> weaponsToCheck, ResourceName magazineToCheck)
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
		
		foreach (COA_Weapon_Class weapon: weaponsToCheck)
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
					if (!weaponMagazineWell)
						continue;
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
	//! Spawn a spawner's vehicle. Called only from DrainSpawnQueue() - never inline from EOnInit.
	//! \param[in] spawner the vehicle spawner that spawned this vehicle
	//! \return true if a vehicle was spawned (or the spawn was deliberately skipped, e.g. no tickets);
	//!         false only if the spawn point was blocked and the caller should retry later
	bool SpawnVehicle(COA_VehicleSpawner spawner)
	{
		if (!spawner)
			return true;

		if (spawner.m_sFactionKey.IsEmpty())
		{
			Debug.Error("No Faction Key set on " + spawner.m_rVehicle + " spawner");
			return true;
		}

		if (spawner.m_rVehicle.IsEmpty())
		{
			Debug.Error("No Vehicle set on " + spawner + " spawner");
			return true;
		}

		// Validate the prefab before handing it to the engine. Vanilla guards every Resource.Load
		// that feeds a spawn (SCR_VehicleSpawner.PerformSpawn: `if (!resource) return;`); passing an
		// unresolved Resource into SpawnEntityPrefab is undefined. This also routes through the
		// resource cache that the rest of this class already uses, so 20 identical spawners stop
		// re-loading the same prefab 20 times.
		Resource vehicleResource = GetCachedResource(spawner.m_rVehicle);
		if (!vehicleResource || !vehicleResource.IsValid())
		{
			Print(string.Format("[CRF_VehicleGearscriptManager] Could not load vehicle prefab '%1' - check the mod set for this mission.", spawner.m_rVehicle), LogLevel.ERROR);
			return true;
		}

		// Remove whatever this spawner put here last, BEFORE the clearance test - otherwise our own
		// wreck is what blocks the pad. Previously the only DeleteEntityAndChildren calls in
		// COA_VehicleSpawner were inside #ifdef WORKBENCH, so on a live server a respawn dropped a
		// fresh vehicle straight on top of the old one.
		// Deletion is not guaranteed to be reflected in world queries the same frame, so if we did
		// remove something, take the retry path and test clearance on the next drain instead.
		if (ClearPreviousVehicle(spawner))
			return false;

		// Clearance gate, matching SCR_AmbientVehicleSpawnPointComponent: prove the pad is clear,
		// then spawn at the spawner's authored transform so the mission-maker's rotation is kept.
		// A blocked pad (a player parked on it, debris) returns false so the caller requeues rather
		// than spawning a rigid body inside another one.
		// This is deliberately ahead of the ticket accounting below: a blocked spawn must cost the
		// faction nothing, or a permanently obstructed pad would drain tickets on every retry.
		vector clearPosition;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(clearPosition, spawner.GetOrigin(), m_fSpawnClearanceRadius, m_fSpawnClearanceRadius))
			return false;

		//Do not spawn the vehicle if the faction doesn't have the tickets
		//Handles subtracting tickets from kills that are on a timer. This means tickets are subtracted WHEN the vehicle is spawned
		// m_RespawnManager is only assigned if COA_RespawnManager.GetInstance() resolved at spawner
		// init; it was previously dereferenced unguarded here, which never fired on the first spawn
		// (m_bWaitingToRespawn is false then) and so only crashed at the first respawn, minutes in.
		if (spawner.m_bWaitingToRespawn && !spawner.m_bShouldRespawnOnSideRespawn && spawner.m_RespawnManager)
		{
			int factionTickets = spawner.m_RespawnManager.GetFactionTickets(spawner.m_sFactionKey);
			if (factionTickets != 0 && factionTickets < spawner.m_iTicketsPerRespawn)
				return true;

			if (spawner.m_RespawnManager.TicketsRemaining(spawner.m_sFactionKey))
				spawner.m_RespawnManager.SubtractTicket(spawner.m_sFactionKey, spawner.m_iTicketsPerRespawn);
		}

		// Consume the pending-respawn flag only now that the spawn is actually going ahead. The
		// ticket block above reads it, so it must not be cleared before this point - the old inline
		// EOnFrame path relied on the same ordering.
		spawner.m_bWaitingToRespawn = false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		spawner.GetTransform(params.Transform);

		IEntity vehicle = GetGame().SpawnEntityPrefab(vehicleResource, GetGame().GetWorld(), params);
		if (!vehicle)
		{
			Print(string.Format("[CRF_VehicleGearscriptManager] SpawnEntityPrefab returned null for '%1'.", spawner.m_rVehicle), LogLevel.ERROR);
			return true;
		}

		SettleSpawnedVehicle(vehicle);
		SetVehicle(vehicle, spawner);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Delete the vehicle this spawner spawned previously, if it is still around.
	//! Resolved through replication rather than the raw m_eVehicle pointer, which the engine does not
	//! null when the entity is deleted.
	//! \return true if an entity was actually deleted, so the caller can let the world settle before
	//!         running a clearance query over the same spot
	protected bool ClearPreviousVehicle(notnull COA_VehicleSpawner spawner)
	{
		if (!spawner.m_VehicleRplId.IsValid())
		{
			spawner.m_eVehicle = null;
			return false;
		}

		RplComponent previousRpl = RplComponent.Cast(Replication.FindItem(spawner.m_VehicleRplId));
		spawner.m_VehicleRplId = RplId.Invalid();
		spawner.m_eVehicle = null;

		if (!previousRpl)
			return false;

		IEntity previousVehicle = previousRpl.GetEntity();
		if (!previousVehicle || previousVehicle.IsDeleted())
			return false;

		SCR_EntityHelper.DeleteEntityAndChildren(previousVehicle);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Post-spawn physics settle, copied from SCR_AmbientVehicleSpawnPointComponent.
	//! Without the handbrake a vehicle spawned on any incline rolls off its pad; without the downward
	//! velocity nudge it can rest interpenetrating the terrain instead of settling onto it.
	protected void SettleSpawnedVehicle(notnull IEntity vehicle)
	{
		CarControllerComponent carController = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
		if (carController)
			carController.SetPersistentHandBrake(true);

		Physics vehiclePhysics = vehicle.GetPhysics();
		if (vehiclePhysics)
			vehiclePhysics.SetVelocity("0 -1 0");
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets the vehicle in the vehicle spawner for internal tracking purposes
	//! \param[in] vehicleEntity the newly spawned vehicle to register in the spawner
	//! \param[in] spawner the vehicle spawner we are registering this vehicle in
	//! \return an array of ints representing the supply values in the same order as the items put in
	void SetVehicle(IEntity vehicleEntity, COA_VehicleSpawner spawner)
	{
		if (!spawner)
			return;

		spawner.m_eVehicle = vehicleEntity;

		// Also record the RplId so the spawner can identify its vehicle later without dereferencing
		// a pointer the engine may already have freed. See ClearPreviousVehicle().
		spawner.m_VehicleRplId = RplId.Invalid();
		if (vehicleEntity)
		{
			RplComponent vehicleRpl = RplComponent.Cast(vehicleEntity.FindComponent(RplComponent));
			if (vehicleRpl)
				spawner.m_VehicleRplId = vehicleRpl.Id();

			// Same pattern as vanilla's SCR_AmbientVehicleSpawnPointComponent: get told synchronously
			// the instant the engine destroys this vehicle, rather than relying solely on ~Vehicle()'s
			// GC-timed cleanup. See COA_VehicleSpawner.OnVehicleDestroyed().
			EventHandlerManagerComponent handler = EventHandlerManagerComponent.Cast(vehicleEntity.FindComponent(EventHandlerManagerComponent));
			if (handler)
				handler.RegisterScriptHandler("OnDestroyed", spawner, spawner.OnVehicleDestroyed);
		}

		Vehicle vehicle = Vehicle.Cast(spawner.m_eVehicle);
		if (vehicle)
		{
			vehicle.m_iVehicleSpawnerIndex = spawner.m_iVehicleSpawnerIndex;
			vehicle.m_sFactionKey = spawner.m_sFactionKey;
			if (spawner.m_OverridedVehicleLoadout)
				vehicle.m_OverridedVehicleLoadout = spawner.m_OverridedVehicleLoadout;
			// These are [Attribute] arrays with no default initialiser, so they are null on any
			// spawner prefab authored before the attribute existed. Count() was called on them
			// unguarded, on every single vehicle spawn.
			if (spawner.m_aVehicleGearscriptOverrides && !spawner.m_aVehicleGearscriptOverrides.IsEmpty())
				vehicle.m_aVehicleGearscriptOverrides = spawner.m_aVehicleGearscriptOverrides;
			if (spawner.m_aAdditionalVehicleItems && !spawner.m_aAdditionalVehicleItems.IsEmpty())
				vehicle.m_aAdditionalVehicleItems = spawner.m_aAdditionalVehicleItems;
			if (!spawner.m_bShouldAddAmmo)
				vehicle.m_bShouldAddAmmo = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_VehicleGearscriptManager m_sInstance;
	void CRF_VehicleGearscriptManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_VehicleGearscriptManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_VehicleGearscriptManager GetInstance()
	{
		return m_sInstance;
	}
}
