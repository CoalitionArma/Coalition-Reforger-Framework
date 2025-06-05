class CRF_PlayableCharacterClass : ScriptComponentClass
{
}

class CRF_PlayableCharacter : ScriptComponent
{
	// Configuration attributes
	[Attribute()]
	string m_sName;
	
	[Attribute("0")]
	bool m_bIsPlayable;

	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_ESlotType))]
	CRF_ESlotType m_SlottingRole;

	// State variables
	protected bool m_bIsSlotSpawned = false;
	
	// Component references
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_PlayerControllerManager m_PlayerControllerComponent;
	protected SCR_PossessingManagerComponent m_PossessingManagerComponent;

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
		
		// Must be calledlater due to a race condition with the ai groups being spawned. Needs refactored.
		GetGame().GetCallqueue().CallLater(SetInitialEntity, 100, false, owner);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool ShouldInitializeCharacter(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return false;
		
		if (!m_Gamemode)
			return false;
		
		if (!m_bIsPlayable)
			return false;
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && 
			m_Gamemode.EnableAIInGameState && 
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
		owner.SetOrigin("0 10000 0");
		
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
			UpdateSpectatorPosition();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateGamePlayerPosition()
	{
		vector mat[4];
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
		m_PlayerControllerComponent.UpdateEntityPos(mat);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateSpectatorPosition()
	{
		vector mat[4];
		mat[1] = vector.Up;
		mat[2] = vector.Forward;
		mat[3][1] = 10000;
		
		m_PlayerControllerComponent.UpdateEntityPos(mat);
		m_PlayerControllerComponent.m_eCamera.SetWorldTransform(mat);
	}
}
