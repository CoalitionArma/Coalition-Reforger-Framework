/*
* Join-In-Progress (JIP) Synchronization Manager
* Centralizes all JIP sync logic to ensure late-joining players receive complete game state
* 
* This manager coordinates JIP syncs for:
* - GunGame player stats and scoreboard
* - Faction radio channel configurations  
* - Vehicle supply cost catalog
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
		
		Print(string.Format("[CRF_JIPSyncManager] Player %1 connected, sending JIP sync data", playerId), LogLevel.NORMAL);
		
		// Sync all game state to the newly connected player
		SyncFactionRadioChannels(playerId);
		SyncGunGameStats(playerId);
		SyncVehicleSupplyCosts(playerId);
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
}
