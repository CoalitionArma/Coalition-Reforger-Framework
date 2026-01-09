//------------------------------------------------------------------------------------------------
// CRF Persistence Manager
// Handles mission-wide save/load operations and auto-save functionality
//------------------------------------------------------------------------------------------------
class CRF_PersistenceManagerClass: ScriptComponentClass
{
};

class CRF_PersistenceManager : ScriptComponent
{
	protected static CRF_PersistenceManager s_Instance;
	
	[Attribute("300", UIWidgets.EditBox, "Auto-save interval in seconds (0 = disabled)")]
	protected float m_fAutoSaveInterval;
	
	[Attribute("10", UIWidgets.EditBox, "Maximum number of auto-saves to keep (0 = unlimited)")]
	protected int m_iMaxAutoSaves;
	
	[Attribute("1", UIWidgets.CheckBox, "Enable auto-save on mission start")]
	protected bool m_bAutoSaveOnStart;
	
	[Attribute("1", UIWidgets.CheckBox, "Save on mission end/shutdown")]
	protected bool m_bSaveOnShutdown;
	
	[Attribute("60", UIWidgets.EditBox, "Delay before first auto-save (seconds)")]
	protected float m_fInitialAutoSaveDelay;
	
	protected float m_fTimeSinceLastSave = 0;
	protected bool m_bInitialized = false;
	protected bool m_bFirstAutoSaveDone = false;
	protected int m_iSaveCounter = 0;
	
	//------------------------------------------------------------------------------------------------
	static CRF_PersistenceManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		s_Instance = this;
		
		// Check if persistence is enabled in mission header
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (!missionHeader || missionHeader.m_eSaveTypes == 0)
		{
			Print("[CRF_PersistenceManager] Persistence disabled in mission header", LogLevel.WARNING);
			return;
		}
		
		// Check if SaveGameManager exists
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
		{
			Print("[CRF_PersistenceManager] SaveGameManager not available", LogLevel.ERROR);
			return;
		}
		
		m_bInitialized = true;
		
		// Enable frame updates for auto-save
		if (m_fAutoSaveInterval > 0)
		{
			SetEventMask(owner, EntityEvent.FRAME);
		}
		
