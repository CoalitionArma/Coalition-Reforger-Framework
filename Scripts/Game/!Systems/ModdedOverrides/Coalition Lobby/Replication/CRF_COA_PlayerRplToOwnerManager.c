modded class COA_PlayerRplToOwnerManager : ScriptComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 LOCAL REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void SharerMarkerGlobal(int markerUID)
	{
		Rpc(RpcDo_SharerMarkerGlobal, markerUID);
	}
	
	//------------------------------------------------------------------------------------------------
	void ShareMarker(array<int> markerUIDs)
	{
		Rpc(RpcDo_ShareMarker, markerUIDs);
	}
	
	//------------------------------------------------------------------------------------------------
	void RefreshGlobalMarkers(array<int> markers)
	{
		Rpc(RpcDo_RefreshGlobalMarkers, markers);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SharerMarkerGlobal(int markerUID)
	{
		SCR_MapMarkerManagerComponent mapMarkersMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mapMarkersMan)
			return;
		
		bool markersUpdated = false;
		foreach (SCR_MapMarkerBase marker: mapMarkersMan.GetStaticMarkers())
		{
			if (marker.GetMarkerID() == markerUID && !marker.m_bIsShared)
			{
				marker.m_bIsShared = true;
				markersUpdated = true;
			}
		}
		
		// Only update visibility if any markers were actually changed
		if (markersUpdated)
			mapMarkersMan.UpdateAllMarkerVisibilities();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
    protected void RpcDo_ShareMarker(array<int> markerUIDs)
    {	
		SCR_MapMarkerManagerComponent mapMarkersMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mapMarkersMan)
			return;
		
		bool markersUpdated = false;
		foreach (SCR_MapMarkerBase marker: mapMarkersMan.GetStaticMarkers())
		{
			if (markerUIDs.Contains(marker.GetMarkerID()) && !marker.m_bIsShared)
			{
				marker.m_bIsShared = true;
				markersUpdated = true;
			}
		}
		
		// Only update visibility if any markers were actually changed
		if (markersUpdated)
			mapMarkersMan.UpdateAllMarkerVisibilities();
    }
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RefreshGlobalMarkers(array<int> markers)
	{
		COA_BandwidthTelemetryManager telemManager = COA_BandwidthTelemetryManager.GetInstance();
		
		int bytes = 0;
		bytes += telemManager.EstimateSize_IntArray(markers);
		telemManager.LogRPC("RpcDo_RefreshGlobalMarkers", bytes);
		SCR_MapMarkerManagerComponent mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mapMarkerManager)
			return;
		
		mapMarkerManager.RefreshGlobalMarkers(markers);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PROP HUNT — CLIENT-TO-SERVER RPC
