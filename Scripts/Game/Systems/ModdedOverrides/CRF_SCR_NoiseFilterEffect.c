modded class SCR_NoiseFilterEffect
{
	//------------------------------------------------------------------------------------------------
	//! Override: When the controlled entity changes (death, spectator, respawn), explicitly reset
	//! the CharacterLifeState audio variable to the new entity's life state. Without this, the
	//! muffled hearing from dying persists because vanilla only resets via SCR_DeployMenuBase.
	//! SGetOnMenuOpen(), which CRF's spectator/respawn flow does not invoke.
	override protected void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		super.DisplayControlledEntityChanged(from, to);

		ECharacterLifeState state = ECharacterLifeState.ALIVE;
		if (m_pCharacterEntity)
		{
			CharacterControllerComponent charController = m_pCharacterEntity.GetCharacterController();
			if (charController)
				state = charController.GetLifeState();
		}

		AudioSystem.SetVariableByName("CharacterLifeState", state, "{A60F08955792B575}Sounds/_SharedData/Variables/GlobalVariables.conf");
	}
}
