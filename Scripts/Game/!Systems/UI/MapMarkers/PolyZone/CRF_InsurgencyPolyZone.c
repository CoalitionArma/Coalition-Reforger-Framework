/*
//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Insurgency", description: "Phase-based polygon zone for Insurgency gamemode", color: "255 128 0 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_InsurgencyPolyZoneClass: COA_PolyZoneClass
{
};

//------------------------------------------------------------------------------------------------
class CRF_InsurgencyPolyZone : COA_PolyZone
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
	
	protected string m_sMarkerPosStr;
	protected string m_sMarkerLabel;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
		
		vector pos = owner.GetOrigin();
		m_sMarkerPosStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
		m_sMarkerLabel  = string.Format("Search Area (Phase %1)", m_iVisibleDuringPhase);
		
		if (Replication.IsClient())
		{
			UpdateZoneMarker();
			GetGame().GetCallqueue().CallLater(UpdateZoneMarker, 1000, true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override bool IsCurrentVisibility()
	{
		if (!super.IsCurrentVisibility())
			return false;
		
		if (!m_InsurgencyGamemode)
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
		
		if (!m_InsurgencyGamemode)
			return false;
		
		if (IsCurrentPlayerDefender())
			return false;
		
		int currentPhase = m_InsurgencyGamemode.GetCurrentPhase();
		
		bool isPhaseActive   = (currentPhase == m_iVisibleDuringPhase);
		bool isPhaseComplete = (currentPhase > m_iVisibleDuringPhase);
		
		if (isPhaseComplete && m_bShowToAllWhenPhaseComplete)
			return IsCurrentPlayerAttacker();
		
		if (!isPhaseActive)
			return false;
		
		if (!m_InsurgencyGamemode.AreCurrentPhaseZonesRevealed())
			return false;
		
		return IsCurrentPlayerAttacker();
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsCurrentPlayerAttacker()
	{
		if (!m_InsurgencyGamemode)
			return false;
		
		FactionKey key = GetLocalPlayerFactionKey();
		if (key.IsEmpty())
			return false;
		
		return (key == m_InsurgencyGamemode.m_AttackingSide);
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsCurrentPlayerDefender()
	{
		if (!m_InsurgencyGamemode)
			return false;
		
		FactionKey key = GetLocalPlayerFactionKey();
		if (key.IsEmpty())
			return false;
		
		return (key == m_InsurgencyGamemode.m_DefendingSide);
	}
	
	//------------------------------------------------------------------------------------------------
	// Get the local player's faction key via SCR_FactionManager.
	// GetGame().GetPlayerController() returns null in component context on clients;
	// SCR_FactionManager.GetLocalPlayerFaction() is the correct client-safe path.
	protected FactionKey GetLocalPlayerFactionKey()
	{
		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMgr)
			return string.Empty;
		
		Faction playerFaction = factionMgr.GetLocalPlayerFaction();
		if (!playerFaction)
			return string.Empty;
		
		return playerFaction.GetFactionKey();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-only: poll and update search area map marker.
	protected void UpdateZoneMarker()
	{
		if (!m_InsurgencyGamemode)
		{
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
			if (!m_InsurgencyGamemode)
				return;
		}
		
		COA_PlayerScriptedMarkerManager psmm = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!psmm)
			return;
		
		bool shouldShow = IsCurrentVisibility();
		
		if (shouldShow && !m_bMarkerActive)
		{
			psmm.AddScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
				m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
			m_bMarkerActive = true;
		}
		else if (!shouldShow && m_bMarkerActive)
		{
			psmm.RemoveScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
				m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
			m_bMarkerActive = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UpdateZoneMarker);
		
		if (m_bMarkerActive)
		{
			COA_PlayerScriptedMarkerManager psmm = COA_PlayerScriptedMarkerManager.GetInstance();
			if (psmm)
				psmm.RemoveScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
					m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
			m_bMarkerActive = false;
		}
		
		super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetVisiblePhase()
	{
		return m_iVisibleDuringPhase;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsVisibleDuringPhase(int phase)
	{
		if (phase == m_iVisibleDuringPhase)
			return true;
		
		if (phase > m_iVisibleDuringPhase && m_bShowToAllWhenPhaseComplete)
			return true;
		
		return false;
	}
}
*/