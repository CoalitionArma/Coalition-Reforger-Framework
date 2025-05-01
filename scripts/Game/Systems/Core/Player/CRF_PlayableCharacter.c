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

	protected bool m_bIsSpectator = false;
	protected bool m_bIsSlotSpawned = false;
	protected bool m_bIsHidden = false;
	protected bool m_bInitTime = false;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_PlayerControllerComponent m_PlayerControllerComponent;
	protected SCR_PossessingManagerComponent m_PossessingManagerComponent;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_Gamemode = CRF_Gamemode.GetInstance();

		if (!GetGame().InPlayMode() || !m_Gamemode)
			return;
		
		// Get all managers we need
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_PlayerControllerComponent = CRF_PlayerControllerComponent.GetInstance();
		m_PossessingManagerComponent = SCR_PossessingManagerComponent.GetInstance();

		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && m_Gamemode.EnableAIInGameState && !CRF_GamemodeManager.IsSpectator(owner))
			m_bIsPlayable = false;

		GetGame().GetCallqueue().CallLater(SetInitTime, 5000, false);

		if (m_bIsPlayable)
		{
			GetGame().GetCallqueue().CallLater(SetInitialEntity, 500, false, owner);
			GetGame().GetCallqueue().CallLater(DisableAI, 0, false, owner);
		}

		if (CRF_GamemodeManager.IsSpectator(owner))
		{
			m_bIsSpectator = true;
			SetEventMask(owner, EntityEvent.FRAME);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void SetInitTime()
	{
		m_bInitTime = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsSlotSpawned()
	{
		m_bIsSlotSpawned = true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeslice);

		if (!owner || !GetGame().InPlayMode() || !m_bIsPlayable)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		};

		if (!EntityUtils.IsPlayer(owner) && m_PossessingManagerComponent.GetIdFromMainEntity(owner) == 0 && RplSession.Mode() != RplMode.Client && m_bInitTime)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
			return;
		};

		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetLocalControlledEntity() == owner)
		{
			if (m_PlayerControllerComponent.m_eCamera && m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			{
				vector mat[4];
				m_PlayerControllerComponent.m_eCamera.GetTransform(mat);
				mat[1] = vector.Up;
				mat[2] = vector.Forward;
				mat[3][1] = mat[3][1] - 1.5;
				m_PlayerControllerComponent.UpdateEntityPos(mat);
				m_PlayerControllerComponent.UpdateStoredCameraPos(mat);
			} else {
				vector mat[4];
				mat[1] = vector.Up;
				mat[2] = vector.Forward;
				mat[3][1] = 10000;
				m_PlayerControllerComponent.UpdateEntityPos(mat);

				if (m_PlayerControllerComponent.m_eCamera)
					m_PlayerControllerComponent.m_eCamera.SetWorldTransform(mat);
			};
		};

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
	void SetInitialEntity(IEntity owner)
	{
		//Logs entity on server and disables AI if not spawned by a slot
		if (RplSession.Mode() != RplMode.Client && !m_bIsSlotSpawned && !m_bIsSpectator)
		{
			SCR_AIGroup playableGroup = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
			if (playableGroup)
				m_SlottingManager.AddPlayableEntityToManager(owner);
		}

		//Sets location and all the physics BS on all machines
		if (m_bIsSpectator)
		{
			owner.SetOrigin("0 10000 0");
			if (!m_bIsHidden)
			{
				Physics physics = owner.GetPhysics();
				if (physics)
				{
					//owner.ClearFlags(EntityFlags.VISIBLE|EntityFlags.TRACEABLE,  false);
					physics.EnableGravity(false);
					physics.ChangeSimulationState(SimulationState.NONE);
					physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
					for (int i = 0; i <= physics.GetNumGeoms(); i++)
					{
						physics.SetGeomInteractionLayer(i, EPhysicsLayerDefs.CharNoCollide);
					}
					m_bIsHidden = true;
				};
			};
		};
	}
}
