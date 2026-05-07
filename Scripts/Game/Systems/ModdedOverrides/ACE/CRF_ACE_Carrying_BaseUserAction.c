modded class ACE_Carrying_BaseUserAction
{
	//------------------------------------------------------------------------------------------------
	//! Override: Show carry/drag action even when the casualty is in water.
	//! The engine registers water bodies as vehicle compartments, causing IsInVehicle() to return
	//! true for unconscious players floating in water and hiding this action. We replace ACE's check
	//! entirely (not calling super) to add the water exception.
	override bool CanBeShownScript(IEntity user)
	{
		SCR_ChimeraCharacter ownerChar = SCR_ChimeraCharacter.Cast(GetOwner());
		if (!ownerChar)
			return false;

		// Block if victim is inside an actual vehicle, but allow if they are in water
		// (water bodies register as vehicles in the engine's compartment system)
		if (ownerChar.IsInVehicle() && !SCR_CharacterControllerComponent.ACE_Carrying_IsCharacterInWater(ownerChar))
			return false;

		SCR_CharacterControllerComponent ownerCharController = SCR_CharacterControllerComponent.Cast(ownerChar.GetCharacterController());
		if (!ownerCharController || ownerCharController.GetLifeState() != ECharacterLifeState.INCAPACITATED)
			return false;

		return true;
	}
}
