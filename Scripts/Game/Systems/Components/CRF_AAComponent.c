//All in one file cause fuck it, I one day may optimize this
//Real reason is linux doesn't update file structure in workbench until I restart it so I couldn't be damned to do that.
//If it works well Ill update this to be super modular and fungular tastic, otherwise for now this works for this specific use case.

class CRF_AAComponentClass: ScriptComponentClass
{

}

class CRF_AAComponent: ScriptComponent
{
	//Meters per second the bullet is traveling
	//This rarely lines up so just fuck with it till your explosions look right
	[Attribute("800")]
	float mps;
	
	CRF_AirdropManager m_AirdropManger;
	ref RandomGenerator m_Random;
	SCR_AIGroup m_OccupiedGroup;
	AIWaypoint m_CurrentOrder;
	bool spawnedGroup = false;
	float m_fDistance = 1;
	float m_fTimer;
	IEntity m_SelectedPlane;
	SCR_TurretControllerComponent m_TurretComp;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer())
			return;
		
		m_Random = new RandomGenerator();
		m_Random.SetSeed(System.GetUnixTime());
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_AirdropManger)
			m_AirdropManger = CRF_AirdropManager.GetInstance();
		if (!m_TurretComp)
		{
			SlotManagerComponent slotMan = SlotManagerComponent.Cast(GetOwner().FindComponent(SlotManagerComponent));
			if (!slotMan)
				return;

			EntitySlotInfo turretSlot = slotMan.GetSlotByName("Turret");
			if (!turretSlot)
				return;
			
			IEntity turret = turretSlot.GetAttachedEntity();
			if (!turret)
				return;
			
			m_TurretComp = SCR_TurretControllerComponent.Cast(turret.FindComponent(SCR_TurretControllerComponent));
		}
		
		//Automatically spawns the group to occupy the AA gun (LAV)
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
		
		//Always have the gun fire, otherwise the AI only shoots every once in a fucking while
		if (m_SelectedPlane)
			m_TurretComp.SetFireWeaponWanted(true);
		
		if (m_CurrentOrder)
			AlignVehicleCompletelyToPlane();
		
		//Could maybe be higher, 5 seconds was entirely too long and the aiming updating would be noticable
		if (m_fBuffer <= 1)
		{
			m_fBuffer += timeSlice;
			return;
		}
		else
			m_fBuffer = 0;
		
		//Calls to the gamemode component to assign a plane target, this ensures we have an equal amount of AA guns per plane.
		if (!m_SelectedPlane)
		{
			CRF_AAGamemodeComponent aaGamemode = CRF_AAGamemodeComponent.GetInstance();
			if (!aaGamemode)
				return;
			
			int index = aaGamemode.AssignAATarget();
			if (index == -1)
				return;

			ref CRF_AirdropFlight flight = m_AirdropManger.m_aFlightObjects.Get(index);
			if (!flight)
				return;
				
			m_SelectedPlane = flight.m_Plane;
			if (!m_SelectedPlane)
				return;
		}
		
		//Places a suppression waypoint 100m ahead of the plane, this leads the gun perfectly on target
		float planeDistance = vector.Distance(m_SelectedPlane.GetOrigin(), GetOwner().GetOrigin());
		m_fDistance =  planeDistance;
		float randDist = m_Random.RandFloatXY(-10.0, 10.0);
		m_fTimer = (m_fDistance + randDist) / mps;
		m_OccupiedGroup.RemoveWaypoint(m_CurrentOrder);
		SCR_EntityHelper.DeleteEntityAndChildren(m_CurrentOrder);
		vector forward = m_SelectedPlane.GetTransformAxis(0);
		vector spawnPos = m_SelectedPlane.GetOrigin() + forward * -100.0;
		
		vector waypointMat[4];
		m_SelectedPlane.GetTransform(waypointMat);
		waypointMat[3] = spawnPos;
		
		EntitySpawnParams params = CRF_EntityHelper.CreateSpawnParams(waypointMat);
		IEntity order = GetGame().SpawnEntityPrefab(
			Resource.Load("{B2A2A69B5F42EC49}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Suppress_Editor.et"),
			null,
			params
		);
		
		m_CurrentOrder = AIWaypoint.Cast(order);
		
		m_OccupiedGroup.AddWaypoint(m_CurrentOrder);
	}
	
	//This is what rotates the LAV to point directly at the plane, otherwise the AI cant aim and shoot at the same time and they end up not shooting anywhere near the plane.
	void AlignVehicleCompletelyToPlane()
	{
		if (!m_CurrentOrder)
			return;
	
		IEntity vehicle = GetOwner();
		if (!vehicle)
			return;
	
		vector vehiclePos = vehicle.GetOrigin();
		vector targetPos = m_CurrentOrder.GetOrigin();
	
		vector dir = vector.Direction(vehiclePos, targetPos);
		if (dir.LengthSq() < 0.0001)
			return;
	
		dir.Normalize();
	
		// Converts direction vector to <yaw, pitch, roll> in degrees
		vector targetYPR = dir.VectorToAngles();
	
		// Keep roll from current vehicle so it does not do weird banking
		vector currentYPR = vehicle.GetYawPitchRoll();
		targetYPR[2] = currentYPR[2];
	
		vehicle.SetYawPitchRoll(targetYPR);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------
//Seperates it so only AA bullets have timer logic on them, optimization or something
class CRF_AATimerTriggerComponentClass: TimerTriggerComponentClass
{
	
}

class CRF_AATimerTriggerComponent: TimerTriggerComponent
{
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------
//Seperates it so only AA bullets have timer logic on them, optimization or something
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
		
		IEntity vehicle = muzzle.GetOwner().GetRootParent();
		if (!vehicle)
			return;
		
		CRF_AAComponent aaComp = CRF_AAComponent.Cast(vehicle.FindComponent(CRF_AAComponent));
		if (!aaComp)
			return;
		
		CRF_AATimerTriggerComponent aaTimer = CRF_AATimerTriggerComponent.Cast(projectileEntity.FindComponent(CRF_AATimerTriggerComponent));
		if (!aaTimer)
			return;
		aaTimer.SetTimer(aaComp.m_fTimer);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------
//A component attached to the gamemode responsible for equally distrubting AA targets so all planes have an equal amount shooting at them
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
		if (m_AirdropManager.m_aFlightObjects.Count() == 0)
			return -1;
		if (m_iPlanesAssigned >= m_AirdropManager.m_aFlightObjects.Count())
		{
			m_iPlanesAssigned = 0;
			return m_iPlanesAssigned;
		}
		
		m_iPlanesAssigned++;
		return m_iPlanesAssigned - 1;
	}
}