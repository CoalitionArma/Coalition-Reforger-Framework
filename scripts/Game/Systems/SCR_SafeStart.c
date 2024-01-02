class CRF_TNK_SafestartComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_TNK_SafestartComponent: SCR_BaseGameModeComponent
{
	[RplProp()]
	protected bool m_SafeStartEnabled;
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_ToggleSafeStartServer(bool status) 
	{
		Print("[CRF Safestart] Enabling safestart server", LogLevel.NORMAL);
		
		if (status) {
			if (m_SafeStartEnabled)
			{
				Print("[CRF Safestart] Safestart already enabled. Exiting.");
				return;
			}
					
			/*SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
			if (!damageManager) 
			{
				Print("[CRF Safestart] Cannot find damage manager. Exiting.");
				return;
			};
			
			damageManager.EnableDamageHandling(false);*/	
			m_SafeStartEnabled = true;
		} else {
			if (!m_SafeStartEnabled)
			{
				Print("[CRF Safestart] Safestart already disabled. Exiting.");
				return;
			}		
			
			m_SafeStartEnabled = false;
		}
		
		Replication.BumpMe(); //Broadcast m_SafeStartEnabled change
		Print("[CRF Safestart] m_SafeStartEnabled: " + GetSafestartStatus());
	};
	
	bool GetSafestartStatus()
	{
		return m_SafeStartEnabled;
	}
	
	protected void OnWeaponFired(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		Print("[CRF Safestart] Gun shot");
		// Get projectile and delete it		
		/*weapon.GetCurrentMuzzle().ClearChamber(0);
		weapon.GetCurrentMagazine().SetAmmoCount(0);
		weapon.GetCurrentMuzzle();*/
		
		// Set safe
		/*CharacterControllerComponent charComp = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
		charComp.SetSafety(true, true);*/
	}
	
	protected void OnGrenadeThrown(int playerID, BaseWeaponComponent weapon, IEntity entity)
	{
		Print("[CRF Safestart] Grenade thrown");
		if (!weapon)
			return;

		// Get grenade and delete it
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
		SetEventMask(owner, EntityEvent.INIT);
		Print("[CRF Safestart] OnPostInit");
		if (Replication.IsServer())
		{
			RpcDo_ToggleSafeStartServer(true);
		}	
	}
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{	
		super.OnPlayerSpawned(playerId, controlledEntity);
		Print("[CRF Safestart] Enabling safestart local");
		
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