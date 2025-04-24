class CRF_PlayableCharacterClass : ScriptComponentClass
{
}

class CRF_PlayableCharacter : ScriptComponent
{
	[Attribute()]
	protected string m_sName;
	[Attribute("0")]
	protected bool m_bIsPlayable;
	[Attribute()]
	protected bool m_bIsLeaderOrMedic;
	[Attribute()]
	protected bool m_bIsSpecialty;

	protected bool m_bIsSpectator = false;
	protected bool m_bIsHidden = false;
	protected bool m_bInitTime = false;

	//------------------------------------------------------------------------------------------------
	bool IsPlayable()
	{
		return m_bIsPlayable;
	}

	//------------------------------------------------------------------------------------------------
	string GetName()
	{
		return m_sName;
	}

	//------------------------------------------------------------------------------------------------
	bool IsLeader()
	{
		return m_bIsLeaderOrMedic;
	}

	//------------------------------------------------------------------------------------------------
	bool IsSpecialty()
	{
		return m_bIsSpecialty;
	}

	//------------------------------------------------------------------------------------------------
	void SetInitTime()
	{
		m_bInitTime = true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !CRF_Gamemode.GetInstance())
			return;

		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_GamemodeState.GAME && CRF_Gamemode.GetInstance().EnableAIInGameState && owner.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			m_bIsPlayable = false;

		GetGame().GetCallqueue().CallLater(SetInitTime, 5000, false);

		if (m_bIsPlayable)
		{
			GetGame().GetCallqueue().CallLater(SetInitialEntity, 500, false, owner);
			GetGame().GetCallqueue().CallLater(DisableAI, 0, false, owner);
		}

		if (owner.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			m_bIsSpectator = true;
			SetEventMask(owner, EntityEvent.FRAME);
		};
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

		#ifdef WORKBENCH
		if (!EntityUtils.IsPlayer(owner) && SCR_PossessingManagerComponent.GetInstance().GetIdFromMainEntity(owner) == 0 && m_bInitTime)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
			return;
		};
		#else
		if (!EntityUtils.IsPlayer(owner) && SCR_PossessingManagerComponent.GetInstance().GetIdFromMainEntity(owner) == 0 && RplSession.Mode() == RplMode.Dedicated && m_bInitTime)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
			return;
		};
		#endif
		
		CRF_PlayerControllerComponent playerControllerComp = CRF_PlayerControllerComponent.GetInstance();

		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetLocalControlledEntity() == owner)
		{
			if (playerControllerComp.m_eCamera && CRF_Gamemode.GetInstance().m_GamemodeState == CRF_GamemodeState.GAME)
			{
				vector mat[4];
				playerControllerComp.m_eCamera.GetTransform(mat);
				mat[3][1] = mat[3][1] - 1.5;
				playerControllerComp.UpdateEntityPos(mat);
				playerControllerComp.UpdateStoredCameraPos(mat);
			} else {
				vector mat[4];
				mat[1] = vector.Up;
				mat[2] = vector.Forward;
				mat[3][1] = 10000;
				playerControllerComp.UpdateEntityPos(mat);

				if (playerControllerComp.m_eCamera)
					playerControllerComp.m_eCamera.SetWorldTransform(mat);
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
		//Logs entity on server and disables AI
		#ifdef WORKBENCH
		SCR_AIGroup playableGroup = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
		if (playableGroup)
			CRF_Gamemode.GetInstance().AddPlayableEntity(owner);
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			SCR_AIGroup playableGroup = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
			if (playableGroup)
				CRF_Gamemode.GetInstance().AddPlayableEntity(owner);
		}
		#endif

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
