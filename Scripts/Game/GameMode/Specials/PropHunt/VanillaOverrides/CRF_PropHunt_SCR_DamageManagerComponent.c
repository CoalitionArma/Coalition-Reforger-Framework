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
	// OnDamage — fires on the entity being HIT.
	//
	// Priority 1: if the damaged entity is a registered Prop disguise
	//   entity, kill the owning player's hidden character immediately.
	//
	// Priority 2: if the instigator is a Hunter, apply the shot penalty.
	//
	// The guard in ApplyHunterShotPenalty (currentHealth <= 0 → return)
	// prevents the lethal-kill damage from re-entering this path.
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

		// Only weapon-fired projectiles count — excludes the internal 9999 TRUE kill-damage
		// (which has no player instigator) and explosion/fire damage.
		if (damageContext.damageType != EDamageType.KINETIC &&
			damageContext.damageType != EDamageType.MELEE)
			return;

		// No player instigator → internal kill-damage loop; skip.
		int shooterId = damageContext.instigator.GetInstigatorPlayerID();
		if (shooterId <= 0)
			return;

		// Confirm shooter is on the Hunters team.
		IEntity shooterEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(shooterId);
		if (!shooterEnt)
			return;

		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(
			shooterEnt.FindComponent(FactionAffiliationComponent)
		);
		if (!facComp || !facComp.GetAffiliatedFaction())
			return;

		if (facComp.GetAffiliatedFaction().GetFactionKey() != propHunt.GetHuntersTeamKey())
			return;

		//--------------------------------------------------------
		// Case A: prop disguise entity was hit.
		// Apply the shot penalty FIRST (costs HP), then kill the
		// hidden prop player. OnControllableDestroyed will restore
		// the hunter to max HP since they made a correct kill.
		// The shooterEnt is passed as the instigator so the restore
		// logic in OnControllableDestroyed can identify the hunter.
		//--------------------------------------------------------
		int propOwnerId = propHunt.GetPlayerForPropEntity(GetOwner());
		if (propOwnerId > 0)
		{
			propHunt.ApplyHunterShotPenalty(shooterId);
			propHunt.KillPropPlayerByProxy(propOwnerId, shooterEnt);
			return;
		}

		//--------------------------------------------------------
		// Case B: non-prop damageable entity hit — apply penalty.
		//--------------------------------------------------------
		propHunt.ApplyHunterShotPenalty(shooterId);
	}
}
