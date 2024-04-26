[ComponentEditorProps(category: "Safe Start Component", description: "")]
class CRF_SafestartGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_SafestartGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("45", "auto", "Mission Time (set to -1 to disable)")]
	int m_iTimeLimitMinutes;
	
	[Attribute("true", "auto", "Should we delete all JIP slots after SafeStart turns off?")]
	bool m_bDeleteJIPSlots;
	
	[RplProp()]
	protected bool m_SafeStartEnabled = false;
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
	[RplProp()]
	protected ref array<string> m_aFactionsStatusArray;
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent = "";
	
	[RplProp(onRplName: "CallDeleteRedundantUnits")]
	protected bool m_bKillRedundantUnitsBool;
	
	protected int m_iTimeSafeStartBegan;
	protected int m_iTimeMissionEnds;
	protected int m_iSafeStartTimeRemaining;
	
	protected bool m_bBluforReady = false;
	protected bool m_bOpforReady = false;
	protected bool m_bIndforReady = false;
	
	protected SCR_BaseGameMode m_GameMode;
	
	protected int m_iPlayedFactionsCount;
	protected ref map<IEntity,bool> m_mPlayersWithEHsMap = new map<IEntity,bool>;
	
	protected PS_PlayableManager m_PlayableManager;
	protected ref map<RplId, PS_PlayableComponent> m_mPlayables = new map<RplId, PS_PlayableComponent>;
	protected int m_mPlayablesCount = 0;
	
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
			
		//Print("[CRF Safestart] OnPostInit");
		if (Replication.IsServer())
		{
			m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		} 
	}
	
	//------------------------------------------------------------------------------------------------

	// Ready Up functions

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
		
		m_iPlayedFactionsCount = 0;
		string bluforString = "#Coal_SS_No_Faction";
		string opforString = "#Coal_SS_No_Faction"; 
		string indforString = "#Coal_SS_No_Faction";

		foreach(SCR_Faction faction : outArray) {
			if (faction.GetPlayerCount() == 0 || faction.GetFactionLabel() == EEditableEntityLabel.FACTION_NONE) continue;
			
			m_iPlayedFactionsCount = m_iPlayedFactionsCount + 1;
			
			Color factionColor = faction.GetOutlineFactionColor();
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
		
		Replication.BumpMe();
	}
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	void ToggleSideReady(string setReady, string playerName) {
		if (!GetSafestartStatus()) return;
		
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
	}
	
	//Call from server
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCountDown()
	{
		int factionsReadyCount = 0;
		foreach(string factionCheckReadyString : m_aFactionsStatusArray) {
			if (factionCheckReadyString != "#Coal_SS_Faction_Ready") continue;
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
			
			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 250, true);
			GetGame().GetCallqueue().CallLater(ActivateSafeStartEHs, 1250, true);
			GetGame().GetCallqueue().CallLater(UpdatePlayedFactions, 500, true);
			
			Replication.BumpMe();//Broadcast m_SafeStartEnabled change
			
		} else { // Turn off safestart
			if (!m_SafeStartEnabled) return;	
			
			CallDeleteRedundantUnits();
			m_bKillRedundantUnitsBool = true;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			
			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(UpdateServerWorldTime);
			GetGame().GetCallqueue().Remove(ActivateSafeStartEHs);
			GetGame().GetCallqueue().Remove(UpdatePlayedFactions);
			
			if (m_iTimeLimitMinutes > 0) {
				m_iTimeMissionEnds = GetGame().GetWorld().GetWorldTime() + (m_iTimeLimitMinutes * 60000);
				GetGame().GetCallqueue().CallLater(UpdateMissionEndTimer, 250, true);
			} else {
				m_sServerWorldTime = "N/A";
			};
			
			Replication.BumpMe();//Broadcast change
			
			DisableSafeStartEHs();
			
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
	// Locality needs verified for workbench and local server hosting
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
			m_mPlayablesCount--;
			if (m_PlayableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
				SCR_EntityHelper.DeleteEntityAndChildren(character);
			}
		} else {
			GetGame().GetCallqueue().Remove(DeleteRedundantUnitsSlowly);
		}
	}
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void ShowMessage()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc) return;

		if (m_sMessageContent == "#Coal_SS_Game_Live") {
			SCR_PopUpNotification.GetInstance().PopupMsg(m_sMessageContent, 8, "#Coal_SS_SafeStart_Started_Subtext");
		} else {
			SCR_PopUpNotification.GetInstance().PopupMsg(m_sMessageContent, 2.5, "#Coal_SS_Countdown_Started_Subtext");
		};
	};
	
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
			if (!controlledEntity) continue;
			
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler) continue;
		
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp) continue;
			
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
			
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntityKey.FindComponent(CharacterControllerComponent));
			if (!charComp) continue;
			
			charComp.SetSafety(false, false);
			
			EventHandlerManagerComponent eventHandler = EventHandlerManagerComponent.Cast(controlledEntityKey.FindComponent(EventHandlerManagerComponent));
			if (!eventHandler) continue;
			
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
};