		// Hook into game mode events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnGameEnd().Insert(OnGameEnd);
			gameMode.GetOnGameStart().Insert(OnGameStart);
		}
		
		Print(string.Format("[CRF_PersistenceManager] Initialized | Auto-save: %1s | Max saves: %2", 
			m_fAutoSaveInterval, 
			m_iMaxAutoSaves
		), LogLevel.NORMAL);
	}
	
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
		Print("[CRF_PersistenceManager] Game ending", LogLevel.NORMAL);
		
		if (m_bSaveOnShutdown)
		{
			SaveGameManager saveManager = GetGame().GetSaveGameManager();
			if (saveManager && saveManager.IsSavingPossible())
			{
				// Blocking save to ensure it completes before shutdown
				saveManager.RequestSavePoint(
					ESaveGameType.SHUTDOWN,
					"Mission End",
					ESaveGameRequestFlags.BLOCKING,
					new SaveGameOperationCb(ShutdownSaveCallback)
				);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Trigger an auto-save
	void TriggerAutoSave()
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
		{
			Print("[CRF_PersistenceManager] SaveGameManager not available", LogLevel.ERROR);
			return;
		}
		
		if (!saveManager.IsSavingPossible())
		{
			Print("[CRF_PersistenceManager] Saving not possible at this time", LogLevel.WARNING);
			return;
		}
		
		m_iSaveCounter++;
		string saveName = string.Format("Auto Save #%1", m_iSaveCounter);
		
		bool success = saveManager.RequestSavePoint(
			ESaveGameType.AUTO,
			saveName,
			0,
			new SaveGameOperationCb(AutoSaveCallback)
		);
		
		if (success)
		{
			Print(string.Format("[CRF_PersistenceManager] Auto-save triggered: %1", saveName), LogLevel.NORMAL);
		}
		else
		{
			Print("[CRF_PersistenceManager] Failed to trigger auto-save", LogLevel.ERROR);
		}
		
		// Cleanup old auto-saves if limit is set
		if (m_iMaxAutoSaves > 0)
		{
			CleanupOldAutoSaves();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Trigger a manual save (admin/player command)
	void TriggerManualSave(string saveName = "Manual Save")
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
		{
			Print("[CRF_PersistenceManager] SaveGameManager not available", LogLevel.ERROR);
			return;
		}
		
		if (!saveManager.IsSavingPossible())
		{
			Print("[CRF_PersistenceManager] Saving not possible at this time", LogLevel.WARNING);
			NotifyAdmins("Cannot save: Saving not possible at this time");
			return;
		}
		
		bool success = saveManager.RequestSavePoint(
			ESaveGameType.MANUAL,
			saveName,
			0,
			new SaveGameOperationCb(ManualSaveCallback)
		);
		
		if (success)
		{
			Print(string.Format("[CRF_PersistenceManager] Manual save triggered: %1", saveName), LogLevel.NORMAL);
		}
		else
		{
			Print("[CRF_PersistenceManager] Failed to trigger manual save", LogLevel.ERROR);
			NotifyAdmins("Failed to trigger save!");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load the most recent save
	bool LoadLatestSave()
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
		{
			Print("[CRF_PersistenceManager] SaveGameManager not available", LogLevel.ERROR);
			return false;
		}
		
		array<SaveGame> saves = new array<SaveGame>();
		ResourceName currentMission = SaveGameManager.GetCurrentMissionResource();
		
		int saveCount = saveManager.GetSaves(saves, currentMission);
		if (saveCount == 0)
		{
			Print("[CRF_PersistenceManager] No saves found for current mission", LogLevel.WARNING);
			return false;
		}
		
		// Get the most recent save (last in array)
		SaveGame latestSave = saves[saves.Count() - 1];
		
		Print(string.Format("[CRF_PersistenceManager] Loading save: %1 (Created: %2)", 
			latestSave.GetSavePointName(),
			GetSaveTimeString(latestSave)
		), LogLevel.NORMAL);
		
		saveManager.Load(latestSave, true); // true = with transition
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load a specific save by UUID
	bool LoadSaveById(string saveIdString)
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
			return false;
		
		// UUID extends string, so we can directly assign the string value
		UUID saveId = saveIdString;
		if (saveId.IsNull() || !UUID.IsUUID(saveIdString))
		{
			Print(string.Format("[CRF_PersistenceManager] Invalid save ID: %1", saveIdString), LogLevel.ERROR);
			return false;
		}
		
		array<SaveGame> saves = new array<SaveGame>();
		ResourceName currentMission = SaveGameManager.GetCurrentMissionResource();
		saveManager.GetSaves(saves, currentMission);
		
		foreach (SaveGame save : saves)
		{
			if (save.GetId() == saveId)
			{
				Print(string.Format("[CRF_PersistenceManager] Loading save: %1", save.GetSavePointName()), LogLevel.NORMAL);
				saveManager.Load(save, true);
				return true;
			}
		}
		
		Print(string.Format("[CRF_PersistenceManager] Save not found with ID: %1", saveIdString), LogLevel.ERROR);
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load save by playthrough and save point number
	bool LoadSaveByNumber(int playthroughNumber, int savePointNumber)
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
			return false;
		
		array<SaveGame> saves = new array<SaveGame>();
		ResourceName currentMission = SaveGameManager.GetCurrentMissionResource();
		saveManager.GetSaves(saves, currentMission);
		
		foreach (SaveGame save : saves)
		{
			if (save.GetPlaythroughNumber() == playthroughNumber && 
				save.GetSavePointNumber() == savePointNumber)
			{
				Print(string.Format("[CRF_PersistenceManager] Loading save P%1-S%2: %3", 
					playthroughNumber, savePointNumber, save.GetSavePointName()), LogLevel.NORMAL);
				saveManager.Load(save, true);
				return true;
			}
		}
		
		Print(string.Format("[CRF_PersistenceManager] Save not found: P%1-S%2", 
			playthroughNumber, savePointNumber), LogLevel.ERROR);
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! List all available saves for current mission
	void ListSaves()
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
			return;
		
		array<SaveGame> saves = new array<SaveGame>();
		ResourceName currentMission = SaveGameManager.GetCurrentMissionResource();
		
		int saveCount = saveManager.GetSaves(saves, currentMission);
		
		Print(string.Format("[CRF_PersistenceManager] Found %1 saves for current mission:", saveCount), LogLevel.NORMAL);
		
		foreach (SaveGame save : saves)
		{
			Print(string.Format("  - %1 | P%2-S%3 | %4 | ID: %5",
				save.GetSavePointName(),
				save.GetPlaythroughNumber(),
				save.GetSavePointNumber(),
				GetSaveTimeString(save),
				save.GetId()
			), LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Cleanup old auto-saves beyond the limit
	protected void CleanupOldAutoSaves()
	{
		SaveGameManager saveManager = GetGame().GetSaveGameManager();
		if (!saveManager)
			return;
		
		array<SaveGame> saves = new array<SaveGame>();
		ResourceName currentMission = SaveGameManager.GetCurrentMissionResource();
		saveManager.GetSaves(saves, currentMission);
		
		// Filter auto-saves only
		array<SaveGame> autoSaves = new array<SaveGame>();
		foreach (SaveGame save : saves)
		{
			if (save.GetType() == ESaveGameType.AUTO)
				autoSaves.Insert(save);
		}
		
		// Delete oldest auto-saves if we exceed the limit
		int excessCount = autoSaves.Count() - m_iMaxAutoSaves;
		if (excessCount > 0)
		{
			for (int i = 0; i < excessCount; i++)
			{
				SaveGame oldSave = autoSaves[i];
				Print(string.Format("[CRF_PersistenceManager] Deleting old auto-save: %1", 
					oldSave.GetSavePointName()), LogLevel.VERBOSE);
				saveManager.Delete(oldSave);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Callback handlers
	//------------------------------------------------------------------------------------------------
	protected void AutoSaveCallback(bool success, Managed context)
	{
		if (success)
		{
			Print("[CRF_PersistenceManager] Auto-save completed successfully", LogLevel.NORMAL);
		}
		else
		{
			Print("[CRF_PersistenceManager] Auto-save failed", LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ManualSaveCallback(bool success, Managed context)
	{
		if (success)
		{
			Print("[CRF_PersistenceManager] Manual save completed successfully", LogLevel.NORMAL);
			NotifyAdmins("Mission saved successfully!");
		}
		else
		{
			Print("[CRF_PersistenceManager] Manual save failed", LogLevel.ERROR);
			NotifyAdmins("Mission save failed!");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ShutdownSaveCallback(bool success, Managed context)
	{
		if (success)
		{
			Print("[CRF_PersistenceManager] Shutdown save completed successfully", LogLevel.NORMAL);
		}
		else
		{
			Print("[CRF_PersistenceManager] Shutdown save failed", LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------------------------------------------
	protected void NotifyAdmins(string message)
	{
		// Send notification to all admins
		array<int> players = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(players);
		
		foreach (int playerId : players)
		{
			PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!playerController)
				continue;
			
			// Check if player is admin (you'll need to implement this based on your admin system)
			if (IsPlayerAdmin(playerId))
			{
				SCR_HintManagerComponent.GetInstance().ShowCustomHint(message, "Persistence", 5, true, playerId);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsPlayerAdmin(int playerId)
	{
		// Implement your admin check here
		// For now, return true for all players (you can integrate with your admin system)
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected string GetSaveTimeString(SaveGame save)
	{
		// SaveGame doesn't have GetSaveDate method, use alternative approach
		// Return save point name and number instead
		return string.Format("P%1-S%2", 
			save.GetPlaythroughNumber(),
			save.GetSavePointNumber()
		);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		
		if (s_Instance == this)
			s_Instance = null;
		
		// Unhook from game mode events
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
		{
			gameMode.GetOnGameEnd().Remove(OnGameEnd);
			gameMode.GetOnGameStart().Remove(OnGameStart);
		}
	}
}
