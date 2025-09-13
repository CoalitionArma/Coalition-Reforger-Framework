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
	
	CRF_RespawnManager m_RespawnManager;
	Faction m_Faction;
	IEntity m_eVehicle;
	float m_fTimer = 0;
	int m_iVehicleSpawnerIndex = -1;
	protected bool m_bWaitingToRespawn = false;
	override void EOnInit(IEntity owner)
	{
		Print("Init");
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RespawnManager = CRF_RespawnManager.GetInstance();
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
		Vehicle.Cast(m_eVehicle).m_iVehicleSpawnerIndex = m_iVehicleSpawnerIndex;
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