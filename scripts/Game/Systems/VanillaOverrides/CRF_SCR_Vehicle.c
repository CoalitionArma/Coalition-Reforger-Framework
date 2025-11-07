modded class Vehicle
{
	[RplProp()] int m_iCurrentSupplies;
	
	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout")]
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute()] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute()]
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
		
		GetGame().GetCallqueue().CallLater(CheckIfSpawnPassenger, 500, false);
		CRF_Gamemode.GetInstance().AddVehicleToArray(this);
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
		
		CRF_Gamemode.GetInstance().RemoveVehicleFromArray(this);
		
		if (m_iVehicleSpawnerIndex == -1)
			return;
		//Just in case the damage manager doesn't actually get it when it blows up.
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		foreach (CRF_VehicleSpawner vehicle: respawnManager.GetVehicleSpawners())
		{
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