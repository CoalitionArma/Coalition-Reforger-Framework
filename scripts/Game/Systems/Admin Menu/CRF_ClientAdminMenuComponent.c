[ComponentEditorProps(category: "GameScripted/Client", description: "", color: "0 0 255 255")]
class CRF_ClientAdminMenuComponentClass : ScriptComponentClass {};

class CRF_ClientAdminMenuComponent : ScriptComponent
{	
	protected CRF_AdminMenuGameComponent m_adminMenuComponent;
	static CRF_ClientAdminMenuComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_ClientAdminMenuComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_ClientAdminMenuComponent));
		else
			return null;
	}
	
	void SpawnGroup(int playerId, string prefab, vector spawnLocation, int groupID)
	{
		Rpc(RpcAsk_SpawnGroup, playerId, prefab, spawnLocation, groupID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SpawnGroup(int playerId, string prefab, vector spawnLocation, int groupID)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SpawnGroupServer(playerId, prefab, spawnLocation, groupID);
	}
	
	void ResetGear(int playerID, string prefab)
	{
		Rpc(RpcAsk_ResetGear, playerID, prefab);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ResetGear(int playerID, string prefab)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.ClearGear(playerID);
		m_adminMenuComponent.SetPlayerGear(playerID, prefab);
	}
}