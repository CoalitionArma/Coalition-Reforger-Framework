	
modded class COA_RespawnManager 
{
	//------------------------------------------------------------------------------------------------
	override void WaveRespawnTimer()
	{
		// Client-side: Just update local timer display
		if (!Replication.IsServer())
		{
			// Timer value already updated via replication
			// Update local display time if needed
			if (m_iRespawnWaveCurrentTime == 0)
			{
				m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
			}
			return;
		}
		
		// Server-side: Update timer and trigger replication
		if (m_Gamemode.m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		m_iRespawnWaveCurrentTime--;
		
		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
			m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;

			RespawnAllVehicles();
		}

		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	
	//------------------------------------------------------------------------------------------------
	override void RespawnSide(FactionKey faction)
	{
		super.RespawnSide(faction);

		RespawnSideVehicles(faction);
	}
    
    //------------------------------------------------------------------------------------------------
	void RespawnAllVehicles()
	{
		//Makes my life 20x easier
		array<string> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string faction: factionKeys)
		{
			//Vehicle respawn logic (without additional ticket operations)
			foreach (COA_VehicleSpawner vehicle: m_aVehicleSpawners)
			{
				if (vehicle.m_sFactionKey != faction)
					continue;
				
				//Do we have enough tickets and are they not at 0.
				if (GetFactionTickets(faction) != 0 && GetFactionTickets(faction) < vehicle.m_iTicketsPerRespawn)
					continue;
				
				//Is the vehicle non existant anymore
				if (!vehicle.m_eVehicle && vehicle.m_bShouldRespawnOnSideRespawn)
				{
					CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
					continue;
				}
				
				//Vehicle is not vehicling wth
				if (!vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
					continue;
				
				SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
				if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
					continue;
				
				//Vehicle is destroyed respawn it.
				CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
				continue;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnSideVehicles(FactionKey faction)
	{
		//Vehicle respawn logic (without additional ticket operations)
		foreach (COA_VehicleSpawner vehicle: m_aVehicleSpawners)
		{
			if (vehicle.m_sFactionKey != faction)
				continue;
			
			//Do we have enough tickets and are they not at 0.
			if (GetFactionTickets(faction) != 0 && GetFactionTickets(faction) < vehicle.m_iTicketsPerRespawn)
				continue;
			
			//Is the vehicle non existant anymore
			if (!vehicle.m_eVehicle && vehicle.m_bShouldRespawnOnSideRespawn)
			{
				CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
				continue;
			}
			
			//Vehicle is not vehicling wth
			if (!vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
				continue;
			
			SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
			if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
				continue;
			
			//Vehicle is destroyed respawn it.
			CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
			continue;
		}
	}
}