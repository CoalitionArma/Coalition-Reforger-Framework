modded class COA_VehicleSpawner
{
	[Attribute("1", desc: "Should we add ammo to this vehicle", category: "CRF Vehicle Spawning")] 
	bool m_bShouldAddAmmo;

	[Attribute("", desc: "Loadout values applied to this vehicle", "conf class=CRF_VehicleGearScriptLoadout", category: "CRF Vehicle Spawning")] 
	ref CRF_VehicleGearScriptLoadout m_OverridedVehicleLoadout;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute(category: "CRF Vehicle Spawning")] 
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
}