[ComponentEditorProps(category: "GameScripted/Client", description: "", color: "0 0 255 255")]
class CRF_ClientAdminMenuComponentClass : ScriptComponentClass {};

class CRF_ClientAdminMenuComponent : ScriptComponent
{	
	string m_sHintText = "Type Here";
	
	// A hashmap that is modified only on each client by a .BumpMe from the authority.
	protected ref map<string, string> m_mLocalPlayerMap = new map<string, string>;
	
	protected CRF_AdminMenuGameComponent m_adminMenuComponent;
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	static CRF_ClientAdminMenuComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_ClientAdminMenuComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_ClientAdminMenuComponent));
		else
			return null;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//Admin Messaging
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data)
	{
		
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(string.Format("Message Sent: \"%1\"", data));
		data = string.Format("PlayerID: %1 | Player Name: %3 | \"%2\"", GetGame().GetPlayerController().GetPlayerId(), data, GetGame().GetPlayerManager().GetPlayerName(GetGame().GetPlayerController().GetPlayerId()));
		Rpc(RpcAsk_SendAdminMessage, data);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SendAdminMessage(string data)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SendAdminMessage(data);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data)
	{
		ref array<string> dataSplit = {};
		data.Split(" ", dataSplit, false);
		int playerID;
		string toSend;
		for(int i = 0; i < dataSplit.Count(); i++)
		{
			if(dataSplit[i] == "0")
			{
				dataSplit.RemoveOrdered(i);
				playerID = 0;
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}
			
			if(dataSplit[i].ToInt() > 0)
			{
				playerID = dataSplit[i].ToInt();
				dataSplit.RemoveOrdered(i);
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}
		}	
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		if(!playerID)
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		if(!GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID))
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		
		chatComponent.ShowMessage(string.Format("Message Sent to %2: \"%1\"", toSend, GetGame().GetPlayerManager().GetPlayerName(playerID)));
		toSend = string.Format("Admin: \"%1\"", toSend);
		Rpc(RpcAsk_ReplyAdminMessage, toSend, playerID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ReplyAdminMessage(string data, int playerID)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.ReplyAdminMessage(data, playerID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Respawn
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SpawnGroup(int playerID, string prefab, vector spawnLocation, int groupID)
	{
		Rpc(RpcAsk_SpawnGroup, playerID, prefab, spawnLocation, groupID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SpawnGroup(int playerID, string prefab, vector spawnLocation, int groupID)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SpawnGroupServer(playerID, prefab, spawnLocation, groupID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Gear
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void ResetGear(int playerID, string prefab)
	{
		Rpc(RpcAsk_ResetGear, playerID, prefab);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ResetGear(int playerID, string prefab)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SetPlayerGear(playerID, prefab);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddItem(int playerID, string prefab)
	{
		Rpc(RpcAsk_AddItem, playerID, prefab);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_AddItem(int playerID, string prefab)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.AddItem(playerID, prefab);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Teleport
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void TeleportLocalPlayer(int playerID1, int playerID2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
	    spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 3);
	    spawnParams.Transform[3] = teleportLocation;
	
		SCR_Global.TeleportPlayer(playerID1, teleportLocation);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerID1, int playerID2)
	{
		Rpc(RpcAsk_TeleportPlayers, playerID1, playerID2);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_TeleportPlayers(int playerID1, int playerID2)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.TeleportPlayers(playerID1, playerID2);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Hint
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SendHintAll(string data)
	{
		Rpc(RpcAsk_SendHintAll, data);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SendHintAll(string data)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SendHintAll(data);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SendHintPlayer(string data, int playerID)
	{
		Rpc(RpcAsk_SendHintPlayer, data, playerID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SendHintPlayer(string data, int playerID)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SendHintPlayer(data, playerID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SendHintFaction(string data, string factionKey)
	{
		Rpc(RpcAsk_SendHintFaction, data, factionKey);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SendHintFaction(string data, string factionKey)
	{
		m_adminMenuComponent = CRF_AdminMenuGameComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_AdminMenuGameComponent));
		m_adminMenuComponent.SendHintFaction(data, factionKey);
	}
}