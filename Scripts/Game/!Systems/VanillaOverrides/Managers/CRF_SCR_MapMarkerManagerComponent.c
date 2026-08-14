
// Prevents map markers from being deleted when a player disconnects or dies/respawns
modded class SCR_MapMarkerManagerComponent
{
	// Maximum distance (in meters) to share markers with nearby players
	static const float MARKER_SHARE_DISTANCE = 8.0;
	ref map<int, string> m_aGlobalMarkers = new map<int, string>;
	ref map<int, ref array<int>> m_MarkersSharedReference = new map<int, ref array<int>>; 
	SCR_PlayerController m_PlayerController;
	protected int m_iCachedLocalPlayerId = -1;
	
	void UpdateSharedMarkers(array<int> markers, int playerId)
	{
		if (!markers)
			return;

		array<int> currentMarkers = m_MarkersSharedReference.Get(playerId);
		if (!currentMarkers)
		{
			currentMarkers = {};
			m_MarkersSharedReference.Set(playerId, currentMarkers);
		}

		foreach (int markerUID: markers)
		{
			if (currentMarkers.Contains(markerUID))
				continue;
			
			currentMarkers.Insert(markerUID);
		}
	}
	
	void RequestGlobalMarkersRefresh()
	{
		if (COA_PlayerRplToAuthorityManager.GetInstance())
			COA_PlayerRplToAuthorityManager.GetInstance().RequestGlobalMarkerRefresh();
	}
	
	//Method used for JIPs to get markers placed during safestart
	void RefreshGlobalMarkers(array<int> globalMarkers)
	{
	    SCR_MapMarkerManagerComponent mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
	    if (!mapMarkerManager)
	        return;
	
	    // Build a map of markerID -> marker for O(1) lookups
	    map<int, SCR_MapMarkerBase> markerMap = new map<int, SCR_MapMarkerBase>;
	    foreach (SCR_MapMarkerBase marker: mapMarkerManager.GetStaticMarkers())
	    {
	        markerMap.Set(marker.GetMarkerID(), marker);
	    }
	
	    // Now just do direct lookups instead of nested iteration
	    foreach (int markerUID: globalMarkers)
	    {
	        SCR_MapMarkerBase marker = markerMap.Get(markerUID);
	        if (marker && !marker.m_bIsShared)
	            marker.m_bIsShared = true;
	    }
	
	    UpdateAllMarkerVisibilities();
	}
	
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		array<int> markers = m_MarkersSharedReference.Get(playerId);
		if (!markers)
		{
			m_MarkersSharedReference.Set(playerId, new array<int>);
			return;
		}
		
		if (markers.Count() == 0)
			return;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
			return;
		COA_PlayerRplToOwnerManager rplToOwnerManager = COA_PlayerRplToOwnerManager.Cast(pc.FindComponent(COA_PlayerRplToOwnerManager));
		if (!rplToOwnerManager)
			return;

		rplToOwnerManager.ShareMarker(markers);
	}
	
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc && marker.GetMarkerOwnerID() == pc.GetPlayerId())
		{
			// During safestart, auto-share every placed marker to all same-faction players
			COA_SafestartManager safestartMan = COA_SafestartManager.GetInstance();
			if (safestartMan && safestartMan.GetSafestartStatus())
			{
				COA_PlayerRplToAuthorityManager auth = COA_PlayerRplToAuthorityManager.GetInstance();
				if (auth)
					auth.SharerMapMarkerGlobal(marker.GetMarkerID(), pc.GetPlayerId());
			}
		}
		
		super.OnAddSynchedMarker(marker);
		
		// Update visibility for the newly added marker
		UpdateMarkerVisibility(marker);
	}
	
	override void OnAskRemoveStaticMarker(int markerID)
	{
		super.OnAskRemoveStaticMarker(markerID);
		m_aGlobalMarkers.Remove(markerID);
	}
	
	//------------------------------------------------------------------------------------------------
	// Update visibility for a single marker based on ownership and shared status
	protected void UpdateMarkerVisibility(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return;
			
		// Cache the local player ID to avoid repeated calls
		if (m_iCachedLocalPlayerId == -1)
			m_iCachedLocalPlayerId = SCR_PlayerController.GetLocalPlayerId();
		
		// If shareable markers are disabled for this faction, all markers are visible (vanilla behaviour)
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		Faction localFaction = SCR_FactionManager.SGetLocalPlayerFaction();
		bool shareableMarkersEnabled = gamemode && localFaction && gamemode.DoesFactionShareMarker(localFaction.GetFactionKey());
		if (!shareableMarkersEnabled)
		{
			// m_bIsShared must be true before calling SetVisible, otherwise the
			// SCR_MapMarkerBase.SetVisible override force-hides the widget
			marker.m_bIsShared = true;
			marker.SetVisible(true);
			return;
		}
		
		bool isPlayersMarker = (marker.GetMarkerOwnerID() == m_iCachedLocalPlayerId);
		if (isPlayersMarker)
			marker.m_bIsShared = true;
		bool shouldBeVisible = isPlayersMarker || marker.m_bIsShared;
		marker.SetVisible(shouldBeVisible);
	}
	
	//------------------------------------------------------------------------------------------------
	// Update all marker visibilities (call this when shared state changes)
	void UpdateAllMarkerVisibilities()
	{
		// Refresh cached player ID
		m_iCachedLocalPlayerId = SCR_PlayerController.GetLocalPlayerId();
		
		foreach (SCR_MapMarkerBase marker: m_aStaticMarkers)
		{
			UpdateMarkerVisibility(marker);
		}
		
		// Also update markers that are temporarily in the disabled list (out-of-frame)
		foreach (SCR_MapMarkerBase marker: m_aDisabledMarkers)
		{
			UpdateMarkerVisibility(marker);
		}
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Override the Override that would delete markers on disconnect
		return;
	}

	//------------------------------------------------------------------------------------------------
	// Prevents map markers from being deleted when a player dies and respawns.
	//
	// CRF routes every death through a transient faction re-assignment: COA_GamemodeManager.
	// InitilizePlayer() puts a dead player into the "SPEC" faction while they wait to respawn, then
	// RespawnManager flips them back to their real faction once they respawn. Vanilla's
	// OnPlayerFactionChanged treats each of those as a genuine faction switch and permanently
	// deletes every marker not flagged for the new faction (RemoveMarkersOfOtherFactions), so
	// players lost all their faction-flagged map markers on every death/respawn cycle. Skip the
	// vanilla cleanup for transitions into or out of SPEC; keep it for real faction changes.
	override protected void OnPlayerFactionChanged(notnull FactionAffiliationComponent owner, Faction previousFaction, Faction newFaction)
	{
		if (newFaction && newFaction.GetFactionKey() == "SPEC")
			return;

		if (previousFaction && previousFaction.GetFactionKey() == "SPEC")
			return;

		super.OnPlayerFactionChanged(owner, previousFaction, newFaction);
	}
}
