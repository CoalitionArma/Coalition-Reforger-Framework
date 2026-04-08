class CRF_AirdropManagerClass: SCR_BaseGameModeComponentClass
{
}

class CRF_AirdropManager: SCR_BaseGameModeComponent
{
	static CRF_AirdropManager m_sInstance;
	ref array<ref CRF_AirdropFlight> m_aFlightObjects = {};
	int m_iPlanesAssigned = 0;
	int m_aAAGunsRegistered = 0;
	
	void CRF_AirdropManager (IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_AirdropManager GetInstance()
	{
		return m_sInstance;
	}
	
	void InitFlight(CRF_AirdropObject planeObject)
	{
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		array<SCR_AIGroup> groups = {};
		groupsMan.GetAllPlayableGroups(groups);
		int planeIndex = 0;
		array<string> playersInPlane = {""};
		int playersAdded = 0;
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach (int playerId: planeObject.m_aPlayerIds)
		{			
			//This group will put us past the 22 slots available in the plane, we gotta spawn another one
			if (playersAdded + 1 > 22)
			{
				planeIndex++;
				playersAdded = 0;
				playersInPlane.Insert("");
			}
				
			string currentPlayers = playersInPlane.Get(planeIndex);
			currentPlayers += playerId.ToString() + "|";
			playersInPlane.Set(planeIndex, currentPlayers);
			playersAdded++;
		}
		
		for (int i = 0; i <= planeIndex; i++)
		{
			GetGame().GetCallqueue().CallLater(SpawnFlight, 5000 * i, false, planeObject, playersInPlane.Get(i));
		}
	}
	
	void SpawnFlight(CRF_AirdropObject planeObject, string players)
	{
		vector angles[3];
		vector rawAngles = Vector(planeObject.m_fAngle, 0, 0);
		Math3D.AnglesToMatrix(rawAngles, angles);
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[0] = angles[0];
		params.Transform[1] = angles[1];
		params.Transform[2] = angles[2];
		params.Transform[3] = planeObject.m_vFlightCoordinates[0];
		IEntity plane = GetGame().SpawnEntityPrefab(Resource.Load(planeObject.m_sPlane), null, params);
//		GetGame().GetCallqueue().CallLater(SetTransformPostSpawn, 500, false, plane, planeObject.m_fAngle, planeObject.m_vFlightCoordinates[0]);
		//Redundant but just in case
		StreamPlaneIntoReplication(plane);
		ref CRF_AirdropFlight flight = new CRF_AirdropFlight(plane, planeObject.m_vFlightCoordinates, 50, planeObject.m_bAutoDeployParachute);
		//Delay so the flight has a chance to actual load the entity
		GetGame().GetCallqueue().CallLater(TeleportPlayers, 2000, false, players, SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent)), plane, flight);
	}
	
