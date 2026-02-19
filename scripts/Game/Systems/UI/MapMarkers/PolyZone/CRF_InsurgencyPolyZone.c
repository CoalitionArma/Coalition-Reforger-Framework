//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Insurgency", description: "Phase-based polygon zone for Insurgency gamemode", color: "255 128 0 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_InsurgencyPolyZoneClass: CRF_PolyZoneClass
{
};

//------------------------------------------------------------------------------------------------
class CRF_InsurgencyPolyZone : CRF_PolyZone
{
	[Attribute("1", UIWidgets.EditBox, "Which phase should this zone be visible during (1, 2, 3, etc)")]
	protected int m_iVisibleDuringPhase;
	
	[Attribute("0", UIWidgets.CheckBox, "Show zone to both teams when phase is complete?")]
	protected bool m_bShowToAllWhenPhaseComplete;
	
	[Attribute(params: "edds", uiwidget: UIWidgets.ResourcePickerThumbnail)]
	private ResourceName m_Image;
	
	protected CRF_InsurgencyGamemodeManager m_InsurgencyGamemode;
	
	protected bool m_bMarkerActive = false;
	protected static const int ZONE_MARKER_COLOR = ARGB(255, 255, 165, 0);
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Cache reference to insurgency gamemode
		if (Replication.IsClient() || Replication.IsServer())
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
			GetGame().GetCallqueue().CallLater(UpdateZoneMarker, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Override visibility check to include phase-based logic with delayed reveal
	 * @return True if zone should be visible to current player
	 */
	override bool IsCurrentVisibility()
	{
		// First check base visibility (faction filters, gamemode state filters)
		if (!super.IsCurrentVisibility())
			return false;
		
		// Get insurgency gamemode instance
		if (!m_InsurgencyGamemode)
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
		
		if (!m_InsurgencyGamemode)
			return false; // Gamemode not ready yet
		
		// CRITICAL: Defenders should NEVER see search area zones
		if (IsVisibleForDefendingTeam())
			return false;
		
		int currentPhase = m_InsurgencyGamemode.GetCurrentPhase();
		
		// Check if this zone's phase is active
		bool isPhaseActive = (currentPhase == m_iVisibleDuringPhase);
		bool isPhaseComplete = (currentPhase > m_iVisibleDuringPhase);
		
		// If phase is complete and we should show to all teams, show it (but still only attackers due to check above)
		if (isPhaseComplete && m_bShowToAllWhenPhaseComplete)
			return IsVisibleForPlayerTeam();
		
		// If phase isn't active, hide the zone
		if (!isPhaseActive)
			return false;
		
		// Phase is active - check if zones are revealed yet to attackers
		if (!m_InsurgencyGamemode.AreCurrentPhaseZonesRevealed())
			return false; // Not revealed yet - hide from everyone
		
		// Zones revealed - show to attacking team only
		return IsVisibleForPlayerTeam();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if current player's team should see this zone (attacking team)
	 * @return True if player's team matches visibility settings
	 */
	protected bool IsVisibleForPlayerTeam()
	{	
		if (!m_InsurgencyGamemode)
			return false;
		
		// Get player's faction
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return false;
		
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction playerFaction = factionComp.GetAffiliatedFaction();
		if (!playerFaction)
			return false;
		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		
		return (playerFactionKey == m_InsurgencyGamemode.m_AttackingSide);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if current player is on defending team
	 * @return True if player is defender
	 */
	protected bool IsVisibleForDefendingTeam()
	{
		if (!m_InsurgencyGamemode)
			return false;
		
		// Get player's faction
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return false;
		
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction playerFaction = factionComp.GetAffiliatedFaction();
		if (!playerFaction)
			return false;
		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		
		return (playerFactionKey == m_InsurgencyGamemode.m_DefendingSide);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateZoneMarker()
	{
    	CRF_PlayerControllerManager pcm = CRF_PlayerControllerManager.GetInstance();
    	if (!pcm)
        	return;
    
    	bool shouldShow = IsCurrentVisibility();
    	
    	if (shouldShow && !m_bMarkerActive)
    	{
			IEntity owner = GetOwner();
        	vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);

	        pcm.AddScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Search Area (Phase %1)", m_iVisibleDuringPhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
        	m_bMarkerActive = true;
    	}
    	else if (!shouldShow && m_bMarkerActive)
    	{
			IEntity owner = GetOwner();
        	vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);

	        pcm.RemoveScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Search Area (Phase %1)", m_iVisibleDuringPhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
        	m_bMarkerActive = false;
    	}
	}
	
	override void OnDelete(IEntity owner)
	{
    	GetGame().GetCallqueue().Remove(UpdateZoneMarker);
    	super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get the phase this zone is visible during
	 * @return Phase number
	 */
	int GetVisiblePhase()
	{
		return m_iVisibleDuringPhase;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if zone should be visible during a specific phase
	 * @param phase Phase number to check
	 * @return True if zone is visible during that phase
	 */
	bool IsVisibleDuringPhase(int phase)
	{
		if (phase == m_iVisibleDuringPhase)
			return true;
		
		// Also visible if phase is complete and flag is set
		if (phase > m_iVisibleDuringPhase && m_bShowToAllWhenPhaseComplete)
			return true;
		
		return false;
	}
}
