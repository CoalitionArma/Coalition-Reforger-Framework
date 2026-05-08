class CRF_PlayerRplToOwnerManagerClass : ScriptComponentClass {}

class CRF_PlayerRplToOwnerManager : ScriptComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 LOCAL REPLICATION ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void InitializeRadioFromServer()
	{
		Rpc(RpcDo_InitializeRadioFromServer);
	}
	
	//------------------------------------------------------------------------------------------------
	void ForwardDeployRequestRejected()
	{
		Rpc(RpcDo_ForwardDeployRequestRejected);
	}
	
	//------------------------------------------------------------------------------------------------
	void TeleportLocalPlayer(vector location)
	{
		Rpc(RpcDo_TeleportLocalPlayer, location);
	}

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
	protected void RpcDo_InitializeRadioFromServer()
	{
		CRF_PlayerController pc = CRF_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		
		if (pc)
			GetGame().GetCallqueue().CallLater(pc.InitializeRadios, 500, false, pc.GetLocalControlledEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ForwardDeployRequestRejected()
	{
		SCR_NotificationsComponent.GetInstance().SendLocal(SCR_NotificationsComponent.SendLocal(ENotification.FASTTRAVEL_PLAYER_LOCATION_WRONG));
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_TeleportLocalPlayer(vector location)
	{
		SCR_Global.TeleportPlayer(SCR_PlayerController.GetLocalPlayerId(), location);
	}
	
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
		CRF_BandwidthTelemetryManager telemManager = CRF_BandwidthTelemetryManager.GetInstance();
		
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

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HVT — CLIENT-TO-SERVER RPC
//	 Declared in the BASE (non-modded) class so that Reforger's RPC registration
//	 table picks it up reliably on dedicated servers.
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Public wrapper called from client-side chat command callbacks in CRF_HighValueTargetGamemodeManager.
	void RequestHVTAdminCommand(string cmd, string param)
	{
		Rpc(RpcDo_AdminHVTCommand, cmd, param);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_AdminHVTCommand(string cmd, string param)
	{
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		if (!SCR_Global.IsAdmin(playerId))
			return;

		CRF_HighValueTargetGamemodeManager hvt = CRF_HighValueTargetGamemodeManager.Cast(CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager));
		if (hvt)
			hvt.HandleAdminCommand(playerId, cmd, param);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerRplToOwnerManager m_sInstance;
	void CRF_PlayerRplToOwnerManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PlayerRplToOwnerManager GetInstance()
	{
		return m_sInstance;
	}
};
