class CRF_VAAR_VehicleTrakcerComponentClass : ScriptComponentClass {}

class CRF_VAAR_VehicleTrakcerComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(onwer);
		
		//if (RplSession.Mode() != RplMode.Dedicated)
		//	return;
		
		CRF_VAAR_GamemodeComponent aarGamemodeComponent = CRF_VAAR_GamemodeComponent.GetInstance();
		if (!aarGamemodeComponent)
			return;
		
		aarGamemodeComponent.RegisterVehicle(owner)
	}
	
	override void OnDelete(IEntity owner)
	{
		//if (RplSession.Mode() != RplMode.Dedicated)
		//	return;
		
		CRF_VAAR_GamemodeComponent aarGamemodeComponent = CRF_VAAR_GamemodeComponent.GetInstance();
		if (!aarGamemodeComponent)
			return;
		
		aarGamemodeComponent.UnregisterVehicle(owner);
		
		super.OnDelete(owner);
	}
}