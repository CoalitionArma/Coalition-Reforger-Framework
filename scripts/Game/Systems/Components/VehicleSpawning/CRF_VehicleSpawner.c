class CRF_VehicleSpawnerClass: BaseGameTriggerEntityClass
{

}

class CRF_VehicleSpawner: BaseGameTriggerEntity
{
	[Attribute("", desc: "Vehicle that will spawn in", category: "CRF Vehicle Spawning")] 
	ResourceName m_rVehicle;
	
	[Attribute("", desc: "VFaction this spawner belongs to", category: "CRF Vehicle Spawning")] 
	string m_sFactionKey;
	
	[Attribute("1", desc: "Whenever a side respawns should this vehicle respawn with it. (Ignores the timer)", category: "CRF Vehicle Spawning")] 
	bool m_bShouldRespawnOnSideRespawn;
	
	[Attribute("300", desc: "How long until the vehicle respawns after its death in seconds", category: "CRF Vehicle Spawning")] 
	int m_iRespawnTimer;
	
	[Attribute("10", desc: "How many tickets is drained every time this spawns", category: "CRF Vehicle Spawning")] 
	int m_iTicketsPerRespawn;
	
	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout", category: "CRF Vehicle Spawning")] 
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
	
	CRF_RespawnManager m_RespawnManager;
	Faction m_Faction;
	IEntity m_eVehicle;
	float m_fTimer = 0;
	int m_iVehicleSpawnerIndex = -1;
	protected bool m_bWaitingToRespawn = false;
	override void EOnInit(IEntity owner)
	{
		if (!m_sFactionKey && CRF_Gamemode.GetInstance())
		{
			Debug.Error("No Faction Key set on " + m_rVehicle + " spawner");
			return;
		}
		Print("Init");
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		if (m_RespawnManager)
			m_iVehicleSpawnerIndex = m_RespawnManager.InsertVehicle(this);
		SpawnVehicle();
		SetEventMask(EntityEvent.FIXEDFRAME);
	}
	
	override void EOnFixedFrame(IEntity owner, float timeSlice)
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
			SpawnVehicle();
			m_bWaitingToRespawn = false;
		}
	}
	
	void StartRespawnTimer()
	{
		m_fTimer = m_iRespawnTimer;
		m_bWaitingToRespawn = true;
	}
	
	void SpawnVehicle()
	{
		if (!m_sFactionKey)
		{
			Debug.Error("No Faction Key set on " + m_rVehicle + " spawner");
			return;
		}
		//Do not spawn the vehicle if the faction doesn't have the tickets
		//Handles subtracting tickets from kills that are on a timer. This means tickets are subtracted WHEN the vehicle is spawned
		if (m_bWaitingToRespawn && !m_bShouldRespawnOnSideRespawn)
		{
			if (m_RespawnManager.GetFactionTickets(m_sFactionKey) != 0 && m_RespawnManager.GetFactionTickets(m_sFactionKey) < m_iTicketsPerRespawn)
				return;
		
			if (m_RespawnManager.TicketsRemaining(m_sFactionKey))
				m_RespawnManager.SubtractTicket(m_sFactionKey, m_iTicketsPerRespawn);
		}
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		this.GetTransform(params.Transform);
		m_eVehicle = GetGame().SpawnEntityPrefab(Resource.Load(m_rVehicle), GetGame().GetWorld(), params);
		Vehicle vehicle = Vehicle.Cast(m_eVehicle);
		if (vehicle)
		{
			vehicle.m_iVehicleSpawnerIndex = m_iVehicleSpawnerIndex;
			vehicle.m_sFactionKey = m_sFactionKey;
			if (m_OverridedVehicleLoadout)
				vehicle.m_OverridedVehicleLoadout = m_OverridedVehicleLoadout;
			if (m_aVehicleGearscriptOverrides.Count() > 0)
				vehicle.m_aVehicleGearscriptOverrides = m_aVehicleGearscriptOverrides;
			if (m_aAdditionalVehicleItems.Count() > 0)
				vehicle.m_aAdditionalVehicleItems = m_aAdditionalVehicleItems;
		}
	}
	
	#ifdef WORKBENCH
	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (key == "m_rVehicle")
			SCR_EntityHelper.DeleteEntityAndChildren(m_eVehicle);
		return false;
	}
	
	override event void _WB_SetTransform(inout vector mat[4], IEntitySource src)
	{
		if(m_eVehicle)
		{
			vector pos[4];
			this.GetTransform(pos);
			UpdateVehiclePos(pos);
		}	
	}
	
	void UpdateVehiclePos(vector pos[4])
	{
		if(!m_eVehicle)
			return;
		m_eVehicle.SetTransform(pos);
		m_eVehicle.Update();
	}
	#endif
	
	void ~CRF_VehicleSpawner()
	{
		#ifdef WORKBENCH
				SCR_EntityHelper.DeleteEntityAndChildren(m_eVehicle);
		#endif
	}
}