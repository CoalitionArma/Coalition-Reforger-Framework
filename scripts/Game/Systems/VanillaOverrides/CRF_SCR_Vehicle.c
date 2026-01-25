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
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if(gamemode)
		{
			GetGame().GetCallqueue().CallLater(SetVehicleGear, 500, false);
			GetGame().GetCallqueue().CallLater(CheckIfSpawnPassenger, 500, false);
	
			gamemode.AddVehicleToArray(this);
		};
	}
	
	void CheckIfSpawnPassenger()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
			SpawnVehiclePassengers();
	}
	
	
	void SpawnVehiclePassengers()
	{
		SCR_BaseCompartmentManagerComponent.Cast(this.FindComponent(SCR_BaseCompartmentManagerComponent)).AddAIToVehicle();
	}
	
	void SetVehicleGear()
	{
		GetGame().GetCallqueue().CallLater(
					CRF_GearscriptManager.GetInstance().SetVehicleGear, 2000, false,
					this, m_sFactionKey
				);
	}
	
	void ~Vehicle()
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (!GetGame().GetWorld())
			return;
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode)
			gamemode.RemoveVehicleFromArray(this);
		
		if (m_iVehicleSpawnerIndex == -1)
			return;
		
		//Just in case the damage manager doesn't actually get it when it blows up.
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (!respawnManager)
			return;
			
		foreach (CRF_VehicleSpawner vehicle: respawnManager.GetVehicleSpawners())
		{
			// Check if vehicle spawner is valid and has a vehicle entity
			if (!vehicle || !vehicle.m_eVehicle)
				continue;
				
			if (vehicle.m_eVehicle != this)
				continue;
			
			if (vehicle.m_bShouldRespawnOnSideRespawn)
				return;
			
			if (vehicle.m_fTimer > 0)
				continue;
			
			vehicle.StartRespawnTimer();
		}
	}
}