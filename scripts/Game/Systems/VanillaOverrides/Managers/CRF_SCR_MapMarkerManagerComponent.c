// Prevents map markers from being deleted when a player disconnects
modded class SCR_MapMarkerManagerComponent
{
	SCR_PlayerController m_PlayerController;
	
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
	}
	
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);
		foreach (SCR_MapMarkerBase marker: m_aStaticMarkers)
		{
			
			if (marker.GetMarkerOwnerID() == SCR_PlayerController.GetLocalPlayerId())
				marker.SetVisible(true);
			else
				marker.SetVisible(marker.m_bIsShared);
		}
	}
	
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Override the Override that would delete markers on disconnect
		return;
	}
}