class CRF_PlayableCharacterClass : ScriptComponentClass
{
}

class CRF_PlayableCharacter : ScriptComponent
{
	[Attribute()]
	string m_sName;
	
	[Attribute("0")]
	bool m_bIsPlayable;

	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_ESlotType))]
	CRF_ESlotType m_SlottingRole;

	protected bool m_bIsSlotSpawned = false;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_PlayerControllerComponent m_PlayerControllerComponent;
	protected SCR_PossessingManagerComponent m_PossessingManagerComponent;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_Gamemode = CRF_Gamemode.GetInstance();

		if (!GetGame().InPlayMode() 
		 || !m_Gamemode 
		 || !m_bIsPlayable
		 || (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && m_Gamemode.EnableAIInGameState && !CRF_GamemodeManager.IsSpectator(owner)))
			return;

		// Get all managers we need
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_PlayerControllerComponent = CRF_PlayerControllerComponent.GetInstance();
		m_PossessingManagerComponent = SCR_PossessingManagerComponent.GetInstance();
		
		GetGame().GetCallqueue().CallLater(SetInitialEntity, 100, false, owner);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsSlotSpawned()
	{
		m_bIsSlotSpawned = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetInitialEntity(IEntity owner)
	{
		// Disable AI
		GetGame().GetCallqueue().CallLater(DisableAI, 0, false, owner);
		
		// Get if we are a spectator
		bool isSpec = CRF_GamemodeManager.IsSpectator(owner);
		
		// Logs entity on server and disables AI if not spawned by a slot
		if (RplSession.Mode() != RplMode.Client && !m_bIsSlotSpawned && !isSpec)
		{
			m_SlottingManager.AddPlayableEntityToManager(owner);
			return;
		};
		
		// Sets location and all the physics BS on all machines
		if (isSpec)
		{
			SetEventMask(owner, EntityEvent.FRAME);
			owner.SetOrigin("0 10000 0");
			
			Physics physics = owner.GetPhysics();
			if (physics)
			{
				physics.EnableGravity(false);
				physics.ChangeSimulationState(SimulationState.NONE);
				physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
				for (int i = 0; i <= physics.GetNumGeoms(); i++)
				{
					physics.SetGeomInteractionLayer(i, EPhysicsLayerDefs.CharNoCollide);
				}
			};
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void DisableAI(IEntity owner)
	{
		if (AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent())
			AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent().DeactivateAI();
		GetGame().GetCallqueue().CallLater(DisableAIWrap, 0, false, owner)
	}

	//------------------------------------------------------------------------------------------------
	void DisableAIWrap(IEntity owner)
	{
		if (AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent())
			AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent().DeactivateAI();
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeslice);
		
		if (RplSession.Mode() != RplMode.Client && !EntityUtils.IsPlayer(owner) && m_PossessingManagerComponent.GetIdFromMainEntity(owner) == 0)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
			return;
		};
		
		if (RplSession.Mode() != RplMode.Dedicated && SCR_PlayerController.GetLocalMainEntity() == owner)
		{
			if (m_PlayerControllerComponent.m_eCamera)
			{		
				if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
				{
					vector mat[4];
					m_PlayerControllerComponent.m_eCamera.GetWorldTransform(mat);
					
					if (GetGame().GetCallqueue().GetRemainingTime(m_PlayerControllerComponent.UpdateStoredCameraPos) <= 0)
						GetGame().GetCallqueue().CallLater(m_PlayerControllerComponent.UpdateStoredCameraPos, 1000, false, mat[0], mat[1], mat[2], mat[3]);
					
					mat[3][1] = mat[3][1] - 1.5;
					m_PlayerControllerComponent.UpdateEntityPos(mat);
				} else {
					vector mat[4];
					mat[1] = vector.Up;
					mat[2] = vector.Forward;
					mat[3][1] = 10000;
					m_PlayerControllerComponent.UpdateEntityPos(mat);
					m_PlayerControllerComponent.m_eCamera.SetWorldTransform(mat);
				};
			};
		} else {
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		}

		Physics physics = owner.GetPhysics();
		if (physics)
		{
			physics.EnableGravity(false);
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
			physics.SetMass(0);
			physics.SetDamping(1, 1);
		};
	}
}
