[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_SearchAndRescueGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}
class CRF_SearchAndRescueGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("360", "auto", "The amount of time between marker updates in seconds.")]
	int m_timeBetweenPings;
	
	[Attribute("false", "auto", "Set the character to unconcious state.")]
	bool m_setUnconcious;
	
	[Attribute("downedPilot", "auto", "The entity that being searched for.")]
	string m_searchForUnit;
	
	[Attribute("Transponder Signal", "auto", "The entity that being searched for.")]
	string m_markerText;
	
	[Attribute("US", "auto", "Faction key for the searching side.")]
	string m_searcherFactionKey;
	
	CRF_SafestartGameModeComponent m_safestart;
	
	SCR_FactionManager factionManager;
	
	[RplProp(onRplName: "updatePilotPos")]
	vector m_sPilotPos;
	vector m_sStoredPilotPos;

	//------------------------------------------------------------------------------------------------
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	}
	
	void WaitTillGameStart()
	{
		if (SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning()) 
		{
	
			IEntity pilot = GetGame().GetWorld().FindEntityByName(m_searchForUnit);
			
			SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(pilot.FindComponent(SCR_CharacterControllerComponent));
			characterController.SetUnconscious(true);
			Print("S&R: Night Night Mr Pilot");
			
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			
			GetGame().GetCallqueue().CallLater(WaitTillSafeStartEnds, 1000, true);
		}
		return;
	}
	
	void WaitTillSafeStartEnds()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (m_safestart.GetSafestartStatus())
		{
			GetGame().GetCallqueue().CallLater(transponderInit, 1000, true); // Change this to game timer
		}
		return;

	}
	
	void transponderInit()
	{
		if (SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning()) 
		{
			
			m_sPilotPos = GetGame().GetWorld().FindEntityByName(m_searchForUnit).GetOrigin();
			Replication.BumpMe();
			updatePilotPos();
		}
		return;
	}
	
	// Function called by the server sent to the clients
	void updatePilotPos()
	{	
		CRF_GameModePlayerComponent gameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
		return;
		
		Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!faction)
		return;
				
		//if (m_sPilotPos == m_sStoredPilotPos)
		//	return;
				
		m_sStoredPilotPos = m_sPilotPos;
		
        Print(m_sPilotPos);
		        
        if (faction.GetFactionKey() == "US")  
		{
			gameModePlayerComponent.AddScriptedMarker("Static Marker", m_sPilotPos, 1, m_markerText, "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", 50, ARGB(255, 0, 0, 225));
		}    
	};
	
}