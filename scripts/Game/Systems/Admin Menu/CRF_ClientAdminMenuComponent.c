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
		m_adminMenuComponent.SetPlayerGearServer(playerID, prefab);
	}
	
	void AddItem(int playerID, string prefab)
	{
		Rpc(RpcAsk_AddItem, playerID, prefab);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_AddItem(int playerID, string prefab)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.AddItem(playerID, prefab);
	}
	
	void TeleportLocalPlayer(int playerID1, int playerID2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID2);
		PrintFormat("Teleporting %1 to %2", playerID1, entity2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
	    spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 3);
	    spawnParams.Transform[3] = teleportLocation;
	
		SCR_Global.TeleportPlayer(playerID1, teleportLocation);
	}
	
	void TeleportPlayers(int playerID1, int playerID2)
	{
		Rpc(RpcAsk_TeleportPlayers, playerID1, playerID2);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_TeleportPlayers(int playerID1, int playerID2)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.TeleportPlayers(playerID1, playerID2);
	}
}