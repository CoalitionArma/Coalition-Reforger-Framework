modded class SCR_VehicleDamageManagerComponent
{
	override void OnDamageStateChanged(EDamageState state)
	{
		super.OnDamageStateChanged(state);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (state == EDamageState.DESTROYED)
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