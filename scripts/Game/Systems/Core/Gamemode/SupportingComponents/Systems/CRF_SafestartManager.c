class CRF_SafestartManagerClass : ScriptComponentClass {}

class CRF_SafestartManager : ScriptComponent
{
	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_bSafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();

	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = {};

	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;

	[RplProp()]
	protected bool m_bKillRedundantUnitsBool;

	int m_iTimeSafeStartBegan;
	int m_iTimeMissionEnds;
	int m_iSafeStartTimeRemaining;

	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	protected bool m_bCivReady = false;

	protected bool m_bAdminForcedReady = false;

	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity, bool> m_mEntitiesWithEHsMap = new map<IEntity, bool>();

	protected SCR_PopUpNotification m_PopUpNotification = null;
	CRF_LoggingServerComponent m_Logging;

	//------------------------------------------------------------------------------------------------
	static CRF_SafestartManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SafestartManager.Cast(gameMode.FindComponent(CRF_SafestartManager));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	// Init method
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only initialize in actual gameplay
		if (!GetGame().InPlayMode())
			return;

		// Check if we're running on a server
		bool isServer;

		#ifdef WORKBENCH
		isServer = Replication.IsServer();
		#else
		isServer = RplSession.Mode() == RplMode.Dedicated;
		#endif

