modded class Vehicle
{
	[RplProp()] int m_iCurrentSupplies;
	
	[Attribute("1", desc: "Should we add ammo to this vehicle", category: "CRF Vehicle Spawning")] 
	bool m_bShouldAddAmmo;
	
	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout", category: "CRF Vehicle Spawning")]
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute(category: "CRF Vehicle Spawning")]
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
	
	string m_sFactionKey = "";
	int m_iVehicleSpawnerIndex = -1;
	
	void UpdateVehicleSupplies(int supply)
	{
		m_iCurrentSupplies = supply;
		Replication.BumpMe();
	}
	
	void Vehicle(IEntitySource src, IEntity parent)
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (!GetGame().GetWorld())
			return;
		
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if(vehicleGearscriptManager)
		{
			GetGame().GetCallqueue().CallLater(SetVehicleGear, 500, false);
			GetGame().GetCallqueue().CallLater(CheckIfSpawnPassenger, 500, false);
	
			vehicleGearscriptManager.AddVehicleToSpawnedArray(this);
		};
	}
	
	void CheckIfSpawnPassenger()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			SpawnVehiclePassengers();
	}
	
	
	void SpawnVehiclePassengers()
	{
		SCR_BaseCompartmentManagerComponent compartmentManager = SCR_BaseCompartmentManagerComponent.Cast(this.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compartmentManager)
			compartmentManager.AddAIToVehicle();
	}
	
	void SetVehicleGear()
	{
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!vehicleGearscriptManager)
			return;

		// Hand the manager an RplId rather than `this`. The queue holds its arguments for the full
		// 2000ms, and a vehicle can easily be deleted inside that window (spawner reshuffle, admin
		// cleanup, destruction) - after which the manager would be handed a freed Vehicle.
		RplComponent rplComponent = RplComponent.Cast(FindComponent(RplComponent));
		if (!rplComponent)
			return;

		GetGame().GetCallqueue().CallLater(
					vehicleGearscriptManager.SetVehicleGearById, 2000, false,
					rplComponent.Id(), m_sFactionKey
				);
	}
	
	//! Teardown for a Vehicle. Runs at an arbitrary GC point, so it must do the absolute minimum:
	//! drop references others hold to this object, and nothing else.
	//!
	//! This deliberately no longer starts respawn timers. Two paths already cover that, both at
	//! well-defined moments rather than during teardown:
	//!   - destroyed in play  -> COA_SCR_VehicleDamageManagerComponent.OnDamageStateChanged()
	//!                           fires on EDamageState.DESTROYED and calls StartRespawnTimer().
	//!   - deleted some other way -> COA_RespawnManager already tests `!vehicle.m_eVehicle` and
	//!                           respawns from there, which the back-pointer clear below enables.
	//! Calling gameplay code (entity deletion, manager traversal, timer starts) from a destructor
	//! is what made this dangerous: nothing guarantees those managers still exist at this point.
	void ~Vehicle()
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif

		// GetGame() itself can be gone during shutdown - the previous `!GetGame().GetWorld()` guard
		// would have faulted on exactly the teardown it was meant to protect against.
		if (!GetGame() || !GetGame().GetWorld())
			return;

		// Cancel anything still queued against this object. Both of these were scheduled in the
		// constructor and target methods on `this`; if the vehicle is deleted inside the 500ms
		// window the queue would invoke them on freed memory.
		GetGame().GetCallqueue().Remove(SetVehicleGear);
		GetGame().GetCallqueue().Remove(CheckIfSpawnPassenger);

		// Pure container removal - pointer comparison only, no dereference of `this`.
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (vehicleGearscriptManager)
			vehicleGearscriptManager.RemoveVehicleFromSpawnedArray(this);

		if (m_iVehicleSpawnerIndex == -1)
			return;

		// Clear any spawner back-pointer aimed at us so it does not keep a dangling handle, and so
		// COA_RespawnManager's existing `!m_eVehicle` branch can pick the respawn up. Assignment and
		// pointer comparison only.
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return;

		array<COA_VehicleSpawner> spawners = respawnManager.GetVehicleSpawners();
		if (!spawners)
			return;

		foreach (COA_VehicleSpawner spawner: spawners)
		{
			if (!spawner || spawner.m_eVehicle != this)
				continue;

			spawner.m_eVehicle = null;
			break;
		}
	}
}
