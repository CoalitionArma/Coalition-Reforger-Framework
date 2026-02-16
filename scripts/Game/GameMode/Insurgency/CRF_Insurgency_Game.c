[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_InsurgencyGamemodeManagerClass: SCR_BaseGameModeComponentClass {}

class CRF_InsurgencyGamemodeManager: SCR_BaseGameModeComponent
{
    [Attribute("OPFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side attacking the MCOM sites")]
	FactionKey m_AttackingSide;
	
	[Attribute("INFDOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side defending the MCOM sites")]
	FactionKey m_DefendingSide;
	
	[Attribute("5", "auto", "Time until next phase zone is revealed to attackers (Set to 0 for instant reveal)")]
	int phaseBufferMinutes;

    // Replicated Properties
    [RplProp()]
    ref array<RplId> m_ObjCachesRplID = {}; // Used for clients
    
    [RplProp()]
    ref array<RplId> m_ObjDestroyedCachesRplID = {}; // Track destroyed caches for clients
    
    [RplProp()]
    int m_iCurrentPhase = 1; // Current active phase

    // Protected Member Variables
    protected ref array<IEntity> m_aObjCaches = {}; // All caches (server)
    protected ref array<IEntity> m_aDestroyedCaches = {}; // Track destroyed caches
    
    // Phase tracking - maps phase number to cache entities in that phase
    protected ref map<int, ref array<IEntity>> m_mPhaseToActiveCaches = new map<int, ref array<IEntity>>();
    protected ref map<int, ref array<IEntity>> m_mPhaseToDestroyedCaches = new map<int, ref array<IEntity>>();

    protected static CRF_InsurgencyGamemodeManager m_sInstance;
	protected static CRF_RplBroadcastManager m_RplBroadcastManager;
	protected static CRF_RespawnManager m_RespawnManager;

    void CRF_InsurgencyGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
    {
        m_sInstance = this;
    }

    //------------------------------------------------------------------------------------------------
    // Singleton accessor
    static CRF_InsurgencyGamemodeManager GetInstance()
    {
        return m_sInstance;
    }
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Gamemode reference to Broadcast Manager
		if (Replication.IsClient() || Replication.IsServer())
			m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
			m_RespawnManager = CRF_RespawnManager.GetInstance();
	}

    //------------------------------------------------------------------------------------------------
    void RegisterCacheObjective(IEntity objectiveItem)
    {
        if (!objectiveItem)
            return;

        RplComponent rplComp = RplComponent.Cast(objectiveItem.FindComponent(RplComponent));
        if (!rplComp)
            return;

        // Retry until RplId is valid
        if (rplComp.Id() == RplId.Invalid())
        {
            GetGame().GetCallqueue().CallLater(RegisterCacheObjective, 100, false, objectiveItem);
            return;
        }

        Print(string.Format("Registering cache objective: %1 (RplId: %2)", objectiveItem, rplComp.Id()));

        // Get the cache component to determine its phase
        CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(objectiveItem.FindComponent(CRF_InsDestructiveComponent));
        if (!cacheComp)
        {
            Print("Warning: Cache registered without CRF_InsDestructiveComponent!", LogLevel.WARNING);
            return;
        }
        
        int phase = cacheComp.GetCachePhase();
        
        // Store cache in overall list
        m_aObjCaches.Insert(objectiveItem);
        m_ObjCachesRplID.Insert(rplComp.Id());
        
        // Organize caches by phase
        if (!m_mPhaseToActiveCaches.Contains(phase))
        {
            m_mPhaseToActiveCaches.Insert(phase, new array<IEntity>());
            m_mPhaseToDestroyedCaches.Insert(phase, new array<IEntity>());
        }
        
        m_mPhaseToActiveCaches.Get(phase).Insert(objectiveItem);
        
        Print(string.Format("Cache registered to phase %1. Total caches in phase: %2", 
            phase, m_mPhaseToActiveCaches.Get(phase).Count()));
        
        Replication.BumpMe();
    }
    
    //------------------------------------------------------------------------------------------------
    void OnCacheDestroyed(IEntity cacheEntity)
    {
        if (!cacheEntity)
            return;
            
        Print(string.Format("Cache destroyed: %1", cacheEntity), LogLevel.NORMAL);
        
        // Prevent duplicate destruction notifications
        if (m_aDestroyedCaches.Contains(cacheEntity))
        {
            Print("Cache already marked as destroyed, skipping duplicate notification", LogLevel.WARNING);
            return;
        }
        
        // Get cache component to determine phase
        CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(cacheEntity.FindComponent(CRF_InsDestructiveComponent));
        if (!cacheComp)
            return;
            
        int phase = cacheComp.GetCachePhase();
        
        // Track the destroyed cache globally
        m_aDestroyedCaches.Insert(cacheEntity);
        
        // Move cache from active to destroyed for this phase
        if (m_mPhaseToActiveCaches.Contains(phase))
        {
            array<IEntity> activeCaches = m_mPhaseToActiveCaches.Get(phase);
            int index = activeCaches.Find(cacheEntity);
            if (index != -1)
                activeCaches.Remove(index);
        }
        
        if (m_mPhaseToDestroyedCaches.Contains(phase))
        {
            m_mPhaseToDestroyedCaches.Get(phase).Insert(cacheEntity);
        }
        
        // Remove from overall active caches and add to destroyed list
        int index = m_aObjCaches.Find(cacheEntity);
        if (index != -1)
        {
            RplComponent rplComp = RplComponent.Cast(cacheEntity.FindComponent(RplComponent));
            if (rplComp)
            {
                RplId rplId = rplComp.Id();
                m_ObjCachesRplID.RemoveItemOrdered(rplId);
                m_ObjDestroyedCachesRplID.Insert(rplId);
            }
            m_aObjCaches.Remove(index);
        }
        
        Print(string.Format("Phase %1 - Active: %2 / Destroyed: %3", 
            phase, 
            m_mPhaseToActiveCaches.Get(phase).Count(), 
            m_mPhaseToDestroyedCaches.Get(phase).Count()), LogLevel.NORMAL);
		
		
        
        // Check if all caches in current phase are destroyed
        CheckPhaseCompletion(phase);
        
        // Replicate changes to clients
        Replication.BumpMe();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckPhaseCompletion(int phase)
    {
        // Check if all caches in this phase are destroyed
        if (!m_mPhaseToActiveCaches.Contains(phase))
            return;
            
        array<IEntity> activeCaches = m_mPhaseToActiveCaches.Get(phase);
        
        if (activeCaches.IsEmpty())
        {
            Print(string.Format("Phase %1 complete! All caches destroyed.", phase), LogLevel.NORMAL);
			
			
            
            // Activate next phase
            int res = ActivateNextPhase(phase + 1);
            
			if (res == -1)
			{
				m_RplBroadcastManager.PopUpNotification(15, string.Format("Phase %1 all caches destroyed. Attackers win!", phase));
			}
        }
		else
		{
			int cachesDestroyedForPhase = m_mPhaseToActiveCaches.Get(phase).Count();
			int cacheTotalForPhase = m_mPhaseToActiveCaches.Get(phase).Count() + m_mPhaseToDestroyedCaches.Get(phase).Count();
			
			m_RplBroadcastManager.PopUpNotification(15, string.Format("Phase %1: (%2/%3) caches destroyed", phase, cachesDestroyedForPhase, cacheTotalForPhase));
		}
    }
    
    //------------------------------------------------------------------------------------------------
    protected int ActivateNextPhase(int nextPhase)
    {
        if (!m_mPhaseToActiveCaches.Contains(nextPhase))
        {
            Print(string.Format("No phase %1 configured. Mission complete!", nextPhase), LogLevel.NORMAL);
            return -1;
        }
        
        m_iCurrentPhase = nextPhase;
        
        array<IEntity> nextPhaseCaches = m_mPhaseToActiveCaches.Get(nextPhase);
        
        Print(string.Format("Activating phase %1 with %2 caches", nextPhase, nextPhaseCaches.Count()), LogLevel.NORMAL);
        
        // Activate all caches in the next phase
        foreach (IEntity cache : nextPhaseCaches)
        {
            CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(cache.FindComponent(CRF_InsDestructiveComponent));
            if (cacheComp)
                cacheComp.SetCacheActive(true);
        }
		
		RespawnPlayersForZoneAdvance();
        
        Replication.BumpMe();
		
		if (phaseBufferMinutes > 0)
		{
			m_RplBroadcastManager.PopUpNotification(15, string.Format("Phase %1 all caches destroyed. Shifting to phase %2.", nextPhase-1, nextPhase),
			string.Format("Search area for attackers revealed in %1 minutes!", phaseBufferMinutes));
		}
		else
		{
			m_RplBroadcastManager.PopUpNotification(15, string.Format("Phase %1 all caches destroyed. Shifting to phase %2.", nextPhase-1, nextPhase),
			string.Format("Attackers need to destroy %1 caches"));
		}
		
		return 1;
    }
	
	//------------------------------------------------------------------------------------------------
	protected void RespawnPlayersForZoneAdvance()
	{
		if (!m_RespawnManager)
			return;

		// Trigger respawn waves for both attacking and defending sides
		m_RespawnManager.RespawnSide(m_AttackingSide);
		m_RespawnManager.RespawnSide(m_DefendingSide);
	}
    
    //------------------------------------------------------------------------------------------------
    protected void CheckGameEndConditions()
    {
        // Check if all caches across all phases are destroyed
        if (m_aObjCaches.IsEmpty() && !m_aDestroyedCaches.IsEmpty())
        {
            Print("All caches destroyed across all phases! Attackers win!", LogLevel.NORMAL);
			m_RplBroadcastManager.PopUpNotification(15, "All caches destroyed across all phases! Attackers win!");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    int GetCurrentPhase()
    {
        return m_iCurrentPhase;
    }
    
    //------------------------------------------------------------------------------------------------
    int GetActiveCacheCount()
    {
        return m_aObjCaches.Count();
    }
    
    //------------------------------------------------------------------------------------------------
    int GetActiveCacheCountForPhase(int phase)
    {
        if (!m_mPhaseToActiveCaches.Contains(phase))
            return 0;
            
        return m_mPhaseToActiveCaches.Get(phase).Count();
    }
    
    //------------------------------------------------------------------------------------------------
    int GetDestroyedCacheCount()
    {
        return m_aDestroyedCaches.Count();
    }
    
    //------------------------------------------------------------------------------------------------
    int GetDestroyedCacheCountForPhase(int phase)
    {
        if (!m_mPhaseToDestroyedCaches.Contains(phase))
            return 0;
            
        return m_mPhaseToDestroyedCaches.Get(phase).Count();
    }
    
    //------------------------------------------------------------------------------------------------
    int GetTotalCacheCount()
    {
        return m_aObjCaches.Count() + m_aDestroyedCaches.Count();
    }
    
    //------------------------------------------------------------------------------------------------
    int GetTotalCacheCountForPhase(int phase)
    {
        return GetActiveCacheCountForPhase(phase) + GetDestroyedCacheCountForPhase(phase);
    }
    
    //------------------------------------------------------------------------------------------------
    array<IEntity> GetActiveCaches()
    {
        return m_aObjCaches;
    }
    
    //------------------------------------------------------------------------------------------------
    array<IEntity> GetActiveCachesForPhase(int phase)
    {
        if (!m_mPhaseToActiveCaches.Contains(phase))
            return null;
            
        return m_mPhaseToActiveCaches.Get(phase);
    }
    
    //------------------------------------------------------------------------------------------------
    array<IEntity> GetDestroyedCaches()
    {
        return m_aDestroyedCaches;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get overall mission progress (0-100%)
    float GetOverallProgress()
    {
        int total = GetTotalCacheCount();
        if (total == 0)
            return 0;
            
        return (m_aDestroyedCaches.Count() / (float)total) * 100.0;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get phase progress (0-100%)
    float GetPhaseProgress(int phase)
    {
        int total = GetTotalCacheCountForPhase(phase);
        if (total == 0)
            return 0;
            
        int destroyed = GetDestroyedCacheCountForPhase(phase);
        return (destroyed / (float)total) * 100.0;
    }
}