class CRF_PlayableCharacterClass: ScriptComponentClass
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
	protected SCR_PlayerController m_PlayerController;
	protected bool m_bInitTime = false;
	
	protected float m_bTimeSliceLimit = 0;
	
	bool IsPlayable()
	{
		return m_bIsPlayable;
	}
	
	string GetName()
	{
		return m_sName;
	}
	
	bool IsLeader()
	{
		return m_bIsLeaderOrMedic;
	}
	
	bool IsSpecialty()
	{
		return m_bIsSpecialty;
	}
	
	void SetInitTime()
	{
		m_bInitTime = true;
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if (!GetGame().InPlayMode())
			return;
		
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_GamemodeState.GAME && CRF_Gamemode.GetInstance().EnableAIInGameState && owner.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			m_bIsPlayable = false;
		
		GetGame().GetCallqueue().CallLater(SetInitTime, 5000, false);

		if (m_bIsPlayable)
		{
			GetGame().GetCallqueue().CallLater(SetInitialEntity, 500, false, owner);
			GetGame().GetCallqueue().CallLater(DisableAI, 0, false, owner);
		}
		
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		if (owner.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			m_bIsSpectator = true;	
		
		SetEventMask(owner, EntityEvent.POSTFIXEDFRAME);
	}
	
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeslice);
		
		if (!owner)
			return;
		
		if (!GetGame().InPlayMode())
			return;
		
		if (!m_bIsPlayable && !m_bIsSpectator)
			return;
		
		#ifdef WORKBENCH
		if (!EntityUtils.IsPlayer(owner) && m_bInitTime)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
		}
		#else
		if (!EntityUtils.IsPlayer(owner) && RplSession.Mode() == RplMode.Dedicated && m_bInitTime)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
		}
		#endif
		
		if (m_PlayerController.GetLocalControlledEntity() == owner)	{
			if (m_PlayerController.m_eCamera) {
				vector mat[4];
				m_PlayerController.m_eCamera.GetTransform(mat);
				mat[3][1] = mat[3][1] - 1.5;
				owner.SetOrigin(mat);
			}
			else {
				owner.SetOrigin("0 10000 0");
			}
		}
		Physics physics = owner.GetPhysics();
		if (physics) {
			physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
			physics.EnableGravity(false);
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
			physics.SetMass(0);
			physics.SetDamping(1, 1);
			physics.SetActive(ActiveState.INACTIVE);
		}	
	}
	
	void DisableAI(IEntity owner)
	{
		if (AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent())
			AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent().DeactivateAI();
		GetGame().GetCallqueue().CallLater(DisableAIWrap, 0, false, owner)
	}
	
	void DisableAIWrap(IEntity owner)
	{
		if (AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent())
			AIControlComponent.Cast(owner.FindComponent(AIControlComponent)).GetAIAgent().DeactivateAI();
	}
	
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
		if (owner.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			owner.GetPhysics().EnableGravity(false);
			owner.SetOrigin("0 10000 0");
			Physics physics = owner.GetPhysics();
			if (physics)
			{
				physics.SetVelocity(vector.Zero);
				physics.SetAngularVelocity(vector.Zero);
				physics.SetMass(0);
				physics.SetDamping(1, 1);
				physics.SetActive(ActiveState.INACTIVE);
			}
		}	
	}
}