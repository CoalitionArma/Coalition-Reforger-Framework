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
			if (m_SlottingManager)
				m_SlottingManager.AddPlayableEntityToManager(owner);
			else
				Print("[CRF_PlayableCharacter] SlottingManager not available", LogLevel.WARNING);
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
		
		// Set all initial entities to a fixed position at 500m elevation
		// Add slight offset to prevent spatial clustering issues with 70+ entities
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
		vector fixedPos = Vector(playerId * 2, 500, playerId * 2); // 2m spacing per player
		m_vSpreadPos = fixedPos;
		owner.SetOrigin(fixedPos);
		
		Physics physics = owner.GetPhysics();
		if (!physics)
			return;
		
		physics.EnableGravity(false);
		physics.ChangeSimulationState(SimulationState.NONE);
		physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
		
		int numGeoms = physics.GetNumGeoms();
		for (int i = 0; i < numGeoms; i++) // Fixed: was i <= numGeoms (off-by-one error)
		{
			physics.SetGeomInteractionLayer(i, EPhysicsLayerDefs.CharNoCollide);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void DisableAI(IEntity owner)
	{
		AIControlComponent aiComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (!agent)
			return;
		
		agent.DeactivateAI();
		
		// Double-check deactivation next frame
		GetGame().GetCallqueue().Call(DisableAIWrap, owner, aiComponent);
	}

	//------------------------------------------------------------------------------------------------
	void DisableAIWrap(IEntity owner, AIControlComponent aiComponent)
	{
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		
		// Check if entity should be deleted - disabled for now
		// if (ShouldDeleteEntity(owner))
		// {
		// 	ClearEventMask(owner, EntityEvent.FRAME);
		// 	SCR_EntityHelper.DeleteEntityAndChildren(owner);
		// 	return;
		// }
		
		UpdateEntityPhysics(owner);
		
		if (m_bCameraUpdateEnabled)
			OnFrameSpectatorCamera();
		
		// Handle position updates for local player entity only
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		if (SCR_PlayerController.GetLocalMainEntity() == owner)
			UpdatePlayerPosition(owner);
		else
			ClearEventMask(owner, EntityEvent.FRAME);
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
			return;
		
		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(m_eSpecEntity.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;
		
		// Get the first-person camera slot
		EntitySlotInfo camera = slotComp.GetSlotByName("CRF_FPP");
		if (!camera)
			return;
		
		// Get transform and modify it to be slightly behind and to the right of the player
		vector transform[4];
		camera.GetTransform(transform);
		
		// Calculate offset position (0.5m back, 0.3m right for over-shoulder view)
		vector offsetPosition = transform[3] - (transform[2] * 0.5) + (transform[0] * 0.3);
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
		
		// AAR state check
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR && m_Gamemode.m_bUseAAR)
		{
			UpdateSpectatorPosition();
			return;
		}
		
		// Default to game player position
		UpdateGamePlayerPosition();
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
}
