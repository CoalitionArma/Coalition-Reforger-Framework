//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Character", description: "Set character playable", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_HintSenderComponentClass: ScriptComponentClass
{
	
}

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class CRF_HintSenderComponent : ScriptComponent
{
	void SendHint(string msg)
	{
		Rpc(RPC_SendHint, msg);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SendHint(string msg)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PlayerController playerController = PlayerController.Cast(GetOwner());
		if (!playerManager.HasPlayerRole(playerController.GetPlayerId(), EPlayerRole.ADMINISTRATOR))
			return;
		
		CRF_HintManager hintManager = CRF_HintManager.GetInstance();
		hintManager.SendHintClients(msg);
	}
}