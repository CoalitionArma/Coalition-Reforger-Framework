// CRF_PropHuntGamemode_SCR_DamageManagerComponent.c
//
// Damage hook for the Prop Hunt gamemode.
//
// Handles two distinct cases:
//
//  A. Prop entity hit (hunt phase):
//     When a Hunter's shot hits a registered prop entity (a world object that
//     a disguised Prop player has spawned as their disguise), the owning Prop
//     player's hidden character is killed immediately via lethal TRUE damage.
//     The normal CRF death→spectator flow fires via OnControllableDestroyed
//     in CRF_PropHuntGamemode, which also cleans up the prop entity.
//
//  B. Hunter shot penalty (hunt phase):
//     Detects when a Hunter fires a weapon and forwards the event to
//     CRF_PropHuntGamemode.ApplyHunterShotPenalty, which:
//       - Decrements the Hunter's CUSTOM penalty health bar (no real HP change per shot).
//       - Sends an updated hint to that Hunter's screen showing their remaining HP.
//       - Applies lethal damage only when the bar reaches 0 (to trigger normal CRF
//         death → spectator flow via OnControllableDestroyed in CRF_PropHuntGamemode).
//
// Death tracking (alive-prop list + prop-kill health restore) is handled entirely
// inside CRF_PropHuntGamemode.OnControllableDestroyed — no OnDeath hook is needed here.

modded class SCR_DamageManagerComponent
{
	//------------------------------------------------------------
	// OnDamage — fires on the entity being HIT (server-side).
	//
	// Detects two cases that matter for Prop Hunt during the HUNT phase:
	//
	//  A. The prop ENTITY itself is hit (only possible if its physics
	//     body is active — left here as a belt-and-suspenders fallback).
	//
	//  B. The invisible-but-TRACEABLE prop player CHARACTER is hit.
	//     Because the prop entity has SimulationState.NONE (no physics
	//     body), bullet raycasts pass through the prop mesh and hit the
	//     character's capsule instead. We detect that here.
	//
	// In both cases: instantly kill the disguised prop player and restore
	// the shooter hunter's penalty HP to maximum.
	//
	// Shot penalty (HP decrement per shot) is handled separately via the
	// "OnProjectileShot" event handler registered in CRF_PropHuntGamemode.
	//------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (!propHunt || !propHunt.IsHuntPhaseActive())
			return;

		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		// Only kinetic (bullet) and melee damage count as a prop kill.
		if (damageContext.damageType != EDamageType.KINETIC &&
			damageContext.damageType != EDamageType.MELEE)
			return;

		// --- Resolve which prop player was hit ---
		// Case A: the hit entity IS a registered prop entity (world object clone).
		int propOwnerId = propHunt.GetPlayerForPropEntity(GetOwner());

		// Case B: the hit entity is the INVISIBLE CHARACTER of a transformed prop player.
		// (Primary path on dedicated server — bullets hit the traceable character capsule.)
		if (propOwnerId <= 0)
			propOwnerId = propHunt.GetPropPlayerForCharacter(GetOwner());

		if (propOwnerId <= 0)
			return;

		// Guard: if the player has already been removed from the transformed map
		// (e.g. killed by an earlier bullet hit that is still propagating), abort.
		if (!propHunt.IsPlayerTransformed(propOwnerId))
			return;

		// Best-effort: get the shooter's character entity so OnControllableDestroyed can
		// identify the hunter and restore their HP. Null is safe — kills still proceed.
		IEntity shooterEnt = null;
		if (damageContext.instigator)
		{
			int shooterId = damageContext.instigator.GetInstigatorPlayerID();
			if (shooterId > 0)
				shooterEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(shooterId);
		}

		propHunt.KillPropPlayerByProxy(propOwnerId, shooterEnt);
	}
}
