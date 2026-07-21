//------------------------------------------------------------------------------------------------
//! Enforces a cooldown between removing objects (e.g. ACE trench/sandbag compositions) with this
//! E-tool via SCR_CampaignBuildingDisassemblyUserAction. Uses WorldTimestamp (a synchronized
//! clock, not System.GetTickCount which is per-machine and meaningless once compared across
//! machines) so each machine can independently compute the same cooldown window from "now" -
//! this field is deliberately NOT replicated. The server enforces the cooldown authoritatively
//! from its own local copy (CanBePerformedScript is re-validated server-side, same as vanilla's
//! own ENEMY_PRESENCE check in this class), and the initiating client predicts its own copy
//! locally (see CRF_SCR_CampaignBuildingDisassemblyUserAction.PerformAction) purely for instant
//! UI feedback - avoids syncing a property for every single removal across every player.
modded class SCR_CampaignBuildingGadgetToolComponent
{
	[Attribute(defvalue: "60", desc: "Cooldown in seconds between removing/disassembling objects with this E-tool. 0 = no cooldown")]
	protected float m_fCRF_RemovalCooldownS;

	protected WorldTimestamp m_CRF_RemovalAvailableSince;

	//------------------------------------------------------------------------------------------------
	//! \return true if this tool's removal cooldown has elapsed
	bool CRF_CanRemoveObject()
	{
		if (m_fCRF_RemovalCooldownS <= 0)
			return true;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return true;

		return !m_CRF_RemovalAvailableSince.Greater(world.GetServerTimestamp());
	}

	//------------------------------------------------------------------------------------------------
	//! Start the cooldown. Called once on the authoritative machine when a composition has
	//! actually been removed, and once on the initiating client to predict the same window
	//! locally (see CRF_SCR_CampaignBuildingDisassemblyUserAction.PerformAction).
	void CRF_ConsumeRemoval()
	{
		if (m_fCRF_RemovalCooldownS <= 0)
			return;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return;

		m_CRF_RemovalAvailableSince = world.GetServerTimestamp().PlusSeconds(m_fCRF_RemovalCooldownS);
	}

	//------------------------------------------------------------------------------------------------
	//! \return seconds remaining before this tool can remove another object, or 0 if ready
	float CRF_GetRemovalCooldownRemaining()
	{
		if (m_fCRF_RemovalCooldownS <= 0)
			return 0;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return 0;

		return Math.Max(0, m_CRF_RemovalAvailableSince.DiffSeconds(world.GetServerTimestamp()));
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHOPPING (trees/bushes)
//=============================================================================================================================================================================================================================================================================================================================================================

	[Attribute(defvalue: "15", desc: "Seconds a non-engineer's chop swing takes to remove a tree/bush")]
	protected float m_fCRF_ChoppingActiveDurationS;

	[Attribute(defvalue: "60", desc: "Full cycle in seconds (chop swing + cooldown) between tree/bush removals for a non-engineer")]
	protected float m_fCRF_ChoppingCycleDurationS;

	[Attribute(defvalue: "2", desc: "How many times faster a combat engineer's chop swing is than everyone else")]
	protected float m_fCRF_EngineerChoppingSpeedMultiplier;

	[Attribute(defvalue: "3", desc: "How many times higher a combat engineer's overall tree/bush removal throughput is than everyone else")]
	protected float m_fCRF_EngineerChoppingThroughputMultiplier;

	protected WorldTimestamp m_CRF_ChoppingAvailableSince;

	//------------------------------------------------------------------------------------------------
	//! \return seconds the active chop swing should take for this player
	float CRF_GetChoppingActiveDurationS(bool isEngineer)
	{
		if (isEngineer && m_fCRF_EngineerChoppingSpeedMultiplier > 0)
			return m_fCRF_ChoppingActiveDurationS / m_fCRF_EngineerChoppingSpeedMultiplier;

		return m_fCRF_ChoppingActiveDurationS;
	}

	//------------------------------------------------------------------------------------------------
	//! \return true if this tool's chopping cooldown has elapsed
	bool CRF_CanChop()
	{
		if (m_fCRF_ChoppingCycleDurationS <= 0)
			return true;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return true;

		return !m_CRF_ChoppingAvailableSince.Greater(world.GetServerTimestamp());
	}

	//------------------------------------------------------------------------------------------------
	//! Start the post-chop cooldown. cycleDuration already includes the active swing time, so only
	//! the remainder (cycle - active) is waited out here, since the swing itself already elapsed
	//! by the time this is called.
	void CRF_ConsumeChop(bool isEngineer)
	{
		if (m_fCRF_ChoppingCycleDurationS <= 0)
			return;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return;

		float cycleS = m_fCRF_ChoppingCycleDurationS;
		if (isEngineer && m_fCRF_EngineerChoppingThroughputMultiplier > 0)
			cycleS = m_fCRF_ChoppingCycleDurationS / m_fCRF_EngineerChoppingThroughputMultiplier;

		float cooldownS = Math.Max(0, cycleS - CRF_GetChoppingActiveDurationS(isEngineer));
		m_CRF_ChoppingAvailableSince = world.GetServerTimestamp().PlusSeconds(cooldownS);
	}
}
