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
	
	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent = "";
	
	protected int m_iSafeStartTimeRemaining = 35;
	
	private ref array<IEntity> m_aControlledEntityArray = new array<IEntity>;
	
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
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{	
		super.OnPlayerSpawned(playerId, controlledEntity);
		//Print("[CRF Safestart] Enabling safestart local");
		if (!Replication.IsServer()) return;
		m_aControlledEntityArray.Insert(controlledEntity);

		EventHandlerManagerComponent m_eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
		if (!m_eventHandler) return;
		
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
		if (!charComp) return;
		
		if (GetSafestartStatus()) {
			// Set safe/lowered
			charComp.SetSafety(true, true);
			//Print("[CRF Safestart] Adding EHs");
			
			// Add EH to prevent bullets from being fired
			m_eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
			m_eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);	
		} else {
			charComp.SetSafety(false, false);
			//Print("[CRF Safestart] Removing EHs");
			
			// Remove EH
			m_eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
			m_eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
		};
	}
	
	//------------------------------------------------------------------------------------------------

	// Ready Up functions

	//------------------------------------------------------------------------------------------------
	
	TBoolArray GetWhosReady() {
		array<bool> outFactionsReady = {m_bBluforReady, m_bOpforReady, m_bIndforReady};
		return outFactionsReady;
	}
	
	//Call from server
	void ToggleSideReady(string setReady) {
		switch (setReady)
		{
			case("Blufor") : {m_bBluforReady = !m_bBluforReady; break;};
			case("Opfor")  : {m_bOpforReady = !m_bOpforReady;   break;};
			case("Indfor") : {m_bIndforReady = !m_bIndforReady; break;};
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
			m_sMessageContent = string.Format("[CRF] : Game Live In: %1", m_iSafeStartTimeRemaining);
			if (m_iSafeStartTimeRemaining == 0) {
				ToggleSafeStartServer(false);
				m_sMessageContent = string.Format("[CRF] : GAME LIVE!", m_iSafeStartTimeRemaining);
			};
		};
		Replication.BumpMe();
	};
	
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void ToggleSafeStartServer(bool status) 
	{
		//Print("[CRF Safestart] Enabling safestart server");
		
		if (status) // Turn on safestart
		{
			if (m_SafeStartEnabled)
			{
				//Print("[CRF Safestart] Safestart already enabled. Exiting.");
				return;
			}
			
			m_iTimeSafeStartBegan = GetGame().GetWorld().GetWorldTime();
			m_SafeStartEnabled = true;
			m_iSafeStartTimeRemaining = 35;
			GetGame().GetCallqueue().CallLater(CheckStartCountDown, 5000, true);
			ToggleSafeStartEHs();
		} else { // Turn off safestart
			if (!m_SafeStartEnabled) 
			{
				//Print("[CRF Safestart] Safestart already disabled. Exiting.");
				return;
			}		
			
			m_SafeStartEnabled = false;
			m_bBluforReady = false;
			m_bOpforReady = false;
			m_bIndforReady = false;
			GetGame().GetCallqueue().Remove(CheckStartCountDown);
			ToggleSafeStartEHs();
		}
		
		Replication.BumpMe(); //Broadcast m_SafeStartEnabled change
	};
	
	protected void ToggleSafeStartEHs()
	{	
		foreach (IEntity controlledEntity : m_aControlledEntityArray) 
		{
			EventHandlerManagerComponent m_eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
			if (!m_eventHandler) return;
		
			CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
			if (!charComp) return;
		
			if (GetSafestartStatus()) {
				// Set safe/lowered
				charComp.SetSafety(true, true);
				//Print("[CRF Safestart] Adding EHs");
			
				// Add EH to prevent bullets from being fired
				m_eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
				m_eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);	
			} else {
				charComp.SetSafety(false, false);
				//Print("[CRF Safestart] Removing EHs");
			
				// Remove EH
				m_eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired);
				m_eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);
			};
		};
	}
	
	bool GetSafestartStatus()
	{
		return m_SafeStartEnabled;
	}
	
	int GetTimeSafeStartBegan()
	{
		return m_iTimeSafeStartBegan;
	}
	
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{		
		//Print("[CRF Safestart] Gun shot");
		
		// Get projectile and delete it
		delete entity;
	}
	
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		//Print("[CRF Safestart] Grenade thrown");
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}
};