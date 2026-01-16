modded class ParachuteComponent
{
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void RpcAskDeployParachute() // ← new parameter
	{
		if (!m_ParachuteItem)
		{
			// Set m_ParachuteItem to parachute if in inventory
			IEntity character = m_PlayerController.GetControlledEntity();
			if (!character)
				return;
	
			SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
				character.FindComponent(SCR_InventoryStorageManagerComponent));
		
			if (!invMgr)
				return;
		
			// grab every root item that is actually worn on the character
			array<IEntity> rootItems = {};
			invMgr.GetItems(rootItems);
			foreach (IEntity item : rootItems)
			{
				if (!item)
					continue;
		
				// does this root item define a parachute
				ParachuteItemComponent pc =
					ParachuteItemComponent.Cast(item.FindComponent(ParachuteItemComponent));
				if (!pc)
					continue;
			
				m_ParachuteItem = pc;
				break;          // found what we need
			}
	
			pilotEntity = character;
			m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(character.FindComponent(SCR_CompartmentAccessComponent));
	
			DeleteParachuteEntity(m_DeployedParachute);
		}
		
		if (m_bParachuteDeployed || !m_ParachuteItem || !pilotEntity || m_ParachuteItem.GetParachuteUsed())
			return;

		/* spawn with setup TRUE so components self‑insert */
		EntitySpawnParams p = new EntitySpawnParams;
		p.TransformMode = ETransformMode.WORLD;
		pilotEntity.GetWorldTransform(p.Transform);

		IEntity parachuteEntity = GetGame().SpawnEntityPrefabEx(
			m_ParachuteItem.GetParachutePrefab(),
			/*setup*/ false,
			GetGame().GetWorld(),
			p);
		
		GetGame().GetCallqueue().CallLater(DeployParachuteDelay, 100, false, parachuteEntity);
	}
	
	void DeployParachuteDelay(IEntity parachuteEntity)
	{
		m_DeployedParachute = ParachuteDeployedEntity.Cast(parachuteEntity);

		giveOwnershipToClient();

		vector v0 = pilotEntity.GetPhysics().GetVelocity();

		GetGame().GetCallqueue().CallLater(RpcAskSyncReplication, 100, false, m_DeployedParachute.GetRplId());
		GetGame().GetCallqueue().CallLater(RpcAskSyncVelocity, 150, false, v0);

		m_bParachuteDeployed = true;
	}
	
	/* 1)  NEW SERVER EXIT / DAMAGE RPC  */
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void Rpc_ServerExitParachute(float velocityAtExit)
	{
		if (!m_DeployedParachute || !pilotEntity || !m_bParachuteDeployed)
			return;

		/* damage logic – ONLY on server */
		if (velocityAtExit >= m_fHardLandingVelocity && velocityAtExit < m_fDeathLandingVelocity)
			Rpc_ServerBreakLegs();
		else if (velocityAtExit >= m_fDeathLandingVelocity)
			Rpc_ServerKillPlayer();

		m_CompartmentAccess.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);

		GetGame().GetCallqueue().CallLater(DeleteParachuteEntity, 200, false, m_DeployedParachute);
		m_DeployedParachute = null;
		m_bParachuteDeployed = false;
		Replication.BumpMe();
		
		GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1000, false, m_ParachuteItem.GetOwner());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)] 
	override void initializeParachute(vector v0)
	{
		if (!m_DeployedParachute || !m_ParachuteItem)
		{
			// Set m_ParachuteItem to parachute if in inventory
			IEntity character = m_PlayerController.GetControlledEntity();
			if (!character)
				return;
	
			SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
				character.FindComponent(SCR_InventoryStorageManagerComponent));
		
			if (!invMgr)
				return;
		
			// grab every root item that is actually worn on the character
			array<IEntity> rootItems = {};
			invMgr.GetItems(rootItems);
			foreach (IEntity item : rootItems)
			{
				if (!item)
					continue;
		
				// does this root item define a parachute
				ParachuteItemComponent pc =
					ParachuteItemComponent.Cast(item.FindComponent(ParachuteItemComponent));
				if (!pc)
					continue;
			
				m_ParachuteItem = pc;
				break;          // found what we need
			}
	
			pilotEntity = character;
			m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(character.FindComponent(SCR_CompartmentAccessComponent));
		}
		m_DeployedParachute.InitializePilot(pilotEntity, this, v0);
		m_ParachuteItem.SetParachuteUsed();
	}
	
	override bool MayDeployParachute()
	{
		Print("Trying to deploy");
		// one chute at a time and you must own one
		if (!m_ParachuteItem)
		{
			// Set m_ParachuteItem to parachute if in inventory
			IEntity character = m_PlayerController.GetControlledEntity();
			if (!character)
				return false;
	
			SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
				character.FindComponent(SCR_InventoryStorageManagerComponent));
		
			if (!invMgr)
				return false;
		
			// grab every root item that is actually worn on the character
			array<IEntity> rootItems = {};
			invMgr.GetItems(rootItems);
			foreach (IEntity item : rootItems)
			{
				if (!item)
					continue;
		
				// does this root item define a parachute
				ParachuteItemComponent pc =
					ParachuteItemComponent.Cast(item.FindComponent(ParachuteItemComponent));
				if (!pc)
					continue;
			
				m_ParachuteItem = pc;
				break;          // found what we need
			}
	
			pilotEntity = character;
			m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(character.FindComponent(SCR_CompartmentAccessComponent));
		}
		if (m_bParachuteDeployed || !m_ParachuteItem)
			return false;

		// if parachute has already been used
		if (m_ParachuteItem.GetParachuteUsed())
			return false;

		SCR_ChimeraCharacter pawn = SCR_ChimeraCharacter.Cast(pilotEntity);
		if (!pawn)
			return false;

		// no deployment while seated in a vehicle
		if (pawn.IsInVehicle())
			return false;

		// sphere test, early out when another body is close
		m_bNearbyFound = false;
		GetGame().GetWorld().QueryEntitiesBySphere(
			pawn.GetOrigin(),
			m_fSafeRadius,
			_CollectFirstNearby,
			null,
			EQueryEntitiesFlags.DYNAMIC || EQueryEntitiesFlags.STATIC);
		if (m_bNearbyFound)
			return false;

		// ordinary free fall, check vertical speed
		float terrainY = SCR_TerrainHelper.GetTerrainY(pawn.GetOrigin(), null, true);
		float heightAGL = pawn.GetOrigin()[1] - terrainY;
		if (pawn.GetPhysics().GetVelocity()[1] >= -m_fMinFallspeed && heightAGL < m_fMinimumAltitude)
			return false;

		return true;
	}
	
	override protected void EnableComponentControls()
	{
		if (!m_InputManager)
			return;

		m_InputManager.AddActionListener("ParachuteDeploy", EActionTrigger.DOWN, OnJumpPressed);
	}
}