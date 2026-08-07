modded class COA_VehicleSpawner
{
	[Attribute("1", desc: "Should we add ammo to this vehicle", category: "Vehicle Spawning")] 
	bool m_bShouldAddAmmo;

	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout", category: "Vehicle Spawning")] 
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute(category: "Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute(category: "Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;

	//! RplId of the vehicle this spawner most recently spawned. Kept alongside the inherited
	//! m_eVehicle because that is a raw IEntity pointer the engine does not null on deletion, so it
	//! cannot be used to tell whether the previous vehicle is still around.
	RplId m_VehicleRplId;

	//------------------------------------------------------------------------------------------------
	//! Register with the gearscript manager and return. This deliberately does NOT spawn.
	//!
	//! Spawning inline here meant every spawner in the mission created its vehicle in the same frame
	//! during world streaming (42 of them on Eden Prejoin), each then queueing a gearscript pass that
	//! landed 2.5s later - again all in the same frame, each spawning a full vehicle inventory.
	//! Vanilla never spawns a gameplay entity from EOnInit: SCR_AmbientVehicleSpawnPointComponent
	//! registers in OnPostInit and SCR_AmbientVehicleSystem spawns one spawnpoint per update tick.
	//! CRF_VehicleGearscriptManager now does the same, so registration is all that belongs here.
	//!
	//! This intentionally replaces rather than extends the base COA_VehicleSpawner.EOnInit - calling
	//! super would spawn the vehicle immediately, which is exactly the behaviour being removed.
	override void EOnInit(IEntity owner)
	{
		if (m_rVehicle.IsEmpty())
		{
			Print(string.Format("No Vehicle set on %1", this), LogLevel.ERROR);
			return;
		}

		if (m_sFactionKey.IsEmpty())
		{
			Print(string.Format("No Faction Key set on %1", this), LogLevel.ERROR);
			return;
		}

		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RespawnManager = COA_RespawnManager.GetInstance();
		if (m_RespawnManager)
			m_iVehicleSpawnerIndex = m_RespawnManager.InsertVehicle(this);

		// Guarded, unlike before. The manager is a component on the game mode and this is a world
		// entity, so their init ordering is an assumption rather than a guarantee - and the line
		// above already treats the sibling manager as optional.
		CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!gearscriptManager)
		{
			Print(string.Format("[CRF_COA_VehicleSpawner] CRF_VehicleGearscriptManager unavailable at init for %1 - vehicle will not spawn.", this), LogLevel.ERROR);
			return;
		}

		gearscriptManager.RegisterSpawner(this);

		// Deliberately no SetEventMask(EntityEvent.FRAME). The respawn timer is ticked centrally by
		// the gearscript manager, so the mission pays one frame callback for every spawner instead
		// of one per spawner. EOnFrame is not overridden here for the same reason.
	}

	//------------------------------------------------------------------------------------------------
	void ~COA_VehicleSpawner()
	{
		CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (gearscriptManager)
			gearscriptManager.UnregisterSpawner(this);
	}
}