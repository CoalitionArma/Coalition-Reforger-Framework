class CRF_RplToAuthorityManagerClass : ScriptComponentClass {}

class CRF_RplToAuthorityManager : ScriptComponent
{	
	// Manager references
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	
	//------------------------------------------------------------------------------------------------
	// Returns the instance of the RplToAuthorityManager
	static CRF_RplToAuthorityManager GetInstance()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
			return CRF_RplToAuthorityManager.Cast(playerController.FindComponent(CRF_RplToAuthorityManager));
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if(!Replication.IsServer())
			return;
		
		InitializeManagerReferences();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initializes all manager references needed by this component
	protected void InitializeManagerReferences()
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	// CLIENT-SIDE METHODS - These send RPCs from client to server
	//------------------------------------------------------------------------------------------------
	
	void RequestInitilizePlayer(int playerId)
	{
		Rpc(RpcAsk_RequestInitilizePlayer, playerId); 
	}
	
	void ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		Rpc(RpcAsk_ToggleSideReady, setReady, playerName, adminForced); 
	}
	
	void ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		Rpc(RpcAsk_ToggleBombPlanted, sitePlanted, togglePlanted); 
	}
	
	void RequestAdvanceGamemodeState(bool overriden)
	{
		if(SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceGamemodeState, overriden);
	}
	
	void RequestAdvanceSlottingPhase()
	{
		if(SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceSlottingPhase); 
	}
	
	// Slot management functions
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		Rpc(RpcAsk_UpdateSlotPlayerID, slotId, playerId); 
	}
	
	void UpdateSlotLockedState(int slotId, bool input)
	{
		Rpc(RpcAsk_UpdateSlotLockedState, slotId, input); 
	}
	
	void UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		Rpc(RpcAsk_UpdateGroupLockedState, groupRplId, input); 
	}
	
	void UpdateSlotDeathState(int slotId, bool input)
	{
		Rpc(RpcAsk_UpdateSlotDeathState, slotId, input); 
	}
	
	void UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		Rpc(RpcAsk_UpdateSlotGroup, slotId, groupRplId); 
	}
	
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		Rpc(RpcAsk_UpdateSlotResource, slotId, resource); 
	}
	
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		Rpc(RpcAsk_UpdateSlotCharacter, slotId, charId); 
	}
	
	// Admin messaging functions
	void SendAdminMessage(string data, int playerID)
	{
		Rpc(RpcAsk_SendAdminMessage, data, playerID); 
	}
	
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if(SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
			Rpc(RpcAsk_ReplyAdminMessage, data, playerId, adminID, logAction); 
	}
	
	void CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_CloseAdminTicket, ticketID, adminID, logAction); 
	}
	
	void AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_AssignAdminTicket, ticketID, adminID, logAction); 
	}
	
	// Player management functions
	void RespawnPlayer(int playerId)
	{
		Rpc(RpcAsk_RespawnPlayer, playerId); 
	}	
	
	// VON channel management
	void RequestToJoinChannel(int channel, int requestId)
	{
		Rpc(RpcAsk_RequestToJoinChannel, channel, requestId); 
	}
	
	void CheckVONRegister(int playerId)
	{
		Rpc(RpcAsk_CheckVONRegister, playerId); 
	}
	
	void CreateChannel(int playerId)
	{
		Rpc(RpcAsk_CreateChannel, playerId); 
	}
	
	void JoinChannel(int playerId, int channel)
	{
		Rpc(RpcAsk_JoinChannel, playerId, channel); 
	}
	
	// Group and spawn management
	void SpawnOnGroup(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		Rpc(RpcAsk_SpawnOnGroup, playerId, spawnLocation, groupID, logAction); 
	}
	
	void RequestGroupIdFromServer(int requestedId, int requesterID)
	{
		Rpc(RpcAsk_RequestGroupIdFromServer, requestedId, requesterID); 
	}
	
	// Equipment management
	void ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		Rpc(RpcAsk_ResetGear, playerId, prefab, logAction); 
	}
	
	void AddItem(int playerId, string prefab, bool logAction)
	{
		Rpc(RpcAsk_AddItem, playerId, prefab, logAction); 
	}
	
	// Admin functions
	void TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		Rpc(RpcAsk_TeleportPlayers, playerId1, playerId2, logAction); 
	}
	
	void SendHint(string data, int playerId = -1, string factionKey = "")
	{
		Rpc(RpcAsk_SendHint, data, playerId, factionKey); 
	}
	
	void Heal(int playerId, bool logAction, bool isVehicle = false)
	{
		Rpc(RpcAsk_Heal, playerId, logAction, isVehicle); 
	}
		
	void LogAdminAction(string data, int playerId, bool sendToPlayer) 
	{
		Rpc(RpcAsk_LogAdminAction, data, playerId, sendToPlayer); 
	}
	
	//------------------------------------------------------------------------------------------------
	// SERVER-SIDE RPC HANDLERS - Executed on the authority (server)
	//------------------------------------------------------------------------------------------------
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestInitilizePlayer(int playerId)
	{
		m_GamemodeManager.InitilizePlayer(playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		m_SafestartManager.ToggleSideReady(setReady, playerName, adminForced);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		CRF_SearchAndDestroyGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGamemodeManager)).ToggleBombPlanted(sitePlanted, togglePlanted);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceGamemodeState(bool overriden)
	{
		m_Gamemode.AdvanceGamemodeState(overriden);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceSlottingPhase()
	{
		m_Gamemode.AdvanceSlottingState();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotPlayerID(int slotId, int playerId)
	{
		m_SlottingManager.UpdateSlotPlayerID(slotId, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotLockedState(int slotId, bool input)
	{
		m_SlottingManager.UpdateSlotLockedState(slotId, input);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(groupRplId));
		if (!rplComponent)
			return;
			
		SCR_AIGroup group = SCR_AIGroup.Cast(rplComponent.GetEntity());
		if (group)
			group.SetPrivate(input);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotDeathState(int slotId, bool input)
	{
		m_SlottingManager.UpdateSlotDeathState(slotId, input); 
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		m_SlottingManager.UpdateSlotGroup(slotId, groupRplId); 
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotResource(int slotId, ResourceName resource)
	{
		m_SlottingManager.UpdateSlotResource(slotId, resource); 
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotCharacter(int slotId, RplId charId)
	{
		m_SlottingManager.UpdateSlotCharacter(slotId, charId); 
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendAdminMessage(string data, int playerID)
	{
		m_RplBroadcastManager.SendAdminMessage(data, playerID);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		m_RplBroadcastManager.ReplyAdminMessage(data, playerId, adminID, logAction);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		m_RplBroadcastManager.CloseAdminTicket(ticketID, adminID, logAction);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		m_RplBroadcastManager.AssignAdminTicket(ticketID, adminID, logAction);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnPlayer(int playerId)
	{
		m_RespawnManager.RespawnPlayer(playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestToJoinChannel(int channel, int requestId)
	{
		m_MenuManager.RequestToJoinChannel(channel, requestId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CheckVONRegister(int playerId)
	{
		int channelIndex;
		if (!m_MenuManager.IsPlayerInAnyChannel(playerId, channelIndex))
		{
			m_MenuManager.AddPlayerToChannel(playerId, 1, false);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CreateChannel(int playerId)
	{
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_MenuManager.CreateChannel(playerName + "'s Channel", playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_JoinChannel(int playerId, int channel)
	{
		m_MenuManager.AddPlayerToChannel(playerId, channel, false);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		m_RespawnManager.RespawnPlayer(playerId, spawnLocation, groupID);

		if (logAction)
		{
			SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
			if (group)
			{
				string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
				string logMessage = string.Format("%1 was respawned to %2", playerName, group.m_faction);
				m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
			}
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestGroupIdFromServer(int requestedId, int requesterID)
	{
		if (m_SlottingManager.IsPlayerInASlot(requestedId))
			return;

		SCR_AIGroup playerGroup = m_SlottingManager.GetPlayerSlotGroup(requestedId);
		if (playerGroup)
			m_RplBroadcastManager.SendGroupIDToPlayer(requesterID, playerGroup.GetGroupID());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		// Close map on client to prevent lockup
		m_RplBroadcastManager.Closemap(playerId);
		
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;

		// Schedule gear setup with appropriate delay
		GetGame().GetCallqueue().Call(
			m_GearscriptManager.SetEntityGear, 
			entity, 
			prefab
		);
		
		m_SlottingManager.UpdateSlotResource(m_SlottingManager.GetPlayerSlotID(playerId), prefab);

		if (logAction)
		{
			string prefabName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1's gear was set to %2", playerName, prefabName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItem(int playerId, string prefab, bool logAction)
	{
		if (playerId == 0 || prefab.IsEmpty())
			return;

		if (logAction)
		{
			string itemName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%2 was added to %1's inventory", playerName, itemName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
		
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;
			
		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!entityInventoryManager)
			return;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		
		Resource resource = Resource.Load(prefab);
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
			delete resourceSpawned;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		m_RplBroadcastManager.TeleportPlayers(playerId1, playerId2, logAction);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendHint(string data, int playerId, string factionKey)
	{
		m_RplBroadcastManager.SendHint(data, playerId, factionKey);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Heal(int playerId, bool logAction, bool isVehicle)
	{
		IEntity entityToFix = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entityToFix)
			return;
		
		if (isVehicle)
		{
			entityToFix = SCR_CompartmentAccessComponent.GetVehicleIn(entityToFix);
			if (!entityToFix)
				return;
		}

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(entityToFix.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1 was healed/vehicle repaired", playerName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		m_RplBroadcastManager.LogAdminAction(data, playerId, sendToPlayer);
	}
};
