//------------------------------------------------------------------------------------
// CRF_BreachChargeAction: an attacker sets a breaching charge on a breach point of the objective chain
// (CRF_ObjectiveChainGamemodeComponent). Shown on a named breach point of the open objective to the
// attacking side; needs a breaching charge (satchel, M112 block, Bangalore) in the inventory, which the
// server takes when the charge is set. Runs on the player's machine and asks the server through
// COA_PlayerRplToAuthorityManager, which checks everything again.
//------------------------------------------------------------------------------------
class CRF_BreachChargeAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CRF_ObjectiveChainGamemodeComponent chain = CRF_ObjectiveChainGamemodeComponent.GetInstance();
		COA_PlayerRplToAuthorityManager relay = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!chain || !relay || !pOwnerEntity)
			return;

		int breachIndex = chain.GetBreachIndex(pOwnerEntity);
		if (breachIndex < 0)
			return;

		relay.RequestObjectiveChainBreach(breachIndex);
		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_ObjectiveChainGamemodeComponent chain = CRF_ObjectiveChainGamemodeComponent.GetInstance();
		if (!chain || !user || !chain.IsRoundRunning())
			return false;

		if (CRF_SpecialsHelper.GetLocalFactionKey() != chain.GetAttackerFactionKey())
			return false;

		return chain.CanBreachNow(chain.GetBreachIndex(GetOwner()));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		CRF_ObjectiveChainGamemodeComponent chain = CRF_ObjectiveChainGamemodeComponent.GetInstance();
		if (!chain)
			return false;

		if (!CRF_ObjectiveChainGamemodeComponent.FindBreachCharge(user, chain.GetBreachChargeKeywords()))
		{
			SetCannotPerformReason("Needs a satchel, M112 or Bangalore");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
