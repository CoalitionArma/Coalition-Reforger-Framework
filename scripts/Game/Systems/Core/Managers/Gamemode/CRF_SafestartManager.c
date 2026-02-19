class CRF_SafestartManagerClass : ScriptComponentClass {}

class CRF_SafestartManager : ScriptComponent
{
	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_bSafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();

	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = {};

	[RplProp()]
	protected bool m_bKillRedundantUnitsBool;

	int m_iTimeSafeStartBegan;
	[RplProp()]
	int m_iTimeMissionEnds;
	[RplProp()]
	int m_iSafeStartTimeRemaining;
	
	[RplProp()]
	protected bool m_bCountdownMode = false; // True if using time limit countdown instead of ready-up countdown
	
	protected int m_iStoredTimeBeforeReadyUp = 0; // Stores the time remaining before all sides readied up

	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	protected bool m_bCivReady = false;

	protected bool m_bAdminForcedReady = false;

	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity, bool> m_mEntitiesWithEHsMap = new map<IEntity, bool>();
	protected ref array<IEntity> m_aSafestartZones = {};

	protected SCR_PopUpNotification m_PopUpNotification = null;
	protected CRF_LoggingManager m_Logging;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	
	protected static CRF_SafestartManager m_sInstance;
	
	protected bool m_bInitComplete = false;
	protected bool m_bUpdatedServerWorldTime = false;
	protected bool m_bUpdatePlayedFactions = false;
	protected bool m_bActivateSafeStartEHs = false;
	protected bool m_bCheckPlayersAlive = false;
	protected bool m_bUpdateMissionEndTimer = false;
	protected bool m_bCheckStartCountdown = false;
	
	
	void CRF_SafestartManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_SafestartManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	// Init method
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only initialize in actual gameplay
		if (!GetGame().InPlayMode())
			return;
		
		// Get all instances we need for this manager.
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();

