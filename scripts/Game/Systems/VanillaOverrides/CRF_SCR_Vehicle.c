modded class Vehicle
{
	int m_iVehicleSpawnerIndex = -1;
	void ~Vehicle()
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (!GetGame().GetWorld())
			return;
		
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