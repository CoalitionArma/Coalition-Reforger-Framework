class CRF_PlayerCharacterClass : SCR_ChimeraCharacterClass
{
}

class CRF_PlayerCharacter : SCR_ChimeraCharacter
{
	[Attribute("1", UIWidgets.CheckBox, "Enable anti lone-wolf cohesion penalties", category: "CRF Player - Cohesion")]
	protected bool m_bEnableCohesionPenalties;

	[Attribute("0", UIWidgets.CheckBox, "Disable lone-wolf penalties", category: "CRF Player - Cohesion")]
	protected bool m_bDisableLoneWolfPenalty;

	[Attribute("4", UIWidgets.EditBox, "Minimum group size required before penalties apply", category: "CRF Player - Cohesion")]
	protected int m_iPenaltyGroupSizeThreshold;

	[Attribute("100", UIWidgets.EditBox, "Required distance to at least one friendly player (meters)", category: "CRF Player - Cohesion")]
	protected float m_fFriendlyProximityMeters;

	[Attribute("0.25", UIWidgets.EditBox, "Penalty ramp-up speed per second", category: "CRF Player - Cohesion")]
	protected float m_fPenaltyRampUpRate;

	[Attribute("0.22", UIWidgets.EditBox, "Penalty recovery speed per second", category: "CRF Player - Cohesion")]
	protected float m_fPenaltyRecoveryRate;

	[Attribute("4.0", UIWidgets.EditBox, "Max additive aiming damage penalty", category: "CRF Player - Cohesion")]
	protected float m_fMaxAimingDamagePenalty;

	[Attribute("5.00", UIWidgets.EditBox, "Max extra stamina drain multiplier", category: "CRF Player - Cohesion")]
	protected float m_fMaxExtraStaminaDrainMultiplier;

	[Attribute("0.5", UIWidgets.EditBox, "How often (seconds) the proximity check runs. Lower = more responsive, higher = better performance.", category: "CRF Player - Cohesion")]
	protected float m_fCohesionCheckInterval;

	[RplProp(condition: RplCondition.OwnerOnly, onRplName: "OnLoneWolfPenaltyReplicated")]
	protected float m_fOwnerTunnelVisionIntensity;

	protected float m_fPenaltyIntensity;
	protected float m_fLastAppliedAimPenalty;
	protected float m_fLastReplicatedTunnelVision = -1.0;
	protected float m_fCohesionAccumulator;
	protected bool m_bCachedPenaltyActive;
	protected bool m_bHasCachedPenalty;
	protected vector m_vLastPosition;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 DISABLE AI METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (SCR_Global.IsEditMode())
			return;

		m_fPenaltyIntensity = 0;
		m_fLastAppliedAimPenalty = 0;
		m_bCachedPenaltyActive = false;
		m_bHasCachedPenalty = false;
		m_fCohesionAccumulator = 0;
		m_fLastReplicatedTunnelVision = -1.0;
		m_vLastPosition = GetOrigin();

		SetEventMask(EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!Replication.IsServer())
			return;

		// Detect teleports: a position jump larger than 5 m in one frame invalidates the proximity cache
		vector currentPos = GetOrigin();
		if (m_bHasCachedPenalty && vector.DistanceSq(currentPos, m_vLastPosition) > 25.0)
		{
			m_bHasCachedPenalty = false;
			m_fCohesionAccumulator = 0;
		}
		m_vLastPosition = currentPos;

		if (m_bDisableLoneWolfPenalty)
		{
			UpdatePenaltyState(0.0, timeSlice);
			return;
		}

		if (!m_bEnableCohesionPenalties)
		{
			UpdatePenaltyState(0.0, timeSlice);
			return;
		}

		float targetPenalty;
		float interval = Math.Max(m_fCohesionCheckInterval, 0.1);
		m_fCohesionAccumulator += timeSlice;
		if (!m_bHasCachedPenalty || m_fCohesionAccumulator >= interval)
		{
			m_fCohesionAccumulator = 0;
			m_bCachedPenaltyActive = ShouldApplyPenalty();
			m_bHasCachedPenalty = true;
		}

