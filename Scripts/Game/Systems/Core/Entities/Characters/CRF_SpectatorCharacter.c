class CRF_SpectatorCharacterClass : CRF_PlayerCharacterClass
{
}

class CRF_SpectatorCharacter : CRF_PlayerCharacter
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHARACTER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		GetGame().GetCallqueue().Call(DisablePhysicsAndDamage);
		
		if (RplSession.Mode() != RplMode.Client)
		{
			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(this.FindComponent(SCR_CharacterControllerComponent)); 
			if (charController)
				charController.m_OnControlledByPlayer.Insert(OnControlledByPlayer);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Does what it says, disables all physics and damage on the character so spectators dont cause issues
	protected void DisablePhysicsAndDamage()
	{
		// - Physics Handling	
		//--------------------------------------------------------
		Physics physics = this.GetPhysics();
		if (!physics)
			return;
		
		physics.EnableGravity(false);
		physics.SetMass(0);
		physics.SetDamping(1, 1);
		physics.ChangeSimulationState(SimulationState.NONE);
		physics.SetInteractionLayer(EPhysicsLayerDefs.CharNoCollide);
		
		int numGeoms = physics.GetNumGeoms();
		for (int i = 0; i < numGeoms; i++) // Fixed: was i <= numGeoms (off-by-one error)
		{
			physics.SetGeomInteractionLayer(i, EPhysicsLayerDefs.CharNoCollide);
		}
		
		// - Damage Handling	
		//--------------------------------------------------------
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(this.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if (damManager)
			damManager.EnableDamageHandling(false);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Hijack the local character OnControlledByPlayer invoker so we can delete spectator entities when no one is controlling them.
	protected void OnControlledByPlayer(IEntity owner, bool controlled)
	{
		if (!controlled)	// Player is no longer in control of this entity
			GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, this); // Need to call on next frame so we dont mess up the player controller.
	}
}