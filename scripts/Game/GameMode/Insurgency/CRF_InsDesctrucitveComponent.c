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
	
    // Replicated so clients can react to visibility and destruction state
    [RplProp(onRplName: "OnActiveStateReplicated")]
    protected bool m_bIsActiveReplicated = false;
    
    [RplProp(onRplName: "OnDestroyedStateReplicated")]
    protected bool m_bIsDestroyedReplicated = false;
	
	protected bool m_bDefenderMarkerActive = false;
	protected bool m_bSiteDestroyed = false;
	protected static const int CACHE_MARKER_COLOR = ARGB(255, 255, 50, 50);
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

	    if (!GetGame().InPlayMode())
	        return;
	
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
	        
	        // Defer SetCacheActive until RplComponent is ready so clients receive the initial state
	        GetGame().GetCallqueue().CallLater(InitialActivationDeferred, 100, false);
	    }
	
	    // Client-only: apply already-replicated state immediately on join, then start marker polling
	    if (Replication.IsClient())
	    {
	        // [RplProp()] values are already populated by the time OnPostInit runs on the client,
	        // so we can apply visibility directly without waiting for the callback
	        ApplyCacheVisibility(m_bIsActiveReplicated);
	        m_bSiteDestroyed = m_bIsDestroyedReplicated;
	        GetGame().GetCallqueue().CallLater(UpdateCacheMarker, 1000, true);
	    }
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Server-only: deferred initial activation, retries until RplId is valid
    protected void InitialActivationDeferred()
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;
        
        RplComponent rplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
        if (!rplComp || rplComp.Id() == RplId.Invalid())
        {
            // Not ready yet, keep retrying
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
    //! Server-only: set active state and replicate to clients
    void SetCacheActive(bool active)
    {
        m_bIsActive = active;
        m_bIsActiveReplicated = active;
        ApplyCacheVisibility(active);
        Replication.BumpMe();
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Called on client when m_bIsActiveReplicated is received
    protected void OnActiveStateReplicated()
    {
        ApplyCacheVisibility(m_bIsActiveReplicated);
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Called on client when m_bIsDestroyedReplicated is received
    protected void OnDestroyedStateReplicated()
    {
        m_bSiteDestroyed = m_bIsDestroyedReplicated;
        // Force marker update immediately rather than waiting for next poll
        UpdateCacheMarker();
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Shared: apply entity visibility flags, safe to call on any context
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
    //! Client-only: poll and update defender map marker
	protected void UpdateCacheMarker()
	{
	    CRF_InsurgencyGamemodeManager ins = CRF_InsurgencyGamemodeManager.GetInstance();
	    if (!ins)
	        return;
	    
	    SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
	    if (!playerController)
	        return;
	    
	    SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
	        playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
	    if (!factionComp)
	        return;
	    
	    Faction playerFaction = factionComp.GetAffiliatedFaction();
	    if (!playerFaction)
	        return;
	    
	    bool isDefender = (playerFaction.GetFactionKey() == ins.m_DefendingSide);
	    bool isPhaseActive = (ins.GetCurrentPhase() == m_iCachePhase);
    	bool shouldShow = isDefender && isPhaseActive && !IsCacheDestroyed();
		
		IEntity owner = GetOwner();
		CRF_PlayerScriptedMarkerManager psmm = CRF_PlayerScriptedMarkerManager.GetInstance();
		
		if (!owner || !psmm)
			return;
		
	    if (shouldShow && !m_bDefenderMarkerActive)
	    {
			vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
			
	        psmm.AddScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Cache (Phase %1)", m_iCachePhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
	        m_bDefenderMarkerActive = true;
	    }
	    else if (!shouldShow && m_bDefenderMarkerActive)
	    {
			vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
			
	        psmm.RemoveScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Cache (Phase %1)", m_iCachePhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
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
		
        // Client-only marker cleanup
        if (Replication.IsClient() && m_bDefenderMarkerActive)
        {
            vector pos = owner.GetOrigin();
            string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
            CRF_PlayerScriptedMarkerManager psmm = CRF_PlayerScriptedMarkerManager.GetInstance();
            if (psmm)
                psmm.RemoveScriptedMarker("Static Marker", posStr, 1000,
                    string.Format("Cache (Phase %1)", m_iCachePhase),
                    m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
        }
		
        // Server-only cleanup
        if (m_DestructionComponent)
            m_DestructionComponent.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
		
        super.OnDelete(owner);
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Works on both server and client - server uses destruction component, client uses replicated bool
    bool IsCacheDestroyed()
    {
        if (Replication.IsClient())
            return m_bIsDestroyedReplicated;
        
        if (!m_DestructionComponent)
            return false;
            
        return (m_DestructionComponent.GetState() == EDamageState.DESTROYED);
    }
}