//	 Declared in the BASE (non-modded) class so that Reforger's RPC registration
//	 table picks it up reliably. RPCs added to a modded class are NOT guaranteed
//	 to be registered on dedicated servers.
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Public wrapper called from client-side chat command callbacks in CRF_PropHuntGamemode.
	//! Fires RpcDo_AdminPropHuntCommand through the base class so the RPC table entry is
	//! always registered on dedicated servers.
	void RequestAdminCommand(string cmd, string param)
	{
		Rpc(RpcDo_AdminPropHuntCommand, cmd, param);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_AdminPropHuntCommand(string cmd, string param)
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		// Re-validate admin status on the server
		if (!SCR_Global.IsAdmin(playerId))
			return;

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (propHunt)
			propHunt.HandleAdminCommand(playerId, cmd, param);
	}

	//------------------------------------------------------------------------------------------------
	//! Client calls this to request transforming into a prop disguise.
	//! Placed here (base class) instead of in the modded PropHunt class so the
	//! RPC index is part of the original class's RPC table and always works on
	//! dedicated servers. The actual spawn logic lives in CRF_PropHuntGamemode.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_RequestPropTransform(ResourceName prefab)
	{
		// Resolve the requesting player via the owning PlayerController.
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (propHunt)
			propHunt.HandleTransformRequest(playerId, prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! Client calls this to request playing a noise hint from their current position.
	//! Placed here (base class) so the RPC index is always in the base class RPC table
	//! and works reliably on dedicated servers. Logic lives in CRF_PropHuntGamemode.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_RequestPropNoise()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (propHunt)
			propHunt.HandleNoiseRequest(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Client calls this to cycle to the next noise in the configured list.
	//! Placed here (base class) so the RPC index is always reliable on dedicated servers.
	//! Logic lives in CRF_PropHuntGamemode which sends a hint back to the calling player.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_RequestPropNextNoise()
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (propHunt)
			propHunt.HandleNoiseCycleRequest(playerId);
	}

	//------------------------------------------------------------------------------------------------
	void SendAARKillStats(array<string> kills, string killedBy)
	{
		Rpc(RpcDo_SendAARKillStats, kills, killedBy);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SendAARKillStats(array<string> kills, string killedBy)
	{
		CRF_AARSessionStats.SetData(kills, killedBy);
	}

	//------------------------------------------------------------------------------------------------
	// Sends this player's numeric session stats to their client for the AAR panel.
	// Called from CRF_ServerStatsManager.OnGameModeEnd staggered send queue.
	void SendAARStats(int kills, int deaths, int shots, int grenades, int bandages, int distKm, int friendlyKills, int xp)
	{
		Rpc(RpcDo_SendAARStats, kills, deaths, shots, grenades, bandages, distKm, friendlyKills, xp);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SendAARStats(int kills, int deaths, int shots, int grenades, int bandages, int distKm, int friendlyKills, int xp)
	{
		CRF_AARSessionStats.SetStats(kills, deaths, shots, grenades, bandages, distKm, friendlyKills, xp);
	}

	//------------------------------------------------------------------------------------------------
	//! Tells this player's own client that their connection was audited while the round was already
	//! in COA_EGamemodeState.GAME - the real "join in progress" signal. Called from
	//! CRF_COA_Gamemode.NotifyJoinInProgressStatus.
	void SetJoinInProgress()
	{
		Rpc(RpcDo_SetJoinInProgress);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_SetJoinInProgress()
	{
		COA_PlayerController pc = COA_PlayerController.Cast(GetOwner());
		if (pc)
			pc.SetIsJoinInProgress(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Sends one map marker (of any faction) to this player's AAR map. Called by CRF_AARManager.
	void ReceiveAARMarker(SCR_MapMarkerBase marker)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveAARMarker(marker);
		else
			Rpc(RpcDo_ReceiveAARMarker, marker);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveAARMarker(SCR_MapMarkerBase marker)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			markerManager.RevealAARMarker(marker);
	}

	//------------------------------------------------------------------------------------------------
	//! Tells this player's AAR screen which coalitiongroup.net/aar/<id> page belongs to this round
	void ReceiveAARMissionLink(int missionId)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveAARMissionLink(missionId);
		else
			Rpc(RpcDo_ReceiveAARMissionLink, missionId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveAARMissionLink(int missionId)
	{
		CRF_AARSessionStats.SetMissionId(missionId);
	}

	//------------------------------------------------------------------------------------------------
	//! AAR screen: whether the website accepted this player's mission rating
	void ReceiveMissionRatingResult(int rating, bool saved)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveMissionRatingResult(rating, saved);
		else
			Rpc(RpcDo_ReceiveMissionRatingResult, rating, saved);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMissionRatingResult(int rating, bool saved)
	{
		CRF_AARSessionStats.SetRatingResult(rating, saved);
	}

	//------------------------------------------------------------------------------------------------
	//! Game Master player list: players with an open admin ticket (see RequestZeusTicketHolders)
	void ReceiveZeusTicketHolders(array<int> playerIds)
	{
		if (IsLocallyOwned())
			RpcDo_ReceiveZeusTicketHolders(playerIds);
		else
			Rpc(RpcDo_ReceiveZeusTicketHolders, playerIds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveZeusTicketHolders(array<int> playerIds)
	{
		CRF_ZeusPlayerList.SetTicketHolders(playerIds);
	}
};
