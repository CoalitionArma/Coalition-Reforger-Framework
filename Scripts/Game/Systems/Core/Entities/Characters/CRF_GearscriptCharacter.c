class CRF_GearscriptCharacterClass : CRF_PlayerCharacterClass
{
}

class CRF_GearscriptCharacter : CRF_PlayerCharacter
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHARACTER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		
		if (!GetGame().InPlayMode() || !gearscriptManager)
			return;
		
		// Schedule gearscript identity setup with appropriate delay
		GetGame().GetCallqueue().Call(
			gearscriptManager.SetEntityIdentity, 
			owner
		);
	
		// Apply gearscript if not on client
		if (RplSession.Mode() != RplMode.Client)
			// Schedule gear setup with appropriate delay
			GetGame().GetCallqueue().Call(
				gearscriptManager.SetEntityGear, 
				owner, 
				owner.GetPrefabData().GetPrefabName()
			);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEAPON SELECTION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! RPC up to the owner to select the primary weapon (should only be executed on characters that a player is controlling)
	void SelectPrimaryWeapon()
	{
		Rpc(RpcDo_SelectPrimaryWeapon);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SelectPrimaryWeapon()
	{
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (!entity)
			return;
		
		// Use the shared weapon selection logic from helper
		CRF_WeaponHelper.SelectPrimaryWeaponForEntity(entity);
	}
}