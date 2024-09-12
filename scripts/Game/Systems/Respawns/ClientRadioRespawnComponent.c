[ComponentEditorProps(category: "GameScripted/Client", description: "", color: "0 0 255 255")]
class CRF_ClientRadioRespawnComponentClass : ScriptComponentClass {};

class CRF_ClientRadioRespawnComponent : ScriptComponent
{	
	static CRF_ClientRadioRespawnComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_ClientRadioRespawnComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_ClientRadioRespawnComponent));
		else
			return null;
	}
	
	void SpawnGroup(int groupID)
	{
		Print(groupID);
		Print("RPC");Rpc(RpcAsk_SpawnGroup, groupID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SpawnGroup(int groupID)
	{
		CRF_RadioRespawnSystemComponent m_radioComponent = CRF_RadioRespawnSystemComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_RadioRespawnSystemComponent));
		m_radioComponent.SpawnGroupServer(groupID);
	}
}