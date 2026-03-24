//------------------------------------------------------------------------------------------------
// CRF Persistence Manager
// Handles mission-wide save/load operations and auto-save functionality
//------------------------------------------------------------------------------------------------
class CRF_PersistenceManagerClass: ScriptComponentClass
{
};

class CRF_PersistenceManager : ScriptComponent
{	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================

	[Attribute("300", UIWidgets.EditBox, "Auto-save interval in seconds (0 = disabled)")]
	protected float m_fAutoSaveInterval;
	
	[Attribute("1", UIWidgets.CheckBox, "Enable auto-save on mission start")]
	protected bool m_bAutoSaveOnStart;
	
	[Attribute("1", UIWidgets.CheckBox, "Save on mission end/shutdown")]
	protected bool m_bSaveOnShutdown;
	
	[Attribute("60", UIWidgets.EditBox, "Delay before first auto-save (seconds)")]
	protected float m_fInitialAutoSaveDelay;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected float m_fTimeSinceLastSave = 0;
	protected bool m_bInitialized = false;
	protected bool m_bFirstAutoSaveDone = false;
	protected int m_iSaveCounter = 0;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		// Defer registration until the persistence system is ready
		GetGame().GetCallqueue().Call(DeferredInit, owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void DeferredInit(IEntity owner)
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence)
		{
			Print("[CRF_PersistenceManager] PersistenceSystem not available — persistence disabled", LogLevel.WARNING);
			return;
		}
		
		m_bInitialized = true;
		
		// Subscribe to save/load events for logging
		SubscribeToPersistenceEvents();
		
		// Enable frame updates for auto-save
		if (m_fAutoSaveInterval > 0)
			SetEventMask(owner, EntityEvent.FRAME);
		
		// Hook into game mode events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnGameEnd().Insert(OnGameEnd);
			gameMode.GetOnGameStart().Insert(OnGameStart);
		}
		
		Print(string.Format("[CRF_PersistenceManager] Initialized | Auto-save: %1s", m_fAutoSaveInterval), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void SubscribeToPersistenceEvents()
	{
		SCR_PersistenceSystem sys = SCR_PersistenceSystem.Cast(PersistenceSystem.GetInstance());
		if (!sys)
			return;
		
		sys.GetOnAfterSave().Insert(OnPersistenceAfterSave);
		sys.GetOnStateChanged().Insert(OnPersistenceStateChanged);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPersistenceAfterSave(ESaveGameType saveType, bool success)
	{
		if (success)
			Print(string.Format("[CRF_PersistenceManager] Save completed (type %1)", saveType), LogLevel.NORMAL);
		else
			Print(string.Format("[CRF_PersistenceManager] Save FAILED (type %1)", saveType), LogLevel.ERROR);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPersistenceStateChanged(EPersistenceSystemState oldState, EPersistenceSystemState newState)
	{
		if (newState == EPersistenceSystemState.ACTIVE)
		{
			bool dataLoaded = PersistenceSystem.GetInstance().WasDataLoaded();
			Print(string.Format("[CRF_PersistenceManager] Persistence active — data restored from save: %1", dataLoaded), LogLevel.NORMAL);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ON FRAME METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bInitialized || m_fAutoSaveInterval <= 0)
			return;
		
		m_fTimeSinceLastSave += timeSlice;
		
		// Wait for initial delay before first auto-save
		if (!m_bFirstAutoSaveDone && m_fTimeSinceLastSave < m_fInitialAutoSaveDelay)
			return;
		
		// Trigger auto-save when interval is reached
		if (m_fTimeSinceLastSave >= m_fAutoSaveInterval)
		{
			TriggerAutoSave();
			m_fTimeSinceLastSave = 0;
			m_bFirstAutoSaveDone = true;
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 BASE GAMEMODE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	// Called when game starts
	protected void OnGameStart()
	{
		Print("[CRF_PersistenceManager] Game started", LogLevel.VERBOSE);
		
		// Optional: Create initial save on game start
		if (m_bAutoSaveOnStart)
		{
			GetGame().GetCallqueue().CallLater(TriggerManualSave, 5000, false, "Game Start");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when game ends
	protected void OnGameEnd()
	{
		if (!m_bSaveOnShutdown)
			return;
		
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence || persistence.GetState() != EPersistenceSystemState.ACTIVE)
			return;
		
		persistence.TriggerSave(ESaveGameType.SHUTDOWN);
		Print("[CRF_PersistenceManager] Shutdown save triggered", LogLevel.NORMAL);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SAVE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Trigger an auto-save
	void TriggerAutoSave()
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence || persistence.GetState() != EPersistenceSystemState.ACTIVE)
		{
			Print("[CRF_PersistenceManager] Cannot auto-save: persistence system not active", LogLevel.WARNING);
			return;
		}
		
		m_iSaveCounter++;
		
		if (!persistence.TriggerSave(ESaveGameType.AUTO))
			Print("[CRF_PersistenceManager] Auto-save failed to start", LogLevel.ERROR);
		else
			Print(string.Format("[CRF_PersistenceManager] Auto-save #%1 triggered", m_iSaveCounter), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Trigger a manual save (admin/player command)
	void TriggerManualSave(string saveName = "Manual Save")
	{
		PersistenceSystem persistence = PersistenceSystem.GetInstance();
		if (!persistence || persistence.GetState() != EPersistenceSystemState.ACTIVE)
		{
			Print("[CRF_PersistenceManager] Cannot save: persistence system not active", LogLevel.WARNING);
			return;
		}
		
		if (!persistence.TriggerSave(ESaveGameType.MANUAL))
			Print("[CRF_PersistenceManager] Manual save failed to start", LogLevel.ERROR);
		else
			Print(string.Format("[CRF_PersistenceManager] Manual save triggered: %1", saveName), LogLevel.NORMAL);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 DESTRUCTOR
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		
		if (m_sInstance == this)
			m_sInstance = null;
		
		// Unhook from game mode events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnGameEnd().Remove(OnGameEnd);
			gameMode.GetOnGameStart().Remove(OnGameStart);
		}
		
		// Unsubscribe from persistence events
		SCR_PersistenceSystem sys = SCR_PersistenceSystem.Cast(PersistenceSystem.GetInstance());
		if (sys)
		{
			sys.GetOnAfterSave().Remove(OnPersistenceAfterSave);
			sys.GetOnStateChanged().Remove(OnPersistenceStateChanged);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PersistenceManager m_sInstance;
	void CRF_PersistenceManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PersistenceManager GetInstance()
	{
		return m_sInstance;
	}
}
