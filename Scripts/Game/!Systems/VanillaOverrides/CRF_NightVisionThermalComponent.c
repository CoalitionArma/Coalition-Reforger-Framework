modded class NightVisionThermalComponent
{
	//------------------------------------------------------------------------------------------------
	// Fixes a crash reported when activating the commander turret thermal sight on the bmp2.
	// Neither re-resolving a fresh Material each call (ruling out a stale cached handle) nor
	// switching from GetParamIndex()/SetParamByIndex() to SetParam() (ruling out that specific
	// native call) stopped the crash - every native call into the resolved Material crashed the
	// same way. That means materialName itself isn't resolving to a genuinely valid, loadable
	// resource for this turret/vehicle combination. Gate on Resource.Load(materialName).IsValid()
	// (the engine's documented "check before use" pattern) before touching Material at all; if the
	// resource isn't valid, skip runtime tuning for this call instead of crashing - the sight still
	// displays, just without the extra runtime tuning pass for that material.
	override protected bool ApplyThermalMaterialTuning(ResourceName materialName, bool verbose)
	{
		if (materialName == string.Empty)
			return false;

		Resource materialResource = Resource.Load(materialName);
		if (!materialResource || !materialResource.IsValid())
			return false;

		return super.ApplyThermalMaterialTuning(materialName, verbose);
	}
}
