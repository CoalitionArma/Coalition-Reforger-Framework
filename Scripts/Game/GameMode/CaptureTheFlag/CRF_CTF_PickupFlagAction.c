//------------------------------------------------------------------------------------
// CRF_CTF_PickupFlagAction: User action to pick up the CTF flag.
//
// Place this action on the flag entity via its ActionsManagerComponent.
// The action is only visible when the flag is not currently held by anyone.
// Performing it sends an RPC to the server to register the pickup.
//------------------------------------------------------------------------------------

class CRF_CTF_PickupFlagAction : ScriptedUserAction
{
	protected CRF_CTFGamemodeManager m_CTFGamemode;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode())
			return;

		m_CTFGamemode = CRF_CTFGamemodeManager.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	//! Execute the pickup: send an authoritative RPC to the server.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;

		CRF_PlayerRplToAuthorityManager rplManager = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.PickupCTFFlag();

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Show the action only when the flag is unclaimed.
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_CTFGamemode)
			m_CTFGamemode = CRF_CTFGamemodeManager.GetInstance();

		if (!m_CTFGamemode)
			return false;

		// Hide if flag is already held by someone
		return !m_CTFGamemode.IsFlagHeld();
	}

	//------------------------------------------------------------------------------------------------
	//! Allow the action only when the game is in play (safestart ended, not game over).
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_CTFGamemode)
			return false;

		// Gamemode is initialised once safestart ends; guard against premature pickup
		return !m_CTFGamemode.IsFlagHeld();
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Pick Up Flag";
		return true;
	}
}
