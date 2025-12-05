// Prevents map markers from being deleted when a player disconnects
modded class SCR_MapMarkerManagerComponent
{
	SCR_PlayerController m_PlayerController;
	protected int m_iCachedLocalPlayerId = -1;
	
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{								
		CRF_SafestartManager safestartMan = CRF_SafestartManager.GetInstance();	
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction playerFaction;
		if (factionMan)
			playerFaction = factionMan.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (safestartMan && gamemode && playerFaction)
			if (safestartMan.GetSafestartStatus())
				marker.m_bIsShared = true;
			else if (!gamemode.DoesFactionShareMarker(playerFaction.GetFactionKey()))
				marker.m_bIsShared = true;
		
		
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