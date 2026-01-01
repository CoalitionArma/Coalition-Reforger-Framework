class CRF_PlayableCharacterClass : ScriptComponentClass
{
}

class CRF_PlayableCharacter : ScriptComponent
{
	// State variables
	protected bool m_bIsSlotSpawned = false;
	protected bool m_bCameraUpdateEnabled;
	
	// Component references
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_PlayerControllerManager m_PlayerControllerComponent;
	protected SCR_PossessingManagerComponent m_PossessingManagerComponent;
	
	protected IEntity m_eSpecEntity;
	//So the client tracks where he needs to teleport his player
	//Since teleporting is mostly client authorative
	vector m_vSpreadPos;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_Gamemode = CRF_Gamemode.GetInstance();

		// Skip initialization if conditions aren't met
		if (!ShouldInitializeCharacter(owner))
			return;

		// Initialize managers
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_PlayerControllerComponent = CRF_PlayerControllerManager.GetInstance();
		m_PossessingManagerComponent = SCR_PossessingManagerComponent.GetInstance();
		
		// Must be called later due to race condition with AI groups and entity initialization
		// This ensures the entity is fully initialized before we process it
		GetGame().GetCallqueue().CallLater(SetInitialEntity, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME + 25, false, owner);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool ShouldInitializeCharacter(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return false;
		
		if (!m_Gamemode)
			return false;
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && 
			m_Gamemode.m_bCurrentEnableAIInGameState && 
			!CRF_GamemodeManager.IsSpectator(owner))
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsSlotSpawned()
	{
		m_bIsSlotSpawned = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraUpdateEnabled(bool updateCamera, IEntity specEntity)
	{
		m_eSpecEntity = specEntity;
		m_bCameraUpdateEnabled = updateCamera;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetInitialEntity(IEntity owner)
	{
		// Disable AI next frame to avoid race condition
		GetGame().GetCallqueue().Call(DisableAI, owner);
		
		// Get if we are a spectator
		bool isSpec = CRF_GamemodeManager.IsSpectator(owner);
		
		// Logs entity on server and disables AI if not spawned by a slot
		if (RplSession.Mode() != RplMode.Client && !m_bIsSlotSpawned && !isSpec)
		{
			m_SlottingManager.AddPlayableEntityToManager(owner);
			return;
		}
		
		// Configure spectator entity
		if (isSpec)
			ConfigureSpectatorEntity(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ConfigureSpectatorEntity(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
		
		// Check if this is a CRF_InitialEntity that needs random positioning
		string prefabName = owner.GetPrefabData().GetPrefabName();
		bool isCRFInitialEntity = prefabName.Contains("CRF_InitialEntity.et");
		
		if (isCRFInitialEntity)
		{
			if (Replication.IsServer())
				GenerateSpreadPosServer(owner);
			else
				RequestSpreadPos();
		}
		else if (!CRF_GamemodeManager.IsValidSpawnVector(owner.GetOrigin()))
		{
			// Use random spread position for other spectators too, instead of hardcoded 0,10000,0
			if (Replication.IsServer())
				GenerateSpreadPosServer(owner);
			else
				RequestSpreadPos();
		}
		
		Physics physics = owner.GetPhysics();
		if (!physics)
			return;
		
		physics.EnableGravity(false);
		physics.ChangeSimulationState(SimulationState.NONE);
		physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
		
		int numGeoms = physics.GetNumGeoms();
		for (int i = 0; i <= numGeoms; i++)
		{
			physics.SetGeomInteractionLayer(i, EPhysicsLayerDefs.CharNoCollide);
		}
	}
	
	void GenerateSpreadPosServer(IEntity entity)
	{
		vector mapCenter;
		float radius;
		CRF_Gamemode.GetInstance().GetAOCenterAndRadius(mapCenter, radius);
		vector spreadPos = GenerateRandomSpreadPosition(mapCenter, radius);
		spreadPos[1] = 1000.0; // Set elevation to 1000m
		m_vSpreadPos = spreadPos;
		entity.SetOrigin(spreadPos);
	}
	
	void RequestSpreadPos()
	{
		if (!GetOwner().FindComponent(RplComponent))
			return;
		
		RplId entityId = RplComponent.Cast(GetOwner().FindComponent(RplComponent)).Id();
		CRF_RplToAuthorityManager.GetInstance().RequestSpreadPos(entityId);
	}
	
	void SendSpreadPos()
	{
		Rpc(RpcDo_SendSpreadPos, m_vSpreadPos);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SendSpreadPos(vector spreadPos)
	{
		m_vSpreadPos = spreadPos;
	}
	
	//------------------------------------------------------------------------------------------------
	void DisableAI(IEntity owner)
	{
		AIControlComponent aiComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
		
		GetGame().GetCallqueue().Call(DisableAIWrap, owner, aiComponent);
	}

	//------------------------------------------------------------------------------------------------
	void DisableAIWrap(IEntity owner, AIControlComponent aiComponent)
	{
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		
		// Check if entity should be deleted
		if (ShouldDeleteEntity(owner))
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
			return;
		}
		
		UpdateEntityPhysics(owner);
		
		if (m_bCameraUpdateEnabled)
			OnFrameSpectatorCamera();
		
		// Handle position updates for local player entity
		if (RplSession.Mode() != RplMode.Dedicated && SCR_PlayerController.GetLocalMainEntity() == owner)
		{
			UpdatePlayerPosition(owner);
		} 
		else if (RplSession.Mode() != RplMode.Dedicated)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
		}
	}
	
	/**
	 * Frame event handler for smooth spectator camera tracking
	 * Called every frame when spectating an entity for smoother camera movement
	 */
	protected void OnFrameSpectatorCamera()
	{
		// Exit if no spectator entity
		if (!m_eSpecEntity)
			return;
		
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		if (!playerControllerComp || !playerControllerComp.m_eCamera)
		{
			return;
		}
		
		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(m_eSpecEntity.FindComponent(SlotManagerComponent));
		if (!slotComp)
		{
			return;
		}
		
		// Get the first-person camera slot
		EntitySlotInfo camera = slotComp.GetSlotByName("CRF_FPP");
		if (!camera)
		{
			return;
		}
		
		// Get transform and modify it to be slightly behind and to the right of the player
		vector transform[4];
		camera.GetTransform(transform);
		
		// Calculate offset position
		vector forward = transform[2];  // Z-axis is forward in the transform matrix
		vector right = transform[0];    // X-axis is right in the transform matrix
		
		// Move camera back by 0.5 meters and right by 0.3 meters (over weapon shoulder)
		vector offsetPosition = transform[3] - (forward * 0.5) + (right * 0.3);
		
		// Apply the offset to the transform
		transform[3] = offsetPosition;
		
		// Apply transform to spectator camera
		playerControllerComp.m_eCamera.SetTransform(transform);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool ShouldDeleteEntity(IEntity owner)
	{
		return RplSession.Mode() != RplMode.Client && 
			   !EntityUtils.IsPlayer(owner) && 
			   m_PossessingManagerComponent.GetIdFromMainEntity(owner) == 0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateEntityPhysics(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		if (!physics)
			return;
		
		physics.EnableGravity(false);
		physics.SetVelocity(vector.Zero);
		physics.SetAngularVelocity(vector.Zero);
		physics.SetMass(0);
		physics.SetDamping(1, 1);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerPosition(IEntity owner)
	{
		if (!m_PlayerControllerComponent.m_eCamera)
			return;
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			UpdateGamePlayerPosition();
		}
		else
		{
			if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR && m_Gamemode.m_bUseAAR)
				UpdateSpectatorPosition();
			else
				UpdateGamePlayerPosition();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateGamePlayerPosition()
	{
		vector mat[4];
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			m_PlayerControllerComponent.m_eCamera.GetWorldTransform(mat);

			if (GetGame().GetCallqueue().GetRemainingTime(m_PlayerControllerComponent.UpdateStoredCameraPos) <= 0)
			{
				GetGame().GetCallqueue().CallLater(
					m_PlayerControllerComponent.UpdateStoredCameraPos, 
					1000, 
					false, 
					mat[0], mat[1], mat[2], mat[3]
				);
			}
			mat[3][1] = mat[3][1] - 1.5;
		}
		else
		{
			mat[1] = vector.Up;
			mat[2] = vector.Forward;
			mat[3] = m_vSpreadPos;
		}
		
		
		m_PlayerControllerComponent.UpdateEntityPos(mat);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateSpectatorPosition()
	{
		vector mat[4];
		mat[1] = vector.Up;
		mat[2] = vector.Forward;
		mat[3] = m_vSpreadPos;
		
		m_PlayerControllerComponent.UpdateEntityPos(mat);
		m_PlayerControllerComponent.m_eCamera.SetWorldTransform(mat);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Generate a random position within specified radius to spread out initial entity spawns
	* This reduces replication congestion when many entities spawn in the same location
	* @param centerPosition Original spawn position to spread from
	* @param maxRadius Maximum radius in meters to spread entities (default 500m)
	* @return New spawn position within the spread radius
	*/
	static vector GenerateRandomSpreadPosition(vector centerPosition, float maxRadius = 500.0)
	{
		// Generate random angle (0-360 degrees)
		float randomAngle = Math.RandomFloat(0, 2 * Math.PI);
		
		// Generate random distance within radius (using square root for uniform distribution)
		float randomDistance = Math.Sqrt(Math.RandomFloat(0, 1)) * maxRadius;
		
		// Calculate offset from center
		float offsetX = Math.Cos(randomAngle) * randomDistance;
		float offsetZ = Math.Sin(randomAngle) * randomDistance;
		
		// Apply offset to center position
		vector spreadPosition = centerPosition;
		spreadPosition[0] = centerPosition[0] + offsetX;
		spreadPosition[2] = centerPosition[2] + offsetZ;
		
		// For initial entities, we don't need terrain validation since they're at 10000m elevation
		// Just return the spread position
		return spreadPosition;
	}
}
