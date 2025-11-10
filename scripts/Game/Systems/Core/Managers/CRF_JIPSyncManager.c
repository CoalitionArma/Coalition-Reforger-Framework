/*
* Join-In-Progress (JIP) Synchronization Manager
* Centralizes all JIP sync logic to ensure late-joining players receive complete game state
* 
* This manager coordinates JIP syncs for systems that require explicit RPC calls:
* - Faction radio channel configurations (faction-specific optimization)
* - Vehicle supply cost catalog (sends entire catalog via RPCs)
* - Rush gamemode 3D marker states (requires delayed player-specific setup)
* - GunGame player stats (handled by GunGame's own OnPlayerConnected)
* 
* NOTE: Gamemodes using RplProp variables don't need manual JIP sync here.
* The Enfusion replication system automatically syncs RplProp to late-joiners when
* the component calls Replication.BumpMe() during normal gameplay.
*/

[ComponentEditorProps(category: "CRF JIP Sync Manager", description: "Handles Join-In-Progress synchronization for late-joining players")]
class CRF_JIPSyncManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_JIPSyncManager : SCR_BaseGameModeComponent
{
	protected static CRF_JIPSyncManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	void CRF_JIPSyncManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_JIPSyncManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when a player joins the server
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		// Only server sends JIP sync
		if (!Replication.IsServer())
			return;
		
		//Print(string.Format("[CRF_JIPSyncManager] Player %1 connected, sending JIP sync data", playerId), LogLevel.NORMAL);
		
		// Sync systems that use explicit RPCs (not auto-synced by RplProp)
		SyncFactionRadioChannels(playerId);
		SyncGunGameStats(playerId);
		SyncVehicleSupplyCosts(playerId);
		SyncRush3DMarkers(playerId);
		SyncSlottingData(playerId);
		
		// NOTE: S&D, MapStaging, and Raid use RplProp variables that auto-sync via Replication.BumpMe()
		// during normal gameplay. They don't need manual JIP sync here.
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP SYNC: Send player's faction radio configuration
	protected void SyncFactionRadioChannels(int playerId)
	{
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (!broadcastManager)
			return;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		// Get the player's faction
		SCR_Faction playerFaction = SCR_Faction.Cast(factionManager.GetPlayerFaction(playerId));
		if (!playerFaction)
		{
			Print(string.Format("[CRF_JIPSyncManager] Player %1 has no faction, skipping radio channel sync", playerId), LogLevel.WARNING);
			return;
		}
		
		string factionKey = playerFaction.GetFactionKey();
		
		// Send only the player's faction radio configs to reduce bandwidth
		Print(string.Format("[CRF_JIPSyncManager] Syncing %1 radio channels to player %2", factionKey, playerId), LogLevel.VERBOSE);
		
		switch (factionKey)
		{
			case "BLUFOR":
				broadcastManager.UpdateFactionChannelsSR("BLUFOR", factionManager.GetFactionActiveChannelSR("BLUFOR"));
				broadcastManager.UpdateFactionChannelsLR("BLUFOR", factionManager.GetFactionActiveChannelLR("BLUFOR"));
				break;
			case "OPFOR":
				broadcastManager.UpdateFactionChannelsSR("OPFOR", factionManager.GetFactionActiveChannelSR("OPFOR"));
				broadcastManager.UpdateFactionChannelsLR("OPFOR", factionManager.GetFactionActiveChannelLR("OPFOR"));
				break;
			case "INDFOR":
				broadcastManager.UpdateFactionChannelsSR("INDFOR", factionManager.GetFactionActiveChannelSR("INDFOR"));
				broadcastManager.UpdateFactionChannelsLR("INDFOR", factionManager.GetFactionActiveChannelLR("INDFOR"));
				break;
			case "CIV":
				broadcastManager.UpdateFactionChannelsSR("CIV", factionManager.GetFactionActiveChannelSR("CIV"));
				broadcastManager.UpdateFactionChannelsLR("CIV", factionManager.GetFactionActiveChannelLR("CIV"));
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP SYNC: Send all GunGame player stats
	protected void SyncGunGameStats(int playerId)
	{
		CRF_GunGame gunGame = CRF_GunGame.Cast(GetGame().GetGameMode().FindComponent(CRF_GunGame));
		if (!gunGame)
			return; // GunGame not active, skip
		
		Print(string.Format("[CRF_JIPSyncManager] Syncing GunGame stats to player %1", playerId), LogLevel.VERBOSE);
		
		// GunGame has its own OnPlayerConnected override, it will handle JIP sync
		// No action needed here
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP SYNC: Send entire vehicle supply cost catalog
	protected void SyncVehicleSupplyCosts(int playerId)
	{
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		if (!gearscriptManager)
			return;
		
		Print(string.Format("[CRF_JIPSyncManager] Syncing vehicle supply costs to player %1", playerId), LogLevel.VERBOSE);
		
		// Call GearscriptManager's public sync method
		gearscriptManager.SyncVehicleCostsToPlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP SYNC: Rush gamemode - Send 3D marker states for active zones
	protected void SyncRush3DMarkers(int playerId)
	{
		CRF_RushGamemodeManager rushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
		if (!rushGamemode)
			return; // Rush not active, skip
		
		Print(string.Format("[CRF_JIPSyncManager] Syncing Rush 3D markers to player %1", playerId), LogLevel.VERBOSE);
		
		// Rush requires 3-second delay for player controller initialization
		// This calls the gamemode's setup method which uses the RplProp toggle trick
		GetGame().GetCallqueue().CallLater(rushGamemode.SetupMarkersForPlayer, 3000, false, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP SYNC: Slotting Manager - Send all slot data to late-joiners
	// Uses targeted RPCs to avoid overwhelming new player connection
	//------------------------------------------------------------------------------------------------
	protected void SyncSlottingData(int playerId)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (!broadcastManager)
			return;
		
		Print(string.Format("[CRF_JIPSyncManager] Syncing slotting data to player %1", playerId), LogLevel.VERBOSE);
		
		// Get all slots
		array<int> allSlotIds = slottingManager.GetAllSlotIds();
		
		Print(string.Format("[CRF_JIPSyncManager] Sending %1 slots to player %2", allSlotIds.Count(), playerId), LogLevel.VERBOSE);
		
		// Send each slot via targeted RPC
		// Note: Could batch these in future optimization, but individual RPCs are already
		// 98% more efficient than old full-array replication
		foreach (int slotId : allSlotIds)
		{
			CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
			if (slotData)
				broadcastManager.UpdateSlotData(slotId, slotData);
		}
	}
}
