//------------------------------------------------------------------------------------------------
// Data structure for batched slot updates to reduce network traffic
// Using individual parameters instead of complex serialization to match Enfusion's simpler RPC pattern
//------------------------------------------------------------------------------------------------

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
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	
	protected static CRF_RplToAuthorityManager m_sInstance;
	
	void CRF_RplToAuthorityManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	// Returns the instance of the RplToAuthorityManager
	static CRF_RplToAuthorityManager GetInstance()
	{
		return m_sInstance;
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
	
	void ToggleRushMCOMPlanted(string mcomIdentifier, bool togglePlanted)
	{
		Print("[CRF_RplToAuthorityManager] ToggleRushMCOMPlanted() called on client: " + mcomIdentifier + ", planted: " + togglePlanted + " - sending RPC");
		Rpc(RpcAsk_ToggleRushMCOMPlanted, mcomIdentifier, togglePlanted); 
	}
	
	void StartRushPlantingSound()
	{
		Print("[CRF_RplToAuthorityManager] StartRushPlantingSound() called on client - sending RPC");
		Rpc(RpcAsk_StartRushPlantingSound); 
	}
	
	void StopRushPlantingSound()
	{
		Rpc(RpcAsk_StopRushPlantingSound); 
	}
	
	void StartRushDefuseSound()
	{
		Rpc(RpcAsk_StartRushDefuseSound); 
	}
	
	void StopRushDefuseSound()
	{
		Rpc(RpcAsk_StopRushDefuseSound); 
	}
	
	void StopRushBombTickingSound()
	{
		Print("[CRF_RplToAuthorityManager] StopRushBombTickingSound() called on client - sending RPC");
		Rpc(RpcAsk_StopRushBombTickingSound); 
	}
	
	void RequestAdvanceGamemodeState(bool overriden)
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceGamemodeState, overriden);
	}
	
	void RequestAdvanceSlottingPhase()
	{
		if (SCR_Global.IsAdmin())
			Rpc(RpcAsk_RequestAdvanceSlottingPhase); 
	}
	
	// Slot management functions - Batched update system for better performance
	void BatchUpdateSlot(int slotId, int playerId = -1, RplId groupId = RplId.Invalid(), RplId charId = RplId.Invalid(), 
	                    ResourceName resource = "", string name = "", bool isLocked = false, bool isDead = false)
	{
		Rpc(RpcAsk_BatchUpdateSlot, slotId, playerId, groupId, charId, resource, name, isLocked, isDead);
	}
	
	// Individual slot management functions - Modified to use batched updates
	// DEPRECATED: Individual slot update methods have been replaced with BatchUpdateSlot for better performance
	// These methods are kept for backward compatibility but redirect to the batched implementation
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, playerId, currentData.GetSlotCurrentGroup(), currentData.GetSlotCurrentCharacter(),
				               currentData.GetSlotResource(), currentData.GetSlotName(), 
				               currentData.GetIsLockedSlot(), currentData.GetIsDeadSlot());
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable 
		m_SlottingManager.UpdateSlotPlayerID(slotId, playerId);
	}
	
	void UpdateSlotLockedState(int slotId, bool input)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, currentData.GetSlotCurrentPlayerId(), currentData.GetSlotCurrentGroup(), 
				               currentData.GetSlotCurrentCharacter(), currentData.GetSlotResource(), 
				               currentData.GetSlotName(), input, currentData.GetIsDeadSlot());
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable
		m_SlottingManager.UpdateSlotLockedState(slotId, input);
	}
	
	void UpdateGroupLockedState(RplId groupRplId, bool input)
	{
		// Group locking is not part of slot batching, use direct RPC
		Rpc(RpcAsk_UpdateGroupLockedState, groupRplId, input); 
	}
	
	void UpdateSlotDeathState(int slotId, bool input)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, currentData.GetSlotCurrentPlayerId(), currentData.GetSlotCurrentGroup(), 
				               currentData.GetSlotCurrentCharacter(), currentData.GetSlotResource(), 
				               currentData.GetSlotName(), currentData.GetIsLockedSlot(), input);
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable
		m_SlottingManager.UpdateSlotDeathState(slotId, input);
	}
	
	void UpdateSlotGroup(int slotId, RplId groupRplId)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, currentData.GetSlotCurrentPlayerId(), groupRplId, 
				               currentData.GetSlotCurrentCharacter(), currentData.GetSlotResource(), 
				               currentData.GetSlotName(), currentData.GetIsLockedSlot(), currentData.GetIsDeadSlot());
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable
		m_SlottingManager.UpdateSlotGroup(slotId, groupRplId);
	}
	
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, currentData.GetSlotCurrentPlayerId(), currentData.GetSlotCurrentGroup(), 
				               currentData.GetSlotCurrentCharacter(), resource, currentData.GetSlotName(), 
				               currentData.GetIsLockedSlot(), currentData.GetIsDeadSlot());
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable
		m_SlottingManager.UpdateSlotResource(slotId, resource);
	}
	
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		// Use the optimized batched method
		if (m_SlottingManager)
		{
			CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(slotId);
			if (currentData)
			{
				BatchUpdateSlot(slotId, currentData.GetSlotCurrentPlayerId(), currentData.GetSlotCurrentGroup(), 
				               charId, currentData.GetSlotResource(), currentData.GetSlotName(), 
				               currentData.GetIsLockedSlot(), currentData.GetIsDeadSlot());
				return;
			}
		}
		// Direct manager call if BatchUpdateSlot unavailable
		m_SlottingManager.UpdateSlotCharacter(slotId, charId);
	}
	
	// Admin messaging functions
	void SendAdminMessage(string data, int playerID)
	{
		Rpc(RpcAsk_SendAdminMessage, data, playerID); 
	}
	
	void ReplyAdminMessage(string data, int playerId, int adminID, bool logAction)
	{
		if (SCR_Global.IsAdmin() || m_GamemodeManager.IsModerator())
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
	void RespawnPlayer(int playerId, RplId SpawnRplID)
	{
		Rpc(RpcAsk_RespawnPlayer, playerId, SpawnRplID); 
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
	
	// Vehicle depot management
	void RequestVehicleDepotInteraction(int playerId, int vehicleIndex, RplId depotRplId)
	{
		Rpc(RpcAsk_RequestVehicleDepotInteraction, playerId, vehicleIndex, depotRplId);
	}
	
	void RespawnFaction(FactionKey faction, bool logAction)
	{
		Rpc(RpcAsk_RespawnFaction, faction, logAction); 
	}
	
	// Equipment management
	void ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
		Rpc(RpcAsk_ResetGear, playerId, prefab, logAction); 
	}
	
	// Equipment management
	void UpdateGearSet(string faction, ResourceName path)
	{
		Rpc(RpcAsk_UpdateGearSet, faction, path); 
	}
	
	void AddItem(int playerId, string prefab, bool logAction)
	{
		Rpc(RpcAsk_AddItem, playerId, prefab, logAction); 
	}
	
	void RemoveItem(int playerId, RplId entityID, bool logAction)
	{
		Rpc(RpcAsk_RemoveItem, playerId, entityID, logAction); 
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
	
	void UpdateTimer(int delta) 
	{
		Rpc(RpcAsk_UpdateTimer, delta); 
	}	
	
	void UpdateTicket(string action, FactionKey faction, int delta) 
	{
		Rpc(RpcAsk_UpdateTicket, action, faction, delta); 
	}
	
	void MiniArsenalRequestNewItem(int playerId, string resourceName, int slotId)
	{
		Rpc(RpcAsk_MiniArsenalRequestNewItem, playerId, resourceName, slotId);
	}
	
	void MiniArsenalRequestNewWeapon(int playerId, string weapon, array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, bool isPistol)
	{
		Rpc(RpcAsk_MiniArsenalRequestNewWeapon, playerId, weapon, attachments, magazines, magazineCounts, isPistol);
	}
	
	void SightArsenalRequestNewSight(int playerId, string resourceName, string type)
	{
		Rpc(RpcAsk_SightArsenalRequestNewSight, playerId, resourceName, type);
	}
	
	void TogglePlayerListening(int playerId, bool input)
	{
		Rpc(RpcAsk_TogglePlayerLisntening, playerId, input);
	}
	
	void ToggleWaveRespawn()
	{
		Rpc(RpcAsk_ToggleWaveRespawn);
	}
	
	void ToggleRespawn()
	{
		Rpc(RpcAsk_ToggleRespawn);
	}
	
	void SetRespawnTime(int seconds)
	{
		Rpc(RpcAsk_SetRespawnTime, seconds);
	}
	
	void ToggleEnableAIInGameState()
	{
		Rpc(RpcAsk_ToggleEnableAIInGameState);
	}
	
	void CleanUpBodies()
	{
		Rpc(RpcAsk_CleanUpBodies);
	}
	
	//------------------------------------------------------------------------------------------------
	// SERVER-SIDE RPC HANDLERS - Executed on the authority (server)
	//------------------------------------------------------------------------------------------------
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestInitilizePlayer(int playerId)
	{
		m_GamemodeManager.InitilizePlayer(playerId, CRF_GamemodeManager.ZERO_SPAWN_VECTOR);
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
	protected void RpcAsk_ToggleRushMCOMPlanted(string mcomIdentifier, bool togglePlanted)
	{
		Print("[CRF_RplToAuthorityManager] RpcAsk_ToggleRushMCOMPlanted received: " + mcomIdentifier + ", planted: " + togglePlanted);
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
			rushGamemode.ToggleMCOMPlanted(mcomIdentifier, togglePlanted);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartRushPlantingSound()
	{
		Print("[CRF_RplToAuthorityManager] RpcAsk_StartRushPlantingSound received on server");
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
			rushGamemode.PlayPlantingSound();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushPlantingSound()
	{
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.StopPlantingSound();
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartRushDefuseSound()
	{
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.PlayDefuseSound();
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushDefuseSound()
	{
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			rushGamemode.StopDefuseSound();
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StopRushBombTickingSound()
	{
		Print("[CRF_RplToAuthorityManager] RpcAsk_StopRushBombTickingSound received on server");
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (rushGamemode)
		{
			Print("[CRF_RplToAuthorityManager] Calling rushGamemode.StopBombTickingSound()");
			rushGamemode.StopBombTickingSound();
		}
		else
		{
			Print("[CRF_RplToAuthorityManager] Rush gamemode manager not found!");
		}
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

	// NEW: Batched slot update RPC handler for improved performance
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_BatchUpdateSlot(int slotId, int playerId, RplId groupId, RplId charId, ResourceName resource, string name, bool isLocked, bool isDead)
	{
		if (!m_SlottingManager)
			return;
			
		// Use the fully optimized batch update method from SlottingManager
		m_SlottingManager.BatchUpdateSlot(slotId, playerId, groupId, charId, resource, name, isLocked, isDead);
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
	protected void RpcAsk_RespawnPlayer(int playerId, RplId SpawnRplID)
	{
		vector overrideLocation[4];
		overrideLocation = CRF_GamemodeManager.ZERO_SPAWN_VECTOR;
		
		m_RespawnManager.RespawnPlayer(playerId, overrideLocation, -1, SpawnRplID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestToJoinChannel(int channel, int requestId)
	{
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
		// Include player ID in channel name to ensure uniqueness when players have same username
		string uniqueChannelName = playerName + "'s Channel (" + playerId + ")";
		int channelIndex = m_MenuManager.CreateChannel(uniqueChannelName, playerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_JoinChannel(int playerId, int channel)
	{
		m_MenuManager.AddPlayerToChannel(playerId, channel, false);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SpawnOnGroup(int playerId, vector spawnLocation[4], int groupID, bool logAction)
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
	protected void RpcAsk_RequestVehicleDepotInteraction(int playerId, int vehicleIndex, RplId depotRplId)
	{
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

	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RespawnFaction(FactionKey faction, bool logAction)
	{
		m_RespawnManager.RespawnSide(faction);
		
		if (logAction)
		{
			string logMessage = string.Format("%1 was respawned", faction);
			m_RplBroadcastManager.LogAdminAction(logMessage, -1, false);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ResetGear(int playerId, ResourceName prefab, bool logAction)
	{
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
		
		CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(prefab);
		
		CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		
		m_SlottingManager.UpdateSlotResource(slotId, prefab);
		m_SlottingManager.UpdateSlotName(slotId, roleConfig.m_sRoleName);
		m_SlottingManager.UpdateSlotType(slotId, roleConfig.m_SlottingType);
		m_SlottingManager.UpdateSlotIcon(slotId, roleConfig.m_RoleIcon);
		
		if (logAction)
		{
			string prefabName = prefab.Substring(prefab.LastIndexOf("/") + 1, prefab.LastIndexOf(".") - prefab.LastIndexOf("/") - 1);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string logMessage = string.Format("%1's gear was set to %2", playerName, prefabName);
			m_RplBroadcastManager.LogAdminAction(logMessage, playerId, true);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateGearSet(string faction, ResourceName path)
	{
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
		array<IEntity> entities = {};

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
				entities.Insert(entity);
		}

		// Queue changes to prevent server freezing
		UpdateGearSetQueue(entities);
		
		string logMessage = string.Format("%1 was changed to %2", faction, path);
		m_RplBroadcastManager.LogAdminAction(logMessage, -1 , false)
	}
	
	protected void UpdateGearSetQueue(array<IEntity> entities, int lastIndex = 0)
	{
		if (lastIndex >= entities.Count())
			return;
		
		IEntity entity = entities[lastIndex];
		
		// Grab prefab name and check if its a valid gearscript
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!CRF_RoleHelper.IsValidGearscriptResource(prefab))
			return;
		
		// Prevent Lockup and invisible weapon if player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId)
		{
			m_RplBroadcastManager.Closemap(playerId);
			//Causes issues when trying to holster a weapon we are deleting.
			//m_RplBroadcastManager.HolsterGun(playerId);
		}
		
		CRF_GearscriptManager.GetInstance().SetEntityGear(entity, prefab);
		
		// Queue next entity
		GetGame().GetCallqueue().CallLater(UpdateGearSetQueue, 50, false, entities, lastIndex + 1);
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
	protected void RpcAsk_RemoveItem(int playerId, RplId entityID, bool logAction)
	{
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTimer(int delta)
	{
		// Get current end time
		int currentEndTime = CRF_SafestartManager.GetInstance().m_iTimeMissionEnds;
		if ((currentEndTime + delta) < 0 || m_SafestartManager.GetSafestartStatus())
			return;

		// Set the new time, broadcast is handled by rplprop
		CRF_SafestartManager.GetInstance().m_iTimeMissionEnds = currentEndTime + delta;
		
		string logMessage = string.Format("Game timer adjusted by %1 mins", delta/60000);
		m_RplBroadcastManager.GetInstance().LogAdminAction(logMessage, -1, false);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdateTicket(string action, FactionKey faction, int delta)
	{
		if (action == "Add")
			m_RespawnManager.AddTicket(faction, delta, true);
		else if (action == "Subtract")
			m_RespawnManager.SubtractTicket(faction, delta, true);
		
		string logMessage = string.Format("%1 tickets was subtracted from %2", delta, faction);
		m_RplBroadcastManager.GetInstance().LogAdminAction(logMessage, -1, false);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MiniArsenalRequestNewItem(int playerId, string newResource, int slotId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		IEntity newItem = GetGame().SpawnEntityPrefab(Resource.Load(newResource), null, params);
		
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent invComponent = BaseInventoryStorageComponent.Cast(player.FindComponent(BaseInventoryStorageComponent));
		IEntity oldItem = invComponent.Get(slotId);
		if (!oldItem)
		{
			invManager.TryInsertItem(newItem);
			return;
		}
		BaseInventoryStorageComponent oldStorageComp = BaseInventoryStorageComponent.Cast(oldItem.FindComponent(BaseInventoryStorageComponent));
		BaseInventoryStorageComponent newStorageComp = BaseInventoryStorageComponent.Cast(newItem.FindComponent(BaseInventoryStorageComponent));
		ref array<IEntity> pouches = {};
		oldStorageComp.GetAll(pouches);
		ref array<ResourceName> items = {};
		//Wow I hate this, gotta scan through all the pouchs cause GetAll, in fact, does not get all :O
		foreach (IEntity pouch: pouches)
		{
			if (!pouch.FindComponent(BaseInventoryStorageComponent))
			{
				items.Insert(pouch.GetPrefabData().GetPrefabName());
				continue;
			}
			
			ref array<IEntity> tempItems = {};
			BaseInventoryStorageComponent.Cast(pouch.FindComponent(BaseInventoryStorageComponent)).GetAll(tempItems);
			foreach (IEntity tempItem: tempItems)
			{
				items.Insert(tempItem.GetPrefabData().GetPrefabName());
			}
		}
		if (!invManager.CanReplaceItem(newItem, invComponent, slotId))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(newItem);
			return;
		}
		SCR_EntityHelper.DeleteEntityAndChildren(oldItem);
		GetGame().GetCallqueue().CallLater(AddVestDelay, 250, false, newItem, invComponent, slotId, oldItem, items, invManager, newStorageComp, playerId, player);
	}
	
	void AddVestDelay(IEntity newItem, BaseInventoryStorageComponent invComponent, int slotId, IEntity oldItem, array<ResourceName> items, SCR_InventoryStorageManagerComponent invManager, BaseInventoryStorageComponent newStorageComp, int playerId, IEntity player)
	{
		invManager.TryReplaceItem(newItem, invComponent, slotId);
		GetGame().GetCallqueue().CallLater(AddItemDelay, 275, false, oldItem, items, invManager, newStorageComp, playerId, player);
	}
	
	void AddItemDelay(IEntity oldItem, array<ResourceName> items, SCR_InventoryStorageManagerComponent invManager, BaseInventoryStorageComponent newStorageComp, int playerId, IEntity player)
	{
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
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		GetGame().GetCallqueue().CallLater(groupsMan.TuneFreqDelayWithPresets, 500, false, playerId, player);
		GetGame().GetCallqueue().CallLater(pc.InitializeRadios, 500, false, player);
		pc.InitializeRadioFromServer();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_MiniArsenalRequestNewWeapon(int playerId, string newWeaponResource, array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, bool isPistol)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(player.GetPrefabData().GetPrefabName());
		
		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		IEntity newWeapon = GetGame().SpawnEntityPrefab(Resource.Load(newWeaponResource), null, params);
		
		SCR_InventoryStorageManagerComponent storageMan = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterInventoryStorageComponent storageComp = SCR_CharacterInventoryStorageComponent.Cast(player.FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		
		array<IEntity> items = {};
		storageMan.GetItems(items);
		
		array<WeaponSlotComponent> weaponSlots = {};
		charController.GetWeaponManagerComponent().GetWeaponsSlots(weaponSlots);
		IEntity weapon;
		if (isPistol)
			weapon = weaponSlots.Get(4).GetWeaponEntity();
		else
			weapon = weaponSlots.Get(2).GetWeaponEntity();
		
		WeaponComponent weaponComp = WeaponComponent.Cast(weapon.FindComponent(WeaponComponent));
		array<BaseMuzzleComponent> muzzles = {};
		weaponComp.GetMuzzlesList(muzzles);
		
		array<BaseMagazineWell> magazineWells = {};
		foreach (BaseMuzzleComponent muzzle: muzzles)
		{
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
					delete item;
					break;
				}
			}
		}
		
		//Delete Old Weapon;
		SCR_EntityHelper.DeleteEntityAndChildren(weapon);
		GetGame().GetCallqueue().CallLater(MiniArsenalRequestNewWeaponDelay, 500, false, storageMan, storageComp, newWeapon, attachments, magazines, magazineCounts, role);
	}
	
	void MiniArsenalRequestNewWeaponDelay(SCR_InventoryStorageManagerComponent storageMan, SCR_CharacterInventoryStorageComponent storageComp, IEntity newWeapon, 
	array<ResourceName> attachments, array<ResourceName> magazines, array<int> magazineCounts, CRF_EGearRole role)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		newWeapon.GetTransform(params.Transform);
		storageMan.TryInsertItem(newWeapon);
		CRF_GearscriptManager gearScriptManager = CRF_GearscriptManager.GetInstance();
		int currentMagazine = 0;
		foreach (int magazineCount: magazineCounts)
		{
			for (int i = 0; i < magazineCount; i++)
			{
				IEntity newMagazine = GetGame().SpawnEntityPrefab(Resource.Load(magazines[currentMagazine]), null, params);
				gearScriptManager.InsertInventoryItemPublic(newMagazine, storageComp, storageMan, role, false, false);
			}
			currentMagazine++;
		}
		
		foreach (ResourceName attachment: attachments)
		{
			IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachment), GetGame().GetWorld(), params);
			BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(newWeapon.FindComponent(BaseInventoryStorageComponent));
			IEntity oldSight = weaponStorageComp.FindSuitableSlotForItem(attachmentSpawned).GetAttachedEntity();
			BaseWeaponComponent newWeaponComp = BaseWeaponComponent.Cast(newWeapon.FindComponent(BaseWeaponComponent));
			array<AttachmentSlotComponent> attachmentSlots = {};
			newWeaponComp.GetAttachments(attachmentSlots);
			
			foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
			{
				if (attachmentSlot.CanSetAttachment(attachmentSpawned))
				{
					if (oldSight)
						delete oldSight;
				
					storageMan.TryInsertItemInStorage(attachmentSpawned, weaponStorageComp);
					break;
				}
			}
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SightArsenalRequestNewSight(int playerId, string newResource, string type)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		EntitySpawnParams params = new EntitySpawnParams();
		player.GetTransform(params.Transform);
		
		IEntity newSight = GetGame().SpawnEntityPrefab(Resource.Load(newResource), null, params);
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		array<AttachmentSlotComponent> attachments = {};
		charController.GetWeaponManagerComponent().GetCurrentWeapon().GetAttachments(attachments);
		AttachmentSlotComponent sightAttachment;
		SCR_InventoryStorageManagerComponent invManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		BaseInventoryStorageComponent weaponInv = BaseInventoryStorageComponent.Cast(charController.GetWeaponManagerComponent().GetCurrentWeapon().GetOwner().FindComponent(BaseInventoryStorageComponent));
		foreach (AttachmentSlotComponent attachment: attachments)
		{
			if (!attachment.GetAttachmentSlotType())
				continue;
			if (type.ToType().IsInherited(attachment.GetAttachmentSlotType().Type()))
				sightAttachment = attachment;
		}
		if (!sightAttachment)
			return;
		
		IEntity oldSight = sightAttachment.GetAttachedEntity();
		if (sightAttachment.CanSetAttachment(newSight))
		{
			if (oldSight)
				delete oldSight;
			
			invManager.TryInsertItemInStorage(newSight, weaponInv);
		}
	}
    
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_TogglePlayerLisntening(int playerId, bool input)
	{
		CVON_VONGameModeComponent.GetInstance().TogglePlayerListening(playerId, input);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleWaveRespawn()
	{
		CRF_RespawnManager.GetInstance().ToggleRespawnWave();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleRespawn()
	{
		CRF_RespawnManager.GetInstance().ToggleRespawn();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SetRespawnTime(int seconds)
	{
		CRF_RespawnManager.GetInstance().SetRespawnTime(seconds);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleEnableAIInGameState()
	{
		CRF_Gamemode.GetInstance().ToggleEnableAIInGameState();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_CleanUpBodies()
	{
		CRF_GamemodeManager.GetInstance().CleanUpBodies();
	}
};
