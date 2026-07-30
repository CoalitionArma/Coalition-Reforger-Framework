[BaseContainerProps()]
modded class COA_GearScriptContainer
{
	//------------------------------------------------------------------------------------------------
	// Vars set by plugin
	
	[Attribute("false", UIWidgets.Hidden)]
	bool m_bEnableShareableMarkers;
	
	//------------------------------------------------------------------------------------------------
	// Vars considered "advanced" and not set by plugin
	
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableMagnifiedOptics;
	
	[Attribute("{E6555DA2F31B0EC0}Configs/Gearscripts/Additional Configs/COA_Global_SightArsenal_Regular.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=COA_SightArsenalConfig")]
	ResourceName m_rSightArsenal;
	
	[Attribute("{9D8E5FA08331042D}Configs/Gearscripts/Additional Configs/COA_Global_SightArsenal_Magnified.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all entities on this faction", "conf class=COA_SightArsenalConfig")]
	ResourceName m_rMagnifiedSightArsenal;
	
	[Attribute("{2E2626C733070162}Configs/Gearscripts/Additional Configs/COA_Global_VehicleGearscriptValues.conf", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all vehicles on this faction", "conf class=COA_VehicleGearscriptConfig")]
	ResourceName m_rVehicleGearscriptValues;
	
	[Attribute("", desc: "Loadout values applied to all vehicles in this faction", "conf class=CRF_VehicleGearScriptLoadout")]
	ref CRF_VehicleGearScriptLoadout m_VehicleLoadout;
	
	[Attribute()] 
	ref array<ref CRF_VehicleGearscriptOverride> m_aVehicleGearscriptOverrides;
	
	[Attribute()]
	ref array<ref CRF_VehicleGearScriptAdditionalItem> m_aAdditionalVehicleItems;
	
	[Attribute()] 
	ref array<ResourceName> m_aSupplyTrucks;
	
	[Attribute()] 
	ref array<ResourceName> m_aAdditonalItemsForSupplyArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniWeaponArsenal;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableSightArsenal;
}


// Simplified Container for Faction Plugin
[BaseContainerProps()]
modded class COA_SimplifiedGearScriptContainer
{
	[Attribute("false", UIWidgets.CheckBox)]
	bool m_bEnableShareableMarkers;
}