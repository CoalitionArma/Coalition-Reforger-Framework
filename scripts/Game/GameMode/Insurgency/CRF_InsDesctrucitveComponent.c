class CRF_InsDestructiveComponentClass: ScriptComponentClass {}
class CRF_InsDestructiveComponent: ScriptComponent
{
    [Attribute("1", "auto", "Will defenders know the location of their cache")]
    bool m_bShowMarkerForDefenders;
    
    [Attribute("1", UIWidgets.EditBox, "Which phase/sequence does this cache belong to (1, 2, 3, etc)")]
    int m_iCachePhase;
    
    [Attribute("0", UIWidgets.CheckBox, "Should this cache start visible/active")]
    bool m_bStartActive;
    
    protected SCR_DestructionMultiPhaseComponent m_DestructionComponent;
    protected bool m_bIsActive;
    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        // Only server should register objectives and listen for destruction
        if (!GetGame().InPlayMode() || !Replication.IsServer())
            return;

        // Get the multi-phase destruction component
        m_DestructionComponent = SCR_DestructionMultiPhaseComponent.Cast(owner.FindComponent(SCR_DestructionMultiPhaseComponent));
        
        if (!m_DestructionComponent)
        {
            Print("CRF_InsDestructiveComponent: No SCR_DestructionMultiPhaseComponent found on entity!", LogLevel.WARNING);
            return;
        }
        
        // Subscribe to the damage state changed event
        m_DestructionComponent.GetOnDamageStateChanged().Insert(OnDamageStateChanged);

        if(CRF_InsurgencyGamemodeManager.GetInstance())
            CRF_InsurgencyGamemodeManager.GetInstance().RegisterCacheObjective(owner);
            
        // Set initial active state
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
        
        // Check if the cache has been destroyed
        if (damageState == EDamageState.DESTROYED)
        {
            Print("Cache has been DESTROYED!", LogLevel.NORMAL);
            
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