//------------------------------------------------------------------------------------
// Client -> server request of the objective chain (CRF_BreachChargeAction). Its own modded file so the
// big CRF relay (CRF_COA_PlayerRplToAuthorityManager.c) stays as it is.
//------------------------------------------------------------------------------------
modded class COA_PlayerRplToAuthorityManager : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	void RequestObjectiveChainBreach(int breachIndex)
	{
		Rpc(RpcAsk_RequestObjectiveChainBreach, breachIndex);
	}

	//------------------------------------------------------------------------------------------------
	//! The sender is the owner of this relay (GetCallerPlayerId); the objective chain checks their
	//! faction, distance and breaching charge itself
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestObjectiveChainBreach(int breachIndex)
	{
		CRF_ObjectiveChainGamemodeComponent chain = CRF_ObjectiveChainGamemodeComponent.GetInstance();
		if (!chain)
			return;

		chain.ServerArmBreach(GetCallerPlayerId(), breachIndex);
	}
}
