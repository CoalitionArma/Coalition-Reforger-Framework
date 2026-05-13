//------------------------------------------------------------------------------------
// CRF_CTF_DropFlagAction: User action to voluntarily drop the CTF flag.
//
// Place this action on the flag entity via its ActionsManagerComponent.
// Because the flag entity teleports to stay near the carrier, this action will
// appear in the carrier's interaction prompt when they look at themselves / nearby.
// The action is only visible to the player who is currently carrying the flag.
// Performing it sends an RPC to the server to release the flag at its current position.
//------------------------------------------------------------------------------------

class CRF_CTF_DropFlagAction : ScriptedUserAction
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
	//! Execute the drop: send an authoritative RPC to the server.
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;

		CRF_PlayerRplToAuthorityManager rplManager = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.DropCTFFlag();

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Show this action only to the player who is currently carrying the flag.
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_CTFGamemode)
			m_CTFGamemode = CRF_CTFGamemodeManager.GetInstance();

		if (!m_CTFGamemode)
			return false;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		return m_CTFGamemode.GetFlagHolderPlayerId() == localPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		outName = "Drop Flag";
		return true;
	}
}