		if (isServer) // Supports both workbench and dedi
		{
			// Initialize server components
			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Polls for game start, then configures safestart based on gamemode settings
	void WaitTillGameStart()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
			return;

		m_bSafeStartEnabled = !CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		GetGame().GetCallqueue().Remove(WaitTillGameStart);
		GetGame().GetCallqueue().CallLater(ToggleSafeStartServer, 1000, false, CRF_Gamemode.GetInstance().m_bSafestartInstantlyEnabled);
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
		string bluforStatus = "#Coal_SS_No_Faction";
		string opforStatus = "#Coal_SS_No_Faction";
		string indforStatus = "#Coal_SS_No_Faction";
		string civStatus = "#Coal_SS_No_Faction";

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
					bluforStatus = "#Coal_SS_Faction_Ready";
				} else {
					bluforStatus = "#Coal_SS_Faction_Not_Ready";
				}
			} else if (factionKey == "OPFOR") {
				if (m_bOpforReady) {
					opforStatus = "#Coal_SS_Faction_Ready";
				} else {
					opforStatus = "#Coal_SS_Faction_Not_Ready";
				}
			} else if (factionKey == "INDFOR") {
				if (m_bIndforReady) {
					indforStatus = "#Coal_SS_Faction_Ready";
				} else {
					indforStatus = "#Coal_SS_Faction_Not_Ready";
				}
			} else if (factionKey == "CIV") {
				if (m_bCivReady) {
					civStatus = "#Coal_SS_Faction_Ready";
				} else {
					civStatus = "#Coal_SS_Faction_Not_Ready";
				}
			}
		}

		// Update faction status array
		m_aFactionsStatusArray = {bluforStatus, opforStatus, indforStatus, civStatus};

		// Count active factions
		m_iPlayedFactionsCount = 0;
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus != "#Coal_SS_No_Faction")
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

			m_sMessageContent = string.Format("An Admin (%1) Has %2 All Sides!", playerName, actionText);

			Replication.BumpMe();
			ShowMessage();
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
					messageKey = "#Coal_SS_Faction_Readied_Blufor";
				} else {
					messageKey = "#Coal_SS_Faction_Unreadied_Blufor";
				}
				break;
			}
			case "OPFOR": {
				m_bOpforReady = !m_bOpforReady;
				newStatus = m_bOpforReady;
				if (newStatus) {
					messageKey = "#Coal_SS_Faction_Readied_Opfor";
				} else {
					messageKey = "#Coal_SS_Faction_Unreadied_Opfor";
				}
				break;
			}
			case "INDFOR": {
				m_bIndforReady = !m_bIndforReady;
				newStatus = m_bIndforReady;
				if (newStatus) {
					messageKey = "#Coal_SS_Faction_Readied_Indfor";
				} else {
					messageKey = "#Coal_SS_Faction_Unreadied_Indfor";
				}
				break;
			}
			case "CIV": {
				m_bCivReady = !m_bCivReady;
				newStatus = m_bCivReady;
				if (newStatus) {
					messageKey = "#Coal_SS_Faction_Readied_Civ";
				} else {
					messageKey = "#Coal_SS_Faction_Unreadied_Civ";
				}
				break;
			}
		}

		m_sMessageContent = string.Format("%1 - %2", messageKey, playerName);
		Replication.BumpMe();
		ShowMessage();
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCountDown()
	{
		// Count how many factions are ready
		int readyFactionsCount = 0;
		foreach (string factionStatus : m_aFactionsStatusArray)
		{
			if (factionStatus == "#Coal_SS_Faction_Ready")
				readyFactionsCount++;
		}

		// Exit if no factions playing or not all factions ready at initial countdown time
		if ((readyFactionsCount == 0 && m_iPlayedFactionsCount == 0) ||
			(readyFactionsCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining == 35))
			return;

		// Cancel countdown if a faction unreadied after countdown began
		if (readyFactionsCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining != 35)
		{
			m_sMessageContent = "#Coal_SS_Countdown_Cancelled";
			Replication.BumpMe();
			m_iSafeStartTimeRemaining = 35;
			ShowMessage();
			return;
		}

		// Process countdown if all factions are ready
		if (readyFactionsCount == m_iPlayedFactionsCount)
		{
			m_iSafeStartTimeRemaining -= 5;
			m_sMessageContent = string.Format("#Coal_SS_Countdown_Started %1 Seconds!", m_iSafeStartTimeRemaining);

			// End safe start when countdown reaches zero
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_sMessageContent = "#Coal_SS_Game_Live";
			}

			Replication.BumpMe();
			ShowMessage();
		}
	};

	//------------------------------------------------------------------------------------------------
	bool GetSafestartStatus()
	{
		return m_bSafeStartEnabled;
	};

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
			m_iSafeStartTimeRemaining = 35;

			GetGame().GetCallqueue().Remove(CRF_GamemodeManager.GetInstance().UpdateMissionEndTimer);
			GetGame().GetCallqueue().Remove(CheckPlayersAlive);

			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(CRF_GamemodeManager.GetInstance().UpdateServerWorldTime, 1000, true);
			GetGame().GetCallqueue().CallLater(ActivateSafeStartEHs, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdatePlayedFactions, 1000, true);

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
			
			CRF_SlottingManager.GetInstance().LockAllOpenSlots();

			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(CRF_GamemodeManager.GetInstance().UpdateServerWorldTime);
			GetGame().GetCallqueue().Remove(ActivateSafeStartEHs);
			GetGame().GetCallqueue().Remove(UpdatePlayedFactions);

			GetGame().GetCallqueue().CallLater(CheckPlayersAlive, 5000, true);

			if (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes * 60000);
				GetGame().GetCallqueue().CallLater(CRF_GamemodeManager.GetInstance().UpdateMissionEndTimer, 1000, true);
			} else {
				CRF_GamemodeManager.GetInstance().SetServerWorldTime("N/A");
			};

			Replication.BumpMe();//Broadcast change

			DeactivateSafeStartEHs();

			// Send notification message
			if (m_Logging)
				m_Logging.GameStarted();

			// Use CallLater to delay the call for the removal of EHs so the changes so m_bSafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 1500);

			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(DeactivateSafeStartEHs, 12500);

			GetGame().GetCallqueue().CallLater(DelayChangeSafeStartDisabled, 250);

			// Update logging component since game is now live
			CRF_MDB_LoggingServerComponent logCom = CRF_MDB_LoggingServerComponent.GetInstance();
			if (logCom)
			{
				logCom.m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
				SCR_FactionManager scrFM = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				logCom.m_iBluforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("BLUFOR"));
				logCom.m_iOpforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("OPFOR"));
				logCom.m_iIndforCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("INDFOR"));
				logCom.m_iCivCount = scrFM.GetFactionPlayerCount(GetGame().GetFactionManager().GetFactionByKey("CIV"));
			};
		}
	};

	//------------------------------------------------------------------------------------------------
	void DelayChangeSafeStartDisabled() {
		m_bSafeStartEnabled = false;
		Replication.BumpMe();//Broadcast m_bSafeStartEnabled change
	};

	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;

		m_PopUpNotification = SCR_PopUpNotification.GetInstance();

		m_sStoredMessageContent = m_sMessageContent;

		if (m_sMessageContent == "All Blufor Players Have Been Eliminated!" || m_sMessageContent == "All Opfor Players Have Been Eliminated!" || m_sMessageContent == "All Indfor Players Have Been Eliminated!" || m_sMessageContent == "All Civilian Players Have Been Eliminated!")
		{
			m_PopUpNotification.PopupMsg(m_sMessageContent, 20);
			return;
		};

		if (m_sMessageContent == "#Coal_SS_Game_Live")
			m_PopUpNotification.PopupMsg(m_sMessageContent, 8, "#Coal_SS_SafeStart_Started_Subtext");
		else
			m_PopUpNotification.PopupMsg(m_sMessageContent, 2.5, "#Coal_SS_Countdown_Started_Subtext");
	};

	//------------------------------------------------------------------------------------------------
	void CheckPlayersAlive()
	{
		foreach (SCR_Faction faction : m_aPlayedFactionsArray)
		{
			switch (true) {
				case(faction.GetFactionKey() == "BLUFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[0] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Blufor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "OPFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[1] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Opfor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "INDFOR" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[2] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Indfor Players Have Been Eliminated!"; break; };
				case(faction.GetFactionKey() == "CIV" && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[3] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Civilian Players Have Been Eliminated!"; break; };
			};
		};

		Replication.BumpMe();
		ShowMessage();
	};

	//------------------------------------------------------------------------------------------------
	// SafeStart EHs
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	/**
	* Activates safe start event handlers for all AI and player-controlled entities.
	* Disables damage and weapon functionality during the safe start period.
	*/
	protected void ActivateSafeStartEHs()
	{
		// Apply safe start to AI-controlled entities
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
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
