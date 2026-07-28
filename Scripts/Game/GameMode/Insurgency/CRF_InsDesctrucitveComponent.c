class CRF_InsDestructiveComponentClass: ScriptComponentClass {}
class CRF_InsDestructiveComponent: ScriptComponent
{
    [Attribute("1", "auto", "Will defenders know the location of their cache")]
    bool m_bShowMarkerForDefenders;
    
    [Attribute("1", UIWidgets.EditBox, "Which phase/sequence does this cache belong to (1, 2, 3, etc)")]
    int m_iCachePhase;
    
    [Attribute("0", UIWidgets.CheckBox, "Should this cache start visible/active")]
    bool m_bStartActive;
	
	[Attribute(params: "edds", uiwidget: UIWidgets.ResourcePickerThumbnail)]
	private ResourceName m_Image;

    protected SCR_DestructionMultiPhaseComponent m_DestructionComponent;
    protected bool m_bIsActive;
	
    [RplProp(onRplName: "OnActiveStateReplicated")]
    protected bool m_bIsActiveReplicated = false;
    
    [RplProp(onRplName: "OnDestroyedStateReplicated")]
    protected bool m_bIsDestroyedReplicated = false;
	
	// FIX: Cache stable marker identity strings at init (same pattern as PolyZone fix)
	protected string m_sMarkerPosStr;
	protected string m_sMarkerLabel;
	
	protected bool m_bDefenderMarkerActive = false;
	protected bool m_bSiteDestroyed = false;
	protected static const int CACHE_MARKER_COLOR = ARGB(255, 255, 50, 50);
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

	    if (!GetGame().InPlayMode())
	        return;
	
		// Pre-compute stable marker identity strings
		vector pos = owner.GetOrigin();
		m_sMarkerPosStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
		m_sMarkerLabel  = string.Format("Cache (Phase %1)", m_iCachePhase);
	
	    if (Replication.IsServer())
	    {
	        m_DestructionComponent = SCR_DestructionMultiPhaseComponent.Cast(owner.FindComponent(SCR_DestructionMultiPhaseComponent));
	        
	        if (!m_DestructionComponent)
	        {
	            Print("CRF_InsDestructiveComponent: No SCR_DestructionMultiPhaseComponent found!", LogLevel.WARNING);
	            return;
	        }
	        
	        m_DestructionComponent.GetOnDamageStateChanged().Insert(OnDamageStateChanged);

	        if (CRF_InsurgencyGamemodeManager.GetInstance())
	            CRF_InsurgencyGamemodeManager.GetInstance().RegisterCacheObjective(owner);
	        
	        GetGame().GetCallqueue().CallLater(InitialActivationDeferred, 100, false);
	    }
	
	    if (Replication.IsClient())
	    {
	        ApplyCacheVisibility(m_bIsActiveReplicated);
	        m_bSiteDestroyed = m_bIsDestroyedReplicated;
	        GetGame().GetCallqueue().CallLater(UpdateCacheMarker, 1000, true);
	    }
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    protected void InitialActivationDeferred()
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;
        
        RplComponent rplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
        if (!rplComp || rplComp.Id() == RplId.Invalid())
        {
            GetGame().GetCallqueue().CallLater(InitialActivationDeferred, 100, false);
            return;
        }
        
