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
			owner.SetOrigin("0 500 0")
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
}