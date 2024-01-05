class CRF_TNK_SafestartComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_TNK_SafestartComponent: SCR_BaseGameModeComponent
{
	[RplProp()]
	protected bool m_SafeStartEnabled;
	
	// Temporary for jam testing
	ref RandomGenerator rand = new RandomGenerator();
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void ToggleSafeStartServer(bool status) 
	{
		Print("[CRF Safestart] Enabling safestart server");
		
		if (status) {
			if (m_SafeStartEnabled)
			{
				Print("[CRF Safestart] Safestart already enabled. Exiting.");
				return;
			}
			
			// Disable damage - server-wide
			/*ChimeraCharacter character = ChimeraCharacter.Cast(GetOwner());
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(character.FindComponent(SCR_DamageManagerComponent));
			if (damageManager) 
			{
				Print("[CRF Safestart] Disabling damage");
			}*/
					
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
		
		// Small test for future mod
		// Adds jamming
		if ((rand.RandInt(1,200)) == 1) {
			weapon.GetCurrentMuzzle().ClearChamber(0);
		}
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