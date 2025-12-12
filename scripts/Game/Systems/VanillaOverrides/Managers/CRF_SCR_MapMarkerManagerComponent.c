// Prevents map markers from being deleted when a player disconnects
modded class SCR_MapMarkerManagerComponent
{
	// Maximum distance (in meters) to share markers with nearby players
	static const float MARKER_SHARE_DISTANCE = 8.0;
	ref map<int, ref array<int>> m_MarkersSharedReference = new map<int, ref array<int>>; 
	SCR_PlayerController m_PlayerController;
	protected int m_iCachedLocalPlayerId = -1;
	
	void UpdateSharedMarkers(array<int> markers, int playerId)
	{
		array<int> currentMarkers = m_MarkersSharedReference.Get(playerId);
		foreach (int markerUID: markers)
		{
			if (currentMarkers.Contains(markerUID))
				continue;
			
			currentMarkers.Insert(markerUID);
		}
	}
	
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;
		
		array<int> markers = m_MarkersSharedReference.Get(playerId);
		if (!markers)
		{
			m_MarkersSharedReference.Set(playerId, new array<int>);
			return;
		}
		
		if (markers.Count() == 0)
			return;

		pc.ShareMarker(markers);
	}
	
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{								
		CRF_SafestartManager safestartMan = CRF_SafestartManager.GetInstance();	
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
		{
			int playerId = pc.GetPlayerId();
			Faction playerFaction;
			if (factionMan)
				playerFaction = factionMan.GetPlayerFaction(playerId);
			CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
			if (safestartMan && gamemode && playerFaction)
				if (safestartMan.GetSafestartStatus() && marker.GetMarkerOwnerID() == playerId)
					CRF_RplToAuthorityManager.GetInstance().SharerMapMarkerGlobal(marker.GetMarkerID(), playerId);
				else if (!gamemode.DoesFactionShareMarker(playerFaction.GetFactionKey()) && marker.GetMarkerOwnerID() == playerId)
					CRF_RplToAuthorityManager.GetInstance().SharerMapMarkerGlobal(marker.GetMarkerID(), playerId);
		}
		
		super.OnAddSynchedMarker(marker);
		
		// Update visibility for the newly added marker
		UpdateMarkerVisibility(marker);
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
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Override the Override that would delete markers on disconnect
		return;
	}
}