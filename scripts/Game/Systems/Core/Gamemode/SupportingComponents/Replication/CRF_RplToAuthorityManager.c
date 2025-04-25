class CRF_RplToAuthorityManagerClass : ScriptComponentClass {}

class CRF_RplToAuthorityManager : ScriptComponent
{	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	
	//------------------------------------------------------------------------------------------------
	static CRF_RplToAuthorityManager GetInstance()
	{
		// Get gamemode so we can pull component
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode && RplSession.Mode() != RplMode.Dedicated) // CANNOT SEND UP TO THE AUTHORITY IF WE ARE THE AUTHORITY
			return CRF_RplToAuthorityManager.Cast(gameMode.FindComponent(CRF_RplToAuthorityManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if(!Replication.IsServer())
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	
	// The stuff that exectues on the child
	
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	void RequestInitilizePlayer(int playerId)
	{
		Rpc(RpcAsk_RequestInitilizePlayer, playerId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		Rpc(RpcAsk_ToggleSideReady, setReady, playerName, adminForced); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		Rpc(RpcAsk_ToggleBombPlanted, sitePlanted, togglePlanted); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestAdvanceGamemodeState()
	{
		if(SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceGamemodeState);
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestAdvanceSlottingPhase()
	{
		if(SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceSlottingPhase); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlot(int index, int playerId)
	{
		Rpc(RpcAsk_SetSlot, index, playerId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SetGroupLocked(int index, bool input)
	{
		Rpc(RpcAsk_SetGroupLocked, index, input); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data)
	{
		if(SCR_Global.IsAdmin() || CRF_Library.IsModerator())
			Rpc(RpcAsk_SendAdminMessage, data); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerId, bool logAction)
	{
		if(SCR_Global.IsAdmin() || CRF_Library.IsModerator())
			Rpc(RpcAsk_ReplyAdminMessage, data, playerId, logAction); 
	}		
	
	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector spawnLocation)
	{
		Rpc(RpcAsk_RespawnPlayer, playerId, spawnLocation); 
	}	
	
	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		Rpc(RpcAsk_RequestToJoinChannel, channel, requestId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void CheckVONRegister(int playerId)
	{
		Rpc(RpcAsk_CheckVONRegister, playerId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateChannel(int playerId)
	{
		Rpc(RpcAsk_CreateChannel, playerId); 
	}
	
	//------------------------------------------------------------------------------------------------
	void JoinChannel(int playerId, int channel)
	{
		Rpc(RpcAsk_JoinChannel, playerId, channel); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SpawnOnGroup(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		Rpc(RpcAsk_SpawnOnGroup, playerId, spawnLocation, groupID, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestGroupIdFromServer(int requestedId, int requesterID)
	{
		Rpc(RpcAsk_RequestGroupIdFromServer, requestedId, requesterID); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		Rpc(RpcAsk_ResetGear, playerId, prefab, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void AddItem(int playerId, string prefab, bool logAction)
	{
		Rpc(RpcAsk_AddItem, playerId, prefab, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		Rpc(RpcAsk_TeleportPlayers, playerId1, playerId2, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void SendHint(string data, int playerId = -1, string factionKey = "")
	{
		Rpc(RpcAsk_SendHint, data, playerId, factionKey); 
	}
	
	//------------------------------------------------------------------------------------------------
	void Heal(int playerId, bool logAction, bool isVehicle = false)
	{
		Rpc(RpcAsk_Heal, playerId, logAction, isVehicle); 
	}
		
	//------------------------------------------------------------------------------------------------
	void LogAdminAction(string data, int playerId, bool sendToPlayer) 
	{
		Rpc(RpcAsk_LogAdminAction, data, playerId, sendToPlayer); 
	}
	
	//------------------------------------------------------------------------------------------------
	
	// The stuff that executes on the authority
	
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestInitilizePlayer(int playerId)
	{
		m_Gamemode.GetInstance().InitilizePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		CRF_SafestartManager.GetInstance().ToggleSideReady(setReady, playerName, adminForced);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		CRF_SearchAndDestroyGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGamemodeManager)).ToggleBombPlanted(sitePlanted, togglePlanted);
	}

	//Communicates to server to advance state
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceGamemodeState()
	{
		m_Gamemode.AdvanceGamemodeState();
	}
	
	//Communicates to server to advance the slotting phase
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceSlottingPhase()
	{
		m_Gamemode.AdvanceSlottingState();
	}

	//Communicates to server to set slot
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetSlot(int index, int playerId)
	{
		m_Gamemode.SetSlot(index, playerId);
	}

	//Communicates to server to set group locked
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetGroupLocked(int index, bool input)
	{
		m_Gamemode.SetGroupLockedStatus(index, input);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendAdminMessage(string data)
	{
		CRF_RplBroadcastManager.GetInstance().SendAdminMessage(data);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReplyAdminMessage(string data, int playerId, bool logAction)
	{
		CRF_RplBroadcastManager.GetInstance().ReplyAdminMessage(data, playerId, logAction);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnPlayer(int playerId, vector spawnLocation)
	{
		CRF_RespawnManager.GetInstance().RespawnPlayer(playerId, spawnLocation);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestToJoinChannel(int channel, int requestId)
	{
		CRF_MenuManager.GetInstance().RequestToJoinChannel(channel, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CheckVONRegister(int playerId)
	{
		int channelIndex;
		if (!CRF_MenuManager.GetInstance().IsPlayerInAnyChannel(playerId, channelIndex))
		{
			CRF_MenuManager.GetInstance().AddPlayerToChannel(playerId, 1, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CreateChannel(int playerId)
	{
		CRF_MenuManager.GetInstance().CreateChannel(GetGame().GetPlayerManager().GetPlayerName(playerId) + "'s Channel", playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_JoinChannel(int playerId, int channel)
	{
		CRF_MenuManager.GetInstance().AddPlayerToChannel(playerId, channel, false);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		CRF_RespawnManager.GetInstance().RespawnPlayer(playerId, spawnLocation, groupID);

		if (logAction)
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(string.Format("%1 was respawned to %2", GetGame().GetPlayerManager().GetPlayerName(playerId), SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID).m_faction), playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestGroupIdFromServer(int requestedId, int requesterID)
	{
		if (m_Gamemode.m_aSlots.Find(requestedId) == -1)
			return;

		RplId groupID = m_Gamemode.m_aActivePlayerGroupsIDs.Get(m_Gamemode.m_aGroupRplIDs.Find(m_Gamemode.m_aPlayerGroupIDs.Get(m_Gamemode.m_aSlots.Find(requestedId))));
		SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
		if (playerGroup)
			CRF_RplBroadcastManager.GetInstance().SendGroupIDToPlayer(requesterID, playerGroup.GetGroupID());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);

		GetGame().GetCallqueue().CallLater(CRF_GearscriptManager.GetInstance().SetupAddGearToEntity, 250, false, entity, prefab);

		if (logAction)
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(string.Format("%1's gear was set to %2", GetGame().GetPlayerManager().GetPlayerName(playerId), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItem(int playerId, string prefab, bool logAction)
	{
		if (playerId == 0 || prefab.IsEmpty())
			return;

		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
			delete resourceSpawned;

		if (logAction)
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(string.Format("%2 was added to %1's inventory", GetGame().GetPlayerManager().GetPlayerName(playerId), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		CRF_RplBroadcastManager.GetInstance().TeleportPlayers(playerId1, playerId2, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendHint(string data, int playerId, string factionKey)
	{
		CRF_RplBroadcastManager.GetInstance().SendHint(data, playerId, factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Heal(int playerId, bool logAction, bool isVehicle)
	{
		IEntity entityToFix = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);;
		
		if(isVehicle)
		{
			entityToFix = SCR_CompartmentAccessComponent.GetVehicleIn(entityToFix);
			if (!entityToFix)
				return;
		};

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(entityToFix.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(string.Format("%1's was healed/repaired", GetGame().GetPlayerManager().GetPlayerName(playerId)), playerId, true);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		CRF_RplBroadcastManager.GetInstance().LogAdminAction(data, playerId, sendToPlayer);
	}
};