[ComponentEditorProps(category: "Safe Start Component", description: "")]
class CRF_GearScriptModeComponentClass: SCR_BaseGameModeComponentClass {}

class CRF_GearScriptModeComponent: SCR_BaseGameModeComponent
{
	[Attribute(defvalue: "BLUFOR")]
	int m_FactionOne;
	[Attribute()]
	static CRF_GearScriptConfig m_BLUFORGearScript;
	
	[Attribute(defvalue: "OPFOR")]
	int m_FactionTwo;
	[Attribute()]
	static CRF_GearScriptConfig m_OPFORGearScript;
	
	[Attribute(defvalue: "INDFOR")]
	int m_FactionThree;
	[Attribute()]
	static CRF_GearScriptConfig m_INDFORGearScript;
	
	[Attribute(defvalue: "CIV")]
	int m_FactionFour;
	[Attribute()]
	static CRF_GearScriptConfig m_CIVGearScript;
	
	static CRF_GearScriptModeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GearScriptModeComponent.Cast(gameMode.FindComponent(CRF_GearScriptModeComponent));
		else
			return null;
	}
	// BLUFOR = 0, OPFOR = 1, INDFOR = 2, CIV = 3
	static CRF_GearScriptConfig getFactionConfig(int factionNum)
	{
		if (factionNum == 0 && m_BLUFORGearScript != null)  return m_BLUFORGearScript; 
		if (factionNum == 1 && m_OPFORGearScript!= null) return m_OPFORGearScript;
		if (factionNum == 2 && m_INDFORGearScript!= null) return m_INDFORGearScript;
		if (factionNum == 3 && m_CIVGearScript!= null) return m_CIVGearScript;
		return null;
	}
	
	/*
	[Attribute("45", "auto", "Mission Time (set to -1 to disable)")]
	int m_iTimeLimitMinutes;
	
	[Attribute("true", "auto", "Should we delete all JIP slots after SafeStart turns off?")]
	bool m_bDeleteJIPSlots;
	
	[RplProp(onRplName: "OnSafeStartChange")]
	protected bool m_SafeStartEnabled = false;
	ref ScriptInvoker m_OnSafeStartChange = new ScriptInvoker();
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	protected ref array<SCR_Faction> m_aPlayedFactionsArray = new array<SCR_Faction> ;
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;
	
	[RplProp(onRplName: "CallDeleteRedundantUnits")]
	protected bool m_bKillRedundantUnitsBool;
	
	protected int m_iTimeSafeStartBegan;
	protected int m_iTimeMissionEnds;
	protected int m_iSafeStartTimeRemaining;
	
	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	
	protected bool m_bAdminForcedReady = false;
	
	protected SCR_BaseGameMode m_GameMode;
	protected CRF_LoggingServerComponent m_Logging;
	
	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity,bool> m_mPlayersWithEHsMap = new map<IEntity,bool>;
	
	protected PS_PlayableManager m_PlayableManager;
	protected ref map<RplId, PS_PlayableComponent> m_mPlayables = new map<RplId, PS_PlayableComponent>;
	protected int m_mPlayablesCount = 0;
	
	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	bool m_bHUDVisible = true;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static CRF_SafestartGameModeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SafestartGameModeComponent.Cast(gameMode.FindComponent(CRF_SafestartGameModeComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on in-game post init
		// Is the the right way to do this? WHO KNOWS !
		if (!GetGame().InPlayMode()) return;
		
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
			
		if (Replication.IsServer())
		{
			m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			m_Logging = CRF_LoggingServerComponent.Cast(m_GameMode.FindComponent(CRF_LoggingServerComponent));
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		} 
	}
	
	//------------------------------------------------------------------------------------------------

	// Ready Up functions

	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void UpdateHUDVisible()
	{	
		m_bHUDVisible = !m_bHUDVisible;
	};
	
	//------------------------------------------------------------------------------------------------
	TStringArray GetWhosReady() {
		return m_aFactionsStatusArray;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayedFactions() 
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		SCR_SortedArray<SCR_Faction> outFaction = new SCR_SortedArray<SCR_Faction>;
		factionManager.GetSortedFactionsList(outFaction);
		
		if (!outFaction || outFaction.IsEmpty()) return;
		
		array<SCR_Faction> outArray = new array<SCR_Faction>;
		outFaction.ToArray(outArray);
		
		m_aPlayedFactionsArray.Clear();
		string bluforString = "#Coal_SS_No_Faction";
		string opforString = "#Coal_SS_No_Faction"; 
		string indforString = "#Coal_SS_No_Faction";

		foreach(SCR_Faction faction : outArray) {
			if (faction.GetPlayerCount() == 0 || faction.GetFactionLabel() == EEditableEntityLabel.FACTION_NONE) 
				continue;
			
			m_aPlayedFactionsArray.Insert(faction);
			
			Color factionColor = faction.GetFactionColor();
			float rg = Math.Max(factionColor.R(), factionColor.G());
			float rgb = Math.Max(rg, factionColor.B());
			
			switch (true) {
				case(!m_bBluforReady && rgb == factionColor.B()) : {bluforString = "#Coal_SS_Faction_Not_Ready"; break;};
				case(m_bBluforReady && rgb == factionColor.B())  : {bluforString = "#Coal_SS_Faction_Ready";     break;};
				case(!m_bOpforReady && rgb == factionColor.R())  : {opforString = "#Coal_SS_Faction_Not_Ready";  break;};
				case(m_bOpforReady && rgb == factionColor.R())   : {opforString = "#Coal_SS_Faction_Ready";      break;};
				case(!m_bIndforReady && rgb == factionColor.G()) : {indforString = "#Coal_SS_Faction_Not_Ready"; break;};
				case(m_bIndforReady && rgb == factionColor.G())  : {indforString = "#Coal_SS_Faction_Ready";     break;};
			};
		};
		
		m_aFactionsStatusArray = {bluforString, opforString, indforString};
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
	void ToggleSideReady(string setReady, string playerName, bool adminForced) {
		if (!GetSafestartStatus()) return;
		
		// If it's an admin-forced action
		if (adminForced)
		{
			if(m_bAdminForcedReady) 
			{
				m_bBluforReady = false;
				m_bOpforReady = false;
				m_bIndforReady = false;
				m_bAdminForcedReady = false;
			
				m_sMessageContent = string.Format("An Admin (%1) Has Force Unreadied All Sides!", playerName);
				Replication.BumpMe();
				ShowMessage();
				return;
			};
		
			m_bBluforReady = true;
			m_bOpforReady = true;
			m_bIndforReady = true;
			m_bAdminForcedReady = true;
			
			m_sMessageContent = string.Format("An Admin (%1) Has Force Readied All Sides!", playerName);
			Replication.BumpMe();
			ShowMessage();
			return;
		}
		
		if(m_bAdminForcedReady) 
			return;
			
		switch (setReady)
		{
			case("Blufor") : {
				m_bBluforReady = !m_bBluforReady; 
				if (m_bBluforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Blufor - %1", playerName);
				if (!m_bBluforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Blufor - %1", playerName);
				break;
			};
			case("Opfor")  : {
				m_bOpforReady = !m_bOpforReady;
				if (m_bOpforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Opfor - %1", playerName);
				if (!m_bOpforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Opfor - %1", playerName);
				break;
			};
			case("Indfor") : {
				m_bIndforReady = !m_bIndforReady; 
				if (m_bIndforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Readied_Indfor - %1", playerName);
				if (!m_bIndforReady) m_sMessageContent = string.Format("#Coal_SS_Faction_Unreadied_Indfor - %1", playerName);
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
		foreach(string factionCheckReadyString : m_aFactionsStatusArray) {
			if (factionCheckReadyString != "#Coal_SS_Faction_Ready") 
				continue;
			factionsReadyCount = factionsReadyCount + 1;
		};
		
		if (factionsReadyCount == 0 && m_iPlayedFactionsCount == 0 || factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining == 35) return;
				
		if (factionsReadyCount != m_iPlayedFactionsCount && m_iSafeStartTimeRemaining != 35) {
			m_sMessageContent = "#Coal_SS_Countdown_Cancelled";
			Replication.BumpMe();
			m_iSafeStartTimeRemaining = 35;
			return;
		};
		
		if (factionsReadyCount == m_iPlayedFactionsCount) {
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
		return m_SafeStartEnabled;
	}
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	void WaitTillGameStart()
	{
		if (!m_GameMode.IsRunning()) 
			return;
		
		GetGame().GetCallqueue().Remove(WaitTillGameStart);		
		ToggleSafeStartServer(true);
	}
	
	void OnSafeStartChange() 
	{
		m_OnSafeStartChange.Invoke(m_SafeStartEnabled);
	}
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void ToggleSafeStartServer(bool status) 
	{
		if (status) { // Turn on safestart
			if (m_SafeStartEnabled) return;
			
			m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_SafeStartEnabled = true;
			m_iSafeStartTimeRemaining = 35;
			
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			GetGame().GetCallqueue().Remove(CheckPlayersAlive);
			
			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 250, true);
			GetGame().GetCallqueue().CallLater(ActivateSafeStartEHs, 1250, true);
			GetGame().GetCallqueue().CallLater(UpdatePlayedFactions, 1000, true);
			
			Replication.BumpMe();//Broadcast m_SafeStartEnabled change
			
		} else { // Turn off safestart
			if (!m_SafeStartEnabled) return;	
			
			CallDeleteRedundantUnits();
			m_bKillRedundantUnitsBool = true;
			m_bAdminForcedReady = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			
			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(UpdateServerWorldTime);
			GetGame().GetCallqueue().Remove(ActivateSafeStartEHs);
			GetGame().GetCallqueue().Remove(UpdatePlayedFactions);
			
			GetGame().GetCallqueue().CallLater(CheckPlayersAlive, 5000, true);
			
			if (m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (m_iTimeLimitMinutes * 60000);
				GetGame().GetCallqueue().CallLater(UpdateMissionEndTimer, 250, true);
			} else {
				m_sServerWorldTime = "N/A";
			};
			
			Replication.BumpMe();//Broadcast change
			
			DisableSafeStartEHs();
			
			// Send notification message 
			m_Logging.GameStarted();
			
			// Use CallLater to delay the call for the removal of EHs so the changes so m_SafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(DisableSafeStartEHs, 1500);
			
			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(DisableSafeStartEHs, 12500);
			
			GetGame().GetCallqueue().CallLater(DelayChangeSafeStartDisabled, 250);
		}
	};
	
	//------------------------------------------------------------------------------------------------
	void DelayChangeSafeStartDisabled() {
		m_SafeStartEnabled = false;
		Replication.BumpMe();//Broadcast m_SafeStartEnabled change
	};
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
  		int totalSeconds = (millis / 1000);
		
		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);
		
		Replication.BumpMe();
	};
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeMissionEnds - currentTime;
  		int totalSeconds = (millis / 1000);
		
		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);
		
		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		};
		
		Replication.BumpMe();
	};
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	void CallDeleteRedundantUnits() 
	{
		m_PlayableManager = PS_PlayableManager.GetInstance();
		if(m_PlayableManager && m_bDeleteJIPSlots) {
			if (m_SafeStartEnabled) {
				// Slowly delete AI on another thread so we dont create any massive lag spikes.
				m_mPlayables = m_PlayableManager.GetPlayables();
				m_mPlayablesCount = m_mPlayables.Count();
				GetGame().GetCallqueue().CallLater(DeleteRedundantUnitsSlowly, 125, true);
			} else {
				// Quickly delete AI on the main thread if they are a JIP and have come in after safestart has been turned off.
				map<RplId, PS_PlayableComponent> playables = m_PlayableManager.GetPlayables();
				for (int i = 0; i < playables.Count(); i++) {
					PS_PlayableComponent playable = playables.GetElement(i);
					if (m_PlayableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
					{
						SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
						SCR_EntityHelper.DeleteEntityAndChildren(character);
					};
				}
			};
		};
	}

	//------------------------------------------------------------------------------------------------
	void DeleteRedundantUnitsSlowly() 
	{
		if (m_mPlayablesCount > 0) {
			PS_PlayableComponent playable = m_mPlayables.GetElement(m_mPlayablesCount - 1);
			if(!playable)
				return;
			m_mPlayablesCount = m_mPlayablesCount - 1;
			if (m_PlayableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
				SCR_EntityHelper.DeleteEntityAndChildren(character);
			}
		} else
			GetGame().GetCallqueue().Remove(DeleteRedundantUnitsSlowly);
	}
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	void ShowMessage()
	{	
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		
		m_sStoredMessageContent = m_sMessageContent;
		
		if (m_sMessageContent == "All Blufor Players Have Been Eliminated!" || m_sMessageContent == "All Opfor Players Have Been Eliminated!" || m_sMessageContent == "All Indfor Players Have Been Eliminated!") 
		{
			m_PopUpNotification.PopupMsg(m_sMessageContent, 30);
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
		foreach(SCR_Faction faction : m_aPlayedFactionsArray) {
			Color factionColor = faction.GetFactionColor();
			float rg = Math.Max(factionColor.R(), factionColor.G());
			float rgb = Math.Max(rg, factionColor.B());
			
			switch (true) {
				case(rgb == factionColor.B() && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[0] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Blufor Players Have Been Eliminated!"; break;};
				case(rgb == factionColor.R() && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[1] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Opfor Players Have Been Eliminated!";  break;};
				case(rgb == factionColor.G() && faction.GetPlayerCount() == 0 && m_aFactionsStatusArray[2] != "#Coal_SS_No_Faction") : { m_sMessageContent = "All Indfor Players Have Been Eliminated!";  break;};
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
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(outPlayers);
		
		foreach (int playerID : outPlayers)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			if (!controlledEntity) 
				continue;
			
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler) 
				continue;
		
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp) 
				continue;
			
			bool alreadyHasEventHandlers = m_mPlayersWithEHsMap.Get(controlledEntity);
		
			if (!alreadyHasEventHandlers) {
				charComp.SetSafety(true, true);
				eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
				eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
				m_mPlayersWithEHsMap.Set(controlledEntity, true);
			};
		};
	}
	
	protected void DisableSafeStartEHs()
	{	
		for (int i = 0; i < m_mPlayersWithEHsMap.Count(); i++)
		{
			IEntity controlledEntityKey = m_mPlayersWithEHsMap.GetKey(i);
			
			if(!controlledEntityKey)
				continue;
			
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntityKey.FindComponent(CharacterControllerComponent));
			if (!charComp) 
				continue;
			
			charComp.SetSafety(false, false);
			
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntityKey.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler) 
				continue;
			
			eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			
			m_mPlayersWithEHsMap.Set(controlledEntityKey, false);
		};
	};
	
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
	*/
};
