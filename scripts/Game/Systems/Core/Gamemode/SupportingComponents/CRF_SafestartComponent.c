class CRF_SafestartComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_SafestartComponent : SCR_BaseGameModeComponent
{
	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_bSafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();

	[RplProp()]
	protected string m_sServerWorldTime;

	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = {};

	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;

	[RplProp()]
	protected bool m_bKillRedundantUnitsBool;

	protected int m_iTimeSafeStartBegan;
	protected int m_iTimeMissionEnds;
	protected int m_iSafeStartTimeRemaining;

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
	static CRF_SafestartComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SafestartComponent.Cast(gameMode.FindComponent(CRF_SafestartComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on in-game post init
		// Is the the right way to do this? WHO KNOWS !
		if (!GetGame().InPlayMode())
			return;

		#ifdef WORKBENCH
		if (Replication.IsServer())
		{
			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			m_Logging = CRF_LoggingServerComponent.Cast(this.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	void WaitTillGameStart()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_GamemodeState.GAME)
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
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		SCR_SortedArray<SCR_Faction> outFaction = new SCR_SortedArray<SCR_Faction>();
		factionManager.GetSortedFactionsList(outFaction);

		if (!outFaction || outFaction.IsEmpty())
			return;

		array<SCR_Faction> outArray = {};
		outFaction.ToArray(outArray);

		m_aPlayedFactionsArray.Clear();
		string bluforString = "#Coal_SS_No_Faction";
		string opforString = "#Coal_SS_No_Faction";
		string indforString = "#Coal_SS_No_Faction";
		string civString = "#Coal_SS_No_Faction";

		foreach (SCR_Faction faction : outArray)
		{
			if (faction.GetPlayerCount() == 0 || (faction.GetFactionKey() != "BLUFOR" && faction.GetFactionKey() != "OPFOR" && faction.GetFactionKey() != "INDFOR" && faction.GetFactionKey() != "CIV"))
				continue;

			m_aPlayedFactionsArray.Insert(faction);

			switch (true) {
				case(!m_bBluforReady && faction.GetFactionKey() == "BLUFOR") : {bluforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bBluforReady && faction.GetFactionKey() == "BLUFOR") : {bluforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bOpforReady && faction.GetFactionKey() == "OPFOR") : {opforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bOpforReady && faction.GetFactionKey() == "OPFOR") : {opforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bIndforReady && faction.GetFactionKey() == "INDFOR") : {indforString = "#Coal_SS_Faction_Not_Ready"; break; };
				case(m_bIndforReady && faction.GetFactionKey() == "INDFOR") : {indforString = "#Coal_SS_Faction_Ready"; break; };
				case(!m_bCivReady && faction.GetFactionKey() == "CIV") : {civString = "#Coal_SS_Faction_Not_Ready"; 			break; };
				case(m_bCivReady && faction.GetFactionKey() == "CIV") : {civString = "#Coal_SS_Faction_Ready"; 			break; };
			};
		};

		m_aFactionsStatusArray = {bluforString, opforString, indforString, civString};
		m_iPlayedFactionsCount = 0;

		foreach (string factionString : m_aFactionsStatusArray)
		{
			if (factionString == "#Coal_SS_No_Faction")
				continue;

			m_iPlayedFactionsCount = m_iPlayedFactionsCount + 1;
		}

		Replication.BumpMe();
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(FactionKey setReady, string playerName, bool adminForced) {
		if (!GetSafestartStatus())
			return;

		// If it's an admin-forced action
		if (adminForced)
		{
			if (m_bAdminForcedReady)
			{
				m_bBluforReady = false;
				m_bOpforReady = false;
				m_bIndforReady = false;
				m_bCivReady = false;
				m_bAdminForcedReady = false;

				m_sMessageContent = string.Format("An Admin (%1) Has Force Unreadied All Sides!", playerName);
				Replication.BumpMe();
				ShowMessage();
				return;
			};

			m_bBluforReady = true;
			m_bOpforReady = true;
			m_bIndforReady = true;
			m_bCivReady = true;
			m_bAdminForcedReady = true;

			m_sMessageContent = string.Format("An Admin (%1) Has Force Readied All Sides!", playerName);
			Replication.BumpMe();
			ShowMessage();
			return;
		}

		if (m_bAdminForcedReady)
			return;

		switch (setReady)
		{
			case("BLUFOR") : {
				m_bBluforReady = !m_bBluforReady;
				if (m_bBluforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Blufor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Blufor - %1", playerName);
				break;
			};
			case("OPFOR") : {
				m_bOpforReady = !m_bOpforReady;
				if (m_bOpforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Opfor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Opfor - %1", playerName);
				break;
			};
			case("INDFOR") : {
				m_bIndforReady = !m_bIndforReady;
				if (m_bIndforReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Indfor - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Indfor - %1", playerName);
				break;
			};
			case("CIV") : {
				m_bCivReady = !m_bCivReady;
				if (m_bCivReady)
					m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Civ - %1", playerName);
				else
					m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Civ - %1", playerName);
				break;
			};
		};
		Replication.BumpMe();
		ShowMessage();
	}

	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCountDown()
	{
		int factionsReadyCount = 0;
		foreach (string factionCheckReadyString : m_aFactionsStatusArray)
		{
			if (factionCheckReadyString != "#Coal_SS_Faction_Ready")
				continue;
			factionsReadyCount = factionsReadyCount + 1;
		};

		if (factionsReadyCount == 0 && m_iPlayedFactionsCount == 0 || factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining == 35)
			return;

		if (factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining != 35)
		{
			m_sMessageContent = "#Coal_SS_Countdown_Cancelled";
			Replication.BumpMe();
			m_iSafeStartTimeRemaining = 35;
			return;
		};

		if (factionsReadyCount == m_iPlayedFactionsCount)
		{
			m_iSafeStartTimeRemaining = m_iSafeStartTimeRemaining - 5;
			m_sMessageContent = string.Format("#Coal_SS_Countdown_Started %1 Seconds!", m_iSafeStartTimeRemaining);
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_sMessageContent = "#Coal_SS_Game_Live";
			};
		};
		Replication.BumpMe();
		ShowMessage();
	};

	//------------------------------------------------------------------------------------------------
	// SafeStart functions
	//------------------------------------------------------------------------------------------------
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}

	//------------------------------------------------------------------------------------------------
	bool GetSafestartStatus()
	{
		return m_bSafeStartEnabled;
	}

	//------------------------------------------------------------------------------------------------
	void OnSafeStartChange()
	{
		m_OnSafeStartChange.Invoke(m_bSafeStartEnabled);
	}

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

			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			GetGame().GetCallqueue().Remove(CheckPlayersAlive);

			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 1000, true);
			GetGame().GetCallqueue().CallLater(ActivateSafeStartEHs, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdatePlayedFactions, 1000, true);

			Replication.BumpMe();//Broadcast m_bSafeStartEnabled change

		} else { // Turn off safestart
			if (!m_bSafeStartEnabled)
				return;

			UpdatePlayedFactions();

			DeleteEmptySlots();
			m_bKillRedundantUnitsBool = true;
			m_bAdminForcedReady = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			m_bCivReady = false;

			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(UpdateServerWorldTime);
			GetGame().GetCallqueue().Remove(ActivateSafeStartEHs);
			GetGame().GetCallqueue().Remove(UpdatePlayedFactions);

			GetGame().GetCallqueue().CallLater(CheckPlayersAlive, 5000, true);

			if (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (CRF_Gamemode.GetInstance().m_iTimeLimitMinutes * 60000);
				GetGame().GetCallqueue().CallLater(UpdateMissionEndTimer, 1000, true);
			} else {
				m_sServerWorldTime = "N/A";
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

	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		Replication.BumpMe();
	};

	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		};

		Replication.BumpMe();
	};

	// Why are these two methods done this way? It should just be one wtf
	//------------------------------------------------------------------------------------------------
	void DeleteEmptySlots()
	{
		if (CRF_Gamemode.GetInstance().m_bDeleteJIPSlots)
			if (m_bSafeStartEnabled)
				GetGame().GetCallqueue().CallLater(DeleteEmptySlotsSlowly, 125, true);
	}

	//------------------------------------------------------------------------------------------------
	void DeleteEmptySlotsSlowly()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode.m_bDeleteJIPSlots && !gamemode.m_bRespawnEnabled)
		{
			for (int i = 0; i < gamemode.m_aEntitySlots.Count(); i++)
			{
				if (gamemode.m_aSlots.Get(i) == 0 || gamemode.m_aSlots.Get(i) == -1)
				{
					//Print("Removing Entity");
					gamemode.RemovePlayableEntity(gamemode.m_aEntitySlots.Get(i));
					return;
				}
			}
			GetGame().GetCallqueue().Remove(DeleteEmptySlotsSlowly);
		}

	}

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
	}

	//------------------------------------------------------------------------------------------------
	// SafeStart EHs
	//------------------------------------------------------------------------------------------------
	protected void ActivateSafeStartEHs()
	{	
		auto aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
		{
			array<AIAgent> aiAgents = {};
			aiWorld.GetAIAgents(aiAgents);
			foreach (AIAgent agent : aiAgents)
			{	
				IEntity controlledEntity = agent.GetControlledEntity();
				if (!controlledEntity)
					continue;
				
				SetSafeStartEHs(controlledEntity);
			}
		}
		
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(outPlayers);

		foreach (int playerID : outPlayers)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			if (!controlledEntity)
				continue;

			SetSafeStartEHs(controlledEntity);
		};
	}

	//------------------------------------------------------------------------------------------------
	protected void DeactivateSafeStartEHs()
	{
		for (int i = 0; i < m_mEntitiesWithEHsMap.Count(); i++)
		{
			IEntity controlledEntity = m_mEntitiesWithEHsMap.GetKey(i);

			if (!controlledEntity)
				continue;

			SCR_CharacterDamageManagerComponent.Cast(controlledEntity.FindComponent(SCR_CharacterDamageManagerComponent)).EnableDamageHandling(true);

			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp)
				continue;

			charComp.SetSafety(false, false);

			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler)
				continue;

			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);

			m_mEntitiesWithEHsMap.Set(controlledEntity, false);
		};
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
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		// Get projectile and delete it
		delete entity;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}
};