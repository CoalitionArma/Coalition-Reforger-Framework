class CRF_RplBroadcastManagerClass : ScriptComponentClass {}

//------------------------------------------------------------------------------------------------
// Enum for slot update field types (batching system)
//------------------------------------------------------------------------------------------------
enum CRF_ESlotUpdateField
{
	PLAYER_ID,
	CHARACTER,
	GROUP,
	RESOURCE,
	LOCKED,
	DEATH
}

//------------------------------------------------------------------------------------------------
// Container for queued slot update
//------------------------------------------------------------------------------------------------
class CRF_SlotUpdateBatch
{
	int m_iSlotId;
	CRF_ESlotUpdateField m_eFieldType;
	int m_iIntValue;
	RplId m_RplIdValue;
	ResourceName m_ResourceValue;
	bool m_bBoolValue;
	
	void CRF_SlotUpdateBatch(int slotId, CRF_ESlotUpdateField fieldType)
	{
		m_iSlotId = slotId;
		m_eFieldType = fieldType;
	}
}

//------------------------------------------------------------------------------------------------
// Broadcast Manager responsible for handling server-client communications
//------------------------------------------------------------------------------------------------
class CRF_RplBroadcastManager : ScriptComponent
{
	// Manager references
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_MenuManager m_MenuManager;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	protected CRF_BandwidthTelemetryManager m_TelemetryManager;
	protected static CRF_RplBroadcastManager m_sInstance;
	
	// Batching system for slot updates
	protected ref array<ref CRF_SlotUpdateBatch> m_aPendingSlotUpdates = new array<ref CRF_SlotUpdateBatch>();
	protected bool m_bBatchingEnabled = true;
	protected bool m_bFlushScheduled = false;
	
