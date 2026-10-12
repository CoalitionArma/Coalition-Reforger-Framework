class CRF_VAAR_VehicleTrakcerComponentClass : ScriptComponentClass {}

//! Puts the vehicle on the VAAR playback. Registration is static so vehicles that initialise before the
//! game mode entity are still tracked.
class CRF_VAAR_VehicleTrakcerComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only the dedicated server records
		if (RplSession.Mode() == RplMode.Client)
			return;

		CRF_VAAR_GamemodeComponent.RegisterVehicle(owner);
	}

	override void OnDelete(IEntity owner)
	{
		CRF_VAAR_GamemodeComponent.UnregisterVehicle(owner);

		super.OnDelete(owner);
	}
}
