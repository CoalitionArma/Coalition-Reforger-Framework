class CRF_TNK_SafestartComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_TNK_SafestartComponent: SCR_BaseGameModeComponent
{
	[RplProp()]
	protected bool m_SafeStartEnabled;
	
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void ToggleSafeStartServer(bool status) 
	{
		Print("[CRF Safestart] Enabling safestart server");
		
		if (status) // Turn on safestart
		{
			if (m_SafeStartEnabled)
			{
				Print("[CRF Safestart] Safestart already enabled. Exiting.");
				return;
			}
			
			m_SafeStartEnabled = true;
		} else { // Turn off safestart
			if (!m_SafeStartEnabled) 
			{
				Print("[CRF Safestart] Safestart already disabled. Exiting.");
				return;
			}		
			
			m_SafeStartEnabled = false;
		}
		
		Replication.BumpMe(); //Broadcast m_SafeStartEnabled change
	};
	
	bool GetSafestartStatus()
	{
		return m_SafeStartEnabled;
	}
	
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{		
		//Print("[CRF Safestart] Gun shot");
		
		// Get projectile and delete it
		delete entity;
	}
	
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		Print("[CRF Safestart] Grenade thrown");
		if (!weapon)
			return;

		// Get grenade and delete it
		delete entity;
	}
	
	void DrawClock()
	{
		// Render clock and status
	}
	
	void RemoveClock()
	{
		// Remove clock if visible
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
			
		Print("[CRF Safestart] OnPostInit");
		if (Replication.IsServer())
		{
			ToggleSafeStartServer(true);
		} 
	}
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{	
		super.OnPlayerSpawned(playerId, controlledEntity);
		Print("[CRF Safestart] Enabling safestart local");
		
		// Set safe/lowered
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
		if (charComp) 
		{
			Print("[CRF Safestart] Enabling safety");
			charComp.SetSafety(true, true);
		}	

		EventHandlerManagerComponent m_eventHandler = EventHandlerManagerComponent.Cast(controlledEntity.FindComponent(EventHandlerManagerComponent));
		if (!m_eventHandler) 
		{
			Print("[CRF Safestart] Cannot find EH Manager Component");
			return;
		}
		
		if (GetSafestartStatus()) {
			Print("[CRF Safestart] Adding EHs");
			// Draw clock
			DrawClock();
			
			// Add EH to prevent bullets from being fired
			m_eventHandler.RegisterScriptHandler("OnProjectileShot", this, OnWeaponFired);
			m_eventHandler.RegisterScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown);	
		} else {
			Print("[CRF Safestart] Removing EHs");
			// Set clock to show "start game" and then fade out
			RemoveClock();
			
			// Remove EH
			m_eventHandler.RemoveScriptHandler("OnProjectileShot", this, OnWeaponFired, false);
			m_eventHandler.RemoveScriptHandler("OnGrenadeThrown", this, OnGrenadeThrown, false);
		}
	}
};