//	void SetTransformPostSpawn(IEntity plane, float angle, vector flightCoordinate)
//	{
//		vector angles[3];
//		vector rawAngles = Vector(angle, 0, 0);
//		Math3D.AnglesToMatrix(rawAngles, angles);
//		EntitySpawnParams params = new EntitySpawnParams();
//		params.Transform[0] = angles[0];
//		params.Transform[1] = angles[1];
//		params.Transform[2] = angles[2];
//		params.Transform[3] = flightCoordinate;
//		
//		int catch = 0;
//		IEntity child = plane.GetChildren();
//		while (true && catch < 10)
//		{
//			catch++;
//			if (!child)
//				return;
//			else
//			{
//				SCR_EditableEntityComponent.Cast(child.FindComponent(SCR_EditableEntityComponent)).SetTransform(params.Transform);
//				child = child.GetChildren();
//			}
//		}
//	}
	
	void StreamPlaneIntoReplication(IEntity plane)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		RplComponent rplComp = RplComponent.Cast(plane.FindComponent(RplComponent));
		if (!rplComp)
			return;
		
		foreach (int playerId: playerIds)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (!pc)
				continue;

			rplComp.EnableStreamingConNode(pc.GetRplIdentity(), false);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PlaySound(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		if (plane.FindComponent(SCR_BaseInteractiveLightComponent))
			SCR_BaseInteractiveLightComponent.Cast(plane.FindComponent(SCR_BaseInteractiveLightComponent)).ToggleLight(true);
	}
	
		
	void TeleportPlayers(string players, SlotManagerComponent slotMan, IEntity plane, CRF_AirdropFlight flight)
	{
		array<string> playerIds = {};
		players.Split("|", playerIds, true);
		int slotId = 0;
		RplId planeRplId = RplComponent.Cast(plane.FindComponent(RplComponent)).Id();
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach (int i, string playerId: playerIds)
		{
			IEntity player = pm.GetPlayerControlledEntity(playerId.ToInt());
			if (!player)
				continue;
			//Wait till players been teleported, for auto deploy logic, if players is not teleported yet it will auto deploy parachute.
			GetGame().GetCallqueue().CallLater(flight.m_PlayersInPlane.Insert, 2500, false, pm.GetPlayerControlledEntity(playerId.ToInt()));
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(player);
			if (!character)
				continue;

			SCR_CompartmentAccessComponent compAccess = SCR_CompartmentAccessComponent.Cast(character.GetCompartmentAccessComponent());
			if (!compAccess)
				continue;
			
			compAccess.ACE_GetInVehicle(plane);
		}
		RpcDo_PlaySound(planeRplId);
		Rpc(RpcDo_PlaySound, planeRplId);
		RedLight(planeRplId);
	}
	
//Old from when we just teleported the player
//	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
//	void RpcDo_TeleportPlayer(int playerId, RplId planeId, int slotId, int index)
//	{
//		if (!Replication.FindItem(planeId))
//			return;
//		
//		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
//		if (!plane)
//			return;
//		
//		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
//		EntitySlotInfo slot = slotMan.GetSlotByName("Slot" + slotId);
//		vector transform[4];
//		slot.GetLocalTransform(transform);
//		if (index < 15)
//        {
//            transform[3][2] = transform[3][2] - (0.8 * index);
//        }
//        else
//        {
//            transform[3][0] = transform[3][0] + 1.4;
//            transform[3][2] = transform[3][2] - (0.8 * (index - 15));
//        }
//		vector pos = plane.CoordToParent(transform[3]);
//		transform[3] = pos;
//		SCR_Global.TeleportPlayer(playerId, transform[3], SCR_EPlayerTeleportedReason.NONE);
//	}
	
	float m_fParachuteCheck = 0;
	float m_fBroadcastTimer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_aFlightObjects)
		{
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
			return;
		}
		
		if (m_aFlightObjects.Count() == 0)
		{
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
			return;
		}
		
		bool checkDeployParachutes = false;
		if (m_fParachuteCheck >= 0.1)
		{
			checkDeployParachutes = true;
			m_fParachuteCheck = 0;
		}
		else
			m_fParachuteCheck += timeSlice;
		
