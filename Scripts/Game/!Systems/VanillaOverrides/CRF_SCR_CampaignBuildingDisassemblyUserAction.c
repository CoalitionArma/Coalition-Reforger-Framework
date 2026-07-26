//------------------------------------------------------------------------------------------------
//! Enforces the removal cooldown added in CRF_SCR_CampaignBuildingGadgetToolComponent: blocks the
//! disassemble/remove action while the held E-tool is still on cooldown, and starts the cooldown
//! each time an object is actually removed.
modded class SCR_CampaignBuildingDisassemblyUserAction
{
	protected const string CRF_REMOVAL_ON_COOLDOWN_REASON = "Prop removal is on cooldown";

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!super.CanBePerformedScript(user))
			return false;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(user);
		if (gadgetComp && !gadgetComp.CRF_CanRemoveObject())
		{
			SetCannotPerformReason(CRF_REMOVAL_ON_COOLDOWN_REASON);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla only reaches this once every one of its own guards passed (proxy/authority check,
	//! character alive, distance, HQ base rules) - i.e. exactly when a removal actually happens.
	//! Only runs on the authoritative machine (see PerformAction's proxy check below).
	override protected void DeleteComposition(notnull IEntity composition, notnull SCR_ChimeraCharacter character)
	{
		super.DeleteComposition(composition, character);

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(character);
		if (gadgetComp)
			gadgetComp.CRF_ConsumeRemoval();
	}

	//------------------------------------------------------------------------------------------------
	//! DeleteComposition (above) only runs on the authoritative machine, so a remote client would
	//! otherwise never see its own tool's cooldown start - it'd only find out the hard way when a
	//! later attempt gets rejected. Predict the same cooldown window locally instead of replicating
	//! it: skipped on dedicated server / listen-host (m_RplComponent not a proxy there - the real
	//! DeleteComposition call above already set it), and only for the player's own action.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		if (!m_RplComponent || !m_RplComponent.IsProxy())
			return;

		if (pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = CRF_GetHeldBuildingTool(pUserEntity);
		if (gadgetComp)
			gadgetComp.CRF_ConsumeRemoval();
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
