class CRF_RplBroadcastManagerClass : ScriptComponentClass {}

class CRF_RplBroadcastManager : ScriptComponent
{
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_MenuManager m_MenuManager;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	
	//------------------------------------------------------------------------------------------------
	static CRF_RplBroadcastManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_RplBroadcastManager.Cast(gameMode.FindComponent(CRF_RplBroadcastManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Get all instances we need for this manager.
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	
	// The stuff that exectues on the authority (and only the authority)
	
	//------------------------------------------------------------------------------------------------
	
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
	void SendRespawnScreen(int playerId)
	{
		#ifdef WORKBENCH
		RpcDo_SendRespawnScreen(playerId);
		#else
		Rpc(RpcDo_SendRespawnScreen, playerId);
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
	
	//------------------------------------------------------------------------------------------------
	
	// The stuff that exectues on the children
	
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PopUpNotification(float life, string titleText, string subtitleText, string sound, string titleTextParam1, string titleTextParam2)
	{
		if(!sound.IsEmpty())
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
		PlayerController pc = GetGame().GetPlayerController();
				if (!pc)
					return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;

		// if its a admin just show the message in chat
		if (SCR_Global.IsAdmin(playerID) || m_GamemodeManager.IsModerator(playerID))
		{
			// Don't double show message
			if (GetGame().GetPlayerController().GetPlayerId() == playerID)
				return;
			
			chatComponent.ShowMessage(string.Format("Admin - %1: %2", playerName, data));
		}
		else
		{
			// Check if its a new ticket and let admins know
			if (!m_AdminMenuManager.isNewTicket(playerID))
			{	
				chatComponent.ShowMessage(string.Format("%1 has created a ticket", playerName));
			}
			
			// Create a new ticket or/and add reply to exsisting ticket
			m_AdminMenuManager.NewTicketMessage(playerID, playerID, data);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("Reply to %1: %2", GetGame().GetPlayerManager().GetPlayerName(playerId), data), playerId, false);

		// Create a new ticket or add reply to exsisting ticket
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
			m_AdminMenuManager.NewTicketMessage(playerId, adminID, data);
		
		if (GetGame().GetPlayerController().GetPlayerId() != playerId)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;

		chatComponent.ShowMessage(string.Format("Admin: %1", data));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("%1 closed %2's ticket", GetGame().GetPlayerManager().GetPlayerName(adminID), GetGame().GetPlayerManager().GetPlayerName(ticketID)), -1, true);

		// Remove the ticket from the array
		m_AdminMenuManager.CloseTicket(ticketID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("%1 assigned to %2's ticket", GetGame().GetPlayerManager().GetPlayerName(adminID), GetGame().GetPlayerManager().GetPlayerName(ticketID)), -1, false);

		// Display a admin assigned them self to the ticket
		m_AdminMenuManager.NewTicketMessage(ticketID, adminID, "Assigned to Ticket");
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendGroupIDToPlayer(int requesterID, int groupId)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != requesterID || groupId == -1)
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
			LogAdminAction(string.Format("%1 was teleported to %2", GetGame().GetPlayerManager().GetPlayerName(playerId1), GetGame().GetPlayerManager().GetPlayerName(playerId2)), playerId1, true);
		
		if (SCR_PlayerController.GetLocalPlayerId() != playerId1)
			return;

		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
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
		if (playerId != -1 && SCR_PlayerController.GetLocalPlayerId() != playerId)
			return;
		
		if (!SCR_FactionManager.SGetLocalPlayerFaction())
			return;

		if (!factionKey.IsEmpty() && (SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction()).GetFactionKey() != factionKey))
			return;

		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");

		if (!widget)
			return;
		
		CRF_PlayerControllerComponent playerControllerComp = CRF_PlayerControllerComponent.GetInstance();

		if (playerControllerComp.m_wSavedHintWidget)
			delete playerControllerComp.m_wSavedHintWidget;

		playerControllerComp.m_wSavedHintWidget = widget;

		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		hint.ShowHint(data, 8000);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		// Add the log to the admin menu logs
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())	
			m_AdminMenuManager.LogAdminAction(data);
		
		// Only send the log to target player
		if (sendToPlayer)
			if (GetGame().GetPlayerController().GetPlayerId() != playerId)
				return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		// Show the target player the log messages
		chatComponent.ShowMessage(data);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRespawnScreen(int playerId)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != playerId)
			return;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();

		GetGame().GetMenuManager().CloseAllMenus();
		MenuBase respawnMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);

		GetGame().GetCallqueue().CallLater(m_RespawnManager.RespawnTimer, 1000, true);
		GetGame().GetCallqueue().CallLater(m_RespawnManager.MenuFuckOff, 100, true);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_InitilizePlayerBroadcast(int playerId, bool isSpectator)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != playerId)
			return;

		CRF_PlayerControllerComponent.GetInstance().InitilizePlayerClient(isSpectator);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRequest(int playerId, int requestId, int channel)
	{
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;

		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());

		if (!specMenu)
			return;
		Widget compWidget = GetGame().GetWorkspace().CreateWidgets("{49490337615BA9B8}UI/Listbox/VONChannelRequestListBox.layout", specMenu.m_wRoot.FindAnyWidget("Requests"));
		specMenu.m_aRequest.Insert(compWidget);
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
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;

		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());

		if (!specMenu)
			return;

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