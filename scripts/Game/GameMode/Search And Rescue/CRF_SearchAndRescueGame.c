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

	//------------------------------------------------------------------------------------------------
	
	override protected void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	}
	
	void WaitTillGameStart()
	{
		if (SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning()) 
		{
			
			if (m_setUnconcious)
			{
				if (Replication.IsServer())
				{
					Print("S&R: Night Night Mr Pilot");
					IEntity pilot = GetGame().GetWorld().FindEntityByName(m_searchForUnit);
					SCR_CharacterControllerComponent characterController = SCR_CharacterControllerComponent.Cast(pilot.FindComponent(SCR_CharacterControllerComponent));
					characterController.SetUnconscious(true);
				}
				
			}
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			GetGame().GetCallqueue().CallLater(WaitTillSafeStartEnds, 1, true);
		}
		return;
	}
	
	void WaitTillSafeStartEnds()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (!m_safestart.GetSafestartStatus())
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
	        
	        if (faction.GetFactionKey() == "US")  
			{
				Print(gameModePlayerComponent.AddScriptedMarker(m_searchForUnit, "0 0 0", m_timeBetweenPings, m_markerText, "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", 50, ARGB(255, 0, 0, 225)));
		  		GetGame().GetCallqueue().Remove(WaitTillSafeStartEnds);
			}    
		}
		return;

	}
	
}