//		// Throttle position broadcast to ~15 Hz instead of every frame
//				Print("[CRF_AirdropManager.EOnFrame] debug line (" + __FILE__ + " L" + __LINE__ + ")", LogLevel.WARNING);
//		bool broadcastPosition = false;
//		if (m_fBroadcastTimer >= 0.067)
//		{
//			broadcastPosition = true;
//			m_fBroadcastTimer = 0;
//		}
//		else
//			m_fBroadcastTimer += timeSlice;
		foreach (CRF_AirdropFlight flight: m_aFlightObjects)
		{
			if (checkDeployParachutes)
			{
				PlayerManager pm = GetGame().GetPlayerManager();
				foreach (int i, IEntity player: flight.m_PlayersInPlane)
				{
					//Remove player if it's null
					//How????
					//Idk
					if (!player)
					{
						flight.m_PlayersInPlane.Remove(i);
						continue;
					}
					
					int playerId = pm.GetPlayerIdFromControlledEntity(player);
					if (playerId <= 0)
						continue;
					
					SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(player);
					if (!character)
						continue;
		
					SCR_CompartmentAccessComponent compAccess = SCR_CompartmentAccessComponent.Cast(character.GetCompartmentAccessComponent());
					if (!compAccess)
						continue;
					
					if (compAccess.IsInCompartment())
						continue;
					
					if (flight.m_bAutoDeployParachute)
					{
						PlayerController pc = pm.GetPlayerController(playerId);
						if (!pc)
							continue;
						
						CRF_ParachutePlayerComponent paraComp = CRF_ParachutePlayerComponent.Cast(pc.FindComponent(CRF_ParachutePlayerComponent));
						if (!paraComp)
							continue;
						
						Print("Requesting parachute");
						GetGame().GetCallqueue().CallLater(paraComp.Rpc_RequestDeploy, 1000, false);
						flight.m_PlayersInPlane.Remove(i);
					}
				}
			}		
			
			if (flight.m_fProgress >= 2.0)
            	m_aFlightObjects.RemoveItem(flight);

			float distance = vector.Distance(flight.m_vFlightCoordinates[0], flight.m_vFlightCoordinates[3]);
			float step = (flight.m_fSpeed * timeSlice) / distance;
			
			vector A = flight.m_vFlightCoordinates[0]; 
			vector B = flight.m_vFlightCoordinates[3]; 
			vector G = flight.m_vFlightCoordinates[2]; 
			
			vector AB = B - A;
			vector AG = G - A;
			
			float lenAB = AB.Length();
			flight.m_fGreenT = vector.Dot(AG, AB) / (lenAB * lenAB);
			flight.m_fGreenT = Math.Clamp(flight.m_fGreenT, 0, 1);

    		float previousProgress = flight.m_fProgress;
			flight.m_fProgress = Math.Clamp(flight.m_fProgress + step, 0, 2);
			
			if (previousProgress < flight.m_fGreenT && flight.m_fProgress >= flight.m_fGreenT)
			    GreenLight(flight.m_RplId);
	
	        vector newPos = vector.Lerp(flight.m_vFlightCoordinates[0], flight.m_vFlightCoordinates[3], flight.m_fProgress);
			vector transform[4];
			flight.m_Plane.GetTransform(transform);
			transform[3] = newPos;
			
			flight.m_EditableComp.SetTransform(transform);
			
//			GenericEntity plane = GenericEntity.Cast(flight.m_Plane);
//	        plane.SetWorldTransform(transform);
//			plane.Update();
//			plane.OnTransformReset();
//			int catch = 0;
//			bool child = true;
//			IEntity parent = plane;
//			while (child && catch < 10)
//			{
//				IEntity childEntity = parent.GetChildren();
//				if (!childEntity)
//				{
//					child = false;
//					break;
//				}
//				parent = childEntity;
//				GenericEntity childPlane = GenericEntity.Cast(parent);
//				childPlane.SetWorldTransform(transform);
//				childPlane.SetScale(2);
//				childPlane.Update();
//				childPlane.OnTransformReset();
//				catch++;
//			}
//			if (broadcastPosition)
				Rpc(RpcDo_BroadcastPositionUpdate, flight.m_RplId, transform);
		}
	}
	
	void RedLight(RplId planeId)
	{
		#ifdef WORKBENCH
		RpcDo_SpawnRedLight(planeId);
		#else
		Rpc(RpcDo_SpawnRedLight, planeId);
		#endif
	}
	
	void GreenLight(RplId planeId)
	{
		#ifdef WORKBENCH
		RpcDo_SpawnGreenLight(planeId);
		#else
		Rpc(RpcDo_SpawnGreenLight, planeId);
		#endif
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SpawnRedLight(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		
		for (int i = 0; i < 5; i++)
		{
			
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
			if (!slot)
				continue;
			
			EntitySpawnParams params = new EntitySpawnParams();
			if (slot.GetAttachedEntity())
				delete slot.GetAttachedEntity();
			
			slot.GetWorldTransform(params.Transform);
			IEntity light = GetGame().SpawnEntityPrefab(Resource.Load("{CA26D8A680895BBD}Prefabs/Vehicles/Airplanes/C-130/Props/Lights/RedLightObject.et"), null, params);
			slot.AttachEntity(light);
		}
		
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SpawnGreenLight(RplId planeId)
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		
		for (int i = 0; i < 5; i++)
		{
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
			if (!slot)
				continue;
			
			EntitySpawnParams params = new EntitySpawnParams();
			if (slot.GetAttachedEntity())
				delete slot.GetAttachedEntity();
			
			slot.GetWorldTransform(params.Transform);
			IEntity light = GetGame().SpawnEntityPrefab(Resource.Load("{7CBC56493AB0430E}Prefabs/Vehicles/Airplanes/C-130/Props/Lights/GreenLightObject.et"), null, params);
			slot.AttachEntity(light);
		}
		
	}
	
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastPositionUpdate(RplId planeId, vector transform[4])
	{
		if (!Replication.FindItem(planeId))
			return;
		
		IEntity plane = RplComponent.Cast(Replication.FindItem(planeId)).GetEntity();
		if (!plane)
			return;
		
		GenericEntity gPlane = GenericEntity.Cast(plane);
        gPlane.SetWorldTransform(transform);
		gPlane.Update();
		gPlane.OnTransformReset();
	}
	
	void RegisterFlight(CRF_AirdropFlight flight)
	{
		m_aFlightObjects.Insert(flight);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	int AssignAATarget()
	{
		if (m_aFlightObjects.Count() == 0)
			return -1;
		if (m_iPlanesAssigned >= m_aFlightObjects.Count())
		{
			m_iPlanesAssigned = 0;
			return m_iPlanesAssigned;
		}
		
		m_iPlanesAssigned++;
		return m_iPlanesAssigned - 1;
	}
}

class CRF_AirdropFlight
{
	void CRF_AirdropFlight(IEntity plane, vector flightCoordinates[4], float speed, bool autoDeployParachute = true)
	{
		m_Plane = plane;
		m_EditableComp = SCR_EditableVehicleComponent.Cast(plane.FindComponent(SCR_EditableVehicleComponent));
		m_RplId = RplComponent.Cast(plane.FindComponent(RplComponent)).Id();
		m_vFlightCoordinates = flightCoordinates;
		m_fSpeed = speed;
		m_bAutoDeployParachute = autoDeployParachute;
		CRF_AirdropManager.GetInstance().RegisterFlight(this);
	}
	
	void ~CRF_AirdropFlight()
	{
		if (!Replication.IsServer())
			return;
		
		SCR_EntityHelper.DeleteEntityAndChildren(m_Plane);
	}
	
	IEntity m_Plane;
	SCR_EditableVehicleComponent m_EditableComp;
	RplId m_RplId;
	vector m_vFlightCoordinates[4];
	bool m_bGreenLight = false;
	float m_fProgress = 0;
	float m_fSpeed;
	float m_fGreenT;
	ref array<IEntity> m_PlayersInPlane = {};
	bool m_bAutoDeployParachute;
}

class CRF_AirdropObject
{
	void CRF_AirdropObject(string factionKey, ResourceName resourceName, vector flightCoordinates[4], float angle, array<int> playerIds, bool autoDeployParachute = true)
	{
		m_sFactionKey = factionKey;
		m_sPlane = resourceName;
		m_vFlightCoordinates = flightCoordinates;
		m_fAngle = angle;
		m_aPlayerIds = playerIds;
		m_bAutoDeployParachute = autoDeployParachute;
	}
	ref array<int> m_aPlayerIds = {};
	string m_sFactionKey;
	ResourceName m_sPlane;
	float m_fAngle;
	//0 - Redlight start
	//1 - Redlight end
	//2 - Greenlight start
	//3 - Greenlight end
	vector m_vFlightCoordinates[4];
	bool m_bAutoDeployParachute;
}