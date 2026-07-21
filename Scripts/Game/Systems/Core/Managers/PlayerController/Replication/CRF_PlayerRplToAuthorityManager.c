//------------------------------------------------------------------------------------------------
// Data structure for batched slot updates to reduce network traffic
// Using individual parameters instead of complex serialization to match Enfusion's simpler RPC pattern
//------------------------------------------------------------------------------------------------

class CRF_PlayerRplToAuthorityManagerClass : ScriptComponentClass {}

class CRF_PlayerRplToAuthorityManager : ScriptComponent
{	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Manager references
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_PermissionManager m_PermissionManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	protected CRF_GearscriptManager m_GearscriptManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_BandwidthTelemetryManager m_TelemetryManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected SCR_MapMarkerManagerComponent m_MapMarkerManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if(!Replication.IsServer())
			return;
		
		InitializeManagerReferences();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initializes all manager references needed by this component
	protected void InitializeManagerReferences()
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_PermissionManager = CRF_PermissionManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_TelemetryManager = CRF_BandwidthTelemetryManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_MapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 TELEMETRY
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Log RPC call to telemetry system (server-side only)
	protected void LogTelemetry(string rpcName, int estimatedBytes)
	{
		if (!Replication.IsServer())
			return;
			
		if (!m_TelemetryManager)
			m_TelemetryManager = CRF_BandwidthTelemetryManager.GetInstance();
			
		if (m_TelemetryManager)
			m_TelemetryManager.LogRPC(rpcName, estimatedBytes);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve a replicated entity without dereferencing a missing or stale replication item.
	protected IEntity ResolveReplicatedEntity(RplId entityId)
	{
		if (entityId == RplId.Invalid())
			return null;

		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(entityId));
		if (!rplComponent)
			return null;

		return rplComponent.GetEntity();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
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
	void ToggleRushMCOMPlanted(string mcomIdentifier, bool togglePlanted)
	{
		Print("[CRF_PlayerRplToAuthorityManager] ToggleRushMCOMPlanted() called on client: " + mcomIdentifier + ", planted: " + togglePlanted + " - sending RPC");
		Rpc(RpcAsk_ToggleRushMCOMPlanted, mcomIdentifier, togglePlanted); 
	}
	
	//------------------------------------------------------------------------------------------------
	void StartRushPlantingSound()
	{
		Print("[CRF_PlayerRplToAuthorityManager] StartRushPlantingSound() called on client - sending RPC");
		Rpc(RpcAsk_StartRushPlantingSound); 
	}
	
	//------------------------------------------------------------------------------------------------
	void StopRushPlantingSound()
	{
		Rpc(RpcAsk_StopRushPlantingSound); 
	}
	
	//------------------------------------------------------------------------------------------------
	void StartRushDefuseSound()
	{
		Rpc(RpcAsk_StartRushDefuseSound); 
	}
	
	//------------------------------------------------------------------------------------------------
	void StopRushDefuseSound()
	{
		Rpc(RpcAsk_StopRushDefuseSound); 
	}
	
	//------------------------------------------------------------------------------------------------
	void StopRushBombTickingSound()
	{
		Print("[CRF_PlayerRplToAuthorityManager] StopRushBombTickingSound() called on client - sending RPC");
		Rpc(RpcAsk_StopRushBombTickingSound); 
	}

	//------------------------------------------------------------------------------------------------
	// Multi-use: true = plant/arm the bomb, false = defuse it.
	// Hold sounds use the generic RequestPlayPositionalSound/RequestStopPositionalSound RPCs.
	void RequestRadioExtractionSetPlanted(bool planted)
	{
		Rpc(RpcAsk_RequestRadioExtractionSetPlanted, planted);
	}

	//------------------------------------------------------------------------------------------------
	void RequestRadioExtractionSendMessage()
	{
		Rpc(RpcAsk_RequestRadioExtractionSendMessage);
	}

	//------------------------------------------------------------------------------------------------
	void RequestAdvanceGamemodeState(bool overriden, string winningFaction = "")
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceGamemodeState, overriden, winningFaction);
	}
	
	//------------------------------------------------------------------------------------------------
	// Broken in 1.7. Check CRF_PersistanceManager for update.
	/*
	void RequestMissionSave(string saveName)
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestMissionSave, saveName);
	}
	*/
	
	//------------------------------------------------------------------------------------------------
	void RequestAdvanceSlottingPhase()
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceSlottingPhase); 
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		Rpc(RpcAsk_UpdateSlotPlayerID, slotId, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotLockedState, slotId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		// Group locking is not part of slot batching, use direct RPC
		Rpc(RpcAsk_UpdateGroupLockedState, groupRplId, input); 
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotDeathState, slotId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotRole(int slotId, CRF_EGearRole role)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotRole, slotId, role);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotGroup, slotId, groupRplId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		// Direct manager call if BatchUpdateSlot unavailable
		Rpc(RpcAsk_UpdateSlotCharacter, slotId, charId);
	}
	
	//------------------------------------------------------------------------------------------------
	void ReportBug(string data, int playerID)
	{
		Rpc(RpcAsk_ReportBug, data, playerID);
	}
	
	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(string data, int playerID)
	{
		Rpc(RpcAsk_SendAdminMessage, data, playerID); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ReportSettingsViolation(int playerId, string violationType)
	{
		Rpc(RpcAsk_ReportSettingsViolation, playerId, violationType);
	}
	
	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if (SCR_Global.IsAdmin() || m_PermissionManager.IsModerator())
			Rpc(RpcAsk_ReplyAdminMessage, data, playerId, adminID, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_CloseAdminTicket, ticketID, adminID, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		Rpc(RpcAsk_AssignAdminTicket, ticketID, adminID, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void GetOpenTickets(int playerID)
	{
		Rpc(RpcAsk_GetOpenTickets, playerID); 
	}
	
	//------------------------------------------------------------------------------------------------
	void GetTicketMessages(int playerID, int ticketID)
	{
		Rpc(RpcAsk_GetTicketMessages, playerID, ticketID); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, int spawnPointID)
	{
		Rpc(RpcAsk_RespawnPlayer, playerId, spawnPointID); 
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
	void SpawnOnGroup(int playerId, int playerIDToSpawnOn, int groupID, bool logAction)
	{
		Rpc(RpcAsk_SpawnOnGroup, playerId, playerIDToSpawnOn, groupID, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestVehicleDepotInteraction(int playerId, int vehicleIndex, RplId depotRplId)
	{
		Rpc(RpcAsk_RequestVehicleDepotInteraction, playerId, vehicleIndex, depotRplId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnFaction(FactionKey faction, bool logAction)
	{
		Rpc(RpcAsk_RespawnFaction, faction, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		Rpc(RpcAsk_ResetGear, playerId, prefab, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateGearSet(string faction, ResourceName path)
	{
		Rpc(RpcAsk_UpdateGearSet, faction, path); 
	}
	
	//------------------------------------------------------------------------------------------------
	void AddItem(int playerId, string prefab, bool logAction)
	{
		Rpc(RpcAsk_AddItem, playerId, prefab, logAction); 
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveItem(int playerId, RplId entityID, bool logAction)
	{
		Rpc(RpcAsk_RemoveItem, playerId, entityID, logAction);
	}

	//------------------------------------------------------------------------------------------------
	//! Atomically swap a world entity the player is standing next to for a different prefab in
	//! their inventory (e.g. CRF_ToggleSatchelType). Unlike calling AddItem+RemoveItem separately,
	//! this only removes oldEntityId once the new item is confirmed inserted, and re-checks that
	//! oldEntityId still exists before doing anything - closes both an item-loss bug (inventory
	//! full -> new item discarded but old one still removed) and a duplication exploit (multiple
	//! players triggering a conversion on the same entity each getting their own copy).
	void ConvertItem(int playerId, RplId oldEntityId, string newPrefab, bool logAction)
	{
		Rpc(RpcAsk_ConvertItem, playerId, oldEntityId, newPrefab, logAction);
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
	void UpdateTimer(int delta) 
	{
		Rpc(RpcAsk_UpdateTimer, delta); 
	}	
	
	//------------------------------------------------------------------------------------------------
	void UpdateTicket(string action, FactionKey faction, int delta) 
	{
		Rpc(RpcAsk_UpdateTicket, action, faction, delta); 
	}
	
	//------------------------------------------------------------------------------------------------
	void MiniArsenalRequestNewItem(int playerId, string resourceName, int slotId)
	{
		Rpc(RpcAsk_MiniArsenalRequestNewItem, playerId, resourceName, slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	void MiniArsenalRequestNewWeapon(int playerId, string weapon, array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, bool isPistol)
	{
		Rpc(RpcAsk_MiniArsenalRequestNewWeapon, playerId, weapon, attachments, magazines, magazineCounts, isPistol);
	}
	
	//------------------------------------------------------------------------------------------------
	void SightArsenalRequestNewSight(int playerId, string resourceName, string type)
	{
		// Convert to indices before sending for bandwidth optimization
		int sightIndex = CRF_SightArsenalRegistry.GetSightIndex(resourceName);
		if (sightIndex < 0)
		{
			// Fallback to old method for unknown sights (mod compatibility)
			Print(string.Format("[CRF] Warning: Sight not in registry, using fallback: %1", resourceName), LogLevel.WARNING);
			Rpc(RpcAsk_SightArsenalRequestNewSight_Fallback, playerId, resourceName, type);
			return;
		}
		
		// Determine sight type from index (more reliable than type string)
		CRF_ESightType sightType = CRF_SightArsenalRegistry.GetSightTypeFromIndex(sightIndex);
		Rpc(RpcAsk_SightArsenalRequestNewSight_Optimized, playerId, sightIndex, sightType);
	}
	
	//------------------------------------------------------------------------------------------------
	void TogglePlayerListening(int playerId, bool input)
	{
		Rpc(RpcAsk_TogglePlayerLisntening, playerId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleWaveRespawn()
	{
		Rpc(RpcAsk_ToggleWaveRespawn);
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawn()
	{
		Rpc(RpcAsk_ToggleRespawn);
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleVAARRecording()
	{
		Rpc(RpcAsk_ToggleVAARRecording);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetRespawnTime(int seconds)
	{
		Rpc(RpcAsk_SetRespawnTime, seconds);
	}
	
	//------------------------------------------------------------------------------------------------
	void CleanUpBodies()
	{
		Rpc(RpcAsk_CleanUpBodies);
	}
	
	//------------------------------------------------------------------------------------------------
	void AddItemToTruck(RplId truckId, ResourceName resource, int amount, array<RplId> supplyItems, array<int> supplyCounts, RplId supplyArsenalId)
	{
		Rpc(RpcAsk_AddItemToTruck, truckId, resource, amount, supplyItems, supplyCounts, supplyArsenalId);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSupplyArsneal(RplId supplyArsnealId)
	{
		Rpc(RpcAsk_UpdateSupplyArsneal, supplyArsnealId);
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateCache(RplId truckId, RplId playerId)
	{
		Rpc(RpcAsk_CreateCache, truckId, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestVehicleSupplies(RplId truckId)
	{
		Rpc(RpcAsk_RequestVehicleSupplies, truckId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RearmVehicle(RplId truckId, array<RplId> supplyItems, array<int> supplyCounts, RplId rearmTruckId)
	{
		Rpc(RpcAsk_RearmVehicle, truckId, supplyItems, supplyCounts, rearmTruckId);
	}
	
	//------------------------------------------------------------------------------------------------
	void MoveSpecCamToSlot(int slotID, int playerID)
	{
		Rpc(RpcAsk_MoveSpecCamToSlot, slotID, playerID);
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		Rpc(RpcAsk_RequestForwardDeploy, cursorWorldPos, factionKey, playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Requests a JIP (Join In Progress) forward deploy to the live position of the given friendly group.
	void RequestJIPForwardDeploy(RplId groupId, int playerId)
	{
		Rpc(RpcAsk_RequestJIPForwardDeploy, groupId, playerId);
	}

	//------------------------------------------------------------------------------------------------
	void SharerMapMarkerGlobal(int markerUID, int playerId)
	{
		Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		if (!playerFaction)
			return;
		
		Rpc(RpcAsk_ShareMapMarkerGlobal, markerUID, playerFaction.GetFactionKey(), playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void ShareMapMarkers()
	{
		if (!m_MapMarkerManager)
			m_MapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
		if (!m_MapMarkerManager)
			return;
		
		array<int> markerUIDs = {};
		foreach (SCR_MapMarkerBase marker: m_MapMarkerManager.GetStaticMarkers())
		{
			if (marker.m_bIsShared)
				markerUIDs.Insert(marker.GetMarkerID());
		}
		
		Rpc(RpcAsk_ShareMapMarkers,markerUIDs, SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestGlobalMarkerRefresh()
	{
		Rpc(RpcAsk_RequestGlobalMarkerRefresh, SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	void RequestSupplyUpdate(RplId supplyArsenalId)
	{
		Rpc(RpcDo_RequestSupplyUpdate, supplyArsenalId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RegisterPlayerForLottery(int playerId, string factionKey, string squadFilter)
	{
		Rpc(RpcAsk_RegisterPlayerForLottery, playerId, factionKey, squadFilter);
	}

	
	//------------------------------------------------------------------------------------------------
	void RunSlotLottery(int requestingPlayerId)
	{
		Rpc(RpcAsk_RunSlotLottery, requestingPlayerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void ClearSlotLottery(int requestingPlayerId)
	{
		Rpc(RpcAsk_ClearSlotLottery, requestingPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	void RequestObjectiveTaskComplete(int taskIndex, int completingSide = 0)
	{
		Rpc(RpcAsk_RequestObjectiveTaskComplete, taskIndex, completingSide);
	}

	//------------------------------------------------------------------------------------------------
	// Tells the server to set the per-object state on the task at taskIndex.
	// The server calls SetTaskObjectState() which replicates to all clients and
	// fires the handler's OnObjectStateChangedServer (for countdowns and sounds).
	void RequestTaskObjectSetState(int taskIndex, int newState, int planterSide = 0)
	{
		Rpc(RpcAsk_RequestTaskObjectSetState, taskIndex, newState, planterSide);
	}

	//------------------------------------------------------------------------------------------------
	// Tells the server to broadcast a 3D positional sound to all clients at the given world position.
	// Position is read by the caller from the relevant entity — no task index needed.
	void RequestPlayPositionalSound(string soundResource, string soundEvent, vector position)
	{
		Rpc(RpcAsk_RequestPlayPositionalSound, soundResource, soundEvent, position);
	}

	//------------------------------------------------------------------------------------------------
	// Tells the server to stop a previously broadcast positional sound on all clients.
	void RequestStopPositionalSound(string soundEvent)
	{
		Rpc(RpcAsk_RequestStopPositionalSound, soundEvent);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestInitilizePlayer(int playerId)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_RequestInitilizePlayer", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		// Use staggered initialization system to prevent server overload
		m_Gamemode.QueuePlayerInitialization(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleSideReady(string setReady, string playerName, bool adminForced)
	{
		// Telemetry: 2 strings + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(setReady);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(playerName);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ToggleSideReady", bytes);
		
		m_SafestartManager.ToggleSideReady(setReady, playerName, adminForced);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		// Telemetry: string + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(sitePlanted);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ToggleBombPlanted", bytes);
		
		CRF_SearchAndDestroyGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGamemodeManager)).ToggleBombPlanted(sitePlanted, togglePlanted);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleRushMCOMPlanted(string mcomIdentifier, bool togglePlanted)
	{
		// Telemetry: string + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(mcomIdentifier);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ToggleRushMCOMPlanted", bytes);
		
		Print("[CRF_PlayerRplToAuthorityManager] RpcAsk_ToggleRushMCOMPlanted received: " + mcomIdentifier + ", planted: " + togglePlanted);
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
			rushGamemode.ToggleMCOMPlanted(mcomIdentifier, togglePlanted);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartRushPlantingSound()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_StartRushPlantingSound", 0);
		
		Print("[CRF_PlayerRplToAuthorityManager] RpcAsk_StartRushPlantingSound received on server");
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
			rushGamemode.PlayPlantingSound();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushPlantingSound()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_StopRushPlantingSound", 0);
		
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.StopPlantingSound();
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartRushDefuseSound()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_StartRushDefuseSound", 0);
		
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.PlayDefuseSound();
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushDefuseSound()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_StopRushDefuseSound", 0);
		
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.StopDefuseSound();
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushBombTickingSound()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_StopRushBombTickingSound", 0);
		
		Print("[CRF_PlayerRplToAuthorityManager] RpcAsk_StopRushBombTickingSound received on server");
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			Print("[CRF_PlayerRplToAuthorityManager] Calling rushGamemode.StopBombTickingSound()");
			rushGamemode.StopBombTickingSound();
		}
		else
		{
			Print("[CRF_PlayerRplToAuthorityManager] Rush gamemode manager not found!");
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestRadioExtractionSetPlanted(bool planted)
	{
		// Telemetry: bool
		LogTelemetry("RpcAsk_RequestRadioExtractionSetPlanted", CRF_BandwidthTelemetryManager.EstimateSize_Bool());

		CRF_RadioExtraction radioExtraction = CRF_RadioExtraction.GetInstance();
		if (!radioExtraction)
			return;

		if (planted)
			radioExtraction.PlantBomb();
		else
			radioExtraction.DefuseBomb();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestRadioExtractionSendMessage()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_RequestRadioExtractionSendMessage", 0);

		CRF_RadioExtraction radioExtraction = CRF_RadioExtraction.GetInstance();
		if (radioExtraction)
			radioExtraction.SendExtractionMessage();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceGamemodeState(bool overriden, string winningFaction)
	{
		// Telemetry: bool
		LogTelemetry("RpcAsk_RequestAdvanceGamemodeState", CRF_BandwidthTelemetryManager.EstimateSize_Bool());
		
		// Set winning faction on the server where the log file handle exists
		if (winningFaction != "")
		{
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
				loggingManager.SetWinningFaction(winningFaction, "manual");
		}
		
		m_Gamemode.AdvanceGamemodeState(overriden);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAdvanceSlottingPhase()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_RequestAdvanceSlottingPhase", 0);
		
		m_Gamemode.AdvanceSlottingState();
	}
	
	//------------------------------------------------------------------------------------------------
	// Broken in 1.7. Check CRF_PersistanceManager for update.
	/*
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestMissionSave(string saveName)
	{
		// Telemetry: string
		LogTelemetry("RpcAsk_RequestMissionSave", CRF_BandwidthTelemetryManager.EstimateSize_String(saveName));
		
		// Get persistence manager and trigger save
		CRF_PersistenceManager persistenceManager = CRF_PersistenceManager.GetInstance();
		if (persistenceManager)
		{
			persistenceManager.TriggerManualSave(saveName);
		}
		else
		{
			Print("[CRF_PlayerRplToAuthorityManager] Persistence manager not available", LogLevel.ERROR);
		}
	}
	*/

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotPlayerID(int slotId, int playerId)
	{
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_UpdateSlotPlayerID", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		m_SlottingManager.UpdateSlotPlayerID(slotId, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotLockedState(int slotId, bool input)
	{
		// Telemetry: int + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateSlotLockedState", bytes);
		
		m_SlottingManager.UpdateSlotLockedState(slotId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		// Telemetry: RplId + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateGroupLockedState", bytes);
		
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(groupRplId));
		if (!rplComponent)
			return;
			
		SCR_AIGroup group = SCR_AIGroup.Cast(rplComponent.GetEntity());
		if (group)
			group.SetPrivate(input);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotDeathState(int slotId, bool input)
	{
		// Telemetry: int + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_UpdateSlotDeathState", bytes);
		
		m_SlottingManager.UpdateSlotDeathState(slotId, input); 
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotRole(int slotId, CRF_EGearRole role)
	{
		// Telemetry: int + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_UpdateSlotRole", bytes);
		
		m_SlottingManager.UpdateSlotRole(slotId, role); 
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		// Telemetry: int + RplId
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("RpcAsk_UpdateSlotGroup", bytes);
		
		m_SlottingManager.UpdateSlotGroup(slotId, groupRplId); 
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSlotCharacter(int slotId, RplId charId)
	{
		// Telemetry: int + RplId
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("RpcAsk_UpdateSlotCharacter", bytes);
		
		m_SlottingManager.UpdateSlotCharacter(slotId, charId); 
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendAdminMessage(string data, int playerID)
	{
		// Telemetry: string + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendAdminMessage", bytes);
		
		// Broadcast a new ticket/message to admins
		bool ticketExists = m_AdminMenuManager.TicketExists(playerID);
		m_RplBroadcastManager.SendAdminMessage(data, playerID, ticketExists);
		
		// Create a new ticket or/and add reply to existing ticket if not a admin/mod
		if (!SCR_Global.IsAdmin(playerID) && !m_PermissionManager.IsModerator(playerID))
			m_AdminMenuManager.NewTicketMessage(playerID, playerID, data);
	}	
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReportBug(string data, int playerID)
	{
		// Telemetry: string + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendAdminMessage", bytes);
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);

		// Setup rest call
		RestApi rest = GetGame().GetRestApi();
		if (!rest)
			return;
	
		RestContext restContext = rest.GetContext("https://api.github.com/");
		string token = CRF_BugReportConfig.GetToken();
		string repo = CRF_BugReportConfig.GetRepo();
		
		// Setup headers
		restContext.SetHeaders(string.Format("Authorization, Bearer %1,Accept,application/vnd.github+json,Content-Type,application/json", token));
		
		// Collect mission details
		string missionName = "Unknown Mission";
		string missionAuthor = "Unknown";
		string time = CRF_AdminMenuManager.GetFormattedTimestamp();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (header)
			missionAuthor = header.m_sAuthor;
		if (GetGame().GetMissionName())
			missionName = GetGame().GetMissionName();
		
		// Collect gearset details
		CRF_EGearRole gearSet = CRF_RoleHelper.ResourceToRole(CRF_SlottingManager.GetInstance().GetPlayerSlotResource(playerID));
		string gearSetName = SCR_Enum.GetEnumName(CRF_EGearRole, gearSet);
		
		// Collect slot info
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		FactionKey faction = slottingManager.GetPlayerSlotFaction(playerID).GetFactionKey();
		int slotID = slottingManager.GetPlayerSlotID(playerID);
		string slotGroup = slottingManager.GetPlayerSlotGroup(playerID).GetCustomNameWithOriginal();
		
		// Set title and body
		string title = string.Format("[CRF General Bug] During %1 by %2", missionName, playerName);
		string body = string.Format("%1 \\n\\n Reported by %2 at %3 \\n\\n Role: %4 \\n Group: %5 | %6 \\n Mission: %7 by %8", data, playerName, time, gearSetName, slotGroup, faction, missionName, missionAuthor);
		
		// UP!
		string payload = string.Format("{\"title\": \"%1\", \"body\": \"%2\"}", title, body);
		
		// SEND IT!
		restContext.POST(null, repo, payload);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		// Telemetry: string + 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ReplyAdminMessage", bytes);
		
		// Create a new ticket or/and add reply to existing ticket
		m_AdminMenuManager.NewTicketMessage(playerId, adminID, data);
		
		// Broadcast to the reply to the player
		m_RplBroadcastManager.ReplyAdminMessage(data, playerId, adminID, logAction);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CloseAdminTicket(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_CloseAdminTicket", bytes);
		
		m_AdminMenuManager.CloseTicket(ticketID);
		
		// Broadcast to admins that ticket was clsoed
		m_RplBroadcastManager.CloseAdminTicket(ticketID, adminID, true);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AssignAdminTicket(int ticketID, int adminID, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_AssignAdminTicket", bytes);
		
		m_AdminMenuManager.AssignAdminTicket(ticketID, adminID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetOpenTickets(int playerID)
	{
		m_RplBroadcastManager.GetOpenTickets(playerID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GetTicketMessages(int playerID, int ticketID)
	{
		m_RplBroadcastManager.GetTicketMessages(playerID, ticketID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnPlayer(int playerId, int spawnPointID)
	{
		// Telemetry: int + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_RespawnPlayer", bytes);
		
		m_RespawnManager.RespawnPlayer(playerId, spawnPointID);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestToJoinChannel(int channel, int requestId)
	{
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_RequestToJoinChannel", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		Print(string.Format("[VON] Server processing join request: channel=%1, requestId=%2", channel, requestId), LogLevel.NORMAL);
		
		// Instead of using BroadcastManager, handle the request directly on the server
		if (channel < 0 || channel >= m_MenuManager.m_aVONChannels.Count())
			return;
		
		// Extract channel creator ID from channel name
		// Channel name format: "PlayerName's Channel (PlayerID)|players..."
		string channelString = m_MenuManager.m_aVONChannels[channel];
		array<string> channelSplit = {};
		channelString.Split("|", channelSplit, true);
		
		if (channelSplit.Count() == 0)
			return;
		
		string channelName = channelSplit[0];
		
		// Find the creator ID from the channel name format: "Name's Channel (ID)"
		int openParen = channelName.IndexOf("(");
		int closeParen = channelName.IndexOf(")");
		
		if (openParen == -1 || closeParen == -1 || closeParen <= openParen)
			return;
		
		string creatorIdStr = channelName.Substring(openParen + 1, closeParen - openParen - 1);
		int creatorId = creatorIdStr.ToInt();
		
		// Don't send a request if the requester is the channel creator
		if (creatorId == requestId)
		{
			Print(string.Format("[VON] Player %1 tried to join their own channel %2, ignoring", requestId, channel), LogLevel.NORMAL);
			return;
		}
		
		// Send notification to the channel creator
		if (creatorId > 0)
		{
			Print(string.Format("[VON] Server sending join request notification to creator %1 from requester %2 for channel %3", creatorId, requestId, channel), LogLevel.NORMAL);
			m_RplBroadcastManager.NotifyChannelJoinRequest(creatorId, requestId, channel);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CheckVONRegister(int playerId)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_CheckVONRegister", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		int channelIndex;
		if (!m_MenuManager.IsPlayerInAnyChannel(playerId, channelIndex))
		{
			m_MenuManager.AddPlayerToChannel(playerId, 1, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CreateChannel(int playerId)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_CreateChannel", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		// Include player ID in channel name to ensure uniqueness when players have same username
		string uniqueChannelName = playerName + "'s Channel (" + playerId + ")";
		int channelIndex = m_MenuManager.CreateChannel(uniqueChannelName, playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_JoinChannel(int playerId, int channel)
	{
		// Telemetry: 2 ints
		LogTelemetry("RpcAsk_JoinChannel", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);
		
		m_MenuManager.AddPlayerToChannel(playerId, channel, false);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, int playerIDToSpawnOn, int groupID, bool logAction)
	{
		// Telemetry: 3 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 3;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_SpawnOnGroup", bytes);
		
		RplId entityRplID;
		
		if (playerIDToSpawnOn != -1)
		{
			// Use the player selected in the group as the spawn point
			CRF_PlayerCharacter spawnEntity = CRF_SlottingManager.GetInstance().GetPlayerSlotCharacter(playerIDToSpawnOn);
			if (!spawnEntity)
				return;
			
			entityRplID = spawnEntity.GetRplComponent().Id();
		}		
		else if (groupID != -1)
		{
			// Use the group leader as the spawn point
			SCR_AIGroup group = SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID);
			if (!group)
				return;
			
			IEntity leaderEntity = group.GetLeaderEntity();
			if (!leaderEntity)
				return;
			
			RplComponent rplComp = RplComponent.Cast(leaderEntity.FindComponent(RplComponent));
			if (!rplComp)
				return;
			
			entityRplID = rplComp.Id();
		}
		else
			entityRplID = RplId.Invalid();
		
		m_RespawnManager.RespawnPlayer(playerId, -1, entityRplID);
		
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
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVehicleDepotInteraction(int playerId, int vehicleIndex, RplId depotRplId)
	{
		// Telemetry: 2 ints + RplId
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		LogTelemetry("RpcAsk_RequestVehicleDepotInteraction", bytes);
		
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(depotRplId));
		if (!rplComponent)
			return;
		
		IEntity depotEntity = rplComponent.GetEntity();
		if (!depotEntity)
			return;
		
		CRF_VehicleDepot depotComponent = CRF_VehicleDepot.Cast(depotEntity.FindComponent(CRF_VehicleDepot));
		if (!depotComponent)
			return;
		
		// Check if viewing/supply update (special values)
		if (playerId == -1 || vehicleIndex == -1)
		{
			// Only refresh supplies for viewing notifications - spawning handles supply updates automatically
			depotComponent.NotifyPlayerViewing();
			return;
		}
		
		// If not viewing/supply update, is spawnvehicle request
		depotComponent.SpawnVehicle(playerId, vehicleIndex);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnFaction(FactionKey faction, bool logAction)
	{
		// Telemetry: string (FactionKey) + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_RespawnFaction", bytes);
		
		m_RespawnManager.RespawnSide(faction);
		
		if (logAction)
		{
			string logMessage = string.Format("%1 was respawned", faction);
			m_RplBroadcastManager.LogAdminAction(logMessage, -1, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		// Telemetry: int + ResourceName + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(prefab);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ResetGear", bytes);
		
		// Prevent stuck on map
		m_RplBroadcastManager.Closemap(playerId);
		
		// Prevent invisible gun 
		m_RplBroadcastManager.HolsterGun(playerId);
		
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;

		// Schedule gear setup with appropriate delay
		GetGame().GetCallqueue().Call(
			m_GearscriptManager.SetEntityGear, 
			entity, 
			prefab
		);
		
		CRF_RolesConfig rolesConfig = CRF_GearscriptManager.GetRolesConfig();
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(prefab);
		
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		CRF_SlotData slotData = m_SlottingManager.GetSlotData(slotId);
		
		// Use delta updates for individual field changes (90%+ bandwidth savings)
		slotData.SetSlotRole(role);
		m_RplBroadcastManager.UpdateSlotRoleDelta(slotId, role);
		
		// Note: Name, Type, and Icon don't have delta updates as they rarely change
		// If they change frequently in the future, add delta methods for them too
		
		if (logAction)
		{
			string prefabName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1's gear was set to %2", playerName, prefabName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateGearSet(string faction, ResourceName path)
	{
		// Telemetry: string + ResourceName
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(path);
		LogTelemetry("RpcAsk_UpdateGearSet", bytes);
		
		// Update gearscript in the gamemode
		CRF_Gamemode.GetInstance().UpdateGearscriptResource(faction, path);

		// Load the AI world
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
		{
			Print("ERROR: AIworld not found, can't update gear sets");
			return;
		}
		
		array<AIAgent> aiAgents = {};
		array<RplId> entityIds = {};

		//Get entities in the faction and store them
		aiWorld.GetAIAgents(aiAgents);
		foreach (AIAgent agent : aiAgents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character)
				continue;

			if (character.GetFactionKey() == faction)
			{
				RplId entityId = Replication.FindItemId(entity);
				if (entityId != RplId.Invalid())
					entityIds.Insert(entityId);
			}
		}

		// Also gather connected human players in the faction, so their gear updates immediately instead of only on reinitialize
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(playerEntity);
			if (!playerCharacter)
				continue;

			if (playerCharacter.GetFactionKey() == faction)
			{
				RplId entityId = Replication.FindItemId(playerEntity);
				if (entityId != RplId.Invalid())
					entityIds.Insert(entityId);
			}
		}

		// Queue changes to prevent server freezing
		UpdateGearSetQueue(entityIds);
		
		string logMessage = string.Format("%1 was changed to %2", faction, path);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1 , false)
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateGearSetQueue(array<RplId> entityIds, int lastIndex = 0)
	{
		if (!entityIds || lastIndex >= entityIds.Count())
			return;
		
		IEntity entity = ResolveReplicatedEntity(entityIds[lastIndex]);
		if (!entity)
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}
		
		// Grab prefab name and check if its a valid gearscript
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}

		ResourceName prefab = prefabData.GetPrefabName();
		if (!CRF_RoleHelper.IsValidGearscriptResource(prefab))
		{
			GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
			return;
		}
		
		// Prevent Lockup and invisible weapon if player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId)
		{
			m_RplBroadcastManager.Closemap(playerId);
			//Causes issues when trying to holster a weapon we are deleting.
			//m_RplBroadcastManager.HolsterGun(playerId);
		}
		
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		if (gearscriptManager)
			gearscriptManager.SetEntityGear(entity, prefab);
		
		// Queue next entity
		GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entityIds, lastIndex + 1);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItem(int playerId, string prefab, bool logAction)
	{
		// Telemetry: int + string + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(prefab);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_AddItem", bytes);
		
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
		if (!resourceSpawned)
			return;

		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			return;
		}
		
		if (resourceSpawned)
			if (resourceSpawned.FindComponent(CVON_RadioComponent))
			{
				GetGame().GetCallqueue().CallLater(InitializePlayerRadiosDelayed, 500, false, playerId);
				CRF_PlayerRplToOwnerManager ownerManager = CRF_PlayerRplToOwnerManager.GetInstance();
				if (ownerManager)
					ownerManager.InitializeRadioFromServer();
			
			}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RemoveItem(int playerId, RplId entityID, bool logAction)
	{
		// Telemetry: int + RplId + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_RemoveItem", bytes);
		
		if (playerId == 0)
			return;
		
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(entityID));
		if (!rplComp)
			return;
			
		IEntity entity = rplComp.GetEntity();
		if (!entity)
			return;
		
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();

		if (logAction && !prefab.IsEmpty())
		{
			string itemName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%2 was added to %1's inventory", playerName, itemName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
		
		SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	//------------------------------------------------------------------------------------------------
	protected const float CONVERT_ITEM_MAX_DISTANCE_M = 5.0;

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ConvertItem(int playerId, RplId oldEntityId, string newPrefab, bool logAction)
	{
		// Telemetry: int + RplId + string + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_RplId();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(newPrefab);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_ConvertItem", bytes);

		if (playerId == 0 || newPrefab.IsEmpty())
			return;

		// If another request already converted/removed this entity (e.g. a second player
		// triggering the same conversion, or a stale/duplicate request), there is nothing left
		// to convert - bail instead of spawning a duplicate item.
		RplComponent oldRplComp = RplComponent.Cast(Replication.FindItem(oldEntityId));
		if (!oldRplComp)
			return;

		IEntity oldEntity = oldRplComp.GetEntity();
		if (!oldEntity)
			return;

		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!entity)
			return;

		if (vector.DistanceSq(entity.GetOrigin(), oldEntity.GetOrigin()) > CONVERT_ITEM_MAX_DISTANCE_M * CONVERT_ITEM_MAX_DISTANCE_M)
			return;

		SCR_InventoryStorageManagerComponent entityInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!entityInventoryManager)
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();

		Resource resource = Resource.Load(newPrefab);
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!resourceSpawned)
			return;

		if (!entityInventoryManager.TryInsertItem(resourceSpawned))
		{
			// No room for the new item - bail WITHOUT touching oldEntity, so the player keeps
			// what they already had instead of losing it.
			SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			return;
		}

		if (logAction)
		{
			ResourceName oldPrefab = oldEntity.GetPrefabData().GetPrefabName();
			if (!oldPrefab.IsEmpty())
			{
				string itemName = oldPrefab.Substring(oldPrefab.LastIndexOf("/") + 1, oldPrefab.LastIndexOf(".") - oldPrefab.LastIndexOf("/") - 1);
				string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
				string logMessage = string.Format("%2 was converted in %1's inventory", playerName, itemName);
				m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
			}
		}

		// Only remove the original now that the new item is confirmed in the player's inventory.
		// Deleting it synchronously here (rather than via a separate RPC) closes the duplication
		// window: a racing RpcAsk_ConvertItem for the same oldEntityId will find it already gone
		// via the Replication.FindItem check above.
		SCR_EntityHelper.DeleteEntityAndChildren(oldEntity);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TeleportPlayers(int playerId1, int playerId2, bool logAction)
	{
		// Telemetry: 2 ints + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_TeleportPlayers", bytes);
		
		m_RplBroadcastManager.TeleportPlayers(playerId1, playerId2, logAction);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SendHint(string data, int playerId, string factionKey)
	{
		// Telemetry: 2 strings + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(factionKey);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_SendHint", bytes);
		
		m_RplBroadcastManager.SendHint(data, playerId, factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_Heal(int playerId, bool logAction, bool isVehicle)
	{
		// Telemetry: int + 2 bools
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool() * 2;
		LogTelemetry("RpcAsk_Heal", bytes);
		
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
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_LogAdminAction(string data, int playerId, bool sendToPlayer)
	{
		// Telemetry: string + int + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(data);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_LogAdminAction", bytes);
		
		m_RplBroadcastManager.LogAdminAction(data, playerId, sendToPlayer);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReportSettingsViolation(int playerId, string violationType)
	{
		// Telemetry: int + string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(violationType);
		LogTelemetry("RpcAsk_ReportSettingsViolation", bytes);
		
		// Get player name on server
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		// Create violation message
		string message = string.Format("[SETTINGS VIOLATION] Player: %1 (ID: %2) attempted to modify display settings - %3", 
			playerName, playerId, violationType);
		
		// Log to admin action logs
		if (m_AdminMenuManager)
			m_AdminMenuManager.StoreAdminLogs(message);
		
		// Broadcast to admin chat (only admins/mods will see this)
		if (m_RplBroadcastManager)
			m_RplBroadcastManager.BroadcastAdminChatMessage(message);
		
		// Also log to server console
		Print(message, LogLevel.WARNING);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTimer(int delta)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_UpdateTimer", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		// Get current end time
		int currentEndTime = CRF_GameTimerManager.GetInstance().m_iTimeMissionEnds;
		if ((currentEndTime + delta) < 0 || m_SafestartManager.GetSafestartStatus())
			return;

		// Set the new time, broadcast is handled by rplprop
		CRF_GameTimerManager.GetInstance().m_iTimeMissionEnds = currentEndTime + delta;
		
		string logMessage = string.Format("Game timer adjusted by %1 mins", delta/60000);
		m_RplBroadcastManager.GetInstance().LogAdminAction(logMessage, -1, false);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTicket(string action, FactionKey faction, int delta)
	{
		// Telemetry: 2 strings + int
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_String(action);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(faction);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_UpdateTicket", bytes);
		
		if (action == "Add")
			m_RespawnManager.AddTicket(faction, delta, true);
		else if (action == "Subtract")
			m_RespawnManager.SubtractTicket(faction, delta, true);
		
		string logMessage = string.Format("%1 tickets was subtracted from %2", delta, faction);
		m_RplBroadcastManager.GetInstance().LogAdminAction(logMessage, -1, false);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MiniArsenalRequestNewItem(int playerId, string newResource, int slotId)
	{
		// Telemetry: 2 ints + string
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(newResource);
		LogTelemetry("RpcAsk_MiniArsenalRequestNewItem", bytes);
		
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		Resource itemResource = Resource.Load(newResource);
		if (!itemResource || !itemResource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent invComponent = BaseInventoryStorageComponent.Cast(player.FindComponent(BaseInventoryStorageComponent));
		if (!invManager || !invComponent)
			return;

		IEntity newItem = GetGame().SpawnEntityPrefab(itemResource, null, params);
		if (!newItem)
			return;

		if (slotId < 0)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newItem);
			return;
		}

		IEntity oldItem = invComponent.Get(slotId);
		if (!oldItem)
		{
			if (!invManager.TryInsertItem(newItem))
				SCR_EntityHelper.DeleteEntityAndChildren(newItem);
			return;
		}
		BaseInventoryStorageComponent oldStorageComp = BaseInventoryStorageComponent.Cast(oldItem.FindComponent(BaseInventoryStorageComponent));
		ref array<IEntity> pouches = {};
		if (oldStorageComp)
			oldStorageComp.GetAll(pouches);
		ref array<ResourceName> items = {};
		//Wow I hate this, gotta scan through all the pouchs cause GetAll, in fact, does not get all :O
		foreach (IEntity pouch: pouches)
		{
			if (!pouch || !pouch.GetPrefabData())
				continue;

			if (!pouch.FindComponent(BaseInventoryStorageComponent))
			{
				items.Insert(pouch.GetPrefabData().GetPrefabName());
				continue;
			}
			
			ref array<IEntity> tempItems = {};
			BaseInventoryStorageComponent.Cast(pouch.FindComponent(BaseInventoryStorageComponent)).GetAll(tempItems);
			foreach (IEntity tempItem: tempItems)
			{
				if (tempItem && tempItem.GetPrefabData())
					items.Insert(tempItem.GetPrefabData().GetPrefabName());
			}
		}
		if (!invManager.CanReplaceItem(newItem, invComponent, slotId))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newItem);
			return;
		}
		InventoryItemComponent oldItemComp = InventoryItemComponent.Cast(oldItem.FindComponent(InventoryItemComponent));
		InventoryItemComponent newItemComp = InventoryItemComponent.Cast(newItem.FindComponent(InventoryItemComponent));
		if (oldItemComp && newItemComp && oldItemComp.GetUIInfo() && newItemComp.GetUIInfo())
		{
			string oldItemName = oldItemComp.GetUIInfo().GetName();
			string newItemName = newItemComp.GetUIInfo().GetName();
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(GetGame().GetPlayerManager().GetPlayerName(playerId) + " has replaced " + oldItemName + " with " + 
			newItemName, playerId, false);
		}
		
		// This instance was only needed for compatibility validation. Recreate it
		// after the delay so the call queue retains value data rather than entities
		// or native inventory components.
		SCR_EntityHelper.DeleteEntityAndChildren(newItem);
		SCR_EntityHelper.DeleteEntityAndChildren(oldItem);
		GetGame().GetCallqueue().CallLater(AddVestDelay, 250, false, playerId, newResource, slotId, items);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void AddVestDelay(int playerId, ResourceName newResource, int slotId, array<ResourceName> items)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;

		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent invComponent = BaseInventoryStorageComponent.Cast(player.FindComponent(BaseInventoryStorageComponent));
		Resource itemResource = Resource.Load(newResource);
		if (!invManager || !invComponent || slotId < 0 || !itemResource || !itemResource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		IEntity newItem = GetGame().SpawnEntityPrefab(itemResource, null, params);
		if (!newItem)
			return;

		if (!invManager.TryReplaceItem(newItem, invComponent, slotId))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newItem);
			return;
		}

		GetGame().GetCallqueue().CallLater(AddItemDelay, 275, false, playerId, slotId, items);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void AddItemDelay(int playerId, int slotId, array<ResourceName> items)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player || !items)
			return;

		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent invComponent = BaseInventoryStorageComponent.Cast(player.FindComponent(BaseInventoryStorageComponent));
		if (!invManager || !invComponent || slotId < 0)
			return;

		IEntity newItem = invComponent.Get(slotId);
		if (!newItem)
			return;

		BaseInventoryStorageComponent newStorageComp = BaseInventoryStorageComponent.Cast(newItem.FindComponent(BaseInventoryStorageComponent));
		if (!newStorageComp)
			return;

		for (int i = 0; i < items.Count(); i++)
		{
			if(!invManager.TrySpawnPrefabToStorage(items[i], newStorageComp))
			{
				ref array<IEntity> newPouches = {};
				newStorageComp.GetAll(newPouches);
				
				bool itemInserted = false;
				foreach (IEntity pouch: newPouches)
				{
					if (!pouch.FindComponent(BaseInventoryStorageComponent))
						continue;
					
					BaseInventoryStorageComponent pouchStorage = BaseInventoryStorageComponent.Cast(pouch.FindComponent(BaseInventoryStorageComponent));
					if(invManager.TrySpawnPrefabToStorage(items[i], pouchStorage))
					{
						itemInserted = true;
						break;
					}
				}
				if (!itemInserted)
					invManager.TrySpawnPrefabToStorage(items[i]);
			}
		}
		GetGame().GetCallqueue().CallLater(InitializePlayerRadiosDelayed, 500, false, playerId);

		CRF_PlayerRplToOwnerManager ownerManager = CRF_PlayerRplToOwnerManager.GetInstance();
		if (ownerManager)
			ownerManager.InitializeRadioFromServer();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializePlayerRadiosDelayed(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (groupsManager)
			groupsManager.TuneFreqDelayWithPresets(playerId, player);

		CRF_PlayerController playerController = CRF_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (playerController)
			playerController.InitializeRadios(player);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MiniArsenalRequestNewWeapon(int playerId, string newWeaponResource, array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, bool isPistol)
	{
		if (!attachments || !magazines || !magazineCounts || magazines.Count() != magazineCounts.Count())
			return;

		// Telemetry: 2 ints + string + 2 ResourceName arrays + int array + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(newWeaponResource);
		
		// Manually calculate ResourceName array sizes
		bytes += 4; // Array length for attachments
		foreach (ResourceName attachment : attachments)
			bytes += CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(attachment);
		
		bytes += 4; // Array length for magazines
		foreach (ResourceName magazine : magazines)
			bytes += CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(magazine);
		
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_IntArray(magazineCounts);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_MiniArsenalRequestNewWeapon", bytes);
		
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player || !player.GetPrefabData())
			return;
		
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(player.GetPrefabData().GetPrefabName());
		
		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		Resource weaponResource = Resource.Load(newWeaponResource);
		if (!weaponResource || !weaponResource.IsValid())
			return;

		IEntity newWeapon = GetGame().SpawnEntityPrefab(weaponResource, null, params);
		if (!newWeapon)
			return;
		
		SCR_InventoryStorageManagerComponent storageMan = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterInventoryStorageComponent storageComp = SCR_CharacterInventoryStorageComponent.Cast(player.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		if (!storageMan || !storageComp || !charController || !charController.GetWeaponManagerComponent())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
			return;
		}
		
		array<IEntity> items = {};
		storageMan.GetItems(items);
		
		array<WeaponSlotComponent> weaponSlots = {};
		charController.GetWeaponManagerComponent().GetWeaponsSlots(weaponSlots);
		int weaponSlotIndex = 2;
		if (isPistol)
			weaponSlotIndex = 4;

		if (!weaponSlots.IsIndexValid(weaponSlotIndex) || !weaponSlots[weaponSlotIndex])
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
			return;
		}

		IEntity weapon;
		weapon = weaponSlots[weaponSlotIndex].GetWeaponEntity();
		
		if (!weapon)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
			return;
		}
		
		WeaponComponent weaponComp = WeaponComponent.Cast(weapon.FindComponent(WeaponComponent));
		if (!weaponComp)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
			return;
		}

		array<BaseMuzzleComponent> muzzles = {};
		weaponComp.GetMuzzlesList(muzzles);
		
		array<BaseMagazineWell> magazineWells = {};
		foreach (BaseMuzzleComponent muzzle: muzzles)
		{
			if (muzzle && muzzle.GetMagazineWell())
				magazineWells.Insert(muzzle.GetMagazineWell());
		}
		//Delete all magazines related to the old weapon.
		foreach (IEntity item: items)
		{
			if (!item.FindComponent(MagazineComponent))
				continue;
			MagazineComponent magComp = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			if (!magComp.GetMagazineWell())
				continue;
			foreach (BaseMagazineWell magazineWell: magazineWells)
			{
				if (magComp.GetMagazineWell().Type() == magazineWell.Type())
				{
					SCR_EntityHelper.DeleteEntityAndChildren(item);
					break;
				}
			}
		}
		
		InventoryItemComponent oldItemComp = InventoryItemComponent.Cast(weapon.FindComponent(InventoryItemComponent));
		InventoryItemComponent newItemComp = InventoryItemComponent.Cast(newWeapon.FindComponent(InventoryItemComponent));
		if (oldItemComp && newItemComp && oldItemComp.GetUIInfo() && newItemComp.GetUIInfo())
		{
			string oldItemName = oldItemComp.GetUIInfo().GetName();
			string newItemName = newItemComp.GetUIInfo().GetName();
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(GetGame().GetPlayerManager().GetPlayerName(playerId) + " has replaced " + oldItemName + " with " + 
			newItemName, playerId, false);
		}
		
		// Recreate the requested weapon after the delay. This avoids retaining
		// entities and native inventory components in ScriptCallQueue arguments.
		SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
		SCR_EntityHelper.DeleteEntityAndChildren(weapon);
		GetGame().GetCallqueue().CallLater(MiniArsenalRequestNewWeaponDelay, 500, false, playerId, newWeaponResource, attachments, magazines, magazineCounts, role);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void MiniArsenalRequestNewWeaponDelay(int playerId, ResourceName newWeaponResource, array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, CRF_EGearRole role)
	{
		if (!attachments || !magazines || !magazineCounts || magazines.Count() != magazineCounts.Count())
			return;

		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;

		SCR_InventoryStorageManagerComponent storageMan = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterInventoryStorageComponent storageComp = SCR_CharacterInventoryStorageComponent.Cast(player.FindComponent(SCR_CharacterInventoryStorageComponent));
		Resource weaponResource = Resource.Load(newWeaponResource);
		if (!storageMan || !storageComp || !weaponResource || !weaponResource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		IEntity newWeapon = GetGame().SpawnEntityPrefab(weaponResource, null, params);
		if (!newWeapon)
			return;

		if (!storageMan.TryInsertItem(newWeapon))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newWeapon);
			return;
		}

		int currentMagazine = 0;
		foreach (int magazineCount: magazineCounts)
		{
			if (!magazines.IsIndexValid(currentMagazine))
				break;

			for (int i = 0; i < magazineCount; i++)
			{
				CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
				if (gearscriptManager)
					gearscriptManager.AddInventoryItem(magazines[currentMagazine], 1, params, storageComp, storageMan, role);
			}
			
			currentMagazine++;
		}
		
		BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(newWeapon.FindComponent(BaseInventoryStorageComponent));
		BaseWeaponComponent newWeaponComp = BaseWeaponComponent.Cast(newWeapon.FindComponent(BaseWeaponComponent));
		if (!weaponStorageComp || !newWeaponComp)
			return;

		array<AttachmentSlotComponent> attachmentSlots = {};
		newWeaponComp.GetAttachments(attachmentSlots);

		foreach (ResourceName attachment: attachments)
		{
			Resource attachmentResource = Resource.Load(attachment);
			if (!attachmentResource || !attachmentResource.IsValid())
				continue;

			IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(attachmentResource, GetGame().GetWorld(), params);
			if (!attachmentSpawned)
				continue;
			
			bool attached = false;
			foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
			{
				if (attachmentSlot && attachmentSlot.CanSetAttachment(attachmentSpawned))
				{
					IEntity oldSight = attachmentSlot.GetAttachedEntity();
					if (oldSight)
						SCR_EntityHelper.DeleteEntityAndChildren(oldSight);
				
					storageMan.TryInsertItemInStorage(attachmentSpawned, weaponStorageComp);
					attached = true;
					break;
				}
			}

			if (!attached)
				SCR_EntityHelper.DeleteEntityAndChildren(attachmentSpawned);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized RPC using index-based lookup (6 bytes vs 134 bytes)
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SightArsenalRequestNewSight_Optimized(int playerId, int sightIndex, CRF_ESightType sightType)
	{
		// Telemetry: int (4) + int as byte (1) + enum as byte (1) = ~6 bytes
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += 2; // sightIndex + sightType as bytes
		LogTelemetry("RpcAsk_SightArsenalRequestNewSight_Optimized", bytes);
		
		// Convert index back to resource
		ResourceName newResource = CRF_SightArsenalRegistry.GetSightResource(sightIndex);
		if (newResource == "")
		{
			Print(string.Format("[CRF] Error: Invalid sight index received: %1", sightIndex), LogLevel.ERROR);
			return;
		}
		
		// Use shared implementation
		ProcessSightChange(playerId, newResource, sightType);
	}
	
	//------------------------------------------------------------------------------------------------
	// Fallback RPC for unknown sights (mod compatibility)
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SightArsenalRequestNewSight_Fallback(int playerId, string newResource, string type)
	{
		// Telemetry: int + 2 strings (original bandwidth)
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(newResource);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(type);
		LogTelemetry("RpcAsk_SightArsenalRequestNewSight_Fallback", bytes);
		
		CRF_ESightType sightType = CRF_SightArsenalRegistry.GetSightTypeFromString(type);
		ProcessSightChange(playerId, newResource, sightType);
	}
	
	//------------------------------------------------------------------------------------------------
	// Legacy RPC - deprecated but kept for backwards compatibility
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SightArsenalRequestNewSight(int playerId, string newResource, string type)
	{
		// Telemetry: int + 2 strings
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(newResource);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_String(type);
		LogTelemetry("RpcAsk_SightArsenalRequestNewSight_Legacy", bytes);
		
		CRF_ESightType sightType = CRF_SightArsenalRegistry.GetSightTypeFromString(type);
		ProcessSightChange(playerId, newResource, sightType);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get attachment type from sight prefab
	protected BaseAttachmentType GetSightAttachmentType(ResourceName sightResource)
	{
		Resource loadedSight = Resource.Load(sightResource);
		if (!loadedSight)
			return null;
			
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedSight);
		if (!entitySource)
			return null;
		
		for (int nComponent = 0, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		{
			IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
			if (componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
			{
				BaseContainer attributesContainer = componentSource.GetObject("Attributes");
				if (!attributesContainer)
					continue;
					
				BaseContainerList itemAttributes = attributesContainer.GetObjectArray("CustomAttributes");
				if (!itemAttributes)
					continue;
					
				for (int i = 0; i < itemAttributes.Count(); i++)
				{
					BaseContainer attributeContainer = itemAttributes.Get(i);
					if (attributeContainer.GetClassName().ToType().IsInherited(WeaponAttachmentAttributes))
					{
						BaseAttachmentType attachmentType;
						attributeContainer.Get("AttachmentType", attachmentType);
						if (attachmentType)
							return attachmentType;
					}
				}
			}
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	// Shared sight change implementation
	protected void ProcessSightChange(int playerId, ResourceName newResource, CRF_ESightType sightType)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
			
		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		IEntity newSight = GetGame().SpawnEntityPrefab(Resource.Load(newResource), null, params);
		if (!newSight)
		{
			Print(string.Format("[CRF] Error: Failed to spawn sight: %1", newResource), LogLevel.ERROR);
			return;
		}
		
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		if (!charController)
		{
			Print("[CRF] Error: Failed to get character controller for sight change", LogLevel.ERROR);
			delete newSight;
			return;
		}
		
		BaseWeaponComponent currentWeapon = charController.GetWeaponManagerComponent().GetCurrentWeapon();
		if (!currentWeapon)
		{
			Print("[CRF] Error: No weapon equipped for sight change", LogLevel.ERROR);
			delete newSight;
			return;
		}
		
		array<AttachmentSlotComponent> attachments = {};
		currentWeapon.GetAttachments(attachments);
		AttachmentSlotComponent sightAttachment;
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent weaponInv = BaseInventoryStorageComponent.Cast(currentWeapon.GetOwner().FindComponent(BaseInventoryStorageComponent));
		
		// Get the attachment type from the sight prefab
		BaseAttachmentType sightAttachmentType = GetSightAttachmentType(newResource);
		if (!sightAttachmentType)
		{
			Print(string.Format("[CRF] Error: Could not determine attachment type for sight: %1", newResource), LogLevel.ERROR);
			delete newSight;
			return;
		}
		
		// Find matching attachment slot
		foreach (AttachmentSlotComponent attachment: attachments)
		{
			if (!attachment.GetAttachmentSlotType())
				continue;
				
			if (sightAttachmentType.Type().IsInherited(attachment.GetAttachmentSlotType().Type()))
			{
				sightAttachment = attachment;
				break;
			}
		}
		
		if (!sightAttachment)
		{
			Print(string.Format("[CRF] Error: No matching sight attachment slot found for type: %1", sightAttachmentType.Type()), LogLevel.ERROR);
			delete newSight;
			return;
		}
		
		IEntity oldSight = sightAttachment.GetAttachedEntity();
		if (sightAttachment.CanSetAttachment(newSight))
		{
			if (oldSight)
				delete oldSight;
			
			invManager.TryInsertItemInStorage(newSight, weaponInv);
		}
		else
		{
			Print("[CRF] Error: Cannot attach sight to slot", LogLevel.ERROR);
			delete newSight;
			return;
		}
		
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(newSight.FindComponent(InventoryItemComponent));
		if (itemComp && itemComp.GetUIInfo())
		{
			CRF_RplBroadcastManager.GetInstance().LogAdminAction(
				GetGame().GetPlayerManager().GetPlayerName(playerId) + " has replaced their sight with " + 
				itemComp.GetUIInfo().GetName(), playerId, false);
		}
	}
    
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TogglePlayerLisntening(int playerId, bool input)
	{
		// Telemetry: int + bool
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Bool();
		LogTelemetry("RpcAsk_TogglePlayerLisntening", bytes);
		
		CVON_VONGameModeComponent cvon = CVON_VONGameModeComponent.GetInstance();
		if (cvon)
			cvon.TogglePlayerListening(playerId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleWaveRespawn()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleWaveRespawn", 0);
		
		CRF_RespawnManager.GetInstance().ToggleRespawnWave();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleRespawn()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleRespawn", 0);
		
		CRF_RespawnManager.GetInstance().ToggleRespawn();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ToggleVAARRecording()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_ToggleVAARRecording", 0);
		
		CRF_VAAR_GamemodeComponent.GetInstance().ToggleRecording();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetRespawnTime(int seconds)
	{
		// Telemetry: int
		LogTelemetry("RpcAsk_SetRespawnTime", CRF_BandwidthTelemetryManager.EstimateSize_Int());
		
		CRF_RespawnManager.GetInstance().SetRespawnTime(seconds);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CleanUpBodies()
	{
		// Telemetry: no parameters
		LogTelemetry("RpcAsk_CleanUpBodies", 0);
		
		CRF_GarbageManager.GetInstance().CleanUpBodies();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_AddItemToTruck(RplId truckId, ResourceName item, int amount, array<RplId> supplyItems, array<int> supplyCounts, RplId supplyArsenalId)
	{
		if (!supplyItems || !supplyCounts || supplyItems.Count() != supplyCounts.Count())
			return;

		// Telemetry: 3 RplIds + ResourceName + int + RplId array + int array
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_RplId() * 3;
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_ResourceName(item);
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_Int();
		bytes += 4 + (supplyItems.Count() * CRF_BandwidthTelemetryManager.EstimateSize_RplId()); // Array length + items
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_IntArray(supplyCounts);
		LogTelemetry("RpcAsk_AddItemToTruck", bytes);
		
		for (int i = 0; i < supplyItems.Count(); i++)
		{
			IEntity supplyDepot = ResolveReplicatedEntity(supplyItems[i]);
			if (!supplyDepot)
				continue;

			SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.Cast(supplyDepot.FindComponent(SCR_ResourceComponent));
            if (!resourceComponent)
                continue;
                
            SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT_STORAGE, EResourceType.SUPPLIES);
			
			if (!consumer)
				consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			
			if (!consumer)
				return;

           	consumer.RequestConsumtion(supplyCounts[i]);
		}
		
		IEntity truck = ResolveReplicatedEntity(truckId);
		if (!truck)
			return;

		IEntity supplyArsenal = ResolveReplicatedEntity(supplyArsenalId);
		if (!supplyArsenal)
			return;

		CRF_SupplyArsenalComponent supplyComp = CRF_SupplyArsenalComponent.Cast(supplyArsenal.FindComponent(CRF_SupplyArsenalComponent));
		if (supplyComp)
			supplyComp.UpdateCurrentSupply();

		SCR_VehicleInventoryStorageManagerComponent vehInventory = SCR_VehicleInventoryStorageManagerComponent.Cast(truck.FindComponent(SCR_VehicleInventoryStorageManagerComponent));
		if (!vehInventory)
			return;

		for (int i = 0; i < amount; i++)
		{
			vehInventory.TrySpawnPrefabToStorage(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateSupplyArsneal(RplId supplyArsenalId)
	{
		// Telemetry: RplId
		LogTelemetry("RpcAsk_UpdateSupplyArsneal", CRF_BandwidthTelemetryManager.EstimateSize_RplId());
		
		IEntity supplyArsenal = ResolveReplicatedEntity(supplyArsenalId);
		if (!supplyArsenal)
			return;

		CRF_SupplyArsenalComponent supplyComp = CRF_SupplyArsenalComponent.Cast(supplyArsenal.FindComponent(CRF_SupplyArsenalComponent));
		if (supplyComp)
			supplyComp.UpdateCurrentSupply();
	}	
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MoveSpecCamToSlot(int slotID, int playerId)
	{
		// Get slot data from the slotting manager
		CRF_SlotData slotData = CRF_SlottingManager.GetInstance().GetSlotData(slotID);
		if (!slotData)
			return;
		
		// Find the entity associated with the slot and set it as the spectator target
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter()));
		if (!rplComponent)
			return;
		
		// Get slot origin
		IEntity slotEntity = rplComponent.GetEntity();
		if (!slotEntity)
			return;

		vector slotPos = slotEntity.GetOrigin();
				
		m_RplBroadcastManager.MoveSpecCamToSlot(slotPos, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CreateCache(RplId truckId, RplId playerId)
	{
		// Telemetry: 2 RplIds
		LogTelemetry("RpcAsk_CreateCache", CRF_BandwidthTelemetryManager.EstimateSize_RplId() * 2);
		
		IEntity truck = ResolveReplicatedEntity(truckId);
		IEntity player = ResolveReplicatedEntity(playerId);
		if (!truck || !player)
			return;

		truck = truck.GetRootParent();
		if (!truck)
			return;

		SCR_VehicleInventoryStorageManagerComponent vehInventory = SCR_VehicleInventoryStorageManagerComponent.Cast(truck.FindComponent(SCR_VehicleInventoryStorageManagerComponent));
		if (!vehInventory)
			return;

		array<IEntity> items = {};
		vehInventory.GetItems(items);
		if (items.Count() == 0)
			return;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = GetSpawn(player);
		
		Vehicle vehicle = Vehicle.Cast(truck);
		if (!vehicle)
			return;

		ResourceName prefab;
		if (vehicle.m_sFactionKey == "BLUFOR")
			prefab = "{0E3A25C772CDDC95}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/US/CRF_BLUFOR_Cache.et";
		else
			prefab = "{F636545E6893F50B}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/USSR/CRF_OPFOR_Cache.et";
		
		Resource cacheResource = Resource.Load(prefab);
		if (!cacheResource || !cacheResource.IsValid())
			return;

		IEntity cache = GetGame().SpawnEntityPrefab(cacheResource, GetGame().GetWorld(), spawnParams);
		if (!cache)
			return;

		RplId cacheId = Replication.FindItemId(cache);
		if (cacheId == RplId.Invalid())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(cache);
			return;
		}

		array<ResourceName> itemResources = {};
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;

			itemResources.Insert(item.GetPrefabData().GetPrefabName());
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}

		GetGame().GetCallqueue().CallLater(CreateCacheDelay, 100, false, cacheId, itemResources);
	}
	
	//------------------------------------------------------------------------------------------------
	protected vector GetSpawn(IEntity player)
	{
		if (!player)
			return "0 0 0";
	
		// Get player transform (position + orientation)
		vector mat[4];
		player.GetTransform(mat);
	
		// Direction player is facing (normalized forward vector)
		vector forwardDir = mat[2];
	
		// Calculate new position 0.5 meters behind the player
		vector behindPos = mat[3] - (forwardDir * 0.5);
		return behindPos;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CreateCacheDelay(RplId cacheId, array<ResourceName> itemResources)
	{
		IEntity cache = ResolveReplicatedEntity(cacheId);
		if (!cache || !itemResources)
			return;

		SCR_InventoryStorageManagerComponent invComp = SCR_InventoryStorageManagerComponent.Cast(cache.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!invComp)
			return;
		
			
		foreach (ResourceName itemResource : itemResources)
		{
			if (itemResource.IsEmpty())
				continue;

			invComp.TrySpawnPrefabToStorage(itemResource);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestVehicleSupplies(RplId truckId)
	{
		// Telemetry: RplId
		LogTelemetry("RpcAsk_RequestVehicleSupplies", CRF_BandwidthTelemetryManager.EstimateSize_RplId());
		
		if (!Replication.FindItem(truckId))
			return;
		
		IEntity truck = ResolveReplicatedEntity(truckId);
		Vehicle vehicle = Vehicle.Cast(truck);
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!vehicle || !vehicleGearscriptManager)
			return;

		vehicle.UpdateVehicleSupplies(vehicleGearscriptManager.GetSuppliesInTruck(truck));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RearmVehicle(RplId truckId, array<RplId> supplyItems, array<int> supplyCounts, RplId rearmTruckId)
	{
		if (!supplyItems || !supplyCounts || supplyItems.Count() != supplyCounts.Count())
			return;

		// Telemetry: 2 RplIds + RplId array + int array
		int bytes = CRF_BandwidthTelemetryManager.EstimateSize_RplId() * 2;
		bytes += 4 + (supplyItems.Count() * CRF_BandwidthTelemetryManager.EstimateSize_RplId()); // Array length + items
		bytes += CRF_BandwidthTelemetryManager.EstimateSize_IntArray(supplyCounts);
		LogTelemetry("RpcAsk_RearmVehicle", bytes);
		
		if (!Replication.FindItem(truckId))
			return;
		
		IEntity truck = ResolveReplicatedEntity(truckId);
		Vehicle vehicle = Vehicle.Cast(truck);
		CRF_VehicleGearscriptManager vehicleGearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
		if (!vehicle || !vehicleGearscriptManager)
			return;

		vehicleGearscriptManager.SetVehicleGear(truck, vehicle.m_sFactionKey);
		for (int i = 0; i < supplyItems.Count(); i++)
		{
			IEntity supplyDepot = ResolveReplicatedEntity(supplyItems[i]);
			if (!supplyDepot)
				continue;

			SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.Cast(supplyDepot.FindComponent(SCR_ResourceComponent));
            if (!resourceComponent)
                continue;
                
            SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT_STORAGE, EResourceType.SUPPLIES);
			
			if (!consumer)
				consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			
			if (!consumer)
				return;

           	consumer.RequestConsumtion(supplyCounts[i]);
		}
		
		IEntity rearmTruck = ResolveReplicatedEntity(rearmTruckId);
		if (!rearmTruck)
			return;

		CRF_SupplyArsenalComponent supplyComp = CRF_SupplyArsenalComponent.Cast(rearmTruck.FindComponent(CRF_SupplyArsenalComponent));
		if (supplyComp)
			supplyComp.UpdateCurrentSupply();

		vehicle.UpdateVehicleSupplies(vehicleGearscriptManager.GetSuppliesInTruck(truck));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestForwardDeploy(vector cursorWorldPos, string factionKey, int playerId)
	{
		LogTelemetry("RpcAsk_RequestForwardDeploy", CRF_BandwidthTelemetryManager.EstimateSize_Vector() + CRF_BandwidthTelemetryManager.EstimateSize_String(factionKey) + CRF_BandwidthTelemetryManager.EstimateSize_Int());
		IEntity polyzone;
		cursorWorldPos[1] = SCR_TerrainHelper.GetTerrainY(cursorWorldPos);
		foreach (IEntity zone: CRF_ForwardDeployManager.GetInstance().GetForwardDeployZones())
		{
			CRF_PolyZone zoneComp = CRF_PolyZone.Cast(zone.FindComponent(CRF_PolyZone));
			if (!zoneComp)
				continue;
			
			if (!zoneComp.IsInsidePolygon(Vector(cursorWorldPos[0], 0, cursorWorldPos[2])))
				continue;
			
			if (!zoneComp.m_aVisibleForFactions.Contains(factionKey))
				continue;
			
			polyzone = zone;
			break;
		}
		
		if (!polyzone)
		{
			CRF_PlayerRplToOwnerManager.GetInstance().ForwardDeployRequestRejected();
			return;
		}
		
		array<IEntity> teleportedVehicles = {};
		array<AIAgent> players = {};
		array<IEntity> entities = {};
		
		SCR_GroupsManagerComponent groupMan = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup playerGroup = groupMan.GetPlayerGroup(playerId);
		if (!playerGroup)
		{
		    CRF_PlayerRplToOwnerManager.GetInstance().ForwardDeployRequestRejected();
		    return;
		}
		playerGroup.GetAgents(players);
		foreach (AIAgent agent : players)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
				
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (!character)
				continue;
			
			entities.Insert(entity);
		}
		foreach (IEntity entity: entities)
		{
			int currentPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
			if (currentPlayerId <= 0)
				continue;
			SCR_CompartmentAccessComponent compartmentAccess = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
			if (compartmentAccess)
			{
				IEntity vehicle = compartmentAccess.GetVehicle();
				
				if (vehicle)
				{
					SCR_BaseCompartmentManagerComponent compartmentMan = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
					array<BaseCompartmentSlot> slots = {};
					compartmentMan.GetCompartments(slots);
					//Check if majority of the vic is the same group, if not don't teleport.
					int amountInGroup = 0;
					int amountNotInGroup = 0;
					foreach (BaseCompartmentSlot slot: slots)
					{
						if (!slot.IsOccupied())
							continue;
						
						if (!slot.GetOccupant().FindComponent(FactionAffiliationComponent))
							continue;
						
						if (entities.Contains(slot.GetOccupant()))
							amountInGroup++;
						else
							amountNotInGroup++;
					}
					if (amountInGroup < amountNotInGroup)
						continue; 
					if (teleportedVehicles.Contains(vehicle))
						continue;
					teleportedVehicles.Insert(vehicle);
					CRF_ForwardDeployManager.GetInstance().CreateForwardDeployRequest(currentPlayerId, cursorWorldPos);
					continue;
				}
			}
			CRF_ForwardDeployManager.GetInstance().CreateForwardDeployRequest(currentPlayerId, cursorWorldPos);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Handles a JIP (Join In Progress) forward deploy request: teleports the requesting player to the
	//! selected friendly group's current live position, unless that unit is in/near contact with the enemy.
	//! Only reachable while JIP is enabled (Disable JIP off) and SafeStart has ended - see CRF_PlayerController.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestJIPForwardDeploy(RplId groupId, int playerId)
	{
		LogTelemetry("RpcAsk_RequestJIPForwardDeploy", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);

		if (!m_Gamemode || m_Gamemode.m_bLockUnusedSlots)
			return;

		if (!m_SafestartManager || m_SafestartManager.GetSafestartStatus())
			return;

		if (!m_SlottingManager)
			return;

		Faction requesterFaction = m_SlottingManager.GetPlayerSlotFaction(playerId, true);
		if (!requesterFaction)
			return;

		SCR_AIGroup targetGroup = CRF_EntityHelper.GetGroupFromRplId(groupId);
		if (!targetGroup || !targetGroup.GetFaction() || targetGroup.GetFaction().GetFactionKey() != requesterFaction.GetFactionKey())
			return;

		vector targetPosition;
		if (!CRF_EntityHelper.GetGroupOrigin(targetGroup, targetPosition))
		{
			if (m_RplBroadcastManager)
				m_RplBroadcastManager.ShowPlayerHint(playerId, "Your unit could not be located.");
			return;
		}

		CRF_ForwardDeployManager forwardDeployManager = CRF_ForwardDeployManager.GetInstance();
		if (!forwardDeployManager)
			return;

		if (forwardDeployManager.IsPositionNearEnemy(targetPosition, requesterFaction))
		{
			if (m_RplBroadcastManager)
				m_RplBroadcastManager.ShowPlayerHint(playerId, "Your unit is in contact with the enemy. Forward deploy is unavailable.");
			return;
		}

		forwardDeployManager.CreateForwardDeployRequest(playerId, targetPosition);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ShareMapMarkerGlobal(int markerUID, string factionKey, int playerId)
	{
		array<int> playerIds = {};
		PlayerManager pm = GetGame().GetPlayerManager();
		pm.GetPlayers(playerIds);
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		if (!m_MapMarkerManager)
			m_MapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
		if (!m_MapMarkerManager)
			return;
		
		Faction localPlayerFaction = factionManager.GetPlayerFaction(playerId);
		
		if (!m_MapMarkerManager.m_aGlobalMarkers.Contains(markerUID) && localPlayerFaction)
			m_MapMarkerManager.m_aGlobalMarkers.Insert(markerUID, localPlayerFaction.GetFactionKey());
		
		foreach (int otherPlayerId: playerIds)
		{
			if (playerId == otherPlayerId)
				continue;
			
			Faction playerFaction = factionManager.GetPlayerFaction(otherPlayerId);
			if (!playerFaction)
				continue;
			
			if (playerFaction.GetFactionKey() != factionKey)
				continue;
			
			PlayerController otherPc = pm.GetPlayerController(otherPlayerId);
			if (!otherPc)
				continue;
			CRF_PlayerRplToOwnerManager rplToOwnerManager = CRF_PlayerRplToOwnerManager.Cast(otherPc.FindComponent(CRF_PlayerRplToOwnerManager));
			if (!rplToOwnerManager)
				continue;
			
			rplToOwnerManager.SharerMarkerGlobal(markerUID);
			array<int> tempMarkerArray = {};
			tempMarkerArray.Insert(markerUID);
			m_MapMarkerManager.UpdateSharedMarkers(tempMarkerArray, otherPlayerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ShareMapMarkers(array<int> markerUIDs, int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		IEntity playerEntity = pm.GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;
		
		if (!m_MapMarkerManager)
			m_MapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
		if (!m_MapMarkerManager)
			return;
		
		// Get the faction of the sharing player
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMan)
			return;
			
		Faction sharingPlayerFaction = factionMan.GetPlayerFaction(playerId);
		if (!sharingPlayerFaction)
			return;
		
		array<int> playerIds = {};
		pm.GetPlayers(playerIds);
		foreach (int otherPlayerId: playerIds)
		{
			if (otherPlayerId == playerId)
				continue;
			
			IEntity entity = pm.GetPlayerControlledEntity(otherPlayerId);
			if (!entity)
				continue;
			
			// Check distance - only share with nearby players
			if (vector.Distance(playerEntity.GetOrigin(), entity.GetOrigin()) > m_MapMarkerManager.MARKER_SHARE_DISTANCE)
				continue;
			
			// Check faction - only share with same faction players
			Faction otherPlayerFaction = factionMan.GetPlayerFaction(otherPlayerId);
			if (!otherPlayerFaction || otherPlayerFaction != sharingPlayerFaction)
				continue;
			
			PlayerController otherPc = pm.GetPlayerController(otherPlayerId);
			if (!otherPc)
				continue;
			CRF_PlayerRplToOwnerManager rplToOwnerManager = CRF_PlayerRplToOwnerManager.Cast(otherPc.FindComponent(CRF_PlayerRplToOwnerManager));
			if (!rplToOwnerManager)
				continue;
				
			rplToOwnerManager.ShareMarker(markerUIDs);
			m_MapMarkerManager.UpdateSharedMarkers(markerUIDs, otherPlayerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestGlobalMarkerRefresh(int playerId)
	{
		int bytes = m_TelemetryManager.EstimateSize_Int();
		LogTelemetry("RpcAsk_RequestGlobalMarkerRefresh", bytes);
		// Get the faction of the sharing player
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMan)
			return;
			
		Faction sharingPlayerFaction = factionMan.GetPlayerFaction(playerId);
		if (!sharingPlayerFaction)
			return;
		
		string playerFactionKey = sharingPlayerFaction.GetFactionKey();
		
		if (!m_MapMarkerManager)
			m_MapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
		if (!m_MapMarkerManager)
			return;

		array<int> globalMarkers = {};
		foreach (int markerUID, string factionKey: m_MapMarkerManager.m_aGlobalMarkers)
		{
			if (factionKey != playerFactionKey)
				continue;
			
			globalMarkers.Insert(markerUID);
		}
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc)
			return;
		CRF_PlayerRplToOwnerManager rplToOwnerManager = CRF_PlayerRplToOwnerManager.Cast(pc.FindComponent(CRF_PlayerRplToOwnerManager));
		if (!rplToOwnerManager)
			return;
		
		rplToOwnerManager.RefreshGlobalMarkers(globalMarkers);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_RequestSupplyUpdate(RplId supplyArsenalId)
	{
		IEntity supplyArsenal = ResolveReplicatedEntity(supplyArsenalId);
		if (!supplyArsenal)
			return;

		CRF_SupplyArsenalComponent supplyComp = CRF_SupplyArsenalComponent.Cast(supplyArsenal.FindComponent(CRF_SupplyArsenalComponent));
		if (!supplyComp)
			return;
		supplyComp.UpdateCurrentSupply();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RegisterPlayerForLottery(int playerId, string factionKey, string squadFilter)
	{
		CRF_SlotLottery slotLottery = CRF_SlotLottery.GetInstance();
		if (slotLottery)
			slotLottery.RegisterPlayerForLottery_Server(playerId, factionKey, squadFilter);
	}

	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RunSlotLottery(int requestingPlayerId)
	{
		CRF_SlotLottery slotLottery = CRF_SlotLottery.GetInstance();
		if (slotLottery)
			slotLottery.RunSlotLottery_Server(requestingPlayerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ClearSlotLottery(int requestingPlayerId)
	{
		CRF_SlotLottery slotLottery = CRF_SlotLottery.GetInstance();
		if (slotLottery)
			slotLottery.ClearSlotLottery_Server(requestingPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestObjectiveTaskComplete(int taskIndex, int completingSide)
	{
		// Telemetry: int x2
		LogTelemetry("RpcAsk_RequestObjectiveTaskComplete", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 2);

		CRF_TaskCreatorComponent taskComp = CRF_TaskCreatorComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_TaskCreatorComponent));
		if (taskComp)
			taskComp.MarkTaskComplete(taskIndex, completingSide);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestTaskObjectSetState(int taskIndex, int newState, int planterSide)
	{
		LogTelemetry("RpcAsk_RequestTaskObjectSetState", CRF_BandwidthTelemetryManager.EstimateSize_Int() * 3);

		CRF_TaskCreatorComponent taskComp = CRF_TaskCreatorComponent.GetInstance();
		if (!taskComp)
			return;

		IEntity taskObject = taskComp.GetSpawnedTaskObject(taskIndex);
		if (!taskObject)
			return;

		CRF_TaskCreatorObjectComponent objComp = CRF_TaskCreatorObjectComponent.Cast(taskObject.FindComponent(CRF_TaskCreatorObjectComponent));
		if (objComp)
		{
			// Set planter side if transitioning to a planted state (newState == 1 for PLANTED in bomb tasks)
			if (planterSide > 0 && newState == 1)
				objComp.SetPlanterSide(planterSide);
			objComp.SetTaskObjectState(newState);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestPlayPositionalSound(string soundResource, string soundEvent, vector position)
	{
		LogTelemetry("RpcAsk_RequestPlayPositionalSound",
			CRF_BandwidthTelemetryManager.EstimateSize_String(soundResource)
			+ CRF_BandwidthTelemetryManager.EstimateSize_String(soundEvent)
			+ CRF_BandwidthTelemetryManager.EstimateSize_Vector());

		CRF_RplBroadcastManager broadcast = CRF_RplBroadcastManager.GetInstance();
		if (broadcast)
			broadcast.PlayPositionalSound(soundResource, soundEvent, position);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestStopPositionalSound(string soundEvent)
	{
		LogTelemetry("RpcAsk_RequestStopPositionalSound",
			CRF_BandwidthTelemetryManager.EstimateSize_String(soundEvent));

		CRF_RplBroadcastManager broadcast = CRF_RplBroadcastManager.GetInstance();
		if (broadcast)
			broadcast.StopPositionalSound(soundEvent);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerRplToAuthorityManager m_sInstance;
	void CRF_PlayerRplToAuthorityManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_PlayerRplToAuthorityManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_PlayerRplToAuthorityManager GetInstance()
	{
		return m_sInstance;
	}
};