		if (m_bCachedPenaltyActive)
			targetPenalty = 1.0;
		else
			targetPenalty = 0.0;
		UpdatePenaltyState(targetPenalty, timeSlice);
	}

	protected bool ShouldApplyPenalty()
	{
		if (!GetGame() || !GetGame().InPlayMode())
			return false;

		CRF_SafestartManager safestartManager = CRF_SafestartManager.GetInstance();
		if (safestartManager && safestartManager.GetSafestartStatus())
			return false;

		if (CRF_EntityHelper.IsSpectator(this))
			return false;

		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(GetDamageManager());
		if (!dmg || dmg.GetState() == EDamageState.DESTROYED)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(this);
		if (playerId <= 0)
			return false;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return false;

		SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(playerId);
		if (!playerGroup)
			return false;

		array<int> groupMembers = playerGroup.GetPlayerIDs();
		if (!groupMembers || groupMembers.Count() < Math.Max(m_iPenaltyGroupSizeThreshold, 1))
			return false;

		float maxDistSq = Math.Pow(Math.Max(m_fFriendlyProximityMeters, 1.0), 2);

		foreach (int otherId : groupMembers)
		{
			if (otherId == playerId)
				continue;

			IEntity otherEntity = playerManager.GetPlayerControlledEntity(otherId);
			if (!otherEntity)
				continue;

			if (CRF_EntityHelper.IsSpectator(otherEntity))
				continue;

			SCR_DamageManagerComponent otherDmg = SCR_DamageManagerComponent.Cast(otherEntity.FindComponent(SCR_DamageManagerComponent));
			if (!otherDmg || otherDmg.GetState() == EDamageState.DESTROYED)
				continue;

			if (vector.DistanceSq(GetOrigin(), otherEntity.GetOrigin()) <= maxDistSq)
				return false;
		}

		return true;
	}

	protected void UpdatePenaltyState(float targetPenalty, float timeSlice)
	{
		if (targetPenalty > m_fPenaltyIntensity)
			m_fPenaltyIntensity = Math.Clamp(m_fPenaltyIntensity + m_fPenaltyRampUpRate * timeSlice, 0, 1);
		else
			m_fPenaltyIntensity = Math.Clamp(m_fPenaltyIntensity - m_fPenaltyRecoveryRate * timeSlice, 0, 1);

		m_fOwnerTunnelVisionIntensity = m_fPenaltyIntensity;
		if (Math.AbsFloat(m_fOwnerTunnelVisionIntensity - m_fLastReplicatedTunnelVision) >= 0.02)
		{
				m_fLastReplicatedTunnelVision = m_fOwnerTunnelVisionIntensity;
			Replication.BumpMe();
		}
	}

	protected void OnLoneWolfPenaltyReplicated()
	{
		ApplyPenalties();
	}

	protected void ApplyPenalties()
	{
		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(GetDamageManager());
		if (!dmg)
		{
			m_fLastAppliedAimPenalty = 0;
			return;
		}

		float currentAimDamage = dmg.GetAimingDamage();
		float baselineAimDamage = Math.Max(currentAimDamage - m_fLastAppliedAimPenalty, 0);
		float nextAppliedPenalty = m_fOwnerTunnelVisionIntensity * Math.Max(m_fMaxAimingDamagePenalty, 0);
		float targetAimDamage = baselineAimDamage + nextAppliedPenalty;

		if (Math.AbsFloat(targetAimDamage - currentAimDamage) >= 0.001)
			dmg.SetAimingDamage(targetAimDamage);

		m_fLastAppliedAimPenalty = nextAppliedPenalty;
	}

	float GetLoneWolfExtraStaminaDrainMultiplier()
	{
		return Math.Max(m_fMaxExtraStaminaDrainMultiplier, 0) * m_fOwnerTunnelVisionIntensity;
	}

	float GetLoneWolfPenaltyIntensity()
	{
		return m_fOwnerTunnelVisionIntensity;
	}

	static float GetLocalTunnelVisionIntensity()
	{
		CRF_PlayerCharacter localCharacter = CRF_PlayerCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!localCharacter)
			return 0;

		return localCharacter.m_fOwnerTunnelVisionIntensity;
	}
	
	//------------------------------------------------------------------------------------------------
	void DisableAI()
	{
		AIControlComponent aiComponent = AIControlComponent.Cast(this.FindComponent(AIControlComponent));
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (!agent)
			return;
		
		agent.DeactivateAI();
		
		// Double-check deactivation next frame
		GetGame().GetCallqueue().Call(DisableAIWrap, aiComponent);
	}

	//------------------------------------------------------------------------------------------------
	protected void DisableAIWrap(AIControlComponent aiComponent)
	{
		if (!aiComponent)
			return;
		
		AIAgent agent = aiComponent.GetAIAgent();
		if (agent)
			agent.DeactivateAI();
	}
}