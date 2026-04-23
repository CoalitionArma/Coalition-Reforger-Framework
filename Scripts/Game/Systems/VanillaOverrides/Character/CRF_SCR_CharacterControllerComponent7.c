modded class SCR_CharacterControllerComponent
{
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		CompartmentAccessComponent compAccess = character.GetCompartmentAccessComponent();
			
		BaseCompartmentSlot compartment = compAccess.GetCompartment();
		if (compartment)
			if (compartment.GetOwner())
				if (compartment.GetOwner().GetPrefabData().GetPrefabName() == "{252AA925952F6888}Prefabs/Vehicles/C47_Drop.et")
					return;
		
		super.OnPrepareControls(owner, am, dt, player);
	}
}