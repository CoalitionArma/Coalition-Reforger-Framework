modded class SCR_VehicleDamageManagerComponent
{
	override void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (newState == EDamageState.DESTROYED)
		{
			CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
			foreach (CRF_VehicleSpawner vehicle: respawnManager.GetVehicleSpawners())
			{
				if (vehicle.m_eVehicle != GetOwner())
					continue;
				
				if (vehicle.m_bShouldRespawnOnSideRespawn)
					return;
				
				vehicle.StartRespawnTimer();
			}
		}
	}
}