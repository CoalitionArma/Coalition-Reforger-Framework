class CRF_XPHandlerComponentClass : SCR_XPHandlerComponentClass {}

//------------------------------------------------------------------------------------
// CRF_XPHandlerComponent: XP handler for CRF_Gamemode
//
// Extends SCR_XPHandlerComponent (the vanilla Conflict XP system) to work correctly
// with CRF's custom gamemode. Key differences from vanilla:
//
//   - OnVehicleDestroyed: vanilla only awards XP when the destroyed vehicle has a
//     CAMPAIGN budget value (Conflict-only). CRF vehicles use the VEHICLES budget, so
//     we override this to award the flat configured ENEMY_VEHICLE_DESTRUCTION XP
//     whenever an enemy vehicle is destroyed, regardless of budget type.
//
//   - OnPostInit: removes the Conflict-specific campaign base attack hooks
//     (GetOnBaseUnderAttack / GetOnBaseAttackEnd) that are registered by the super call.
//     These events are never fired in CRF since there are no SCR_CampaignMilitaryBaseComponents.
//
// Everything else (kills, transport, medical, survival, repair, group cohesion, etc.)
// works automatically through the SCR_BaseGameModeComponent dispatch chain and does not
// need to be touched.
//
// Setup required in Workbench (cannot be done from script):
//   1. Add CRF_XPHandlerComponent to the CRF_Gamemode world entity.
//   2. Ensure SCR_PlayerXPHandlerComponent is present on the PlayerController prefab.
//------------------------------------------------------------------------------------
class CRF_XPHandlerComponent : SCR_XPHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Initialize the XP system and remove Conflict-only event subscriptions.
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// These static invokers are subscribed by the super call but will never fire in
		// CRF because the gamemode has no SCR_CampaignMilitaryBaseComponents.
		// Remove them so there are no dangling references and the destructor is clean.
		SCR_CampaignMilitaryBaseComponent.GetOnBaseUnderAttack().Remove(OnBaseAttackStarted);
		SCR_CampaignMilitaryBaseComponent.GetOnBaseAttackEnd().Remove(OnBaseAttackEnded);
	}

	//------------------------------------------------------------------------------------------------
	//! Award XP for destroying an enemy vehicle.
	//! Overrides the vanilla version which requires EEditableEntityBudget.CAMPAIGN — a
	//! budget that only exists in the Conflict gamemode. In CRF we award the flat
	//! configured ENEMY_VEHICLE_DESTRUCTION reward whenever any enemy vehicle is killed.
	protected override void OnVehicleDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		if (instigatorContextData.GetInstigator().GetInstigatorType() != InstigatorType.INSTIGATOR_PLAYER)
			return;

		Vehicle vehicle = Vehicle.Cast(instigatorContextData.GetVictimEntity());
		if (!vehicle)
			return;

		int playerID = instigatorContextData.GetInstigator().GetInstigatorPlayerID();
		if (playerID == 0)
			return;

		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerID);
		if (!playerFaction)
			return;

		Faction vehicleFaction = vehicle.GetFaction();

		// Fall back to the vehicle's default faction if it has no runtime faction assigned
		if (!vehicleFaction)
			vehicleFaction = vehicle.GetDefaultFaction();

		// Do not reward destroying a friendly vehicle
		if (!vehicleFaction || playerFaction.IsFactionFriendly(vehicleFaction))
			return;

		// Award the flat configured XP amount — no campaign budget lookup required
		AwardXP(playerID, SCR_EXPRewards.ENEMY_VEHICLE_DESTRUCTION);
	}
}
