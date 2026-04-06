class CRF_AAComponentClass: ScriptComponentClass
{

}

class CRF_AAComponent: ScriptComponent
{
	[Attribute()]
	float distance;
	
	[Attribute()]
	float mps;
	CRF_AirdropManager m_AirdropManger;
	ref RandomGenerator m_Random;
	SCR_AIGroup m_OccupiedGroup;
	AIWaypoint m_CurrentOrder;
	bool spawnedGroup = false;
	float m_fDistance = 1;
	IEntity m_SelectedPlane;
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer())
			return;
		
		m_AirdropManger = CRF_AirdropManager.GetInstance();
		m_Random = new RandomGenerator();
		m_Random.SetSeed(System.GetUnixTime());
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!spawnedGroup)
		{
			SCR_BaseCompartmentManagerComponent compManager = SCR_BaseCompartmentManagerComponent.Cast(owner.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compManager)
			{
				spawnedGroup = true;
				return;
			}
			
			vector location[4];
			owner.GetTransform(location);
			EntitySpawnParams params = CRF_EntityHelper.CreateSpawnParams(location);
			IEntity group =  GetGame().SpawnEntityPrefab(Resource.Load("{8EC6B2FB5C407949}Prefabs/Groups/OPFOR/Vehicle/CRF_OPFOR_VehicleCrew_p.et"), null, params);
			
			m_OccupiedGroup = SCR_AIGroup.Cast(group);
			
			GetGame().GetCallqueue().CallLater(compManager.AddToCompartment, 2000, false, m_OccupiedGroup);
			spawnedGroup = true;
			return;
		}
		if (m_fBuffer <= 5)
		{
			m_fBuffer += timeSlice;
			return;
		}
		else
			m_fBuffer = 0;
		
//		if (!m_SelectedPlane)
//		{
//			CRF_AAGamemodeComponent aaGamemode = CRF_AAGamemodeComponent.GetInstance();
//			if (!aaGamemode)
//				return;
//			
//			int index = aaGamemode.AssignAATarget();
//			ref CRF_AirdropFlight flight = m_AirdropManger.m_aFlightObjects.Get(index);
//			if (!flight)
//				return;
//				
//			IEntity m_SelectedPlane = flight.m_Plane;
//			if (!m_SelectedPlane)
//				return;
//		}

		
		IEntity m_SelectedPlane = GetGame().GetPlayerManager().GetPlayerControlledEntity(1);
		
		float randDist = m_Random.RandFloatXY(-5.0, 5.0);
		float planeDistance = vector.Distance(m_SelectedPlane.GetOrigin(), GetOwner().GetOrigin());
		m_fDistance =  planeDistance;
		m_OccupiedGroup.RemoveWaypoint(m_CurrentOrder);
		SCR_EntityHelper.DeleteEntityAndChildren(m_CurrentOrder);
		vector planeLocation[4];
		m_SelectedPlane.GetTransform(planeLocation);
		
		// Move 5 meters to the RIGHT
		planeLocation[3][0] = planeLocation[3][0] + planeLocation[2][0] * 5.0;
		planeLocation[3][1] = planeLocation[3][1] + planeLocation[2][1] * 5.0;
		planeLocation[3][2] = planeLocation[3][2] + planeLocation[2][2] * 5.0;
		EntitySpawnParams params = CRF_EntityHelper.CreateSpawnParams(planeLocation);
		IEntity order = GetGame().SpawnEntityPrefab(Resource.Load("{B2A2A69B5F42EC49}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Suppress_Editor.et"), null, params);
		
		m_CurrentOrder = AIWaypoint.Cast(order);
		
		m_OccupiedGroup.AddWaypoint(m_CurrentOrder);
	}
}

class CRF_AATimerTriggerComponentClass: TimerTriggerComponentClass
{
	
}

class CRF_AATimerTriggerComponent: TimerTriggerComponent
{
}

class CRF_AAMuzzleEffectComponentClass: SCR_MuzzleEffectComponentClass
{
}

class CRF_AAMuzzleEffectComponent: SCR_MuzzleEffectComponent
{
	override void OnFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnFired(effectEntity, muzzle, projectileEntity);
		if (!muzzle.GetOwner())
			return;
		
		if (!projectileEntity)
			return;
		
		ShellMoveComponent shellMove = ShellMoveComponent.Cast(projectileEntity.FindComponent(ShellMoveComponent));
		if (!shellMove)
			return;
		

		
		IEntity vehicle = muzzle.GetOwner().GetRootParent();
		if (!vehicle)
			return;
		
		CRF_AAComponent aaComp = CRF_AAComponent.Cast(vehicle.FindComponent(CRF_AAComponent));
		if (!aaComp)
			return;
		
		float distance = aaComp.m_fDistance;
		
		CRF_AATimerTriggerComponent aaTimer = CRF_AATimerTriggerComponent.Cast(projectileEntity.FindComponent(CRF_AATimerTriggerComponent));
		if (!aaTimer)
			return;
		
		float timer = (distance - 5) / (shellMove.GetBulletSpeedCoef() * muzzle.GetBulletInitSpeedCoef());
		aaTimer.SetTimer(timer);
	}
}

class CRF_AAGamemodeComponentClass: SCR_BaseGameModeComponentClass
{

}

class CRF_AAGamemodeComponent: SCR_BaseGameModeComponent
{
	CRF_AirdropManager m_AirdropManager;
	int m_iPlanesAssigned = 0;
	static CRF_AAGamemodeComponent m_sInstance;
	
	void CRF_AAGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_AAGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}
	
	override void OnPostInit(IEntity owner)
	{
		m_AirdropManager = CRF_AirdropManager.GetInstance();
	}
	
	int AssignAATarget()
	{
		if (m_iPlanesAssigned >= m_AirdropManager.m_aFlightObjects.Count())
		{
			m_iPlanesAssigned = 0;
			return m_iPlanesAssigned;
		}
		
		m_iPlanesAssigned++;
		return m_iPlanesAssigned - 1;
	}
}