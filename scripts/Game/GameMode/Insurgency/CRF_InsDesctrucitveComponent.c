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
	
	protected bool m_bDefenderMarkerActive = false;
	protected bool m_bSiteDestroyed = false;
	protected static const int CACHE_MARKER_COLOR = ARGB(255, 255, 50, 50); // red for cache
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

	    if (!GetGame().InPlayMode())
	        return;
	
	    // Server-only: register objective and subscribe to destruction
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
	            
	        SetCacheActive(m_bStartActive);
	    }
	
	    // Client-only: start marker polling (not on dedicated server)
	    if (!RplSession.Mode() == RplMode.Dedicated)
	        GetGame().GetCallqueue().CallLater(UpdateCacheMarker, 1000, true);
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
        
        // Check if the cache has been destroyed
        if (damageState == EDamageState.DESTROYED)
        {
			IEntity owner = GetOwner();
			vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
			CRF_PlayerControllerManager pcm = CRF_PlayerControllerManager.GetInstance();
			pcm.RemoveScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Cache (Phase %1)", m_iCachePhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
			
            Print("Cache has been DESTROYED!", LogLevel.NORMAL);
            m_bSiteDestroyed = true;
            // Notify the gamemode manager
            if (CRF_InsurgencyGamemodeManager.GetInstance())
                CRF_InsurgencyGamemodeManager.GetInstance().OnCacheDestroyed(GetOwner());
        }
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    void SetCacheActive(bool active)
    {
        m_bIsActive = active;
        IEntity owner = GetOwner();
        
        if (!owner)
            return;
        
        // Hide/show the entity
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
	protected void UpdateCacheMarker()
	{
	    CRF_PlayerControllerManager pcm = CRF_PlayerControllerManager.GetInstance();
	    CRF_InsurgencyGamemodeManager ins = CRF_InsurgencyGamemodeManager.GetInstance();
	    if (!pcm || !ins)
	        return;
	    
	    // Defenders only
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
	    
	    // Show only if defender, cache is active, and not destroyed
	    IEntity owner = GetOwner();
	    bool isVisible = owner && (owner.GetFlags() & EntityFlags.VISIBLE) != 0;
	    bool shouldShow = isDefender && isVisible && !IsCacheDestroyed();
	    
	    if (shouldShow && !m_bDefenderMarkerActive)
	    {
			vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
			
	        pcm.AddScriptedMarker("Static Marker", posStr, 1000,
	            string.Format("Cache (Phase %1)", m_iCachePhase),
	            m_Image.GetPath(), 500, ARGB(255, 255, 50, 50));
	        m_bDefenderMarkerActive = true;
	    }
	    else if (m_bSiteDestroyed || (!shouldShow && m_bDefenderMarkerActive))
	    {
			vector pos = owner.GetOrigin();
			string posStr = string.Format("%1 %2 %3", pos[0], pos[1], pos[2]);
			
	        pcm.RemoveScriptedMarker("Static Marker", posStr, 1000,
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
		
		super.OnDelete(owner);

        GetGame().GetCallqueue().Remove(UpdateCacheMarker);
		
        // Clean up event subscription
        if (m_DestructionComponent)
            m_DestructionComponent.GetOnDamageStateChanged().Remove(OnDamageStateChanged);
    }
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //! Check if cache is fully destroyed
    bool IsCacheDestroyed()
    {
        if (!m_DestructionComponent)
            return false;
            
        EDamageState state = m_DestructionComponent.GetState();
        return (state == EDamageState.DESTROYED);
    }
}