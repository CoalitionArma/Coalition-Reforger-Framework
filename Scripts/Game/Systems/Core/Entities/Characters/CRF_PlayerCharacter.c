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

	[Attribute("0.10", UIWidgets.EditBox, "Penalty ramp-up speed per second", category: "CRF Player - Cohesion")]
	protected float m_fPenaltyRampUpRate;

	[Attribute("0.22", UIWidgets.EditBox, "Penalty recovery speed per second", category: "CRF Player - Cohesion")]
	protected float m_fPenaltyRecoveryRate;

	[Attribute("0.45", UIWidgets.EditBox, "Max additive aiming damage penalty", category: "CRF Player - Cohesion")]
	protected float m_fMaxAimingDamagePenalty;

	[Attribute("1.00", UIWidgets.EditBox, "Max extra stamina drain multiplier", category: "CRF Player - Cohesion")]
	protected float m_fMaxExtraStaminaDrainMultiplier;

	[Attribute("2.00", UIWidgets.EditBox, "Seconds between cohesion checks", category: "CRF Player - Cohesion")]
	protected float m_fCohesionCheckInterval;

	[RplProp(condition: RplCondition.OwnerOnly)]
	protected float m_fOwnerTunnelVisionIntensity;

	protected float m_fPenaltyIntensity;
	protected float m_fLastAppliedAimPenalty;
	protected float m_fCohesionAccumulator;
	protected float m_fLastReplicatedTunnelVision = -1.0;
	protected bool m_bCachedPenaltyConditionActive;
	protected bool m_bHasCachedPenaltyCondition;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 DISABLE AI METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (SCR_Global.IsEditMode())
			return;

		SetEventMask(EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!Replication.IsServer())
			return;

		if (m_bDisableLoneWolfPenalty)
		{
			m_bHasCachedPenaltyCondition = false;
			m_bCachedPenaltyConditionActive = false;
			m_fCohesionAccumulator = 0;
			UpdatePenaltyState(0.0, timeSlice);
			return;
		}

		if (!m_bEnableCohesionPenalties)
		{
			m_bHasCachedPenaltyCondition = false;
			m_bCachedPenaltyConditionActive = false;
			m_fCohesionAccumulator = 0;
			UpdatePenaltyState(0.0, timeSlice);
			return;
		}

		float cohesionCheckInterval = Math.Max(m_fCohesionCheckInterval, 0.1);
		m_fCohesionAccumulator += timeSlice;
		if (!m_bHasCachedPenaltyCondition || m_fCohesionAccumulator >= cohesionCheckInterval)
		{
			m_bCachedPenaltyConditionActive = ShouldApplyPenalty();
			m_bHasCachedPenaltyCondition = true;
			m_fCohesionAccumulator = 0;
		}

		float targetPenalty;
		if (m_bCachedPenaltyConditionActive)
			targetPenalty = 1.0;
		else
			targetPenalty = 0.0;
		UpdatePenaltyState(targetPenalty, timeSlice);
	}

	protected bool ShouldApplyPenalty()
	{
		if (m_bDisableLoneWolfPenalty)
			return false;

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

		FactionKey myFaction = CRF_EntityHelper.DetermineFactionKey(this);
		float maxDistSq = Math.Pow(Math.Max(m_fFriendlyProximityMeters, 1.0), 2);

		array<int> allPlayers = {};
		playerManager.GetPlayers(allPlayers);

		foreach (int otherId : allPlayers)
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

			if (CRF_EntityHelper.DetermineFactionKey(otherEntity) != myFaction)
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
		float nextAppliedPenalty = m_fPenaltyIntensity * Math.Max(m_fMaxAimingDamagePenalty, 0);
		float targetAimDamage = baselineAimDamage + nextAppliedPenalty;

		if (Math.AbsFloat(targetAimDamage - currentAimDamage) >= 0.001)
			dmg.SetAimingDamage(targetAimDamage);

		m_fLastAppliedAimPenalty = nextAppliedPenalty;
	}

	float GetLoneWolfExtraStaminaDrainMultiplier()
	{
		return Math.Max(m_fMaxExtraStaminaDrainMultiplier, 0) * m_fPenaltyIntensity;
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