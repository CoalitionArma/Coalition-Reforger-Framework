	
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
			RespawnSideVehicles(faction);
	}

	//------------------------------------------------------------------------------------------------
	//! Queue every eligible vehicle spawner for this faction.
	//!
	//! This used to call CRF_VehicleGearscriptManager.SpawnVehicle() directly in the loop, so a side
	//! respawn spawned every matching vehicle in a single frame - and RespawnAllVehicles() runs on
	//! every respawn wave, so that burst repeated all mission rather than only at start.
	//! It now enqueues, and the manager drains a couple per tick.
	//!
	//! RespawnAllVehicles() previously duplicated this body verbatim; it now just calls this per
	//! faction, so the two paths cannot drift apart.
	void RespawnSideVehicles(FactionKey faction)
	{
		CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!gearscriptManager)
			return;

		if (!m_aVehicleSpawners)
			return;

		//Vehicle respawn logic (without additional ticket operations)
		foreach (COA_VehicleSpawner vehicle: m_aVehicleSpawners)
		{
			if (!vehicle || vehicle.m_sFactionKey != faction)
				continue;

			//Do we have enough tickets and are they not at 0.
			int factionTickets = GetFactionTickets(faction);
			if (factionTickets != 0 && factionTickets < vehicle.m_iTicketsPerRespawn)
				continue;

			//Is the vehicle non existant anymore
			if (!vehicle.m_eVehicle)
			{
				// Only the side-respawn flavour of spawner respawns from here. Note the original
				// fell through to the FindComponent() call below when m_eVehicle was null and
				// m_bShouldRespawnOnSideRespawn was false - a straight null dereference.
				if (vehicle.m_bShouldRespawnOnSideRespawn)
					gearscriptManager.QueueSpawn(vehicle);

				continue;
			}

			//Vehicle is not vehicling wth
			SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
			if (!vehicleDamageManager)
				continue;

			if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
				continue;

			//Vehicle is destroyed respawn it.
			gearscriptManager.QueueSpawn(vehicle);
		}
	}
}