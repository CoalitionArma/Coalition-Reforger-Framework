class CRF_LoneWolfPenaltyManagerClass : ScriptComponentClass {}

class CRF_LoneWolfPenaltyManager : ScriptComponent
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

	[Attribute("1", UIWidgets.CheckBox, "Enable role whitelist for lone-wolf penalties", category: "CRF Player - Cohesion")]
	protected bool m_bEnableRoleWhitelist;

	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EGearRole), category: "CRF Player - Cohesion")]
	protected ref array<ref COA_EGearRole> m_aWhitelistedRoles;

	[Attribute("1", UIWidgets.CheckBox, "Enable faction whitelist for lone-wolf penalties", category: "CRF Player - Cohesion")]
	protected bool m_bEnableFactionWhitelist;

	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(COA_EFactions), desc: "Factions exempt from lone-wolf penalties", category: "CRF Player - Cohesion")]
	protected ref array<ref COA_EFactions> m_aWhitelistedFactions;

	[RplProp(condition: RplCondition.OwnerOnly, onRplName: "OnLoneWolfPenaltyReplicated")]
	protected float m_fOwnerTunnelVisionIntensity;

	protected float m_fPenaltyIntensity;
	protected float m_fLastAppliedAimPenalty;
	protected float m_fLastReplicatedTunnelVision = -1.0;
	protected float m_fCohesionAccumulator;
	protected bool m_bCachedPenaltyActive;
	protected bool m_bHasCachedPenalty;
	protected vector m_vLastPosition;
	protected IEntity m_eLastPenaltyTarget;
	protected int m_iLastGroupId = -1;
	protected bool m_bHasLastGroupId;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_fPenaltyIntensity = 0;
		m_fLastAppliedAimPenalty = 0;
		m_bCachedPenaltyActive = false;
		m_bHasCachedPenalty = false;
		m_fCohesionAccumulator = 0;
		m_fLastReplicatedTunnelVision = -1.0;
		m_vLastPosition = vector.Zero;
		m_iLastGroupId = -1;
		m_bHasLastGroupId = false;

		if (!SCR_Global.IsEditMode())
			SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!Replication.IsServer())
		{
			UpdateLocalPenaltyTarget();
			return;
		}

		IEntity character = GetControlledCharacter(owner);
		if (!character)
		{
			UpdatePenaltyState(0.0, timeSlice);
			return;
		}

		if (UpdateCachedGroup(character))
		{
			ResetPenaltyState();
			return;
		}

		vector currentPos = character.GetOrigin();
		if (m_vLastPosition == vector.Zero)
			m_vLastPosition = currentPos;

		if (m_bHasCachedPenalty && vector.DistanceSq(currentPos, m_vLastPosition) > 25.0)
		{
			m_bHasCachedPenalty = false;
			m_fCohesionAccumulator = 0;
		}
		m_vLastPosition = currentPos;

		if (m_bDisableLoneWolfPenalty || !m_bEnableCohesionPenalties)
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
			m_bCachedPenaltyActive = ShouldApplyPenalty(character);
			m_bHasCachedPenalty = true;
		}

		if (m_bCachedPenaltyActive)
			targetPenalty = 1.0;
		else
			targetPenalty = 0.0;
		UpdatePenaltyState(targetPenalty, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity GetControlledCharacter(IEntity owner)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(owner);
		if (!playerController)
			return null;

		return playerController.GetControlledEntity();
	}

	//------------------------------------------------------------------------------------------------
	protected bool UpdateCachedGroup(IEntity character)
	{
		int currentGroupId = GetCurrentGroupId(character);
		if (!m_bHasLastGroupId)
		{
			m_iLastGroupId = currentGroupId;
			m_bHasLastGroupId = true;
			return false;
		}

		if (m_iLastGroupId == currentGroupId)
			return false;

		m_iLastGroupId = currentGroupId;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetCurrentGroupId(IEntity character)
	{
		if (!GetGame())
			return -1;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return -1;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return -1;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return -1;

		SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(playerId);
		if (!playerGroup)
			return -1;

		return playerGroup.GetID();
	}

	//------------------------------------------------------------------------------------------------
	protected bool ShouldApplyPenalty(IEntity character)
	{
		if (!GetGame() || !GetGame().InPlayMode())
			return false;

		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		if (safestartManager && safestartManager.GetSafestartStatus())
			return false;

		if (COA_EntityHelper.IsSpectator(character))
			return false;

		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
		if (!dmg || dmg.GetState() == EDamageState.DESTROYED)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return false;

		if (IsRoleWhitelisted(character))
			return false;

		if (IsFactionWhitelisted(playerId))
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

			if (COA_EntityHelper.IsSpectator(otherEntity))
				continue;

			SCR_DamageManagerComponent otherDmg = SCR_DamageManagerComponent.Cast(otherEntity.FindComponent(SCR_DamageManagerComponent));
			if (!otherDmg || otherDmg.GetState() == EDamageState.DESTROYED)
				continue;

			if (vector.DistanceSq(character.GetOrigin(), otherEntity.GetOrigin()) <= maxDistSq)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetPenaltyState()
	{
		m_fPenaltyIntensity = 0;
		m_fOwnerTunnelVisionIntensity = 0;
		m_bCachedPenaltyActive = false;
		m_bHasCachedPenalty = false;
		m_fCohesionAccumulator = 0;
		m_vLastPosition = vector.Zero;

		if (Math.AbsFloat(m_fLastReplicatedTunnelVision) >= 0.001)
		{
			m_fLastReplicatedTunnelVision = 0;
			Replication.BumpMe();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsRoleWhitelisted(IEntity character)
	{
		if (!m_bEnableRoleWhitelist)
			return false;

		if (!m_aWhitelistedRoles || m_aWhitelistedRoles.IsEmpty())
			return COA_RoleHelper.IsMedicRole(character);

		ResourceName prefab = character.GetPrefabData().GetPrefabName();
		if (!COA_RoleHelper.IsValidGearscriptResource(prefab))
			return false;

		COA_EGearRole role = COA_RoleHelper.ResourceToRole(prefab);
		foreach (COA_EGearRole whitelistedRole : m_aWhitelistedRoles)
		{
			if (role == whitelistedRole)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsFactionWhitelisted(int playerId)
	{
		if (!m_bEnableFactionWhitelist)
			return false;

		if (!m_aWhitelistedFactions || m_aWhitelistedFactions.IsEmpty())
			return false;

		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
		if (!playerFaction)
			return false;

		FactionKey playerFactionKey = playerFaction.GetFactionKey();

		foreach (COA_EFactions whitelistedFaction : m_aWhitelistedFactions)
		{
			FactionKey whitelistedKey;
			switch (whitelistedFaction)
			{
				case COA_EFactions.BLUFOR:  whitelistedKey = "BLUFOR"; break;
				case COA_EFactions.OPFOR:   whitelistedKey = "OPFOR";  break;
				case COA_EFactions.INDFOR:  whitelistedKey = "INDFOR"; break;
				case COA_EFactions.CIV:     whitelistedKey = "CIV";    break;
			}

			if (playerFactionKey == whitelistedKey)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
	protected void OnLoneWolfPenaltyReplicated()
	{
		ApplyPenalties();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyPenalties()
	{
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		if (character != m_eLastPenaltyTarget)
		{
			m_eLastPenaltyTarget = character;
			m_fLastAppliedAimPenalty = 0;
		}

		SCR_DamageManagerComponent dmg = GetDamageManager(character);
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

	//------------------------------------------------------------------------------------------------
	protected void UpdateLocalPenaltyTarget()
	{
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		if (character == m_eLastPenaltyTarget)
			return;

		ApplyPenalties();
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_DamageManagerComponent GetDamageManager(IEntity character)
	{
		if (!character)
			return null;

		return SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
	}

	//------------------------------------------------------------------------------------------------
	float GetLoneWolfExtraStaminaDrainMultiplier()
	{
		return Math.Max(m_fMaxExtraStaminaDrainMultiplier, 0) * m_fOwnerTunnelVisionIntensity;
	}

	//------------------------------------------------------------------------------------------------
	float GetLoneWolfPenaltyIntensity()
	{
		return m_fOwnerTunnelVisionIntensity;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_LoneWolfPenaltyManager GetLocalInstance()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return null;

		return CRF_LoneWolfPenaltyManager.Cast(playerController.FindComponent(CRF_LoneWolfPenaltyManager));
	}

	//------------------------------------------------------------------------------------------------
	static CRF_LoneWolfPenaltyManager GetForCharacter(IEntity character)
	{
		if (!character || !GetGame())
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;

		int playerId = playerManager.GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return null;

		PlayerController playerController = playerManager.GetPlayerController(playerId);
		if (!playerController)
			return null;

		return CRF_LoneWolfPenaltyManager.Cast(playerController.FindComponent(CRF_LoneWolfPenaltyManager));
	}

	//------------------------------------------------------------------------------------------------
	static float GetLocalTunnelVisionIntensity()
	{
		CRF_LoneWolfPenaltyManager manager = GetLocalInstance();
		if (!manager)
			return 0;

		return manager.GetLoneWolfPenaltyIntensity();
	}
}