        SetCacheActive(m_bStartActive);
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    protected void OnDamageStateChanged()
    {
        if (!m_DestructionComponent)
            return;
            
        EDamageState damageState = m_DestructionComponent.GetState();
        int currentPhase = m_DestructionComponent.GetDamagePhase();
        int maxPhases = m_DestructionComponent.GetNumDamagePhases();
        
        Print(string.Format("Cache state changed - Phase: %1/%2, State: %3", 
            currentPhase, maxPhases, typename.EnumToString(EDamageState, damageState)), LogLevel.NORMAL);
        
        if (damageState == EDamageState.DESTROYED)
        {
            Print("Cache has been DESTROYED!", LogLevel.NORMAL);
            m_bSiteDestroyed = true;
            m_bIsDestroyedReplicated = true;
            Replication.BumpMe();
            
            if (CRF_InsurgencyGamemodeManager.GetInstance())
                CRF_InsurgencyGamemodeManager.GetInstance().OnCacheDestroyed(GetOwner());
        }
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    void SetCacheActive(bool active)
    {
        m_bIsActive = active;
        m_bIsActiveReplicated = active;
        ApplyCacheVisibility(active);
        Replication.BumpMe();
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    protected void OnActiveStateReplicated()
    {
        ApplyCacheVisibility(m_bIsActiveReplicated);
        // FIX: Also force a marker update immediately when active state changes so the
        //      defender marker appears/disappears without waiting for the next poll tick.
        UpdateCacheMarker();
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    protected void OnDestroyedStateReplicated()
    {
        m_bSiteDestroyed = m_bIsDestroyedReplicated;
        UpdateCacheMarker();
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    protected void ApplyCacheVisibility(bool active)
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;
        
        if (active)
        {
            owner.SetFlags(EntityFlags.VISIBLE, true);
            owner.ClearFlags(EntityFlags.DISABLED, true);
            Print(string.Format("Cache activated: Phase %1", m_iCachePhase), LogLevel.NORMAL);
        }
        else
        {
            owner.ClearFlags(EntityFlags.VISIBLE, true);
            owner.SetFlags(EntityFlags.DISABLED, true);
            Print(string.Format("Cache hidden: Phase %1", m_iCachePhase), LogLevel.NORMAL);
        }
    }
	
	//------------------------------------------------------------------------------------------------
    //! Client-only: poll and update defender map marker.
	protected void UpdateCacheMarker()
	{
	    CRF_InsurgencyGamemodeManager ins = CRF_InsurgencyGamemodeManager.GetInstance();
	    if (!ins)
	        return;
	    
	    // Use SCR_FactionManager instead of GetPlayerController() which returns null
	    // in component context on clients connected to a dedicated server.
	    SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	    if (!factionMgr)
	        return;
	    
	    Faction playerFaction = factionMgr.GetLocalPlayerFaction();
	    if (!playerFaction)
	        return;
		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		bool isDefender = (playerFactionKey == ins.m_DefendingSide);
	    bool isPhaseActive = (ins.GetCurrentPhase() == m_iCachePhase);
	
		// FIX: Respect the m_bShowMarkerForDefenders attribute - was ignored before.
    	bool shouldShow = m_bShowMarkerForDefenders && isDefender && isPhaseActive && m_bIsActiveReplicated && !IsCacheDestroyed();
		
		COA_PlayerScriptedMarkerManager psmm = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!psmm)
			return;
		
	    if (shouldShow && !m_bDefenderMarkerActive)
	    {
	        psmm.AddScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
	            m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
	        m_bDefenderMarkerActive = true;
	    }
	    else if (!shouldShow && m_bDefenderMarkerActive)
	    {
	        psmm.RemoveScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
	            m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
	        m_bDefenderMarkerActive = false;
	    }
	}

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    bool IsCacheActive()
    {
        return m_bIsActive;
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    int GetCachePhase()
    {
        return m_iCachePhase;
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    override void OnDelete(IEntity owner)
    {
        GetGame().GetCallqueue().Remove(UpdateCacheMarker);
        GetGame().GetCallqueue().Remove(InitialActivationDeferred);
		
        if (Replication.IsClient() && m_bDefenderMarkerActive)
        {
            COA_PlayerScriptedMarkerManager psmm = COA_PlayerScriptedMarkerManager.GetInstance();
            if (psmm)
                psmm.RemoveScriptedMarker("Static Marker", m_sMarkerPosStr, 1000,
                    m_sMarkerLabel, m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
        }
		
        if (m_DestructionComponent)
            m_DestructionComponent.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
		
        super.OnDelete(owner);
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    bool IsCacheDestroyed()
    {
        if (Replication.IsClient())
            return m_bIsDestroyedReplicated;
        
        if (!m_DestructionComponent)
            return false;
            
        return (m_DestructionComponent.GetState() == EDamageState.DESTROYED);
    }
}