class CRF_RplBroadcastManagerClass : ScriptComponentClass {}

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
	
	//------------------------------------------------------------------------------------------------
	// Get singleton instance of the broadcast manager
	//------------------------------------------------------------------------------------------------
	static CRF_RplBroadcastManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
		{
			return CRF_RplBroadcastManager.Cast(gameMode.FindComponent(CRF_RplBroadcastManager));
		}
		else
		{
			return null;
		}
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
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
	}
	
	//================================================================================================
	// AUTHORITY BROADCAST METHODS
	// These methods execute on the server to send data to clients
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void PopUpNotification(float life, string titleText, string subtitleText = "", string sound = "", string titleTextParam1 = "", string titleTextParam2 = "")
	{
		#ifdef WORKBENCH
		RpcDo_PopUpNotification(life, titleText, subtitleText, sound, titleTextParam1, titleTextParam2);
		#else
		Rpc(RpcDo_PopUpNotification, life, titleText, subtitleText, sound, titleTextParam1, titleTextParam2);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data, int playerID)
	{
		#ifdef WORKBENCH
		RpcDo_SendAdminMessage(data, playerID);
		#else
		Rpc(RpcDo_SendAdminMessage, data, playerID);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		#ifdef WORKBENCH
		RpcDo_ReplyAdminMessage(data, playerId, adminID, logAction);
		#else
		Rpc(RpcDo_ReplyAdminMessage, data, playerId, adminID, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		#ifdef WORKBENCH
		RpcDo_CloseAdminTicket(ticketID, adminID, logAction);
		#else
		Rpc(RpcDo_CloseAdminTicket, ticketID, adminID, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		#ifdef WORKBENCH
		RpcDo_AssignAdminTicket(ticketID, adminID, logAction);
		#else
		Rpc(RpcDo_AssignAdminTicket, ticketID, adminID, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendGroupIDToPlayer(int requesterID, int groupId)
	{
		#ifdef WORKBENCH
		RpcDo_SendGroupIDToPlayer(requesterID, groupId);
		#else
		Rpc(RpcDo_SendGroupIDToPlayer, requesterID, groupId);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		#ifdef WORKBENCH
		RpcDo_TeleportPlayers(playerId1, playerId2, logAction);
		#else
		Rpc(RpcDo_TeleportPlayers, playerId1, playerId2, logAction);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void Closemap(int playerID)
	{
		#ifdef WORKBENCH
		RpcDo_Closemap(playerID);
		#else
		Rpc(RpcDo_Closemap, playerID);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendHint(string data, int playerId = -1, string factionKey = "")
	{
		#ifdef WORKBENCH
		RpcDo_SendHint(data, playerId, factionKey);
		#else
		Rpc(RpcDo_SendHint, data, playerId, factionKey);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		#ifdef WORKBENCH
		RpcDo_LogAdminAction(data, playerId, sendToPlayer);
		#else
		Rpc(RpcDo_LogAdminAction, data, playerId, sendToPlayer);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendRespawnScreenUpdate(RplId rplID, bool active)
	{
		#ifdef WORKBENCH
		RpcDo_SendRespawnScreenUpdate(rplID, active);
		#else
		Rpc(RpcDo_SendRespawnScreenUpdate, rplID, active);
		#endif
	}	

	//------------------------------------------------------------------------------------------------
	void SendRespawnScreen(int playerId)
	{
		#ifdef WORKBENCH
		RpcDo_SendRespawnScreen(playerId);
		#else
		Rpc(RpcDo_SendRespawnScreen, playerId);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendCharacterLoadingScreen(int playerId)
	{
		#ifdef WORKBENCH
		RpcDo_SendCharacterLoadingScreen(playerId);
		#else
		Rpc(RpcDo_SendCharacterLoadingScreen, playerId);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void InitilizePlayerBroadcast(int playerId, bool isSpectator)
	{
		#ifdef WORKBENCH
		RpcDo_InitilizePlayerBroadcast(playerId, isSpectator);
		#else
		Rpc(RpcDo_InitilizePlayerBroadcast, playerId, isSpectator);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void SendRequest(int playerId, int requestId, int channel)
	{
		#ifdef WORKBENCH
		RpcDo_SendRequest(playerId, requestId, channel);
		#else
		Rpc(RpcDo_SendRequest, playerId, requestId, channel);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void Deny(int playerId, int requestId)
	{
		#ifdef WORKBENCH
		RpcDo_Deny(playerId, requestId);
		#else
		Rpc(RpcDo_Deny, playerId, requestId);
		#endif
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
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PopUpNotification(float life, string titleText, string subtitleText, string sound, string titleTextParam1, string titleTextParam2)
	{
		if (!sound.IsEmpty())
			AudioSystem.PlaySound(sound);
	
		SCR_PopUpNotification.GetInstance().PopupMsg(string.Format(titleText, titleTextParam1, titleTextParam2), life, subtitleText);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendAdminMessage(string data, int playerID)
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
			if (!m_AdminMenuManager.TicketExists(playerID))
				chatComponent.ShowMessage(string.Format("%1 has created a ticket", playerName));
			
			// Create a new ticket or/and add reply to existing ticket
			m_AdminMenuManager.NewTicketMessage(playerID, playerID, data);
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

		// Create a new ticket or add reply to existing ticket
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
			m_AdminMenuManager.NewTicketMessage(playerId, adminID, data);
		
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
		if (logAction)
		{
			string adminName = GetGame().GetPlayerManager().GetPlayerName(adminID);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(ticketID);
			LogAdminAction(string.Format("%1 closed %2's ticket", adminName, playerName), -1, false);
		}

		// Remove the ticket from the array
		m_AdminMenuManager.CloseTicket(ticketID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		if (logAction)
		{
			string adminName = GetGame().GetPlayerManager().GetPlayerName(adminID);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(ticketID);
			LogAdminAction(string.Format("%1 assigned to %2's ticket", adminName, playerName), -1, false);
		}

		// Display a admin assigned them self to the ticket
		m_AdminMenuManager.NewTicketMessage(ticketID, adminID, "Assigned to Ticket");
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendGroupIDToPlayer(int requesterID, int groupId)
	{
		if (!IsLocalPlayer(requesterID) || groupId == -1)
			return;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);

		if (adminMenu)
			adminMenu.UpdateSpawnGroup(groupId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		if (logAction)
		{
			string player1Name = GetGame().GetPlayerManager().GetPlayerName(playerId1);
			string player2Name = GetGame().GetPlayerManager().GetPlayerName(playerId2);
			LogAdminAction(string.Format("%1 was teleported to %2", player1Name, player2Name), playerId1, true);
		}
		
		if (!IsLocalPlayer(playerId1))
			return;

		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		if (!entity2)
			return;
			
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerId1, teleportLocation);
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
	void RpcDo_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		// Add the log to the admin menu logs
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
		{	
			m_AdminMenuManager.LogAdminAction(data);
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
		m_RespawnManager.m_iRespawnTimer = m_RespawnManager.GetCurrentWaveTimer();
		GetGame().GetCallqueue().CallLater(m_RespawnManager.RespawnTimer, 1000, true);
		GetGame().GetCallqueue().CallLater(m_RespawnManager.CloseSlottingMenu, 100, true);
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
	void RpcDo_InitilizePlayerBroadcast(int playerId, bool isSpectator)
	{
		if (!IsLocalPlayer(playerId))
			return;

		CRF_PlayerControllerManager.GetInstance().InitilizePlayerClient(isSpectator);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRequest(int playerId, int requestId, int channel)
	{
		if (!IsLocalPlayer(playerId))
			return;

		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());
		if (!specMenu)
			return;
			
		// Create request widget
		Widget compWidget = GetGame().GetWorkspace().CreateWidgets(
			"{49490337615BA9B8}UI/Listbox/VONChannelRequestListBox.layout",
			specMenu.m_wRoot.FindAnyWidget("Requests")
		);
		
		specMenu.m_aRequest.Insert(compWidget);
		
		// Configure the request component
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(compWidget.FindHandler(CRF_ListBoxElementComponent));
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

		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());
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
};