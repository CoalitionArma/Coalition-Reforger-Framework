class CRF_TNK_SafestartComponentClass: SCR_BaseGameModeComponentClass
{
	
}

// really should be CRF_TNK_SafestartComponentManager because it's on the game mode, but i'm not going through and changing everthing.
class CRF_TNK_SafestartComponent: SCR_BaseGameModeComponent
{
	[RplProp()]
	protected bool m_SafeStartEnabled;
	
	[RplProp()]
	protected int m_iTimeSafeStartBegan;
	
	[RplProp()]
	protected bool m_bBluforReady = false;
	
	[RplProp()]
	protected bool m_bOpforReady = false;
	
	[RplProp()]
	protected bool m_bIndforReady = false;
	
	// We use every m_sServerWorldTime update to check all players EHs. (NOTE: this only updates when the string updates, so even though the loop m_sServerWorldTime refreshes every 250ms in practice it's every second)
	[RplProp(onRplName: "ToggleSafeStartEHs")]
	protected string m_sServerWorldTime;
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent = "";
	
	protected int m_iSafeStartTimeRemaining = 35;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static CRF_TNK_SafestartComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_TNK_SafestartComponent.Cast(gameMode.FindComponent(CRF_TNK_SafestartComponent));
		else
			return null;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on in-game post init
		// Is the the right way to do this? WHO KNOWS !
		if (!GetGame().InPlayMode()) 
		{
			return;
		}
			
		//Print("[CRF Safestart] OnPostInit");
		if (Replication.IsServer())
		{
			ToggleSafeStartServer(true);
		} 
	}
	
	//------------------------------------------------------------------------------------------------

	// Ready Up functions

	//------------------------------------------------------------------------------------------------
	
	TBoolArray GetWhosReady() {
		array<bool> outFactionsReady = {m_bBluforReady, m_bOpforReady, m_bIndforReady};
		return outFactionsReady;
	}
	
	//Call from server
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
		
  		int totalSeconds = (millis / 1000);
		
		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);
		
		// We also use this loop to check all players EHs
		ToggleSafeStartEHs();
		
		Replication.BumpMe();
	};
	
	//Call from server
	void ToggleSideReady(string setReady, string playerName) {
		
		if (!GetSafestartStatus()) return;
		
		switch (setReady)
		{
			case("Blufor") : {
				m_bBluforReady = !m_bBluforReady; 
				if (m_bBluforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Readied Up Blufor", playerName);
				if (!m_bBluforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Unreadied Up Blufor", playerName);
				break;
			};
			case("Opfor")  : {
				m_bOpforReady = !m_bOpforReady;
				if (m_bOpforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Readied Up Opfor", playerName);
				if (!m_bOpforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Unreadied Up Opfor", playerName);
				break;
			};
			case("Indfor") : {
				m_bIndforReady = !m_bIndforReady; 
				if (m_bIndforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Readied Up Indfor", playerName);
				if (!m_bIndforReady) m_sMessageContent = string.Format("[CRF] : %1 Has Unreadied Up Indfor", playerName);
				break;
			};
		};
		Replication.BumpMe();
	}
	
	//Call from server
	protected void CheckStartCountDown()
	{		
	 	SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		SCR_SortedArray<SCR_Faction> outFaction = new SCR_SortedArray<SCR_Faction>;
		factionManager.GetSortedFactionsList(outFaction);
		
		if (!outFaction || outFaction.IsEmpty()) return;
		
		array<SCR_Faction> outArray = new array<SCR_Faction>;
		outFaction.ToArray(outArray);
		
		int playedFactions = 0;
		foreach(SCR_Faction faction : outArray) {
			if (faction.GetPlayerCount() == 0) continue;
			playedFactions = playedFactions + 1;
		};
		
		array<bool> factionsReady = {m_bBluforReady, m_bOpforReady, m_bIndforReady};
		
		int factionsReadyCount = 0;
		foreach(bool factionready : factionsReady) {
			if (!factionready) continue;
			factionsReadyCount = factionsReadyCount + 1;
		};
		
		if (factionsReadyCount == 0 && playedFactions == 0 || factionsReadyCount != playedFactions && m_iSafeStartTimeRemaining == 35) return;
				
		if (factionsReadyCount != playedFactions && m_iSafeStartTimeRemaining != 35) {
			m_sMessageContent = "[CRF] : Game Live Countdown Canceled!";
			Replication.BumpMe();
			m_iSafeStartTimeRemaining = 35;
			return;
		};
		
		if (factionsReadyCount == playedFactions) {
			m_iSafeStartTimeRemaining = m_iSafeStartTimeRemaining - 5;
			m_sMessageContent = string.Format("[CRF] : Game Live In: %1 Seconds!", m_iSafeStartTimeRemaining);
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_sMessageContent = string.Format("[CRF] : GAME LIVE!", m_iSafeStartTimeRemaining);
			};
		};
		Replication.BumpMe();
	};
	
	//Called from server to all clients
	void ShowMessage()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc) return;

		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent) return;
		
		chatComponent.ShowMessage(m_sMessageContent);
	};
	
	//------------------------------------------------------------------------------------------------

	// SafeStart functions

	//------------------------------------------------------------------------------------------------
	
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}
	
	bool GetSafestartStatus()
	{
		return m_SafeStartEnabled;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ToggleSafeStartServer(bool status) 
	{
		if (status) { // Turn on safestart
			if (m_SafeStartEnabled) return;
			
			m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_SafeStartEnabled = true;
			m_iSafeStartTimeRemaining = 35;
			
			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 250, true);
			
			Replication.BumpMe();//Broadcast m_SafeStartEnabled change
			
		} else { // Turn off safestart
			if (!m_SafeStartEnabled) return;	
			
			m_SafeStartEnabled = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			
			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			GetGame().GetCallqueue().Remove(UpdateServerWorldTime);
			
			Replication.BumpMe();//Broadcast m_SafeStartEnabled change
			
			// Use UpdateServerWorldTime to essentially delay the call for the removal of EHs so the changes to m_SafeStartEnabled can propagate.
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 2850);
			
			// Even longer delay just in case there's any edge cases we didnt anticipate.
			GetGame().GetCallqueue().CallLater(UpdateServerWorldTime, 12500);
		}
	};
	
	protected void ToggleSafeStartEHs()
	{	
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetPlayers(outPlayers);
		
		array<string> eventHandlerStrings = {"OnProjectileShot", "OnGrenadeThrown"};
		
		foreach (int playerID : outPlayers)
		{
			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			if (!controlledEntity) continue;
			
			EventHandlerManagerComponent m_eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!m_eventHandler) continue;
		
			array<BaseEventHandler> outEventHandlers = {};
			m_eventHandler.GetEventHandlers(outEventHandlers);
		
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp) continue;
			
			bool alreadyHasEventHandlers = false;
			foreach (BaseEventHandler eventHandlers : outEventHandlers) {
				string eventHandlerName = eventHandlers.GetEventName();
				if (eventHandlerStrings.Contains(eventHandlerName)) {
					alreadyHasEventHandlers = true;
					continue;
				};
			};
		
			if (!alreadyHasEventHandlers && GetSafestartStatus()) {
				charComp.SetSafety(true, true);
				m_eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
				m_eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			};
		
			if (alreadyHasEventHandlers && !GetSafestartStatus()) {
				charComp.SetSafety(false, false);
				m_eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
				m_eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			};
		};
	}
	
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{		
		// Get projectile and delete it
		delete entity;
	}
	
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		if (!weapon)
			return;
		
		// Get grenade and delete it
		delete entity;
	}
};