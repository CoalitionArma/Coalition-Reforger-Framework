class CRF_GamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeComponent : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GamemodeComponent.Cast(gameMode.FindComponent(CRF_GamemodeComponent));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on in-game post init
		// Is the the right way to do this? WHO KNOWS !
		if (!GetGame().InPlayMode())
			return;
		
		m_WeaponConfig = CRF_GearScriptWeaponsConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{AF5B2639B4B12580}Configs/Gearscripts/CRF_Global_Weapons_Config.conf").GetResource().ToBaseContainer()));
		m_EquipmentConfig = CRF_GearScriptEquipmentConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer("{DE26DF4B9B934889}Configs/Gearscripts/CRF_Global_Equipment_Config.conf").GetResource().ToBaseContainer()));

		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		GetGame().GetCallqueue().CallLater(AddMsgAction, 0, false);

		#ifdef WORKBENCH
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(UpdatePlayerGearScriptsArray, m_RNG.RandInt(10000, 20000), true);

			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			GetGame().GetCallqueue().CallLater(UpdatePlayerGearScriptsArray, m_RNG.RandInt(10000, 20000), true);

			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void WaitTillGameStart()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_GamemodeState.GAME)
			return;

		m_bSafeStartEnabled = !CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		GetGame().GetCallqueue().Remove(WaitTillGameStart);
		GetGame().GetCallqueue().CallLater(ToggleSafeStartServer, 1000, false, CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled);
	}

	void OnGamemodeStateChanged()
	{}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Admin Menu Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------

	private Widget m_wSavedHintWidget;

	//------------------------------------------------------------------------------------------------
	// Admin Messaging
	//------------------------------------------------------------------------------------------------
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage_Callback);
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage_Callback);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
		CRF_PlayerControllerComponent.GetInstance().SendAdminMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
		if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
			return;

		CRF_PlayerControllerComponent.GetInstance().ReplyAdminMessage(data, true);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data)
	{
		Rpc(RpcAsk_SendAdminMessage, data);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_SendAdminMessage(string data)
	{
		if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerID, bool logAction)
	{
		Rpc(RpcAsk_ReplyAdminMessage, data, playerID, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_ReplyAdminMessage(string data, int playerID, bool logAction)
	{
		if (logAction)
			LogAdminAction(string.Format("Reply to %1: %2", GetGame().GetPlayerManager().GetPlayerName(playerID), data), playerID, false);

		if (GetGame().GetPlayerController().GetPlayerId() != playerID)
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
	// Respawn
	//------------------------------------------------------------------------------------------------
	void SpawnOnGroupServer(int playerId, vector spawnLocation, int groupID, bool logAction)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		CRF_Gamemode.GetInstance().RespawnPlayer(playerId, spawnLocation, groupID);

		if (logAction)
			LogAdminAction(string.Format("%1 was respawned to %2", GetGame().GetPlayerManager().GetPlayerName(playerId), SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID).m_faction), playerId, true);
	}

	//------------------------------------------------------------------------------------------------
	void SendGroupIDToPlayer(int requestedId, int requesterID)
	{
		CRF_Gamemode gm = CRF_Gamemode.GetInstance();

		if (gm.m_aSlots.Find(requestedId) == -1)
			return;

		RplId groupID = gm.m_aActivePlayerGroupsIDs.Get(gm.m_aGroupRplIDs.Find(gm.m_aPlayerGroupIDs.Get(gm.m_aSlots.Find(requestedId))));
		SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
		if (playerGroup)
		{
			Rpc(RpcDo_SendGroupIDToPlayer, requesterID, playerGroup.GetGroupID());
			RpcDo_SendGroupIDToPlayer(requesterID, playerGroup.GetGroupID());
		};
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
	// Gear
	//------------------------------------------------------------------------------------------------
	void SetPlayerGear(int playerID, ResourceName prefab, bool logAction)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		GetGame().GetCallqueue().CallLater(SetupAddGearToEntity, m_RNG.RandInt(250, 1000), false, entity, prefab);
		SetPlayerGearScriptsMapValue(prefab, playerID, "GSR"); // GSR = Gear Script Resource

		if (logAction)
			LogAdminAction(string.Format("%1's gear was set to %2", GetGame().GetPlayerManager().GetPlayerName(playerID), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	void AddItem(int playerID, string prefab, bool logAction)
	{
		if (playerID == 0 || prefab.IsEmpty())
			return;

		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
			delete resourceSpawned;

		if (logAction)
			LogAdminAction(string.Format("%2 was added to %1's inventory", GetGame().GetPlayerManager().GetPlayerName(playerID), prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	// Teleport
	//------------------------------------------------------------------------------------------------
	void TeleportPlayers(int playerID1, int playerID2, bool logAction)
	{
		Rpc(Rpc_TeleportPlayers, playerID1, playerID2, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_TeleportPlayers(int playerID1, int playerID2, bool logAction)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != playerID1)
			return;

		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerID1, teleportLocation);

		if (logAction)
			LogAdminAction(string.Format("%1 was teleported to %2", GetGame().GetPlayerManager().GetPlayerName(playerID1), GetGame().GetPlayerManager().GetPlayerName(playerID2)), playerID1, true);
	}

	//------------------------------------------------------------------------------------------------
	// Hints
	//------------------------------------------------------------------------------------------------
	void SendHintAll(string data)
	{
		Rpc(Rpc_SendHintAll, data);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintAll(string data)
	{
		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintPlayer(string data, int playerID)
	{
		Rpc(Rpc_SendHintPlayer, data, playerID);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintPlayer(string data, int playerID)
	{
		if (playerID == 0 || SCR_PlayerController.GetLocalPlayerId() != playerID)
			return;

		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendHintFaction(string data, string factionKey)
	{
		Rpc(Rpc_SendHintFaction, data, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintFaction(string data, string factionKey)
	{
		if (factionKey.IsEmpty() && !SCR_FactionManager.SGetLocalPlayerFaction() && (SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction()).GetFactionKey() != factionKey))
			return;

		SendAdminHint(data);
	}

	//------------------------------------------------------------------------------------------------
	void SendAdminHint(string data)
	{
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");

		if (!widget)
			return;

		if (m_wSavedHintWidget)
			delete m_wSavedHintWidget;

		m_wSavedHintWidget = widget;

		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		hint.ShowHint(data, 8000);
	}

	//------------------------------------------------------------------------------------------------
	// Heal
	//------------------------------------------------------------------------------------------------
	void HealPlayer(int playerID, bool logAction)
	{
		IEntity PlayerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(PlayerEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
			LogAdminAction(string.Format("%1's was healed", GetGame().GetPlayerManager().GetPlayerName(playerID)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	void HealPlayerVehicle(int playerID, bool logAction)
	{
		IEntity PlayerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);

		IEntity VehicleEntity = SCR_CompartmentAccessComponent.GetVehicleIn(PlayerEntity);
		if (!VehicleEntity)
			return;

		SCR_DamageManagerComponent damageComponent = SCR_DamageManagerComponent.Cast(VehicleEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageComponent)
			return;

		damageComponent.FullHeal();
		damageComponent.SetHealthScaled(1);

		if (logAction)
			LogAdminAction(string.Format("%1's vehicle was repaired", GetGame().GetPlayerManager().GetPlayerName(playerID)), playerID, true);
	}

	//------------------------------------------------------------------------------------------------
	// Log Admin Actions
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	void SendAdminMessage(string data)
	void LogAdminAction(string data, int playerID, bool sendToPlayer)
	{
		Rpc(RpcAsk_LogAdminAction, data, playerID, sendToPlayer);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_LogAdminAction(string data, int playerID, bool sendToPlayer)
	{
		if (sendToPlayer)
		{
			if (GetGame().GetPlayerController().GetPlayerId() != playerID && (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator()))
				return;
		} else {
			if (!SCR_Global.IsAdmin() && !SCR_Global.IsModerator())
				return;
		}

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Moderator Functions/Variables
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	
	override void OnPlayerAuditSuccess(int playerId)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		if (!playerIdentity || playerIdentity == "")
			return;
		
		if (CRF_ModeratorConfig.IsModerator(playerIdentity))
			GetGame().GetCallqueue().CallLater(SetPlayerModerator, 5000, false, playerId);
	};
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		GetGame().GetPlayerManager().GivePlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	};
}
