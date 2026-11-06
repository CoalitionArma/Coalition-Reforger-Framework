//------------------------------------------------------------------------------------------------
//! Client-side half of the tree/bush chopping rework: scales the active swing to an exact target
//! duration (not a multiplier of some unknown baseline - GetActionDuration() is native and not
//! script-overridable, so instead the progress delta is scaled by actionDuration/targetS each
//! frame, which makes real wall-clock completion time land on targetS regardless of whatever the
//! prefab's native duration happens to be) and greys out the action while on cooldown.
//!
//! ACE Core 1.5.41 replaced ACE Chopping's own ACE_Chopping_UserAction with this generic action,
//! which ACE_Chopping_TreeHelper/BushHelper now carry. Other ACE helpers can use it too, so all of
//! this only applies when the helper's entity is a tree or bush (Tree, as in the server gate).
//!
//! This action's PerformAction/CanBePerformedScript only ever run locally on the client that
//! invoked it (see ACE_DestroyEntityUserAction.HasLocalEffectOnlyScript - it never runs server-side),
//! so what's here is prediction only, purely for responsive UI. Real, unbypassable enforcement of
//! the cooldown is in the modded SCR_PlayerController.ACE_IsDeletionGrantedByServer, since that's
//! the actual server RPC gate the tree-deletion request goes through.
//! See CRF_SCR_CampaignBuildingGadgetToolComponent for the tunable durations/multipliers.
modded class ACE_DestroyEntityUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!super.CanBePerformedScript(user))
			return false;

		if (!CRF_IsChopping())
			return true;

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
		if (actionDuration <= 0 || !m_pUserChar || !CRF_IsChopping())
			return fProgress + timeSlice;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(m_pUserChar);
		float targetS = actionDuration;
		if (gadgetComp)
			targetS = gadgetComp.CRF_GetChoppingActiveDurationS(COA_RoleHelper.IsCombatEngineerRole(m_pUserChar));

		if (targetS <= 0)
			targetS = actionDuration;

		return fProgress + timeSlice * (actionDuration / targetS);
	}

	//------------------------------------------------------------------------------------------------
	//! Predict the cooldown locally right after the (locally-run) removal request fires.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Before super, which deletes the helper this reads
		bool chopping = CRF_IsChopping();

		super.PerformAction(pOwnerEntity, pUserEntity);

		if (!chopping || !pUserEntity)
			return;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(pUserEntity);
		if (gadgetComp)
			gadgetComp.CRF_ConsumeChop(COA_RoleHelper.IsCombatEngineerRole(pUserEntity));
	}

	//------------------------------------------------------------------------------------------------
	//! A tree or bush, the only things ACE Chopping's helpers are put on
	protected bool CRF_IsChopping()
	{
		ACE_ActionsHelperEntity helper = ACE_ActionsHelperEntity.Cast(GetOwner());
		if (!helper)
			return false;

		return Tree.Cast(helper.GetAssociatedEntity()) != null;
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
