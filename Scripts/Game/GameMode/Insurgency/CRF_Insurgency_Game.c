[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_InsurgencyGamemodeManagerClass: SCR_BaseGameModeComponentClass {}

class CRF_InsurgencyGamemodeManager: SCR_BaseGameModeComponent
{
    [Attribute("OPFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side attacking the cache sites")]
	FactionKey m_AttackingSide;
	
	// FIX: Was "INFDOR" (typo), corrected to "INDFOR"
	[Attribute("INDFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")}, desc: "The side defending the cache sites")]
	FactionKey m_DefendingSide;
	
	[Attribute("0", UIWidgets.Slider, "Time until next phase zone is revealed to attackers (in minutes, 0 for instant reveal)", params: "0 30 1")]
	int m_iPhaseBufferMinutes;

    // Replicated Properties
    [RplProp()]
    ref array<RplId> m_ObjCachesRplID = {};
    
    [RplProp()]
    ref array<RplId> m_ObjDestroyedCachesRplID = {};
    
    [RplProp()]
    int m_iCurrentPhase = 1;
    
    [RplProp()]
    float m_iTimePhaseZoneReveals = -1;

    // Protected Member Variables
    //! Server-only: do not call phase map getters from client context
    protected ref array<IEntity> m_aObjCaches = {};
    protected ref array<IEntity> m_aDestroyedCaches = {};
    
    //! Server-only: phase tracking maps
    protected ref map<int, ref array<IEntity>> m_mPhaseToActiveCaches = new map<int, ref array<IEntity>>();
    protected ref map<int, ref array<IEntity>> m_mPhaseToDestroyedCaches = new map<int, ref array<IEntity>>();

    protected static CRF_InsurgencyGamemodeManager m_sInstance;

    // FIX: Lazy-init references instead of caching at OnPostInit to avoid initialization order issues
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_RespawnManager m_RespawnManager;
    protected CRF_GamemodeManager m_GamemodeManager;
    
    protected bool m_bZoneRevealNotificationSent = false;

    void CRF_InsurgencyGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
    {
        m_sInstance = this;
    }

    //------------------------------------------------------------------------------------------------
    void ~CRF_InsurgencyGamemodeManager()
    {
        if (m_sInstance == this)
            m_sInstance = null;
    }

    //------------------------------------------------------------------------------------------------
    static CRF_InsurgencyGamemodeManager GetInstance()
    {
        return m_sInstance;
    }
	
	//------------------------------------------------------------------------------------------------
    // FIX: Lazy getters for manager references to avoid initialization order dependency
    protected CRF_RplBroadcastManager GetBroadcastManager()
    {
        if (!m_RplBroadcastManager)
            m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
        return m_RplBroadcastManager;
    }
    
    protected CRF_RespawnManager GetRespawnManager()
    {
        if (!m_RespawnManager)
            m_RespawnManager = CRF_RespawnManager.GetInstance();
        return m_RespawnManager;
    }
    
    protected CRF_GamemodeManager GetGamemodeManager()
    {
        if (!m_GamemodeManager)
            m_GamemodeManager = CRF_GamemodeManager.GetInstance();
        return m_GamemodeManager;
    }
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
        
        // Phase 1 zones are always immediately visible
        m_iTimePhaseZoneReveals = -1;
	}

    //------------------------------------------------------------------------------------------------
    static const int MAX_CACHE_REGISTER_RETRIES = 20;

    //------------------------------------------------------------------------------------------------
    void RegisterCacheObjective(IEntity objectiveItem, int attempt = 0)
    {
        if (!objectiveItem)
            return;

        RplComponent rplComp = RplComponent.Cast(objectiveItem.FindComponent(RplComponent));
        if (!rplComp)
            return;

        // Retry until RplId is valid, up to ~2 seconds total
        if (rplComp.Id() == RplId.Invalid())
        {
            if (attempt + 1 >= MAX_CACHE_REGISTER_RETRIES)
            {
                Print("CRF_InsurgencyGamemodeManager: Failed to register cache — RplId never became valid after max retries", LogLevel.WARNING);
                return;
            }
            GetGame().GetCallqueue().CallLater(RegisterCacheObjective, 100, false, objectiveItem, attempt + 1);
            return;
        }

        CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(objectiveItem.FindComponent(CRF_InsDestructiveComponent));
        if (!cacheComp)
        {
            Print("CRF_InsurgencyGamemodeManager: Warning: Cache registered without CRF_InsDestructiveComponent!", LogLevel.WARNING);
            return;
        }
        
        int phase = cacheComp.GetCachePhase();
        
        m_aObjCaches.Insert(objectiveItem);
        m_ObjCachesRplID.Insert(rplComp.Id());
        
        if (!m_mPhaseToActiveCaches.Contains(phase))
        {
            m_mPhaseToActiveCaches.Insert(phase, new array<IEntity>());
            m_mPhaseToDestroyedCaches.Insert(phase, new array<IEntity>());
        }
        
        m_mPhaseToActiveCaches.Get(phase).Insert(objectiveItem);
        
        // FIX: Removed spammy Print statements, kept one summary log
        Print(string.Format("CRF_InsurgencyGamemodeManager: Cache registered to phase %1. Total in phase: %2", 
            phase, m_mPhaseToActiveCaches.Get(phase).Count()), LogLevel.NORMAL);
        
        Replication.BumpMe();
    }
    
    //------------------------------------------------------------------------------------------------
    void OnCacheDestroyed(IEntity cacheEntity)
    {
        if (!cacheEntity)
            return;
        
        if (m_aDestroyedCaches.Contains(cacheEntity))
        {
            Print("CRF_InsurgencyGamemodeManager: Cache already marked as destroyed, skipping duplicate", LogLevel.WARNING);
            return;
        }
        
        CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(cacheEntity.FindComponent(CRF_InsDestructiveComponent));
        if (!cacheComp)
            return;
            
        int phase = cacheComp.GetCachePhase();
        
        m_aDestroyedCaches.Insert(cacheEntity);
        
        if (m_mPhaseToActiveCaches.Contains(phase))
        {
            array<IEntity> activeCaches = m_mPhaseToActiveCaches.Get(phase);
            int index = activeCaches.Find(cacheEntity);
            if (index != -1)
                activeCaches.Remove(index);
        }
        
        if (m_mPhaseToDestroyedCaches.Contains(phase))
            m_mPhaseToDestroyedCaches.Get(phase).Insert(cacheEntity);
        
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
        
        Print(string.Format("CRF_InsurgencyGamemodeManager: Phase %1 - Active: %2 / Destroyed: %3", 
            phase, 
            m_mPhaseToActiveCaches.Get(phase).Count(), 
            m_mPhaseToDestroyedCaches.Get(phase).Count()), LogLevel.NORMAL);
        
        CheckPhaseCompletion(phase);
        
        Replication.BumpMe();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void CheckPhaseCompletion(int phase)
    {
        if (!m_mPhaseToActiveCaches.Contains(phase))
            return;
            
        array<IEntity> activeCaches = m_mPhaseToActiveCaches.Get(phase);
        
        if (activeCaches.IsEmpty())
        {
            Print(string.Format("CRF_InsurgencyGamemodeManager: Phase %1 complete! All caches destroyed.", phase), LogLevel.NORMAL);
            
            int res = ActivateNextPhase(phase + 1);
            
            // FIX: Removed CheckGameEndConditions() call (was dead code) - win condition handled here directly
			if (res == -1)
			{
				CRF_RplBroadcastManager broadcastMgr = GetBroadcastManager();
				if (broadcastMgr)
                    broadcastMgr.PopUpNotification(15, string.Format("Phase %1 complete! All caches destroyed. Attackers win!", phase));
			}
        }
		else
		{
			int cachesDestroyed = m_mPhaseToDestroyedCaches.Get(phase).Count();
			int cacheTotal = activeCaches.Count() + cachesDestroyed;
			
			CRF_RplBroadcastManager broadcastMgr = GetBroadcastManager();
			if (broadcastMgr)
                broadcastMgr.PopUpNotification(15, string.Format("Phase %1: %2/%3 caches destroyed", phase, cachesDestroyed, cacheTotal));
		}
    }
    
    //------------------------------------------------------------------------------------------------
    protected int ActivateNextPhase(int nextPhase)
    {
        if (!m_mPhaseToActiveCaches.Contains(nextPhase))
        {
            Print(string.Format("CRF_InsurgencyGamemodeManager: No phase %1 configured. Mission complete!", nextPhase), LogLevel.NORMAL);
            return -1;
        }
        
        m_iCurrentPhase = nextPhase;
        
        array<IEntity> nextPhaseCaches = m_mPhaseToActiveCaches.Get(nextPhase);
        
        Print(string.Format("CRF_InsurgencyGamemodeManager: Activating phase %1 with %2 caches", nextPhase, nextPhaseCaches.Count()), LogLevel.NORMAL);
        
        foreach (IEntity cache : nextPhaseCaches)
        {
            CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(cache.FindComponent(CRF_InsDestructiveComponent));
            if (cacheComp)
                cacheComp.SetCacheActive(true);
        }
        
        if (m_iPhaseBufferMinutes > 0)
        {
            float currentTime = GetGame().GetWorld().GetWorldTime();
            m_iTimePhaseZoneReveals = currentTime + (m_iPhaseBufferMinutes * 60 * 1000);
            m_bZoneRevealNotificationSent = false;
            
            Print(string.Format("CRF_InsurgencyGamemodeManager: Phase %1 zones reveal in %2 min (world time: %3)", 
                nextPhase, m_iPhaseBufferMinutes, m_iTimePhaseZoneReveals), LogLevel.NORMAL);
                
            GetGame().GetCallqueue().Remove(UpdateZoneRevealTimer);
            GetGame().GetCallqueue().CallLater(UpdateZoneRevealTimer, 1000, true);
        }
        else
        {
            m_iTimePhaseZoneReveals = -1;
            m_bZoneRevealNotificationSent = true;
        }
		
		RespawnPlayersForZoneAdvance();
        
        Replication.BumpMe();
		
		CRF_RplBroadcastManager broadcastMgr = GetBroadcastManager();
		if (broadcastMgr)
        {
            if (m_iPhaseBufferMinutes > 0)
            {
                broadcastMgr.PopUpNotification(15, 
                    string.Format("Phase %1 complete! Advancing to Phase %2", nextPhase - 1, nextPhase),
                    string.Format("Search area will be revealed to attackers in %1 minute(s)", m_iPhaseBufferMinutes));
            }
            else
            {
                broadcastMgr.PopUpNotification(15, 
                    string.Format("Phase %1 complete! Advancing to Phase %2", nextPhase - 1, nextPhase),
                    string.Format("Attackers must destroy %1 cache(s)", nextPhaseCaches.Count()));
            }
        }
		
		return 1;
    }
    
    //------------------------------------------------------------------------------------------------
    void UpdateZoneRevealTimer()
    {
        float currentTime = GetGame().GetWorld().GetWorldTime();
        float millis = m_iTimePhaseZoneReveals - currentTime;
        int totalSeconds = (millis * 0.001);
        
        if (totalSeconds <= 0)
        {
            GetGame().GetCallqueue().Remove(UpdateZoneRevealTimer);
            
			CRF_RplBroadcastManager broadcastMgr = GetBroadcastManager();
            if (!m_bZoneRevealNotificationSent && broadcastMgr)
            {
                m_bZoneRevealNotificationSent = true;
                
                broadcastMgr.PopUpNotification(15, 
                    string.Format("Phase %1 search area revealed!", m_iCurrentPhase),
                    string.Format("Attackers must destroy %1 cache(s)", GetActiveCacheCountForPhase(m_iCurrentPhase)));
                
                Print(string.Format("CRF_InsurgencyGamemodeManager: Phase %1 zones now revealed", m_iCurrentPhase), LogLevel.NORMAL);
            }
            
            Replication.BumpMe();
        }
    }
	
	//------------------------------------------------------------------------------------------------
	protected void RespawnPlayersForZoneAdvance()
	{
		CRF_RespawnManager respawnMgr = GetRespawnManager();
		if (!respawnMgr)
			return;

		respawnMgr.RespawnSide(m_AttackingSide);
		respawnMgr.RespawnSide(m_DefendingSide);
	}
    
    //------------------------------------------------------------------------------------------------
    bool AreCurrentPhaseZonesRevealed()
    {
        if (m_iTimePhaseZoneReveals <= 0)
            return true;
        
        float currentTime = GetGame().GetWorld().GetWorldTime();
        return currentTime >= m_iTimePhaseZoneReveals;
    }
    
    //------------------------------------------------------------------------------------------------
    string GetTimeUntilZoneReveal()
    {
        if (m_iTimePhaseZoneReveals <= 0)
            return "";
        
        float currentTime = GetGame().GetWorld().GetWorldTime();
        float millis = m_iTimePhaseZoneReveals - currentTime;
        
        if (millis <= 0)
            return "0:00";
        
        int totalSeconds = (millis * 0.001);
        return SCR_FormatHelper.FormatTime(totalSeconds);
    }
    
    //------------------------------------------------------------------------------------------------
    int GetSecondsUntilZoneReveal()
    {
        if (m_iTimePhaseZoneReveals <= 0)
            return 0;
        
        float currentTime = GetGame().GetWorld().GetWorldTime();
        float millis = m_iTimePhaseZoneReveals - currentTime;
        
        if (millis <= 0)
            return 0;
        
        return Math.Ceil(millis * 0.001);
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
    //! Server-only: phase maps are not populated on clients
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
    //! Server-only: phase maps are not populated on clients
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
    //! Server-only: phase maps are not populated on clients
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
    //! Server-only: phase maps are not populated on clients
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
    float GetOverallProgress()
    {
        int total = GetTotalCacheCount();
        if (total == 0)
            return 0;
            
        return (m_aDestroyedCaches.Count() / (float)total) * 100.0;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Server-only: phase maps are not populated on clients
    float GetPhaseProgress(int phase)
    {
        int total = GetTotalCacheCountForPhase(phase);
        if (total == 0)
            return 0;
            
        int destroyed = GetDestroyedCacheCountForPhase(phase);
        return (destroyed / (float)total) * 100.0;
    }
}