		if (RplSession.Mode() != RplMode.Client) // Supports both workbench and dedi
		{
			// Initialize server components
			m_Logging = CRF_LoggingManager.Cast(m_Gamemode.FindComponent(CRF_LoggingManager));
			SetEventMask(owner, EntityEvent.FIXEDFRAME);
		}
	}
	
	float m_fUpdateBuffer = 0;
	float m_fMediumUpdateBuffer = 0;
	float m_fLongUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		
		if (m_fUpdateBuffer >= 1)
		{
			if (!m_bInitComplete)
				if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
				{
					m_bSafeStartEnabled = !m_Gamemode.m_bSafestartInstantlyEnabled;
					Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
			
					GetGame().GetCallqueue().CallLater(ToggleSafeStartServer, 1000, false, m_Gamemode.m_bSafestartInstantlyEnabled);
					m_bInitComplete = true;
				}
			
			if (m_bUpdatedServerWorldTime)
				m_GamemodeManager.UpdateServerWorldTime();
			
			if (m_bUpdateMissionEndTimer)
				m_GamemodeManager.UpdateMissionEndTimer();
			
			// Handle countdown mode (time limit based) - Update every second
			if (m_bCountdownMode && m_bSafeStartEnabled)
				CheckCountdownMode();
			
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		
		if (m_fMediumUpdateBuffer >= 5)
		{
			// Handle traditional ready-up countdown - Keep at 5 second intervals
			if (!m_bCountdownMode && m_bCheckStartCountdown)
				if(FactionsReadyCount() != 0 && m_iPlayedFactionsCount != 0 && FactionsReadyCount() == m_iPlayedFactionsCount)
					CheckStartCountDown();
			m_fMediumUpdateBuffer = 0;
		}
		m_fMediumUpdateBuffer += timeSlice;
		
		if (m_fLongUpdateBuffer >= 10)
		{
			if (m_bActivateSafeStartEHs)
			{
				SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
				ActivateSafeStartEHs(aiWorld);
			}
			
			if (m_bUpdatePlayedFactions)
				UpdatePlayedFactions();
			
			if (m_bCheckPlayersAlive)
				CheckPlayersAlive(CRF_Gamemode.GetInstance());
			m_fLongUpdateBuffer = 0;	
		}
		m_fLongUpdateBuffer += timeSlice;
	}

	//------------------------------------------------------------------------------------------------
	// Ready Up functions
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	TStringArray GetWhosReady() {
		return m_aFactionsStatusArray;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayedFactions()
	{
		// Get faction manager and retrieve all factions
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;

		// Get sorted factions
		SCR_SortedArray<SCR_Faction> sortedFactions = new SCR_SortedArray<SCR_Faction>();
		factionManager.GetSortedFactionsList(sortedFactions);

		if (!sortedFactions || sortedFactions.IsEmpty())
			return;

		// Convert to regular array for iteration
		array<SCR_Faction> factionArray = {};
		sortedFactions.ToArray(factionArray);

		// Reset faction tracking
		m_aPlayedFactionsArray.Clear();

		// Initialize default faction status strings
		string bluforStatus = "N/A";
		string opforStatus = "N/A";
		string indforStatus = "N/A";
		string civStatus = "N/A";

		// Process each faction
		foreach (SCR_Faction faction : factionArray)
		{
			// Skip factions with no players or not matching our supported faction keys
			string factionKey = faction.GetFactionKey();
			if (faction.GetPlayerCount() == 0 || (factionKey != "BLUFOR" && factionKey != "OPFOR" &&
				factionKey != "INDFOR" && factionKey != "CIV"))
				continue;

			// Add to active factions list
			m_aPlayedFactionsArray.Insert(faction);

			// Set appropriate status string based on faction and ready state
			if (factionKey == "BLUFOR") {
				if (m_bBluforReady) {
					bluforStatus = "Ready";
				} else {
					bluforStatus = "Not Ready";
				}
			} else if (factionKey == "OPFOR") {
				if (m_bOpforReady) {
					opforStatus = "Ready";
				} else {
					opforStatus = "Not Ready";
				}
			} else if (factionKey == "INDFOR") {
				if (m_bIndforReady) {
					indforStatus = "Ready";
				} else {
					indforStatus = "Not Ready";
				}
			} else if (factionKey == "CIV") {
				if (m_bCivReady) {
					civStatus = "Ready";
				} else {
					civStatus = "Not Ready";
				}
			}
		}

		// Update faction status array
		m_aFactionsStatusArray = {bluforStatus, opforStatus, indforStatus, civStatus};

		// Count active factions
		m_iPlayedFactionsCount = 0;
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus != "N/A")
				m_iPlayedFactionsCount++;
		}

		// Notify clients of changes
		Replication.BumpMe();
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(FactionKey setReady, string playerName, bool adminForced) {
		if (!GetSafestartStatus())
			return;
		
		string message;

		// Handle admin force ready/unready all factions
		if (adminForced) {
			bool newReadyState = !m_bAdminForcedReady;
			m_bAdminForcedReady = newReadyState;

			// Set all factions to the same ready state
			m_bBluforReady = newReadyState;
			m_bOpforReady = newReadyState;
			m_bIndforReady = newReadyState;
			m_bCivReady = newReadyState;

			string actionText;
			if (newReadyState) {
				actionText = "Force Readied";
			} else {
				actionText = "Force Unreadied";
			}

			message = string.Format("An Admin (%1) Has %2 All Sides!", playerName, actionText);

			Replication.BumpMe();
			m_RplBroadcastManager.PopUpNotification(3.25, message);
			
			UpdatePlayedFactions();
			m_bCheckStartCountdown = true;
			return;
		}

		// If admin forced ready is active, don't allow individual faction changes
		if (m_bAdminForcedReady)
			return;

		// Toggle faction ready status
		bool newStatus = false;
		string messageKey = "";

		// Update faction status and prepare message
		switch (setReady) {
			case "BLUFOR": {
				m_bBluforReady = !m_bBluforReady;
				newStatus = m_bBluforReady;
				if (newStatus) {
					messageKey = "[CRF] : BLUFOR READY";
				} else {
					messageKey = "[CRF] : BLUFOR NOT READY";
				}
				break;
			}
			case "OPFOR": {
				m_bOpforReady = !m_bOpforReady;
				newStatus = m_bOpforReady;
				if (newStatus) {
					messageKey = "[CRF] : OPFOR READY";
				} else {
					messageKey = "[CRF] : OPFOR NOT READY";
				}
				break;
			}
			case "INDFOR": {
				m_bIndforReady = !m_bIndforReady;
				newStatus = m_bIndforReady;
				if (newStatus) {
					messageKey = "[CRF] : INDFOR READY";
				} else {
					messageKey = "[CRF] : INDFOR NOT READY";
				}
				break;
			}
			case "CIV": {
				m_bCivReady = !m_bCivReady;
				newStatus = m_bCivReady;
				if (newStatus) {
					messageKey = "[CRF] : CIV READY";
				} else {
					messageKey = "[CRF] : CIV NOT READY";
				}
				break;
			}
		}

		message = string.Format("%1 - %2", messageKey, playerName);
		
		m_RplBroadcastManager.PopUpNotification(3.25, message, "Any leader can toggle ready to cancel the countdown");
		
		UpdatePlayedFactions();
		m_bCheckStartCountdown = true;
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCountDown()
	{
		string message;
		string submessage = "Any leader can toggle ready to cancel the countdown";
		float popupLife = 3.25;
		
		// Count how many factions are ready
		int readyFactionsCount = FactionsReadyCount();

		// Exit if no factions playing or not all factions ready at initial countdown time
		if ((readyFactionsCount == 0 && m_iPlayedFactionsCount == 0) ||
			(readyFactionsCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining == 35))
			return;

		// Cancel countdown if a faction unreadied after countdown began
		if (readyFactionsCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining != 35)
		{
			message = "[CRF] : Game Live Countdown Canceled!";
			m_iSafeStartTimeRemaining = 35;
			m_bCheckStartCountdown = false;
			m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
			return;
		}

		// Process countdown if all factions are ready
		if (readyFactionsCount == m_iPlayedFactionsCount)
		{
			// Prevent going below zero
			if (m_iSafeStartTimeRemaining > 0)
				m_iSafeStartTimeRemaining -= 5;
			
			// Clamp to zero just in case
			if (m_iSafeStartTimeRemaining < 0)
				m_iSafeStartTimeRemaining = 0;
			
			message = string.Format("[CRF] : Game Live In: %1 Seconds!", m_iSafeStartTimeRemaining);

			// End safe start when countdown reaches zero
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_bCheckStartCountdown = false;
				message = "[CRF] : GAME LIVE!";
				submessage = "Safestart has ended, weapons are now live!";
				popupLife = 8;
			}

			m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
		}
	};
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckCountdownMode()
	{
		string message;
		string submessage = "";
		float popupLife = 3.25;
		bool showMessage = false; // Only show messages at specific intervals
		
		// Check if all factions are ready - if so, skip to 30 second countdown
		int readyFactionsCount = FactionsReadyCount();
		if (readyFactionsCount != 0 && m_iPlayedFactionsCount != 0 && readyFactionsCount == m_iPlayedFactionsCount)
		{
			// All factions are ready - jump to 30 second countdown if we're above that
			if (m_iSafeStartTimeRemaining > 30)
			{
				// Store the current time before jumping to 30 seconds
				m_iStoredTimeBeforeReadyUp = m_iSafeStartTimeRemaining;
				
				m_iSafeStartTimeRemaining = 30;
				message = "[CRF] : All Factions Ready! Game Live In: 30 Seconds!";
				submessage = "Any leader can toggle ready to cancel the countdown";
				popupLife = 5;
				showMessage = true;
				Replication.BumpMe();
				
				// Send notification and continue countdown
				if (showMessage)
					m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
				return;
			}
		}
		else if (m_iSafeStartTimeRemaining <= 30 && m_iStoredTimeBeforeReadyUp > 0)
		{
			// A faction unreadied during the 30 second countdown - restore the original time
			message = "[CRF] : Game Live Countdown Canceled!";
			submessage = "A faction has unreadied";
			popupLife = 5;
			showMessage = true;
			
			// Restore to the time we had before all sides readied up
			m_iSafeStartTimeRemaining = m_iStoredTimeBeforeReadyUp;
			m_iStoredTimeBeforeReadyUp = 0; // Reset the stored time
			Replication.BumpMe();
			
			if (showMessage)
				m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
			return;
		}
		
		// Countdown from time limit (every second) - only if above zero
		if (m_iSafeStartTimeRemaining > 0)
			m_iSafeStartTimeRemaining -= 1;
		
		// Clamp to zero to prevent negatives
		if (m_iSafeStartTimeRemaining < 0)
			m_iSafeStartTimeRemaining = 0;
		
		// Replicate the updated time to all clients
		Replication.BumpMe();
		
		// Only show popup warnings when 5 minutes or less remain
		if (m_iSafeStartTimeRemaining <= 300)
		{
			// Format time remaining as MM:SS
			int minutesRemaining = m_iSafeStartTimeRemaining / 60;
			int secondsRemaining = m_iSafeStartTimeRemaining % 60;
			string timeString;
			
			if (minutesRemaining > 0)
				timeString = string.Format("%1:%2", minutesRemaining, secondsRemaining.ToString(2)); // 2 digits for seconds
			else
				timeString = string.Format("%1 Seconds", secondsRemaining);
			
			message = string.Format("[CRF] : Safestart Ends In: %1", timeString);
			
			// Give warnings at specific intervals (5 minutes or less)
			if (m_iSafeStartTimeRemaining == 300) // 5 minutes
			{
				submessage = "5 minutes remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 240) // 4 minutes
			{
				submessage = "4 minutes remaining";
				popupLife = 4;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 180) // 3 minutes
			{
				submessage = "3 minutes remaining";
				popupLife = 4;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 120) // 2 minutes
			{
				submessage = "2 minutes remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 60) // 1 minute
			{
				submessage = "1 minute remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining == 30) // 30 seconds
			{
				submessage = "30 seconds remaining";
				popupLife = 5;
				showMessage = true;
			}
			else if (m_iSafeStartTimeRemaining <= 10 && m_iSafeStartTimeRemaining > 0) // Final 10 seconds
			{
				submessage = "Get ready!";
				popupLife = 1;
				showMessage = true;
			}
		}
		
		// End safe start when countdown reaches zero
		if (m_iSafeStartTimeRemaining <= 0)
		{
			// Force all sides to be ready
			m_bBluforReady = true;
			m_bOpforReady = true;
			m_bIndforReady = true;
			m_bCivReady = true;
			m_bAdminForcedReady = true;
			UpdatePlayedFactions();
			
			ToggleSafeStartServer(false);
			m_bCountdownMode = false;
			message = "[CRF] : GAME LIVE!";
			submessage = "Safestart timer expired, mission is now live!";
			popupLife = 8;
			showMessage = true;
		}
		
		// Only send popup notification if we have something to show
		if (showMessage)
			m_RplBroadcastManager.PopUpNotification(popupLife, message, submessage);
	}
	
	//------------------------------------------------------------------------------------------------
	protected int FactionsReadyCount()
	{
		int readyFactionsCount = 0;
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus == "Ready")
				readyFactionsCount++;
		}
		
		return readyFactionsCount;
	}
	

	//------------------------------------------------------------------------------------------------
	bool GetSafestartStatus()
	{
		return m_bSafeStartEnabled;
	};
	
	//------------------------------------------------------------------------------------------------
	bool GetCountdownMode()
	{
		return m_bCountdownMode;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSafeStartTimeRemaining()
	{
		return m_iSafeStartTimeRemaining;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetFormattedSafeStartTimeRemaining()
	{
		if (m_iSafeStartTimeRemaining <= 0)
			return "00:00";
			
		int minutes = m_iSafeStartTimeRemaining / 60;
		int seconds = m_iSafeStartTimeRemaining % 60;
		
		return string.Format("%1:%2", minutes.ToString(2), seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	void OnSafeStartChange()
	{
		m_OnSafeStartChange.Invoke(m_bSafeStartEnabled);
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void ToggleSafeStartServer(bool status)
	{
		if (status)
		{ // Turn on safestart
			if (m_bSafeStartEnabled)
				return;

			m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_bSafeStartEnabled = true;
			
			// Check if using countdown mode (boolean enabled AND time limit > 0)
			if (m_Gamemode.m_bUseSafestartTimeLimit && m_Gamemode.m_iSafestartTimeLimit > 0)
			{
				m_bCountdownMode = true;
				m_iSafeStartTimeRemaining = m_Gamemode.m_iSafestartTimeLimit * 60; // Convert minutes to seconds
			}
			else
			{
				m_bCountdownMode = false;
				m_iSafeStartTimeRemaining = 35; // Default ready-up countdown
			}

			m_bUpdateMissionEndTimer = false;
			m_bCheckPlayersAlive = false;

			m_bUpdatedServerWorldTime = true;
			
			SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
			ActivateSafeStartEHs(aiWorld);
			m_bActivateSafeStartEHs = true;
			
			UpdatePlayedFactions();
			m_bUpdatePlayedFactions = true;

			Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		} else { // Turn off safestart
			if (!m_bSafeStartEnabled)
				return;

			UpdatePlayedFactions();

			m_bKillRedundantUnitsBool = true;
			m_bAdminForcedReady = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			m_bCivReady = false;
			
			if(m_Gamemode.m_bLockUnusedSlots)
				m_SlottingManager.LockAllOpenSlots();

			m_bUpdatedServerWorldTime = false;
			m_bActivateSafeStartEHs = false;
			m_bUpdatePlayedFactions = false;
			
			CRF_Gamemode gm = CRF_Gamemode.GetInstance();
			m_bCheckPlayersAlive = true;

			if (m_Gamemode.m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (m_Gamemode.m_iTimeLimitMinutes * 60000);
				m_bUpdateMissionEndTimer = true;
			} else {
				m_GamemodeManager.SetServerWorldTime("N/A");
			};

			Replication.BumpMe();//Broadcast change

			DeactivateSafeStartEHs();

			// Send notification message
			if (m_Logging)
				m_Logging.GameStarted();

			// Use CallLater to delay the call for the removal of EHs so the changes so m_bSafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 1500);
			
			// Delete Temp Group Spawn Points
			GetGame().GetCallqueue().CallLater(DeleteTempGroupSpawnPoints, 8500);
			
			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 12500);

			// Delay the change being broadcasted
			GetGame().GetCallqueue().CallLater(DelayChangeSafeStartDisabled, 250);
			
			DeleteAllSafestartZones();
			CRF_GamemodeManager.GetInstance().DeleteAllForwardDeployZones();
		}
	};
	
	void DeleteTempGroupSpawnPoints()
	{
		array<IEntity> tempSpawns = CRF_RespawnManager.GetInstance().GetTempGroupSpawnPoints();
	
		foreach (IEntity tempSpawn : tempSpawns)
			SCR_EntityHelper.DeleteEntityAndChildren(tempSpawn);
		
		CRF_RespawnManager.GetInstance().ClearTempGroupSpawnPoints();
	}
	
	//------------------------------------------------------------------------------------------------
	void AddSafestartZone(IEntity entity)
	{
		m_aSafestartZones.Insert(entity);
	}

	//------------------------------------------------------------------------------------------------
	void DelayChangeSafeStartDisabled() {
		m_bSafeStartEnabled = false;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
	};

	//------------------------------------------------------------------------------------------------
	void CheckPlayersAlive(CRF_Gamemode gm)
	{
		string message;
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		foreach (SCR_Faction faction : m_aPlayedFactionsArray)
		{
			FactionKey factionKey = faction.GetFactionKey();
			int factionTickets;
			if (respawnManager) {
				factionTickets = respawnManager.GetFactionTickets(factionKey);
			} else {
				factionTickets = 0;
			}
			
			switch (true) {
				case(factionKey == "BLUFOR" && faction.GetPlayerCount() == 0 && factionTickets <= 0 && m_aFactionsStatusArray[0] != "N/A") : { message = "All Blufor Players Have Been Eliminated!"; GetGame().GetCallqueue().Remove(CheckPlayersAlive); break; };
				case(factionKey == "OPFOR" && faction.GetPlayerCount() == 0 && factionTickets <= 0 && m_aFactionsStatusArray[1] != "N/A") : { message = "All Opfor Players Have Been Eliminated!"; GetGame().GetCallqueue().Remove(CheckPlayersAlive); break; };
				case(factionKey == "INDFOR" && faction.GetPlayerCount() == 0 && factionTickets <= 0 && m_aFactionsStatusArray[2] != "N/A") : { message = "All Indfor Players Have Been Eliminated!"; GetGame().GetCallqueue().Remove(CheckPlayersAlive); break; };
				case(factionKey == "CIV" && faction.GetPlayerCount() == 0 && factionTickets <= 0 && m_aFactionsStatusArray[3] != "N/A") : { message = "All Civilian Players Have Been Eliminated!"; GetGame().GetCallqueue().Remove(CheckPlayersAlive); break; };
			};
		};

		if (!message.IsEmpty())
			m_RplBroadcastManager.PopUpNotification(20, message);
	};
	
	//------------------------------------------------------------------------------------------------
	void DeleteAllSafestartZones()
	{
		foreach(IEntity zone : m_aSafestartZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aSafestartZones.Clear()
	}

	//------------------------------------------------------------------------------------------------
	// SafeStart EHs
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	/**
	* Activates safe start event handlers for all AI and player-controlled entities.
	* Disables damage and weapon functionality during the safe start period.
	*/
	protected void ActivateSafeStartEHs(SCR_AIWorld aiWorld)
	{
		// Apply safe start to AI-controlled entities
		if (aiWorld)
		{
			array<AIAgent> aiAgents = {};
			aiWorld.GetAIAgents(aiAgents);

			foreach (AIAgent agent : aiAgents)
			{
				IEntity controlledEntity = agent.GetControlledEntity();
				if (controlledEntity)
					SetSafeStartEHs(controlledEntity);
			}
		}

		// Apply safe start to player-controlled entities
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (controlledEntity)
				SetSafeStartEHs(controlledEntity);
		}
	};

	//------------------------------------------------------------------------------------------------
	/**
	* Deactivates all safe start event handlers and re-enables combat functionality
	* for all entities that had safe start restrictions applied.
	*/
	protected void DeactivateSafeStartEHs()
	{
		foreach (IEntity controlledEntity, bool hasHandlers : m_mEntitiesWithEHsMap)
		{
			if (!controlledEntity)
				continue;

			// Re-enable damage handling
			SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(
				controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
			if (damageManager)
				damageManager.EnableDamageHandling(true);

			// Turn off weapon safety
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(
				controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp)
				continue;

			charComp.SetSafety(false, false);

			// Remove weapon event handlers
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(
				controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler)
				continue;

			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			
			CRF_PolyZoneEffectHandler polyZoneEffectHandler = CRF_PolyZoneEffectHandler.Cast(controlledEntity.FindComponent(CRF_PolyZoneEffectHandler));
			polyZoneEffectHandler.ClearAllEffects();

			m_mEntitiesWithEHsMap.Set(controlledEntity, false);
		}
	};

	//------------------------------------------------------------------------------------------------
	protected void SetSafeStartEHs(IEntity controlledEntity)
	{
		SCR_CharacterDamageManagerComponent damageHandler = SCR_CharacterDamageManagerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damageHandler)
			damageHandler.EnableDamageHandling(false);

		EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));

		bool alreadyHasEventHandlers = m_mEntitiesWithEHsMap.Get(controlledEntity);

		if (!alreadyHasEventHandlers && charComp && eventHandler) {
			charComp.SetSafety(true, true);
			eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			m_mEntitiesWithEHsMap.Set(controlledEntity, true);
		};
	};

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired(int playerId, BaseWeaponComponent weapon, IEntity entity)
	{
		// Get projectile and delete it
		delete entity;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerId, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}
}
