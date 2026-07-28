modded class COA_VehicleSpawner
{
	[Attribute("1", desc: "Should we add ammo to this vehicle", category: "CRF Vehicle Spawning")] 
	bool m_bShouldAddAmmo;

	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout", category: "CRF Vehicle Spawning")] 
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;

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

		CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(this);
		SetEventMask(EntityEvent.FRAME);
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (m_fTimer > 0)
			m_fTimer -= timeSlice;
		
		if (m_bWaitingToRespawn && m_fTimer <= 0)
		{
			CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(this);
			m_bWaitingToRespawn = false;
		}
	}
}