	void CRF_RplBroadcastManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	// Get singleton instance of the broadcast manager
	//------------------------------------------------------------------------------------------------
	static CRF_RplBroadcastManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		InitializeManagerReferences();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize references to other manager systems
	//------------------------------------------------------------------------------------------------
	protected void InitializeManagerReferences()
	{
		// Cache all manager references to avoid repeated GetInstance() calls
		if (!m_GamemodeManager)
			m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		if (!m_RespawnManager)
			m_RespawnManager = CRF_RespawnManager.GetInstance();
		if (!m_MenuManager)
			m_MenuManager = CRF_MenuManager.GetInstance();
		if (!m_AdminMenuManager)
			m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
		if (!m_TelemetryManager)
			m_TelemetryManager = CRF_BandwidthTelemetryManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	// Log RPC call to telemetry system (server-side only)
	//------------------------------------------------------------------------------------------------
	protected void LogTelemetry(string rpcName, int estimatedBytes)
	{
		if (!Replication.IsServer())
			return;
			
		// Ensure telemetry manager is cached
		if (!m_TelemetryManager)
			m_TelemetryManager = CRF_BandwidthTelemetryManager.GetInstance();
			
		if (m_TelemetryManager)
			m_TelemetryManager.LogRPC(rpcName, estimatedBytes);
	}
	
	//================================================================================================
	// BATCHING SYSTEM
	// Collects multiple slot updates and sends them together to reduce RPC overhead
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// Queue a slot update for batching
	//------------------------------------------------------------------------------------------------
	protected void QueueSlotUpdate(CRF_SlotUpdateBatch batch)
	{
		if (!Replication.IsServer())
			return;
		
		// Check if we already have a pending update for this slot+field combination
		// If so, replace it (latest value wins)
		for (int i = m_aPendingSlotUpdates.Count() - 1; i >= 0; i--)
		{
			CRF_SlotUpdateBatch existing = m_aPendingSlotUpdates[i];
			if (existing.m_iSlotId == batch.m_iSlotId && existing.m_eFieldType == batch.m_eFieldType)
			{
				m_aPendingSlotUpdates.Remove(i);
				break;
			}
		}
		
		m_aPendingSlotUpdates.Insert(batch);
		
		// Schedule flush if not already scheduled
		if (!m_bFlushScheduled)
		{
			m_bFlushScheduled = true;
			GetGame().GetCallqueue().CallLater(FlushSlotUpdates, 1, false); // Flush next frame
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Flush all pending slot updates
	//------------------------------------------------------------------------------------------------
	protected void FlushSlotUpdates()
	{
		if (!Replication.IsServer())
			return;
		
		m_bFlushScheduled = false;
		
		if (m_aPendingSlotUpdates.Count() == 0)
			return;
		
		// Process all pending updates
		foreach (CRF_SlotUpdateBatch batch : m_aPendingSlotUpdates)
		{
			switch (batch.m_eFieldType)
			{
				case CRF_ESlotUpdateField.PLAYER_ID:
					SendSlotPlayerIdUpdate(batch.m_iSlotId, batch.m_iIntValue);
					break;
				
				case CRF_ESlotUpdateField.CHARACTER:
					SendSlotCharacterUpdate(batch.m_iSlotId, batch.m_RplIdValue);
					break;
				
				case CRF_ESlotUpdateField.GROUP:
					SendSlotGroupUpdate(batch.m_iSlotId, batch.m_RplIdValue);
					break;
				
				case CRF_ESlotUpdateField.RESOURCE:
					SendSlotResourceUpdate(batch.m_iSlotId, batch.m_ResourceValue);
					break;
				
				case CRF_ESlotUpdateField.LOCKED:
					SendSlotLockedUpdate(batch.m_iSlotId, batch.m_bBoolValue);
					break;
				
				case CRF_ESlotUpdateField.DEATH:
					SendSlotDeathUpdate(batch.m_iSlotId, batch.m_bBoolValue);
					break;
			}
		}
		
		// Clear the queue
		m_aPendingSlotUpdates.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	// Internal methods that actually send the RPCs (called by flush or immediate mode)
	//------------------------------------------------------------------------------------------------
	protected void SendSlotPlayerIdUpdate(int slotId, int playerId)
	{
		LogTelemetry("UpdateSlotPlayerIdDelta", 8);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotPlayerIdDelta(slotId, playerId);
		#else
		Rpc(RpcDo_UpdateSlotPlayerIdDelta, slotId, playerId);
		#endif
	}
	
	protected void SendSlotCharacterUpdate(int slotId, RplId characterId)
	{
		LogTelemetry("UpdateSlotCharacterDelta", 8);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotCharacterDelta(slotId, characterId);
		#else
		Rpc(RpcDo_UpdateSlotCharacterDelta, slotId, characterId);
		#endif
	}
	
	protected void SendSlotGroupUpdate(int slotId, RplId groupId)
	{
		LogTelemetry("UpdateSlotGroupDelta", 8);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotGroupDelta(slotId, groupId);
		#else
		Rpc(RpcDo_UpdateSlotGroupDelta, slotId, groupId);
		#endif
	}
	
	protected void SendSlotResourceUpdate(int slotId, ResourceName resource)
	{
		int bytes = 4 + CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(resource);
		LogTelemetry("UpdateSlotResourceDelta", bytes);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotResourceDelta(slotId, resource);
		#else
		Rpc(RpcDo_UpdateSlotResourceDelta, slotId, resource);
		#endif
	}
	
	protected void SendSlotLockedUpdate(int slotId, bool isLocked)
	{
		LogTelemetry("UpdateSlotLockedDelta", 5);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotLockedDelta(slotId, isLocked);
		#else
		Rpc(RpcDo_UpdateSlotLockedDelta, slotId, isLocked);
		#endif
	}
	
	protected void SendSlotDeathUpdate(int slotId, bool isDead)
	{
		LogTelemetry("UpdateSlotDeathDelta", 5);
		#ifdef WORKBENCH
		RpcDo_UpdateSlotDeathDelta(slotId, isDead);
		#else
		Rpc(RpcDo_UpdateSlotDeathDelta, slotId, isDead);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	// Enable/disable batching (useful for debugging or critical updates)
	//------------------------------------------------------------------------------------------------
	void SetBatchingEnabled(bool enabled)
	{
		m_bBatchingEnabled = enabled;
		
		// If disabling, flush any pending updates immediately
		if (!enabled && m_aPendingSlotUpdates.Count() > 0)
			FlushSlotUpdates();
	}
	
	bool IsBatchingEnabled()
	{
		return m_bBatchingEnabled;
	}
	
	//================================================================================================
	// AUTHORITY BROADCAST METHODS
	// These methods execute on the server to send data to clients
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void PopUpNotification(float life, string titleText, string subtitleText = "", string sound = "", string titleTextParam1 = "", string titleTextParam2 = "")
	{
		// Telemetry: float + 5 strings
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Float();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(titleText);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(subtitleText);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(sound);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(titleTextParam1);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(titleTextParam2);
		LogTelemetry("PopUpNotification", bytes);
		
		#ifdef WORKBENCH
		RpcDo_PopUpNotification(life, titleText, subtitleText, sound, titleTextParam1, titleTextParam2);
		#else
		Rpc(RpcDo_PopUpNotification, life, titleText, subtitleText, sound, titleTextParam1, titleTextParam2);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data, int playerID, bool ticketExists)
	{
		// Telemetry: string + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("SendAdminMessage", bytes);
		
		#ifdef WORKBENCH
		RpcDo_SendAdminMessage(data, playerID, ticketExists);
		#else
		Rpc(RpcDo_SendAdminMessage, data, playerID, ticketExists);
		#endif
	}
	//------------------------------------------------------------------------------------------------
	void GetOpenTickets(int playerID)
	{
		#ifdef WORKBENCH
		RpcDo_GetOpenTickets(playerID, CRF_AdminMenuManager.GetInstance().GetOpenTickets());
		#else
		Rpc(RpcDo_GetOpenTickets, playerID, CRF_AdminMenuManager.GetInstance().GetOpenTickets());
		#endif
	}
	//------------------------------------------------------------------------------------------------
	void GetTicketMessages(int playerID, int ticketID)
	{
		#ifdef WORKBENCH
		RpcDo_GetTicketMessages(playerID, CRF_AdminMenuManager.GetInstance().GetTicketMessages(ticketID));
		#else
		Rpc(RpcDo_GetTicketMessages, playerID, CRF_AdminMenuManager.GetInstance().GetTicketMessages(ticketID));
		#endif
	}
	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		// Telemetry: string + 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("ReplyAdminMessage", bytes);
		
		#ifdef WORKBENCH
		RpcDo_ReplyAdminMessage(data, playerId, adminID, logAction);
		#else
		Rpc(RpcDo_ReplyAdminMessage, data, playerId, adminID, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("CloseAdminTicket", bytes);
		
		#ifdef WORKBENCH
		RpcDo_CloseAdminTicket(ticketID, adminID, logAction);
		#else
		Rpc(RpcDo_CloseAdminTicket, ticketID, adminID, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void NotifiyTicketAssigned(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("NotifiyTicketAssigned", bytes);
		
		#ifdef WORKBENCH
		RpcDo_NotifiyTicketAssigned(ticketID, adminID, logAction);
		#else
		Rpc(RpcDo_NotifiyTicketAssigned, ticketID, adminID, logAction);
		#endif
	}
	//------------------------------------------------------------------------------------------------
	void RefreshAdminMenuLists()
	{
		#ifdef WORKBENCH
		RpcDo_RefreshAdminMenuLists();
		#else
		Rpc(RpcDo_RefreshAdminMenuLists);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		IEntity player2Char = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		
		if(player2Char)
		{
			vector player2Origin = player2Char.GetOrigin();
			
			// Telemetry: 2 ints + vector + bool
			int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
			bytes += CRF_BandwidthTelemetryManager.EstimateSize_Vector();
			bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
			LogTelemetry("TeleportPlayers", bytes);
		
			#ifdef WORKBENCH
			RpcDo_TeleportPlayers(playerId1, playerId2, player2Origin, logAction);
			#else
			Rpc(RpcDo_TeleportPlayers, playerId1, playerId2, player2Origin, logAction);
			#endif
		};
	}
	
	void BroadcastMessage(string message)
	{
		// Telemetry: string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(message);
		LogTelemetry("BroadcastMessage", bytes);
		
		Rpc(RpcDo_BroadcastMessage, message);
	}
	
	//------------------------------------------------------------------------------------------------
	void Closemap(int playerID)
	{
		// Telemetry: int
		LogTelemetry("Closemap", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_Closemap(playerID);
		#else
		Rpc(RpcDo_Closemap, playerID);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void HolsterGun(int playerID)
	{
		// Telemetry: int
		LogTelemetry("HolsterGun", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_HolsterGun(playerID);
		#else
		Rpc(RpcDo_HolsterGun, playerID);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendHint(string data, int playerId = -1, string factionKey = "")
	{
		// Telemetry: 2 strings + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(factionKey);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("SendHint", bytes);
		
		#ifdef WORKBENCH
		RpcDo_SendHint(data, playerId, factionKey);
		#else
		Rpc(RpcDo_SendHint, data, playerId, factionKey);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		// Telemetry: string + int + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("LogAdminAction", bytes);
		
		#ifdef WORKBENCH
		RpcDo_LogAdminAction(data, playerId, sendToPlayer);
		#else
		Rpc(RpcDo_LogAdminAction, data, playerId, sendToPlayer);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendRespawnScreenUpdate(RplId rplID, bool active)
	{
		// Telemetry: RplId + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("SendRespawnScreenUpdate", bytes);
		
		#ifdef WORKBENCH
		RpcDo_SendRespawnScreenUpdate(rplID, active);
		#else
		Rpc(RpcDo_SendRespawnScreenUpdate, rplID, active);
		#endif
	}	

	//------------------------------------------------------------------------------------------------
	void SendRespawnScreen(int playerId)
	{
		// Telemetry: int
		LogTelemetry("SendRespawnScreen", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_SendRespawnScreen(playerId);
		#else
		Rpc(RpcDo_SendRespawnScreen, playerId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendCharacterLoadingScreen(int playerId)
	{
		// Telemetry: int
		LogTelemetry("SendCharacterLoadingScreen", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_SendCharacterLoadingScreen(playerId);
		#else
		Rpc(RpcDo_SendCharacterLoadingScreen, playerId);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void InitilizePlayerBroadcast(int playerId, RplId playerCharID)
	{
		// Telemetry: int + RplId
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("InitilizePlayerBroadcast", bytes);
		
		#ifdef WORKBENCH
		RpcDo_InitilizePlayerBroadcast(playerId, playerCharID);
		#else
		Rpc(RpcDo_InitilizePlayerBroadcast, playerId, playerCharID);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendRequest(int playerId, int requestId, int channel)
	{
		Print(string.Format("[VON] Server sending RPC to targetPlayerId=%1, requestId=%2, channel=%3", playerId, requestId, channel), LogLevel.NORMAL);
		
		// Get the player controller for the target player
		PlayerController targetController = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!targetController)
		{
			Print(string.Format("[VON] Target player %1 controller not found", playerId), LogLevel.ERROR);
			return;
		}
		
		// Telemetry: 3 ints
		LogTelemetry("SendRequest", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 3);
		
		// Send RPC specifically to the target player
		Rpc(RpcDo_SendRequest, playerId, requestId, channel);
	}
	
	//------------------------------------------------------------------------------------------------
	void NotifyChannelJoinRequest(int targetPlayerId, int requesterId, int channel)
	{
		Print(string.Format("[VON] Server notifying player %1 of join request from player %2 for channel %3", targetPlayerId, requesterId, channel), LogLevel.NORMAL);
		
		// Telemetry: 3 ints
		LogTelemetry("NotifyChannelJoinRequest", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 3);
		
		#ifdef WORKBENCH
		RpcDo_NotifyChannelJoinRequest(targetPlayerId, requesterId, channel);
		#else
		Rpc(RpcDo_NotifyChannelJoinRequest, targetPlayerId, requesterId, channel);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void TestTargetedBroadcast(int targetPlayerId, int testValue)
	{
		// Telemetry: 2 ints
		LogTelemetry("TestTargetedBroadcast", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		#ifdef WORKBENCH
		RpcDo_TestTargetedBroadcast(targetPlayerId, testValue);
		#else
		Rpc(RpcDo_TestTargetedBroadcast, targetPlayerId, testValue);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void TestChannelCreatorRPC(int creatorPlayerId)
	{
		// Telemetry: int
		LogTelemetry("TestChannelCreatorRPC", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_TestChannelCreatorRPC(creatorPlayerId);
		#else
		Rpc(RpcDo_TestChannelCreatorRPC, creatorPlayerId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void TestBroadcastConnectivity(int testId)
	{
		// Telemetry: int
		LogTelemetry("TestBroadcastConnectivity", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_TestBroadcastConnectivity(testId);
		#else
		Rpc(RpcDo_TestBroadcastConnectivity, testId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void ConfirmRequestReceived(int requestId, int channel)
	{
		// Telemetry: 2 ints
		LogTelemetry("ConfirmRequestReceived", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		#ifdef WORKBENCH
		RpcDo_ConfirmRequestReceived(requestId, channel);
		#else
		Rpc(RpcDo_ConfirmRequestReceived, requestId, channel);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void Deny(int playerId, int requestId)
	{
		// Telemetry: 2 ints
		LogTelemetry("Deny", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		#ifdef WORKBENCH
		RpcDo_Deny(playerId, requestId);
		#else
		Rpc(RpcDo_Deny, playerId, requestId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void NotifyRequestAccepted(int requesterId)
	{
		// Telemetry: int
		LogTelemetry("NotifyRequestAccepted", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_NotifyRequestAccepted(requesterId);
		#else
		Rpc(RpcDo_NotifyRequestAccepted, requesterId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void NotifyRequestDenied(int requesterId)
	{
		// Telemetry: int
		LogTelemetry("NotifyRequestDenied", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_NotifyRequestDenied(requesterId);
		#else
		Rpc(RpcDo_NotifyRequestDenied, requesterId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void PlayRushMCOMSound(string soundEvent, vector position)
	{
		// Telemetry: string + vector
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(soundEvent);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Vector();
		LogTelemetry("PlayRushMCOMSound", bytes);
		
		#ifdef WORKBENCH
		RpcDo_PlayRushMCOMSound(soundEvent, position);
		#else
		Rpc(RpcDo_PlayRushMCOMSound, soundEvent, position);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void StopRushMCOMSound(string soundEvent, vector position)
	{
		// Telemetry: string + vector
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(soundEvent);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Vector();
		LogTelemetry("StopRushMCOMSound", bytes);
		
		#ifdef WORKBENCH
		RpcDo_StopRushMCOMSound(soundEvent, position);
		#else
		Rpc(RpcDo_StopRushMCOMSound, soundEvent, position);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateGroupLeaderMarker(int playerId, string groupName)
	{
		// Telemetry: int + string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(groupName);
		LogTelemetry("CreateGroupLeaderMarker", bytes);
		
		#ifdef WORKBENCH
		RpcDo_CreateGroupLeaderMarker(playerId, groupName);
		#else
		Rpc(RpcDo_CreateGroupLeaderMarker, playerId, groupName);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveGroupLeaderMarker(int playerId)
	{
		// Telemetry: int
		LogTelemetry("RemoveGroupLeaderMarker", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_RemoveGroupLeaderMarker(playerId);
		#else
		Rpc(RpcDo_RemoveGroupLeaderMarker, playerId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void ClearAllGroupLeaderMarkers()
	{
		// Telemetry: no parameters
		LogTelemetry("ClearAllGroupLeaderMarkers", 0);
		
		#ifdef WORKBENCH
		RpcDo_ClearAllGroupLeaderMarkers();
		#else
		Rpc(RpcDo_ClearAllGroupLeaderMarkers);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestGroupLeaderMarkerState()
	{
		// Telemetry: int
		LogTelemetry("RequestGroupLeaderMarkerState", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		#ifdef WORKBENCH
		RpcDo_RequestGroupLeaderMarkerState(SCR_PlayerController.GetLocalPlayerId());
		#else
		Rpc(RpcDo_RequestGroupLeaderMarkerState, SCR_PlayerController.GetLocalPlayerId());
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateGroupLeaderMarkerForPlayer(int targetPlayerId, int leaderPlayerId, string groupName)
	{
		// Telemetry: 2 ints + string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(groupName);
		LogTelemetry("CreateGroupLeaderMarkerForPlayer", bytes);
		
		#ifdef WORKBENCH
		RpcDo_CreateGroupLeaderMarkerForPlayer(targetPlayerId, leaderPlayerId, groupName);
		#else
		Rpc(RpcDo_CreateGroupLeaderMarkerForPlayer, targetPlayerId, leaderPlayerId, groupName);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void DeleteRushMCOMEntity(string mcomIdentifier)
	{
		if (!Replication.IsServer())
			return;
		
		// Telemetry: string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(mcomIdentifier);
		LogTelemetry("DeleteRushMCOMEntity", bytes);
			
		#ifdef WORKBENCH
		RpcDo_DeleteRushMCOMEntity(mcomIdentifier);
		#else
		Rpc(RpcDo_DeleteRushMCOMEntity, mcomIdentifier);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void MoveSpecCamToSlot(vector slotPos, int playerId)
	{
		// Telemetry: vector + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Vector();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("MoveSpecCamToSlot", bytes);
		
		#ifdef WORKBENCH
		RpcDo_MoveSpecCamToSlot(slotPos, playerId);
		#else
		Rpc(RpcDo_MoveSpecCamToSlot, slotPos, playerId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	// GunGame: Update player stats
	//------------------------------------------------------------------------------------------------
	void UpdateGunGamePlayerStats(int playerId, int level, int killsThisLevel, int totalKills)
	{
		// Telemetry: 4 ints = 16 bytes
		LogTelemetry("UpdateGunGamePlayerStats", 16);
		
		#ifdef WORKBENCH
		RpcDo_UpdateGunGamePlayerStats(playerId, level, killsThisLevel, totalKills);
		#else
		Rpc(RpcDo_UpdateGunGamePlayerStats, playerId, level, killsThisLevel, totalKills);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	// FactionManager: Update SR radio channels
	//------------------------------------------------------------------------------------------------
	void UpdateFactionChannelsSR(string factionId, array<string> channels)
	{
		// Telemetry: factionId string + array of strings
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(factionId);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_StringArray(channels);
		LogTelemetry("UpdateFactionChannelsSR", bytes);
		
		#ifdef WORKBENCH
		RpcDo_UpdateFactionChannelsSR(factionId, channels);
		#else
		Rpc(RpcDo_UpdateFactionChannelsSR, factionId, channels);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	// FactionManager: Update LR radio channels
	//------------------------------------------------------------------------------------------------
	void UpdateFactionChannelsLR(string factionId, array<string> channels)
	{
		// Telemetry: factionId string + array of strings
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(factionId);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_StringArray(channels);
		LogTelemetry("UpdateFactionChannelsLR", bytes);
		
		#ifdef WORKBENCH
		RpcDo_UpdateFactionChannelsLR(factionId, channels);
		#else
		Rpc(RpcDo_UpdateFactionChannelsLR, factionId, channels);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	// GearscriptManager: Add vehicle supply cost
	//------------------------------------------------------------------------------------------------
	void AddVehicleSupplyCost(ResourceName vehicleResource, int supplyCost)
	{
		// Telemetry: ResourceName string + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(vehicleResource);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("AddVehicleSupplyCost", bytes);
		
		#ifdef WORKBENCH
		RpcDo_AddVehicleSupplyCost(vehicleResource, supplyCost);
		#else
		Rpc(RpcDo_AddVehicleSupplyCost, vehicleResource, supplyCost);
		#endif
	}
	
	//================================================================================================
	// SLOTTING DELTA UPDATES - OPTIMIZED BANDWIDTH
	// Individual field updates instead of full container broadcast
	// Reduces bandwidth by 90%+ compared to UpdateSlotData
	// Uses batching system to collect and send multiple updates together
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot player ID only (~8 bytes vs 366 bytes)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerIdDelta(int slotId, int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.PLAYER_ID);
			batch.m_iIntValue = playerId;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotPlayerIdUpdate(slotId, playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot character only (~8 bytes)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacterDelta(int slotId, RplId characterId)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.CHARACTER);
			batch.m_RplIdValue = characterId;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotCharacterUpdate(slotId, characterId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot group only (~8 bytes)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroupDelta(int slotId, RplId groupId)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.GROUP);
			batch.m_RplIdValue = groupId;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotGroupUpdate(slotId, groupId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot resource only (~20-60 bytes depending on path length)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResourceDelta(int slotId, ResourceName resource)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.RESOURCE);
			batch.m_ResourceValue = resource;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotResourceUpdate(slotId, resource);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot locked state only (~5 bytes)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedDelta(int slotId, bool isLocked)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.LOCKED);
			batch.m_bBoolValue = isLocked;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotLockedUpdate(slotId, isLocked);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update slot death state only (~5 bytes)
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathDelta(int slotId, bool isDead)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_bBatchingEnabled)
		{
			CRF_SlotUpdateBatch batch = new CRF_SlotUpdateBatch(slotId, CRF_ESlotUpdateField.DEATH);
			batch.m_bBoolValue = isDead;
			QueueSlotUpdate(batch);
		}
		else
		{
			SendSlotDeathUpdate(slotId, isDead);
		}
	}
	
	
	//================================================================================================
	// CLIENT RPC HANDLERS
	// These methods execute on client machines when receiving server RPCs
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// Helper method to get chat component
	//------------------------------------------------------------------------------------------------
	protected SCR_ChatComponent GetLocalChatComponent()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return null;
			
		return SCR_ChatComponent.Cast(playerController.FindComponent(SCR_ChatComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	// Check if current player matches target player ID
	//------------------------------------------------------------------------------------------------
	protected bool IsLocalPlayer(int playerId)
	{
		return SCR_PlayerController.GetLocalPlayerId() == playerId;
	}
	
	//------------------------------------------------------------------------------------------------
	// Find MCOM entity at specified position
	//------------------------------------------------------------------------------------------------
	protected IEntity FindMCOMEntityAtPosition(vector position)
	{
		Print("[CRF_RplBroadcastManager] FindMCOMEntityAtPosition: Looking for MCOM at " + position);
		
		// Method 1: Try to get MCOM entity from Rush gamemode manager (most reliable)
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			Print("[CRF_RplBroadcastManager] Found rush gamemode, checking all MCOMs");
			
			// Check all MCOM entities from the gamemode
			array<string> mcomIdentifiers = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
			foreach (string identifier : mcomIdentifiers)
			{
				IEntity mcomEntity = rushGamemode.GetMCOMEntity(identifier);
				if (mcomEntity)
				{
					float distance = vector.Distance(mcomEntity.GetOrigin(), position);
					Print("[CRF_RplBroadcastManager] Checking " + identifier + " at " + mcomEntity.GetOrigin() + ", distance: " + distance);
					
					if (distance <= 15.0) // Allow some tolerance for positioning
					{
						// Verify it has a SoundComponent
						SoundComponent soundComp = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
						if (soundComp)
						{
							Print("[CRF_RplBroadcastManager] Found matching MCOM: " + identifier);
							return mcomEntity;
						}
					}
				}
			}
		}
		
		// Method 2: Fallback - try to find entity by name
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			Print("[CRF_RplBroadcastManager] No world found");
			return null;
		}
		
		// Try to find MCOM entities by their common naming patterns
		array<string> mcomNames = {
			"zone1_alpha", "zone1_beta", 
			"zone2_alpha", "zone2_beta", 
			"zone3_alpha", "zone3_beta"
		};
		
		foreach (string mcomName : mcomNames)
		{
			IEntity entity = world.FindEntityByName(mcomName);
			if (entity)
			{
				// Check if this entity is close to our target position
				float distance = vector.Distance(entity.GetOrigin(), position);
				Print("[CRF_RplBroadcastManager] Found entity by name: " + mcomName + " at " + entity.GetOrigin() + ", distance: " + distance);
				
				if (distance <= 15.0) // Allow some tolerance for positioning
				{
					// Verify it has a SoundComponent
					SoundComponent soundComp = SoundComponent.Cast(entity.FindComponent(SoundComponent));
					if (soundComp)
					{
						Print("[CRF_RplBroadcastManager] Found matching MCOM by name: " + mcomName);
						return entity;
					}
				}
			}
		}
		
		Print("[CRF_RplBroadcastManager] No MCOM entity found at position " + position);
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PopUpNotification(float life, string titleText, string subtitleText, string sound, string titleTextParam1, string titleTextParam2)
	{
		if (!sound.IsEmpty())
			AudioSystem.PlaySound(sound);
	
		SCR_PopUpNotification.GetInstance().PopupMsg(string.Format(titleText, titleTextParam1, titleTextParam2), life, subtitleText);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendAdminMessage(string data, int playerID, bool ticketExists)
	{
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);
		SCR_ChatComponent chatComponent = GetLocalChatComponent();
		if (!chatComponent)
			return;

		// If it's an admin just show the message in chat
		if (SCR_Global.IsAdmin(playerID) || m_GamemodeManager.IsModerator(playerID))
		{
			// Don't double show message
			if (GetGame().GetPlayerController().GetPlayerId() == playerID)
				return;
			
			chatComponent.ShowMessage(string.Format("Admin - %1: %2", playerName, data));
		} else {
			// Check if it's a new ticket and let admins know
			if (!ticketExists)
				chatComponent.ShowMessage(string.Format("%1 has created a ticket", playerName));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_GetOpenTickets(int playerID, array<int> tickets)
	{
		if (!IsLocalPlayer(playerID))
			return;
		
		// Check if the top menu is the admin menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (!topMenu)
			return;
			
		if (!topMenu.IsInherited(CRF_AdminMenu))
			return;
			
		// Repopulate menu components
		CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);
		if (adminMenu.GetCurrentOpenTab() == "Tickets")
		{
			adminMenu.PopulateOpenTicketList(tickets);
		}
		
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_GetTicketMessages(int playerID, array<ref CRF_TicketMessageData> messages)
	{
		if (!IsLocalPlayer(playerID))
			return;
		
		// Check if the top menu is the admin menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (!topMenu)
			return;
			
		if (!topMenu.IsInherited(CRF_AdminMenu))
			return;
			
		// Repopulate menu components
		CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);
		if (adminMenu.GetCurrentOpenTab() == "Tickets")
		{
			adminMenu.PopulateTicketMessages(messages);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("Reply to %1: %2", 
				GetGame().GetPlayerManager().GetPlayerName(playerId), data), 
				playerId, 
				false);
			
		if (!IsLocalPlayer(playerId))
			return;

		SCR_ChatComponent chatComponent = GetLocalChatComponent();
		if (!chatComponent)
			return;

		chatComponent.ShowMessage(string.Format("Admin: %1", data));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		string adminName = GetGame().GetPlayerManager().GetPlayerName(adminID);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(ticketID);
		
		if (logAction)
			LogAdminAction(string.Format("%1 closed %2's ticket", adminName, playerName), -1, false);
		
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		SCR_ChatComponent chatComponent = GetLocalChatComponent();
		if (!chatComponent)
			return;
		
		// Display an admin closed a ticket
		chatComponent.ShowMessage(string.Format("%1 closed %2's ticket", adminName, playerName));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_NotifiyTicketAssigned(int ticketID, int adminID, bool logAction)
	{
		string adminName = GetGame().GetPlayerManager().GetPlayerName(adminID);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(ticketID);
		
		if (logAction)
			LogAdminAction(string.Format("%1 assigned to %2's ticket", adminName, playerName), -1, false);
		
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		SCR_ChatComponent chatComponent = GetLocalChatComponent();
		if (!chatComponent)
			return;

		// Display an admin assigned them self to the ticket
		chatComponent.ShowMessage(string.Format("%1 assigned to %2's ticket", adminName, playerName));
	}
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_RefreshAdminMenuLists()
	{
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		m_AdminMenuManager.GetInstance().RefreshLists();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TeleportPlayers(int playerId1, int playerId2, vector player2Origin, bool logAction)
	{
		if (logAction)
		{
			string player1Name = GetGame().GetPlayerManager().GetPlayerName(playerId1);
			string player2Name = GetGame().GetPlayerManager().GetPlayerName(playerId2);
			LogAdminAction(string.Format("%1 was teleported to %2", player1Name, player2Name), playerId1, true);
		}
		
		if (!IsLocalPlayer(playerId1))
			return;
	
		// Find a safe spot near the destination
		vector finalSpawnLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, player2Origin, 3);
		
		// Perform the teleportation
		SCR_Global.TeleportLocalPlayer(finalSpawnLocation, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendHint(string data, int playerId, string factionKey)
	{
		// Check if this hint is for this specific player
		if (playerId != -1 && !IsLocalPlayer(playerId))
			return;
		
		// Check player faction
		SCR_Faction localFaction = SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction());
		if (!localFaction)
			return;

		// Check if hint is for specific faction
		if (!factionKey.IsEmpty() && (localFaction.GetFactionKey() != factionKey))
			return;

		// Create hint widget
		Widget widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");
		if (!widget)
			return;
		
		// Update player controller with hint
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		if (playerControllerComp.m_wSavedHintWidget)
		{
			delete playerControllerComp.m_wSavedHintWidget;
		}

		playerControllerComp.m_wSavedHintWidget = widget;

		// Display the hint
		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		hint.ShowHint(data, 8000);
	}	
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_Closemap(int playerId)
	{	
		// Check if this close is for this specific player
		if (playerId != -1 && !IsLocalPlayer(playerId))
			return;
		
		// Close the map if open
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			if (topMenu.IsInherited(SCR_MapMenuUI))
				topMenu.Close();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_HolsterGun(int playerId)
	{	
		// Check if this holster is for this specific player
		if (playerId != -1 && !IsLocalPlayer(playerId))
			return;
		
		IEntity controlledEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (!controlledEntity)
			return;
		
		CharacterControllerComponent characterController = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
		if (!characterController)
			return;

		// Force player to holster gun
		characterController.SelectWeapon(null);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		// Add the log to the admin menu logs
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
		{	
			m_AdminMenuManager.StoreAdminLogs(data);
		}
		
		// Only send the log to target player if required
		if (sendToPlayer)
		{
			if (!IsLocalPlayer(playerId))
				return;

			SCR_ChatComponent chatComponent = GetLocalChatComponent();
			if (!chatComponent)
				return;
			
			// Show the target player the log messages
			chatComponent.ShowMessage(data);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRespawnScreenUpdate(RplId rplID, bool active)
	{
		CRF_RespawnManager.GetInstance().OnRespawnPointStateChanged().Invoke(rplID, active);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRespawnScreen(int playerId)
	{
		if (!IsLocalPlayer(playerId))
			return;

		// Close any open menus
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
		{
			topMenu.Close();
		}

		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open respawn menu
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);

		
		// Set up respawn timers
		m_RespawnManager.m_iLocalTimeToRespawn = m_RespawnManager.m_iCurrentTimeToRespawn;
		m_RespawnManager.m_fRespawnTimer = (float)m_RespawnManager.GetCurrentWaveTimer();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendCharacterLoadingScreen(int playerId)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != playerId)
			return;
		
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_CharacterLoading);
	};
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_InitilizePlayerBroadcast(int playerId, RplId playerCharID)
	{
		if (!IsLocalPlayer(playerId))
			return;

		CRF_PlayerControllerManager.GetInstance().InitilizePlayerClient(playerCharID);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRequest(int playerId, int requestId, int channel)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[VON] RpcDo_SendRequest received: targetPlayerId=%1, requestId=%2, channel=%3, localPlayerId=%4", playerId, requestId, channel, localPlayerId), LogLevel.NORMAL);
		
		// Only show the request popup for the intended recipient (channel creator)
		if (!IsLocalPlayer(playerId))
		{
			Print(string.Format("[VON] Not local player, ignoring request (target=%1, local=%2)", playerId, localPlayerId), LogLevel.NORMAL);
			return;
		}

		Print(string.Format("[VON] Processing join request for local player"), LogLevel.NORMAL);

		// Check if spectator menu is available
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		Print(string.Format("[VON] Top menu: %1", topMenu), LogLevel.NORMAL);

		// Try to get the spectator menu with retry logic
		CRF_SpectatorMenu specMenu = CRF_SpectatorMenu.Cast(topMenu);
		if (!specMenu)
		{
			Print(string.Format("[VON] Spectator menu not available (topMenu=%1), retrying in 100ms", topMenu), LogLevel.NORMAL);
			// If spectator menu isn't available immediately, try again after a short delay
			GetGame().GetCallqueue().CallLater(RpcDo_SendRequest, 100, false, playerId, requestId, channel);
			return;
		}
		
		Print(string.Format("[VON] Found spectator menu: %1", specMenu), LogLevel.NORMAL);
		
		// Find the Requests widget container
		Widget requestsContainer = specMenu.m_wRoot.FindAnyWidget("Requests");
		if (!requestsContainer)
		{
			// Requests container not found, skip this request
			return;
		}
			
		// Create request widget
		Widget compWidget = GetGame().GetWorkspace().CreateWidgets(
			"{49490337615BA9B8}UI/Listbox/VONChannelRequestListBox.layout",
			requestsContainer
		);
		
		if (!compWidget)
			return;
		
		specMenu.m_aRequest.Insert(compWidget);
		
		// Configure the request component
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(compWidget.FindHandler(CRF_ListBoxElementComponent));
		if (!comp)
			return;
			
		comp.m_iPlayerId = requestId;
		comp.m_iChannelId = channel;
		comp.GetAccept().m_OnClicked.Insert(m_MenuManager.Accept);
		comp.GetDeny().m_OnClicked.Insert(m_MenuManager.Deny);
		FrameSlot.SetPosX(compWidget.FindAnyWidget("ButtonAnim"), 500);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_Deny(int playerId, int requestId)
	{
		if (!IsLocalPlayer(playerId))
			return;

		CRF_SpectatorMenu specMenu = CRF_SpectatorMenu.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!specMenu)
			return;

		// Find and mark the request for deletion
		foreach (Widget request : specMenu.m_aRequest)
		{
			CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(request.FindHandler(CRF_ListBoxElementComponent));
			if (!comp)
				continue;

			if (requestId == comp.m_iPlayerId)
			{
				comp.GetAccept().m_OnClicked.Clear();
				comp.GetDeny().m_OnClicked.Clear();
				comp.m_bDeleteRequest = true;
				return;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_NotifyRequestAccepted(int requesterId)
	{
		if (!IsLocalPlayer(requesterId))
			return;
			
	// Play acceptance sound using UI sound system - more reliable
	SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_DEPLOYED_RADIO_ENTER_ZONE);
	
	// Show notification - PERFORMANCE OPTIMIZATION: cache GetInstance()
	SCR_PopUpNotification popupNotification = SCR_PopUpNotification.GetInstance();
	if (popupNotification)
		popupNotification.PopupMsg("Request Accepted", 3.0);
}	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_NotifyRequestDenied(int requesterId)
	{
		if (!IsLocalPlayer(requesterId))
			return;
			
	// Play denial sound using UI sound system - more reliable
	SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_INV_DROP_ERROR);
	
	// Show notification - PERFORMANCE OPTIMIZATION: cache GetInstance()
	SCR_PopUpNotification popupNotification = SCR_PopUpNotification.GetInstance();
	if (popupNotification)
		popupNotification.PopupMsg("Request Denied", 3.0);
}	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TestTargetedBroadcast(int targetPlayerId, int testValue)
	{
		if (!IsLocalPlayer(targetPlayerId))
			return;
			
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[VON] TARGETED BROADCAST TEST: Player %1 received targeted RPC (testValue=%2)", 
			localPlayerId, testValue), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TestChannelCreatorRPC(int creatorPlayerId)
	{
		if (!IsLocalPlayer(creatorPlayerId))
			return;
			
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[VON] CHANNEL CREATOR TEST: Player %1 received RPC after creating channel", localPlayerId), LogLevel.NORMAL);
		
		// Check current menu state
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		string menuType = "None";
		if (topMenu)
			menuType = topMenu.Type().ToString();
		
		Print(string.Format("[VON] Channel creator menu state: %1", menuType), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TestBroadcastConnectivity(int testId)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		
		// Check current game state
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		string menuType = "None";
		if (topMenu)
			menuType = topMenu.Type().ToString();
		
		Print(string.Format("[VON] Broadcast test received by Player %1 (testId=%2, menu=%3)", 
			localPlayerId, testId, menuType), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ConfirmRequestReceived(int requestId, int channel)
	{
		Print(string.Format("[VON] Confirmation received: Player %1 received join request for channel %2", 
			SCR_PlayerController.GetLocalPlayerId(), channel), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PlayRushMCOMSound(string soundEvent, vector position)
	{
		Print("[CRF_RplBroadcastManager] RpcDo_PlayRushMCOMSound received on client: " + soundEvent + " at " + position);
		
		// Find the MCOM entity at the specified position
		IEntity mcomEntity = FindMCOMEntityAtPosition(position);
		if (!mcomEntity)
		{
			Print("[CRF_RplBroadcastManager] Could not find MCOM entity at position: " + position.ToString(), LogLevel.WARNING);
			return;
		}
		
		Print("[CRF_RplBroadcastManager] Found MCOM entity for sound playback");
		
		// Get the SoundComponent from the MCOM entity
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (!soundComponent)
		{
			Print("[CRF_RplBroadcastManager] No SoundComponent found on MCOM entity", LogLevel.WARNING);
			return;
		}
		
		// Play the sound event using the SoundComponent
		AudioHandle soundHandle = soundComponent.SoundEvent(soundEvent);
		Print("[CRF_RplBroadcastManager] Playing 3D sound: " + soundEvent + " at position: " + position.ToString());
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_StopRushMCOMSound(string soundEvent, vector position)
	{
		// Find the MCOM entity at the specified position
		IEntity mcomEntity = FindMCOMEntityAtPosition(position);
		if (!mcomEntity)
		{
			Print("[CRF_RplBroadcastManager] Could not find MCOM entity at position: " + position.ToString(), LogLevel.WARNING);
			return;
		}
		
		// Get the SoundComponent from the MCOM entity
		SoundComponent soundComponent = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
		if (!soundComponent)
		{
			Print("[CRF_RplBroadcastManager] No SoundComponent found on MCOM entity", LogLevel.WARNING);
			return;
		}
		
		// Terminate all sounds on this component (as we can't target specific events)
		soundComponent.TerminateAll();
		Print("[CRF_RplBroadcastManager] Stopped all sounds on MCOM at position: " + position.ToString());
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_CreateGroupLeaderMarker(int playerId, string groupName)
	{
		CRF_GroupLeaderMarkerManager markerManager = CRF_GroupLeaderMarkerManager.GetInstance();
		if (markerManager)
		{
			markerManager.CreateMarkerForPlayerRPC(playerId, groupName);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_RemoveGroupLeaderMarker(int playerId)
	{
		CRF_GroupLeaderMarkerManager markerManager = CRF_GroupLeaderMarkerManager.GetInstance();
		if (markerManager)
		{
			markerManager.RemoveMarkerForPlayerRPC(playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ClearAllGroupLeaderMarkers()
	{
		CRF_GroupLeaderMarkerManager markerManager = CRF_GroupLeaderMarkerManager.GetInstance();
		if (markerManager)
		{
			markerManager.ClearAllMarkersRPC();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_RequestGroupLeaderMarkerState(int requestingPlayerId)
	{
		CRF_GroupLeaderMarkerManager markerManager = CRF_GroupLeaderMarkerManager.GetInstance();
		if (markerManager)
		{
			markerManager.SendCurrentStateToClient(requestingPlayerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_CreateGroupLeaderMarkerForPlayer(int targetPlayerId, int leaderPlayerId, string groupName)
	{
		// Only create marker for the target player
		if (SCR_PlayerController.GetLocalPlayerId() != targetPlayerId)
			return;
		
		CRF_GroupLeaderMarkerManager markerManager = CRF_GroupLeaderMarkerManager.GetInstance();
		if (markerManager)
		{
			markerManager.CreateMarkerForPlayerRPC(leaderPlayerId, groupName);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_DeleteRushMCOMEntity(string mcomIdentifier)
	{
		Print("[CRF_RplBroadcastManager] RpcDo_DeleteRushMCOMEntity received: " + mcomIdentifier + " (Server: " + Replication.IsServer() + ")");
		
		// Get the Rush gamemode manager
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (!rushGamemode)
		{
			Print("[CRF_RplBroadcastManager] Could not find Rush gamemode manager", LogLevel.WARNING);
			return;
		}
		
		// On clients, set the destroyed status FIRST before any marker operations
		if (!Replication.IsServer())
		{
			// Set the destroyed status in the gamemode arrays so RefreshMapMarkers knows to exclude this MCOM
			rushGamemode.SetMCOMDestroyedStatusFromRPC(mcomIdentifier, true);
			Print("[CRF_RplBroadcastManager] Set destroyed status for: " + mcomIdentifier);
			
			// Clear all markers and refresh with proper destroyed state
			CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
			if (playerControllerManager)
			{
				playerControllerManager.RemoveALLScriptedMarkers();
				Print("[CRF_RplBroadcastManager] Removed all map markers for MCOM destruction: " + mcomIdentifier);
				
				// Immediately refresh markers with proper destroyed status
				rushGamemode.RefreshMapMarkers();
				Print("[CRF_RplBroadcastManager] Refreshed map markers with destroyed status for: " + mcomIdentifier);
			}
		}
		
		// Get the MCOM entity to delete
		IEntity mcomEntity = rushGamemode.GetMCOMEntity(mcomIdentifier);
		if (!mcomEntity)
		{
			Print("[CRF_RplBroadcastManager] MCOM entity not found (may be already deleted): " + mcomIdentifier);
			// Entity might already be deleted - just clean up references
			rushGamemode.CleanupMCOMReference(mcomIdentifier);
			return;
		}
		
		Print("[CRF_RplBroadcastManager] Processing MCOM deletion: " + mcomIdentifier + " (Entity ID: " + mcomEntity.GetID() + ")");
		
		// Always hide the 3D marker component first
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (markerComponent)
			markerComponent.SetVisible(false);
		
		// Always clean up gamemode references
		rushGamemode.CleanupMCOMReference(mcomIdentifier);
		
		// Handle entity deletion based on whether we're server or client
		if (Replication.IsServer())
		{
			// On server, the entity will be deleted by the main deletion logic
			Print("[CRF_RplBroadcastManager] Server: Skipping entity deletion (handled by main logic)");
		}
		else
		{
			// On client, delete the entity immediately
			Print("[CRF_RplBroadcastManager] Client: Deleting entity immediately for: " + mcomIdentifier);
			SCR_EntityHelper.DeleteEntityAndChildren(mcomEntity);
			Print("[CRF_RplBroadcastManager] Client: Successfully deleted entity: " + mcomIdentifier);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_NotifyChannelJoinRequest(int targetPlayerId, int requesterId, int channel)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[VON] RpcDo_NotifyChannelJoinRequest received: targetPlayerId=%1, requesterId=%2, channel=%3, localPlayerId=%4", targetPlayerId, requesterId, channel, localPlayerId), LogLevel.NORMAL);
		
		// Only show the request popup for the intended recipient (channel creator)
		if (!IsLocalPlayer(targetPlayerId))
		{
			Print(string.Format("[VON] Not target player, ignoring request (target=%1, local=%2)", targetPlayerId, localPlayerId), LogLevel.NORMAL);
			return;
		}

		Print(string.Format("[VON] Processing join request for local player (channel creator)"), LogLevel.NORMAL);

		// Check if spectator menu is available
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		Print(string.Format("[VON] Top menu: %1", topMenu), LogLevel.NORMAL);

		// Try to get the spectator menu with retry logic
		CRF_SpectatorMenu specMenu = CRF_SpectatorMenu.Cast(topMenu);
		if (!specMenu)
		{
			Print(string.Format("[VON] Spectator menu not available (topMenu=%1), retrying in 100ms", topMenu), LogLevel.NORMAL);
			// If spectator menu isn't available immediately, try again after a short delay
			GetGame().GetCallqueue().CallLater(RpcDo_NotifyChannelJoinRequest, 100, false, targetPlayerId, requesterId, channel);
			return;
		}
		
		Print(string.Format("[VON] Found spectator menu: %1", specMenu), LogLevel.NORMAL);
		
		// Find the Requests widget container
		Widget requestsContainer = specMenu.m_wRoot.FindAnyWidget("Requests");
		if (!requestsContainer)
		{
			Print(string.Format("[VON] Requests container not found, skipping request"), LogLevel.WARNING);
			return;
		}
			
		// Create request widget
		Widget compWidget = GetGame().GetWorkspace().CreateWidgets(
			"{49490337615BA9B8}UI/Listbox/VONChannelRequestListBox.layout",
			requestsContainer
		);
		
		if (!compWidget)
		{
			Print(string.Format("[VON] Failed to create request widget"), LogLevel.ERROR);
			return;
		}
		
		specMenu.m_aRequest.Insert(compWidget);
		
		// Configure the request component
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(compWidget.FindHandler(CRF_ListBoxElementComponent));
		if (!comp)
		{
			Print(string.Format("[VON] Failed to get list box component"), LogLevel.ERROR);
			return;
		}
			
		comp.m_iPlayerId = requesterId;
		comp.m_iChannelId = channel;
		comp.GetAccept().m_OnClicked.Insert(m_MenuManager.Accept);
		comp.GetDeny().m_OnClicked.Insert(m_MenuManager.Deny);
		
		// Get requester's name for the popup
		string requesterName = GetGame().GetPlayerManager().GetPlayerName(requesterId);
		comp.SetPlayerText(requesterName + " requested to join");

		Print(string.Format("[VON] Successfully created join request popup for %1", requesterName), LogLevel.NORMAL);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastMessage(string message)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(message);
	}	
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_MoveSpecCamToSlot(vector slotPos, int targetPlayerId)
	{		
		if (!IsLocalPlayer(targetPlayerId))
			return;
		
		// Get the current camera
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera)
			return;

		// Find the teleport component and use it to move the camera
		SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(
			camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent)
		);
		
		if (teleportComponent)
		{
			teleportComponent.TeleportCamera(slotPos, true, false);
		}
		
		// Try attach spec cam again
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (!topMenu)
			return;
			
		if (!topMenu.IsInherited(CRF_SpectatorMenu))
			return;
			
		CRF_SpectatorMenu spectatorMenu = CRF_SpectatorMenu.Cast(topMenu);
		spectatorMenu.SelectSpec();
	}
	
	//------------------------------------------------------------------------------------------------
	// GunGame: Update player stats on all clients
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateGunGamePlayerStats(int playerId, int level, int killsThisLevel, int totalKills)
	{
		CRF_GunGame gunGame = CRF_GunGame.Cast(GetGame().GetGameMode().FindComponent(CRF_GunGame));
		if (gunGame)
			gunGame.UpdatePlayerStatsClient(playerId, level, killsThisLevel, totalKills);
	}
	
	//------------------------------------------------------------------------------------------------
	// FactionManager: Update SR channels on all clients
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateFactionChannelsSR(string factionId, array<string> channels)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
			factionManager.UpdateChannelsSRClient(factionId, channels);
	}
	
	//------------------------------------------------------------------------------------------------
	// FactionManager: Update LR channels on all clients
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateFactionChannelsLR(string factionId, array<string> channels)
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionManager)
			factionManager.UpdateChannelsLRClient(factionId, channels);
	}
	
	//------------------------------------------------------------------------------------------------
	// GearscriptManager: Add vehicle supply cost on all clients
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_AddVehicleSupplyCost(ResourceName vehicleResource, int supplyCost)
	{
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		if (gearscriptManager)
			gearscriptManager.AddVehicleCostClient(vehicleResource, supplyCost);
	}
	
	//================================================================================================
	// SLOTTING MANAGER BROADCAST METHODS
	// Delta-based slot updates to replace array replication (98% bandwidth reduction)
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Update single slot data on all clients (LEGACY - USE DELTA UPDATES INSTEAD)
	// 
	// ⚠️ PERFORMANCE WARNING: This method sends ~366 bytes per call
	// ⚠️ Use UpdateSlot*Delta() methods instead for 90%+ bandwidth savings:
	//    - UpdateSlotPlayerIdDelta()   : 8 bytes (vs 366)
	//    - UpdateSlotCharacterDelta()  : 8 bytes (vs 366)
	//    - UpdateSlotGroupDelta()      : 8 bytes (vs 366)
	//    - UpdateSlotResourceDelta()   : ~40 bytes (vs 366)
	//    - UpdateSlotLockedDelta()     : 5 bytes (vs 366)
	//    - UpdateSlotDeathDelta()      : 5 bytes (vs 366)
	//
	// Only use UpdateSlotData() for:
	//   - Creating new slots (all fields are new)
	//   - JIP sync (initial state transmission)
	//
	// Bandwidth: ~366 bytes vs 8-40 bytes for delta updates
	//------------------------------------------------------------------------------------------------
	void UpdateSlotData(CRF_SlotDataContainer slotData)
	{
		if (!Replication.IsServer())
			return;
		
		// Estimate bandwidth: slot data (~366 bytes avg)
		LogTelemetry("UpdateSlotData", 366);
		
		RpcDo_UpdateSlotData(slotData);
		Rpc(RpcDo_UpdateSlotData, slotData);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotData(CRF_SlotDataContainer slotData)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (slottingManager)
			slottingManager.UpdateSlotDataClient(slotData);
	}
	
	//================================================================================================
	// SLOTTING DELTA UPDATE RPC HANDLERS
	// Client-side handlers for optimized field-specific updates
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotPlayerIdDelta(int slotId, int playerId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetSlotCurrentPlayerId(playerId);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotCharacterDelta(int slotId, RplId characterId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetSlotCurrentCharacter(characterId);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotGroupDelta(int slotId, RplId groupId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetSlotCurrentGroup(groupId);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotResourceDelta(int slotId, ResourceName resource)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetSlotResource(resource);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotLockedDelta(int slotId, bool isLocked)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetIsLockedSlot(isLocked);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateSlotDeathDelta(int slotId, bool isDead)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
		if (slotData)
		{
			slotData.SetIsDeadSlot(isDead);
			slotData.GetOnDataUpdate().Invoke();
			
			// Trigger global slotting update for UI refresh
			ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
			if (invoker)
				invoker.Invoke();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Remove slot from all clients
	//------------------------------------------------------------------------------------------------
	void RemoveSlot(int slotId)
	{
		if (!Replication.IsServer())
			return;
		
		// Bandwidth: Just slotId (4 bytes)
		LogTelemetry("RemoveSlot", 4);
		
		RpcDo_RemoveSlot(slotId);
		Rpc(RpcDo_RemoveSlot, slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_RemoveSlot(int slotId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (slottingManager)
			slottingManager.RemoveSlotClient(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	// SlottingManager: Notify all clients that slotting phase changed (triggers UI refresh)
	//------------------------------------------------------------------------------------------------
	void NotifySlottingPhaseChanged()
	{
		if (!Replication.IsServer())
			return;
		
		// Bandwidth: No parameters (0 bytes, just RPC overhead)
		LogTelemetry("NotifySlottingPhaseChanged", 0);
		
		#ifdef WORKBENCH
		RpcDo_NotifySlottingPhaseChanged();
		#else
		Rpc(RpcDo_NotifySlottingPhaseChanged);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_NotifySlottingPhaseChanged()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		// Trigger global slotting update for UI refresh
		ScriptInvoker invoker = slottingManager.GetOnSlottingUpdate();
		if (invoker)
			invoker.Invoke();
		
		Print("[CRF_RplBroadcastManager] Slotting phase changed - UI updated", LogLevel.VERBOSE);
	}
	
	void BroadcastOutro()
	{
		#ifdef WORKBENCH
		RpcDo_BroadcastOutro();
		#else
		Rpc(RpcDo_BroadcastOutro);
		#endif
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastOutro()
	{
		AudioSystem.PlaySound("{3D7F63CCD32B2F17}Sounds/Intro/outroCrescendo.wav");
		GetGame().GetCallqueue().CallLater(OpenOutro, 2831, false);
	}
	
	void OpenOutro()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_Outro);
	}
	
	void BroadcastVehiclePosUpdate(vector pos, int playerId)
	{
		Rpc(RpcDo_BroadcastVehiclePosUpdate, pos, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastVehiclePosUpdate(vector pos, int playerId)
	{
		SCR_Global.TeleportPlayer(playerId, pos, SCR_EPlayerTeleportedReason.NONE);
	}
};