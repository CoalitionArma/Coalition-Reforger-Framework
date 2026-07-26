//------------------------------------------------------------------------------------------------
//! Real, server-authoritative enforcement of the chopping cooldown from
//! CRF_SCR_CampaignBuildingGadgetToolComponent. ACE_Chopping_UserAction's own cooldown check is
//! client-side prediction only (see CRF_ACE_Chopping_UserAction) - it never runs on the server.
//! Tree/bush deletion actually happens via RpcAsk_ACE_DestroyEntity / RpcAsk_ACE_DeleteEntityByID
//! (in ACE-Core's SCR_PlayerController), both gated through this single ACE_IsDeletionGrantedByServer
//! check, which is the one place a modified client can't bypass the rate limit.
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override protected bool ACE_IsDeletionGrantedByServer(IEntity entityToDelete)
	{
		if (!super.ACE_IsDeletionGrantedByServer(entityToDelete))
			return false;

		// Only rate-limit chopping (trees/bushes) - leave other ACE deletion requests untouched
		if (!Tree.Cast(entityToDelete))
			return true;

		SCR_ChimeraCharacter playerChar = SCR_ChimeraCharacter.Cast(GetControlledEntity());
		if (!playerChar)
			return false;

		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(playerChar);
		if (!gadgetManager)
			return true;

		SCR_CampaignBuildingGadgetToolComponent gadgetComp = SCR_CampaignBuildingGadgetToolComponent.Cast(gadgetManager.GetHeldGadgetComponent());
		if (!gadgetComp)
			return true;

		if (!gadgetComp.CRF_CanChop())
			return false;

		gadgetComp.CRF_ConsumeChop(COA_RoleHelper.IsCombatEngineerRole(playerChar));
		return true;
	}
}
