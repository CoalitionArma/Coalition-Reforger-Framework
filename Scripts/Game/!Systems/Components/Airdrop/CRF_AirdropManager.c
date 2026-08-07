class CRF_AirdropManagerClass: SCR_BaseGameModeComponentClass
{
}

class CRF_AirdropManager: SCR_BaseGameModeComponent
{
	/*
	static CRF_AirdropManager m_sInstance;
	protected ref array<ref CRF_AirdropFlight> m_aFlightObjects = {};
	
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
			//This group will put us past the 30 slots available in the plane, we gotta spawn another one
			if (playersAdded + 1 > 30)
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
		Math3D.AnglesToMatrix(Vector(planeObject.m_fAngle, 0, 0), angles);
		EntitySpawnParams params = new EntitySpawnParams();
		params.Transform[0] = angles[0];
		params.Transform[1] = angles[1];
		params.Transform[2] = angles[2];
		params.Transform[3] = planeObject.m_vFlightCoordinates[0];
		IEntity plane = GetGame().SpawnEntityPrefab(Resource.Load(planeObject.m_sPlane), null, params);
		//Redundant but just in case
		StreamPlaneIntoReplication(plane);
		ref CRF_AirdropFlight flight = new CRF_AirdropFlight(plane, planeObject.m_vFlightCoordinates, 65, planeObject.m_bAutoDeployParachute);
		//Delay so the flight has a chance to actual load the entity
		GetGame().GetCallqueue().CallLater(TeleportPlayers, 2000, false, players, SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent)), plane, flight);
	}
	
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
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(planeId));
		if (!rpl)
			return;

		IEntity plane = rpl.GetEntity();
		if (!plane)
			return;

		SCR_BaseInteractiveLightComponent lightComp = SCR_BaseInteractiveLightComponent.Cast(plane.FindComponent(SCR_BaseInteractiveLightComponent));
		if (lightComp)
			lightComp.ToggleLight(true);
	}
	
		
	void TeleportPlayers(string players, SlotManagerComponent slotMan, IEntity plane, CRF_AirdropFlight flight)
	{
		array<string> playerIds = {};
		players.Split("|", playerIds, true);
		int slotId = 0;
		RplComponent planeRpl = RplComponent.Cast(plane.FindComponent(RplComponent));
		if (!planeRpl)
			return;
		RplId planeRplId = planeRpl.Id();
		PlayerManager pm = GetGame().GetPlayerManager();
		foreach (int i, string playerId: playerIds)
		{
			//Let's delay adding them until the player has had time to teleport into the plane
			// Register by player ID, not by entity pointer. This previously scheduled Insert()
			// directly on the flight's member array with an entity resolved 2000ms early - so the
			// queue held both a pointer into another object's container and a character that could
			// die in transit. RegisterPlayerInPlane re-resolves everything at call time.
			GetGame().GetCallqueue().CallLater(RegisterPlayerInPlane, 2000, false, flight.m_RplId, playerId.ToInt());
			EntitySlotInfo slot = slotMan.GetSlotByName("Slot" + slotId.ToString());
			vector transform[4];
			slot.GetLocalTransform(transform);
			if (i < 15)
	        {
	            transform[3][2] = transform[3][2] - (0.8 * i);
	        }
	        else
	        {
	            transform[3][0] = transform[3][0] + 1.4;
	            transform[3][2] = transform[3][2] - (0.8 * (i - 15));
	        }
			vector pos = plane.CoordToParent(transform[3]);
			transform[3] = pos;
			SCR_Global.TeleportPlayer(playerId.ToInt(), transform[3], SCR_EPlayerTeleportedReason.NONE);
			Rpc(RpcDo_TeleportPlayer, playerId.ToInt(), planeRplId, slotId, i);
		}
		RpcDo_PlaySound(planeRplId);
		Rpc(RpcDo_PlaySound, planeRplId);
		RedLight(planeRplId);
	}

	//------------------------------------------------------------------------------------------------
	//! Deferred registration of a player as "in the plane", resolved entirely at call time.
	//! Both the flight and the player may be gone by the time this runs (flight completed, player
	//! died or disconnected during the 2s teleport window), so nothing is captured up front.
	protected void RegisterPlayerInPlane(RplId flightRplId, int playerId)
	{
		CRF_AirdropFlight flight = FindFlightByRplId(flightRplId);
		if (!flight)
			return;

		if (!GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId))
			return;

		if (!flight.m_aPlayerIdsInPlane.Contains(playerId))
			flight.m_aPlayerIdsInPlane.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Look up a live flight by the RplId of its plane. Returns null once the flight has ended.
	protected CRF_AirdropFlight FindFlightByRplId(RplId flightRplId)
	{
		if (!m_aFlightObjects)
			return null;

		foreach (CRF_AirdropFlight flight : m_aFlightObjects)
		{
			if (flight && flight.m_RplId == flightRplId)
				return flight;
		}

		return null;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TeleportPlayer(int playerId, RplId planeId, int slotId, int index)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(planeId));
		if (!rpl)
			return;

		IEntity plane = rpl.GetEntity();
		if (!plane)
			return;

		SlotManagerComponent slotMan = SlotManagerComponent.Cast(plane.FindComponent(SlotManagerComponent));
		if (!slotMan)
			return;

		EntitySlotInfo slot = slotMan.GetSlotByName("Slot" + slotId);
		if (!slot)
			return;

		vector transform[4];
		slot.GetLocalTransform(transform);
		if (index < 15)
        {
            transform[3][2] = transform[3][2] - (0.8 * index);
        }
        else
        {
            transform[3][0] = transform[3][0] + 1.4;
            transform[3][2] = transform[3][2] - (0.8 * (index - 15));
        }
		vector pos = plane.CoordToParent(transform[3]);
		transform[3] = pos;
		SCR_Global.TeleportPlayer(playerId, transform[3], SCR_EPlayerTeleportedReason.NONE);
	}
	
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
		
		// Throttle position broadcast to ~15 Hz instead of every frame
		bool broadcastPosition = false;
		if (m_fBroadcastTimer >= 0.067)
		{
			broadcastPosition = true;
			m_fBroadcastTimer = 0;
		}
		else
			m_fBroadcastTimer += timeSlice;
		// Flights that finished this tick. Collected here and ended after the loop: ending a flight
		// deletes its plane, and the original code called RemoveItem() mid-iteration and then kept
		// using `flight` and `flight.m_Plane` for the rest of the body - dereferencing an object it
		// had just destroyed. Never mutate the array being iterated.
		array<CRF_AirdropFlight> completedFlights = {};

		foreach (CRF_AirdropFlight flight: m_aFlightObjects)
		{
			if (!flight)
				continue;

			if (flight.m_fProgress >= 2.0)
			{
				completedFlights.Insert(flight);
				continue;
			}

			if (checkDeployParachutes)
			{
				PlayerManager pm = GetGame().GetPlayerManager();

				// Players are tracked by ID and resolved here, so a player who died or disconnected
				// mid-flight simply fails to resolve. The old code stored raw character pointers and
				// had a "//Remove player if it's null / How???? / Idk" guard - that null was a
				// character deleted out from under the array.
				// Iterate backwards so removals do not shift entries we have not visited yet.
				for (int i = flight.m_aPlayerIdsInPlane.Count() - 1; i >= 0; i--)
				{
					int playerId = flight.m_aPlayerIdsInPlane[i];

					IEntity player = pm.GetPlayerControlledEntity(playerId);
					if (!player)
					{
						flight.m_aPlayerIdsInPlane.Remove(i);
						continue;
					}

					SCR_CharacterControllerComponent charCon = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
					if (!charCon)
						continue;

					CharacterAnimationComponent animComp = charCon.GetAnimationComponent();
					if (!animComp || animComp.PhysicsIsLinked())
						continue;

					if (flight.m_bAutoDeployParachute)
					{
						PlayerController playerController = pm.GetPlayerController(playerId);
						if (playerController)
						{
							ParachuteComponent parachute = ParachuteComponent.Cast(playerController.FindComponent(ParachuteComponent));
							if (parachute)
								parachute.RpcAskDeployParachute();
						}
					}

					flight.m_aPlayerIdsInPlane.Remove(i);
				}
			}

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

			// Resolve the plane through replication rather than the stored m_Plane pointer, so a
			// plane destroyed by any other path (shot down, world cleanup) ends the flight instead
			// of being dereferenced.
			GenericEntity plane = GenericEntity.Cast(ResolveFlightPlane(flight));
			if (!plane)
			{
				completedFlights.Insert(flight);
				continue;
			}

			vector transform[4];
			plane.GetTransform(transform);
			transform[3] = newPos;

	        plane.SetWorldTransform(transform);
			plane.Update();
			plane.OnTransformReset();
			if (broadcastPosition)
				Rpc(RpcDo_BroadcastPositionUpdate, flight.m_RplId, transform);
		}

		// Safe to mutate now that iteration is finished.
		foreach (CRF_AirdropFlight completed : completedFlights)
			EndFlight(completed);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve a flight's plane entity from its RplId. Returns null once the plane is gone.
	protected IEntity ResolveFlightPlane(notnull CRF_AirdropFlight flight)
	{
		RplComponent planeRpl = RplComponent.Cast(Replication.FindItem(flight.m_RplId));
		if (!planeRpl)
			return null;

		return planeRpl.GetEntity();
	}

	//------------------------------------------------------------------------------------------------
	//! Explicitly end a flight: delete its plane, then drop it from the tracking array.
	//! This work used to live in ~CRF_AirdropFlight(), which meant an engine entity was deleted from
	//! a script destructor at an arbitrary GC point - with no guarantee the plane still existed, and
	//! no way for callers to control when it happened. Doing it here makes the ordering explicit.
	protected void EndFlight(CRF_AirdropFlight flight)
	{
		if (!flight)
			return;

		if (Replication.IsServer())
		{
			IEntity plane = ResolveFlightPlane(flight);
			if (plane)
				SCR_EntityHelper.DeleteEntityAndChildren(plane);
		}

		flight.m_Plane = null;

		if (m_aFlightObjects)
			m_aFlightObjects.RemoveItem(flight);
	}
	
	void RedLight(RplId planeId)
	{
		Rpc(RpcDo_SpawnRedLight, planeId);
	}
	
	void GreenLight(RplId planeId)
	{
		Rpc(RpcDo_SpawnGreenLight, planeId);
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
			EntitySpawnParams params = new EntitySpawnParams();
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
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
			EntitySpawnParams params = new EntitySpawnParams();
			EntitySlotInfo slot = slotMan.GetSlotByName("LightSlot" + i.ToString());
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
	*/
}
/*
class CRF_AirdropFlight
{
	
	void CRF_AirdropFlight(IEntity plane, vector flightCoordinates[4], float speed, bool autoDeployParachute = true)
	{
		m_Plane = plane;
		m_RplId = RplComponent.Cast(plane.FindComponent(RplComponent)).Id();
		m_vFlightCoordinates = flightCoordinates;
		m_fSpeed = speed;
		m_bAutoDeployParachute = autoDeployParachute;
		CRF_AirdropManager.GetInstance().RegisterFlight(this);
	}
	
	// NOTE: deliberately no destructor.
	// Deleting the plane from ~CRF_AirdropFlight() ran engine entity deletion at an arbitrary GC
	// point, on a raw m_Plane pointer that may already have been freed by another path. Teardown is
	// now explicit in CRF_AirdropManager.EndFlight(), which resolves the plane through replication
	// first. Do not reintroduce a destructor here.

	IEntity m_Plane;
	RplId m_RplId;
	vector m_vFlightCoordinates[4];
	bool m_bGreenLight = false;
	float m_fProgress = 0;
	float m_fSpeed;
	float m_fGreenT;
	//! Players currently aboard, tracked by player ID. Previously an array<IEntity> of character
	//! pointers, which went stale whenever a passenger died or disconnected mid-flight.
	ref array<int> m_aPlayerIdsInPlane = {};
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
*/