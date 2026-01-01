/*
* Join-In-Progress (JIP) Synchronization Manager
* DEPRECATED: This manager is being phased out in favor of RplSave/RplLoad methods
* 
* RplSave/RplLoad provides automatic JIP synchronization built into the Enfusion engine.
* Systems should override these methods in their components instead of using manual RPCs.
* 
* Migration status:
* - Vehicle supply costs: Now uses RplSave/RplLoad in CRF_GearscriptManager ✓
* - Faction radio channels: Now uses RplSave/RplLoad in SCR_FactionManager ✓
* - Rush 3D markers: Uses RplProp + BumpMe() (already automatic) ✓
* - GunGame stats: Uses OnPlayerConnected override (already automatic) ✓
* - Slotting data: Uses RplSave/RplLoad in CRF_SlottingManager ✓
* 
* TODO: Remove this component entirely once confirmed all JIP sync works via RplSave/RplLoad
*/

[ComponentEditorProps(category: "CRF JIP Sync Manager (DEPRECATED)", description: "DEPRECATED - Use RplSave/RplLoad instead")]
class CRF_JIPSyncManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_JIPSyncManager : SCR_BaseGameModeComponent
{
	protected static CRF_JIPSyncManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	void CRF_JIPSyncManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_JIPSyncManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when a player joins the server
	// DEPRECATED: All JIP sync now handled by RplSave/RplLoad and OnPlayerConnected overrides
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		// No longer needed - all systems now handle their own JIP sync via:
		// - RplSave/RplLoad methods (automatic engine-level sync)
		// - OnPlayerConnected overrides (for systems that need custom logic)
	}
}
