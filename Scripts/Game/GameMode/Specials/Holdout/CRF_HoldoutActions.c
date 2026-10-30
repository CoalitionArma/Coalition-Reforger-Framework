//------------------------------------------------------------------------------------------------
//! Holdout: put a board back on a barricade (CRF_HoldoutBarricadeComponent). Needs an entrenching tool
//! (any building tool gadget) in the player's kit. One board per use; the hold time is the action's
//! Duration in the prefab.
class CRF_HoldoutRepairBarricadeAction : ScriptedUserAction
{
	protected const string NO_TOOL = "Needs an entrenching tool";

	//------------------------------------------------------------------------------------------------
	//! Performed on the server (not local-only); nothing for clients to do, so not broadcast to them.
	//! The result reaches everyone through replicated state.
	override bool CanBroadcastScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_HoldoutBarricadeComponent GetBarricade()
	{
		return CRF_HoldoutBarricadeComponent.Cast(GetOwner().FindComponent(CRF_HoldoutBarricadeComponent));
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_HoldoutBarricadeComponent barricade = GetBarricade();
		return barricade && !barricade.IsFull();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(user);
		if (!gadgets || !gadgets.GetGadgetByType(EGadgetType.BUILDING_TOOL))
		{
			SetCannotPerformReason(NO_TOOL);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		CRF_HoldoutBarricadeComponent barricade = GetBarricade();
		if (!barricade)
			return false;

		outName = string.Format("Repair barricade (%1/%2)", barricade.GetBoards(), barricade.GetMaxBoards());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Runs where it is performed and on the server; only the server's board counts
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		CRF_HoldoutBarricadeComponent barricade = GetBarricade();
		if (barricade)
			barricade.AddBoard(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity));
	}
}

//------------------------------------------------------------------------------------------------
//! Holdout: one entry of the quartermaster's catalogue (CRF_HoldoutGamemodeComponent.GetShopEntry), bought
//! with requisition money at the supply crate. The crate prefab carries one of these per catalogue slot;
//! slots past the end of the catalogue, and upgrades already bought, don't show.
class CRF_HoldoutRequisitionAction : ScriptedUserAction
{
	[Attribute("0", UIWidgets.EditBox, "Catalogue entry this action buys (0 = the first)")]
	protected int m_iEntry;

	//------------------------------------------------------------------------------------------------
	//! Performed on the server (not local-only); nothing for clients to do, so not broadcast to them.
	//! The result reaches everyone through replicated state.
	override bool CanBroadcastScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		return holdout && holdout.CanShowShopEntry(m_iEntry);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (!holdout)
			return false;

		string reason;
		if (!holdout.CanBuy(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user), m_iEntry, reason))
		{
			SetCannotPerformReason(reason);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (!holdout)
			return false;

		outName = holdout.GetShopEntryLabel(m_iEntry);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout)
			holdout.Buy(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity), m_iEntry);
	}
}

//------------------------------------------------------------------------------------------------
//! Holdout: the radio. Every few waves it can call in support (CRF_HoldoutGamemodeComponent.CallSupport):
//! the fallen come back and a resupply is dropped.
class CRF_HoldoutCallSupportAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	//! Performed on the server (not local-only); nothing for clients to do, so not broadcast to them.
	//! The result reaches everyone through replicated state.
	override bool CanBroadcastScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return CRF_HoldoutGamemodeComponent.GetInstance() != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (!holdout)
			return false;

		if (!holdout.IsSupportReady())
		{
			SetCannotPerformReason(holdout.GetSupportNotReadyReason());
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Call for support";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout)
			holdout.CallSupport(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity));
	}
}
