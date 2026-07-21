//------------------------------------------------------------------------------------------------
//! Client-side half of the tree/bush chopping rework: scales the active swing to an exact target
//! duration (not a multiplier of some unknown baseline - GetActionDuration() is native and not
//! script-overridable, so instead the progress delta is scaled by actionDuration/targetS each
//! frame, which makes real wall-clock completion time land on targetS regardless of whatever the
//! prefab's native duration happens to be) and greys out the action while on cooldown.
//!
//! This action's PerformAction/CanBePerformedScript only ever run locally on the client that
//! invoked it (see ACE_Chopping_UserAction.HasLocalEffectOnlyScript - it never runs server-side),
//! so what's here is prediction only, purely for responsive UI. Real, unbypassable enforcement of
//! the cooldown is in the modded SCR_PlayerController.ACE_IsDeletionGrantedByServer, since that's
//! the actual server RPC gate the tree-deletion request goes through.
//! See CRF_SCR_CampaignBuildingGadgetToolComponent for the tunable durations/multipliers.
modded class ACE_Chopping_UserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!super.CanBePerformedScript(user))
			return false;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(user);
		if (gadgetComp && !gadgetComp.CRF_CanChop())
		{
			SetCannotPerformReason("#AR-UserActionUnavailable");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override float GetActionProgressScript(float fProgress, float timeSlice)
	{
		float actionDuration = GetActionDuration();
		if (actionDuration <= 0 || !m_pUserChar)
			return fProgress + timeSlice;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(m_pUserChar);
		float targetS = actionDuration;
		if (gadgetComp)
			targetS = gadgetComp.CRF_GetChoppingActiveDurationS(CRF_RoleHelper.IsCombatEngineerRole(m_pUserChar));

		if (targetS <= 0)
			targetS = actionDuration;

		return fProgress + timeSlice * (actionDuration / targetS);
	}

	//------------------------------------------------------------------------------------------------
	//! Predict the cooldown locally right after the (locally-run) removal request fires.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(pUserEntity);
		if (gadgetComp)
			gadgetComp.CRF_ConsumeChop(CRF_RoleHelper.IsCombatEngineerRole(pUserEntity));
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_CampaignBuildingGadgetToolComponent CRF_GetHeldBuildingTool(notnull IEntity user)
	{
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(user);
		if (!gadgetManager)
			return null;

		return SCR_CampaignBuildingGadgetToolComponent.Cast(gadgetManager.GetHeldGadgetComponent());
	}
}
