//------------------------------------------------------------------------------------------------
// Broadcast Manager responsible for handling server-client communications
//------------------------------------------------------------------------------------------------
modded class COA_RplBroadcastManager
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Client-side audio handles for positional sounds, keyed by sound event name
	protected ref map<string, AudioHandle> m_mSoundHandles = new map<string, AudioHandle>();

	// AAR outro — winning faction received from the server (set in RpcDo_BroadcastOutro)
	string m_sOutroWinningFaction = "";

	// Area Timer state — replicated to clients via BroadcastAreaTimerUpdate
	CRF_EAreaTimerState m_eAreaTimerState;
	int m_iAreaTimerCountdown;
	string m_sAreaTimerFaction;
	string m_sAreaTimerZoneLabel;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 AUTHORITY REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void BroadcastMessage(string message)
	{
		// Telemetry: string
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(message);
		LogTelemetry("BroadcastMessage", bytes);
		
		#ifdef WORKBENCH
		RpcDo_BroadcastMessage(message);
		#else
		Rpc(RpcDo_BroadcastMessage, message);
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
		LogTelemetry("SendRequest", COA_BandwidthTelemetryManager.EstimateSize_Int() * 3);
		
		// Send RPC specifically to the target player
		Rpc(RpcDo_SendRequest, playerId, requestId, channel);
	}
	
	//------------------------------------------------------------------------------------------------
	void TestTargetedBroadcast(int targetPlayerId, int testValue)
	{
		// Telemetry: 2 ints
		LogTelemetry("TestTargetedBroadcast", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
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
		LogTelemetry("TestChannelCreatorRPC", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
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
		LogTelemetry("TestBroadcastConnectivity", COA_BandwidthTelemetryManager.EstimateSize_Int());
		
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
		LogTelemetry("ConfirmRequestReceived", COA_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		#ifdef WORKBENCH
		RpcDo_ConfirmRequestReceived(requestId, channel);
		#else
		Rpc(RpcDo_ConfirmRequestReceived, requestId, channel);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void PlayPositionalSound(string resource, string soundEvent, vector position)
	{
		// Telemetry: resource string + event string + vector
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(resource);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(soundEvent);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Vector();
		LogTelemetry("PlayPositionalSound", bytes);
		
		#ifdef WORKBENCH
		RpcDo_PlayPositionalSound(resource, soundEvent, position);
		#else
		Rpc(RpcDo_PlayPositionalSound, resource, soundEvent, position);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void StopPositionalSound(string soundEvent)
	{
		LogTelemetry("StopPositionalSound", COA_BandwidthTelemetryManager.EstimateSize_String(soundEvent));
		
		#ifdef WORKBENCH
		RpcDo_StopPositionalSound(soundEvent);
		#else
		Rpc(RpcDo_StopPositionalSound, soundEvent);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void DeleteRushMCOMEntity(string mcomIdentifier)
	{
		if (!Replication.IsServer())
			return;
		
		// Telemetry: string
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(mcomIdentifier);
		LogTelemetry("DeleteRushMCOMEntity", bytes);
			
		#ifdef WORKBENCH
		RpcDo_DeleteRushMCOMEntity(mcomIdentifier);
		#else
		Rpc(RpcDo_DeleteRushMCOMEntity, mcomIdentifier);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	//! GunGame: Update player stats
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
	//! GearscriptManager: Add vehicle supply cost
	void AddVehicleSupplyCost(ResourceName vehicleResource, int supplyCost)
	{
		// Telemetry: ResourceName string + int
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_ResourceName(vehicleResource);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("AddVehicleSupplyCost", bytes);
		
		#ifdef WORKBENCH
		RpcDo_AddVehicleSupplyCost(vehicleResource, supplyCost);
		#else
		Rpc(RpcDo_AddVehicleSupplyCost, vehicleResource, supplyCost);
		#endif
	}

	//================================================================================================
	// CLIENT RPC HANDLERS
	// These methods execute on client machines when receiving server RPCs
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Find MCOM entity at specified position
	protected IEntity FindMCOMEntityAtPosition(vector position)
	{
		Print("[COA_RplBroadcastManager] FindMCOMEntityAtPosition: Looking for MCOM at " + position);
		
		// Method 1: Try to get MCOM entity from Rush gamemode manager (most reliable)
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			Print("[COA_RplBroadcastManager] Found rush gamemode, checking all MCOMs");
			
			// Check all MCOM entities from the gamemode
			array<string> mcomIdentifiers = {"Zone1Alpha", "Zone1Beta", "Zone2Alpha", "Zone2Beta", "Zone3Alpha", "Zone3Beta"};
			foreach (string identifier : mcomIdentifiers)
			{
				IEntity mcomEntity = rushGamemode.GetMCOMEntity(identifier);
				if (mcomEntity)
				{
					float distance = vector.Distance(mcomEntity.GetOrigin(), position);
					Print("[COA_RplBroadcastManager] Checking " + identifier + " at " + mcomEntity.GetOrigin() + ", distance: " + distance);
					
					if (distance <= 15.0) // Allow some tolerance for positioning
					{
						// Verify it has a SoundComponent
						SoundComponent soundComp = SoundComponent.Cast(mcomEntity.FindComponent(SoundComponent));
						if (soundComp)
						{
							Print("[COA_RplBroadcastManager] Found matching MCOM: " + identifier);
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
			Print("[COA_RplBroadcastManager] No world found");
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
				Print("[COA_RplBroadcastManager] Found entity by name: " + mcomName + " at " + entity.GetOrigin() + ", distance: " + distance);
				
				if (distance <= 15.0) // Allow some tolerance for positioning
				{
					// Verify it has a SoundComponent
					SoundComponent soundComp = SoundComponent.Cast(entity.FindComponent(SoundComponent));
					if (soundComp)
					{
						Print("[COA_RplBroadcastManager] Found matching MCOM by name: " + mcomName);
						return entity;
					}
				}
			}
		}
		
		Print("[COA_RplBroadcastManager] No MCOM entity found at position " + position);
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Area Timer: push current state to all clients every server tick
	void BroadcastAreaTimerUpdate(CRF_EAreaTimerState state, int countdown, string faction, string zoneLabel)
	{
		int bytes = 8; // state + countdown ints
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(zoneLabel);
		LogTelemetry("BroadcastAreaTimerUpdate", bytes);

		#ifdef WORKBENCH
		RpcDo_BroadcastAreaTimerUpdate(state, countdown, faction, zoneLabel);
		#else
		Rpc(RpcDo_BroadcastAreaTimerUpdate, state, countdown, faction, zoneLabel);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	//! Area Timer: notify all clients that a faction won
	void BroadcastAreaTimerWin(string faction, string zoneName)
	{
		int bytes = COA_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += COA_BandwidthTelemetryManager.EstimateSize_String(zoneName);
		LogTelemetry("BroadcastAreaTimerWin", bytes);

		#ifdef WORKBENCH
		RpcDo_BroadcastAreaTimerWin(faction, zoneName);
		#else
		Rpc(RpcDo_BroadcastAreaTimerWin, faction, zoneName);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	void BroadcastOutro(string winningFaction = "")
	{
		#ifdef WORKBENCH
		RpcDo_BroadcastOutro(winningFaction);
		#else
		Rpc(RpcDo_BroadcastOutro, winningFaction);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	//! Shows a custom hint to a single specific player (e.g. a JIP forward deploy denial reason).
	void ShowPlayerHint(int playerId, string message, string title = "Forward Deploy", float duration = 8)
	{
		Rpc(RpcDo_ShowPlayerHint, playerId, message, title, duration);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastAdminChatMessage(string message)
	{
		// Only show to admins and moderators
		if (!SCR_Global.IsAdmin() && !m_PermissionManager.IsModerator())
			return;
		
		SCR_ChatComponent chatComponent = GetLocalChatComponent();
		if (!chatComponent)
			return;
		
		// Show message in admin chat (similar to /a messages)
		chatComponent.ShowMessage(message);
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
	void RpcDo_PlayPositionalSound(string resource, string soundEvent, vector position)
	{
		// AudioSystem.PlayEvent requires a full world transform; build identity rotation at the target position.
		vector soundTransform[4];
		soundTransform[0] = Vector(1, 0, 0);
		soundTransform[1] = Vector(0, 1, 0);
		soundTransform[2] = Vector(0, 0, 1);
		soundTransform[3] = position;
		
		// Stop any previous instance of this event before starting a new one.
		if (m_mSoundHandles.Contains(soundEvent))
			AudioSystem.TerminateSound(m_mSoundHandles.Get(soundEvent));
		m_mSoundHandles.Set(soundEvent, AudioSystem.PlayEvent(resource, soundEvent, soundTransform));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_StopPositionalSound(string soundEvent)
	{
		if (m_mSoundHandles.Contains(soundEvent))
		{
			AudioSystem.TerminateSound(m_mSoundHandles.Get(soundEvent));
			m_mSoundHandles.Set(soundEvent, AudioHandle.Invalid);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_DeleteRushMCOMEntity(string mcomIdentifier)
	{
		Print("[COA_RplBroadcastManager] RpcDo_DeleteRushMCOMEntity received: " + mcomIdentifier + " (Server: " + Replication.IsServer() + ")");
		
		// Get the Rush gamemode manager
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(gameMode.FindComponent(CRF_RushGamemodeManager));
		if (!rushGamemode)
		{
			Print("[COA_RplBroadcastManager] Could not find Rush gamemode manager", LogLevel.WARNING);
			return;
		}
		
		// On clients, set the destroyed status FIRST before any marker operations
		if (!Replication.IsServer())
		{
			// Set the destroyed status in the gamemode arrays so RefreshMapMarkers knows to exclude this MCOM
			rushGamemode.SetMCOMDestroyedStatusFromRPC(mcomIdentifier, true);
			Print("[COA_RplBroadcastManager] Set destroyed status for: " + mcomIdentifier);
			
			// Clear all markers and refresh with proper destroyed state
			COA_PlayerScriptedMarkerManager playerScriptedMarkerManager = COA_PlayerScriptedMarkerManager.GetInstance();
			if (playerScriptedMarkerManager)
			{
				playerScriptedMarkerManager.RemoveALLScriptedMarkers();
				Print("[COA_RplBroadcastManager] Removed all map markers for MCOM destruction: " + mcomIdentifier);
				
				// Immediately refresh markers with proper destroyed status
				rushGamemode.RefreshMapMarkers();
				Print("[COA_RplBroadcastManager] Refreshed map markers with destroyed status for: " + mcomIdentifier);
			}
		}
		
		// Get the MCOM entity for presentation cleanup. The server owns the
		// authority and replication owns proxy destruction on clients.
		IEntity mcomEntity = rushGamemode.GetMCOMEntity(mcomIdentifier);
		if (!mcomEntity)
		{
			Print("[COA_RplBroadcastManager] MCOM entity not found (may be already deleted): " + mcomIdentifier);
			// Entity might already be deleted - just clean up references
			rushGamemode.CleanupMCOMReference(mcomIdentifier);
			return;
		}
		
		Print("[COA_RplBroadcastManager] Processing MCOM deletion: " + mcomIdentifier + " (Entity ID: " + mcomEntity.GetID() + ")");
		
		// Always hide the 3D marker component first
		CRF_Rush_3DMarkerComponent markerComponent = CRF_Rush_3DMarkerComponent.Cast(mcomEntity.FindComponent(CRF_Rush_3DMarkerComponent));
		if (markerComponent)
			markerComponent.SetVisible(false);
		
		// Always clean up gamemode references
		rushGamemode.CleanupMCOMReference(mcomIdentifier);
		
		if (Replication.IsServer())
			Print("[COA_RplBroadcastManager] Server: Entity deletion handled by main logic");
		else
			Print("[COA_RplBroadcastManager] Client: Waiting for replicated proxy removal");
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastMessage(string message)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(message);
	}	
	
	//------------------------------------------------------------------------------------------------
	// GunGame: Update player stats on all clients
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_UpdateGunGamePlayerStats(int playerId, int level, int killsThisLevel, int totalKills)
	{
		CRF_GunGame gunGame = CRF_GunGame.Cast(GetGame().GetGameMode().FindComponent(CRF_GunGame));
		if (gunGame)
			gunGame.UpdatePlayerStatsClient(playerId, level, killsThisLevel, totalKills);
	}
	
	//------------------------------------------------------------------------------------------------
	// GearscriptManager: Add vehicle supply cost on all clients
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_AddVehicleSupplyCost(ResourceName vehicleResource, int supplyCost)
	{
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (vehicleGearscriptManager)
			vehicleGearscriptManager.AddVehicleCostClient(vehicleResource, supplyCost);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastOutro(string winningFaction)
	{
		m_sOutroWinningFaction = winningFaction;
		AudioSystem.PlaySound("{3D7F63CCD32B2F17}Sounds/Intro/outroCrescendo.wav");
		GetGame().GetCallqueue().CallLater(OpenOutro, 2831, false);
	}
	
	//------------------------------------------------------------------------------------------------
	void OpenOutro()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_Outro);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_ShowPlayerHint(int playerId, string message, string title, float duration)
	{
		if (!IsLocalPlayer(playerId))
			return;

		SCR_HintManagerComponent.ShowCustomHint(message, title, duration);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastAreaTimerUpdate(CRF_EAreaTimerState state, int countdown, string faction, string zoneLabel)
	{
		m_eAreaTimerState    = state;
		m_iAreaTimerCountdown = countdown;
		m_sAreaTimerFaction  = faction;
		m_sAreaTimerZoneLabel = zoneLabel;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastAreaTimerWin(string faction, string zoneName)
	{
		SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		string displayName = faction;
		if (fm)
		{
			Faction f = fm.GetFactionByKey(faction);
			if (f)
				displayName = f.GetFactionName();
		}

		SCR_PopUpNotification popUp = SCR_PopUpNotification.GetInstance();
		if (popUp)
			popUp.PopupMsg(displayName + " controls " + zoneName + "!", 10);

		AudioSystem.PlaySound("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav");
	}
};
