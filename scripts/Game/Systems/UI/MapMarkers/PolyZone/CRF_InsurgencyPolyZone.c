//------------------------------------------------------------------------------------------------
// Enum for team visibility options
enum EInsurgencyTeam
{
	ATTACKERS,
	DEFENDERS,
	BOTH
}

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
	
	[Attribute("2", UIWidgets.ComboBox, "Which team should see this zone", "", ParamEnumArray.FromEnum(EInsurgencyTeam))]
	protected EInsurgencyTeam m_eVisibleForTeam;
	
	[Attribute("0", UIWidgets.CheckBox, "Show zone to both teams when phase is complete?")]
	protected bool m_bShowToAllWhenPhaseComplete;
	
	protected CRF_InsurgencyGamemodeManager m_InsurgencyGamemode;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Cache reference to insurgency gamemode
		if (Replication.IsClient() || Replication.IsServer())
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Override visibility check to include phase-based logic
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
		
		int currentPhase = m_InsurgencyGamemode.GetCurrentPhase();
		
		// Check if this zone's phase is active
		bool isPhaseActive = (currentPhase == m_iVisibleDuringPhase);
		bool isPhaseComplete = (currentPhase > m_iVisibleDuringPhase);
		
		// If phase is complete and we should show to all teams, show it
		if (isPhaseComplete && m_bShowToAllWhenPhaseComplete)
			return true;
		
		// If phase isn't active, hide the zone
		if (!isPhaseActive)
			return false;
		
		// Phase is active - check team visibility
		return IsVisibleForPlayerTeam();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if current player's team should see this zone
	 * @return True if player's team matches visibility settings
	 */
	protected bool IsVisibleForPlayerTeam()
	{
		// If visible for both teams, always show
		if (m_eVisibleForTeam == EInsurgencyTeam.BOTH)
			return true;
		
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
		
		// Check if player is on attacking team
		if (m_eVisibleForTeam == EInsurgencyTeam.ATTACKERS)
			return (playerFactionKey == m_InsurgencyGamemode.attackingSide);
		
		// Check if player is on defending team
		if (m_eVisibleForTeam == EInsurgencyTeam.DEFENDERS)
			return (playerFactionKey == m_InsurgencyGamemode.defendingSide);
		
		return false;
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
	 * Get which team can see this zone
	 * @return Team enum value
	 */
	EInsurgencyTeam GetVisibleTeam()
	{
		return m_eVisibleForTeam;
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
