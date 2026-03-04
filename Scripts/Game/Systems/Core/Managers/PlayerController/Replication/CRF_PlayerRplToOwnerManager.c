class CRF_PlayerRplToOwnerManagerClass : ScriptComponentClass {}

class CRF_PlayerRplToOwnerManager : ScriptComponent
{	
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

	void SharerMarkerGlobal(int markerUID)
	{
		Rpc(RpcDo_SharerMarkerGlobal, markerUID);
	}
	
	void ShareMarker(array<int> markerUIDs)
	{
		Rpc(RpcDo_ShareMarker, markerUIDs);
	}
	
	void RefreshGlobalMarkers(array<int> markers)
	{
		Rpc(RpcDo_RefreshGlobalMarkers, markers);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_InitializeRadioFromServer()
	{
		CRF_PlayerController pc = CRF_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(SCR_PlayerController.GetLocalPlayerId()));
		
		if (pc)
			GetGame().GetCallqueue().CallLater(pc.InitializeRadios, 500, false, pc.GetLocalControlledEntity());
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ForwardDeployRequestRejected()
	{
		SCR_NotificationsComponent.GetInstance().SendLocal(SCR_NotificationsComponent.SendLocal(ENotification.FASTTRAVEL_PLAYER_LOCATION_WRONG));
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_TeleportLocalPlayer(vector location)
	{
		SCR_Global.TeleportPlayer(SCR_PlayerController.GetLocalPlayerId(), location);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SharerMarkerGlobal(int markerUID)
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_RefreshGlobalMarkers(array<int> markers)
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
