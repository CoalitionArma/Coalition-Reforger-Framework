//! Holdout: zombies get tougher each wave - they take only part of the damage dealt to them
//! (CRF_HoldoutGamemodeComponent.GetZombieDamageScale). Same hook and the same exclusions as the
//! Slasher's: the character damage manager's HijackDamageHandling runs before the hit is applied, and
//! the default (health) hit zone is skipped because damage passed on to it from a body part was
//! already scaled. Damage players do to zombies is also passed on for assists (OnZombieDamaged).
modded class SCR_CharacterDamageManagerComponent
{
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout && damageContext.damageValue > 0 && damageContext.struckHitZone != GetDefaultHitZone()
			&& damageContext.damageType != EDamageType.TRUE && damageContext.damageType != EDamageType.HEALING
			&& damageContext.damageType != EDamageType.REGENERATION)
		{
			// Assists count the damage before the wave's toughness, so the bar doesn't rise with the waves
			if (Replication.IsServer() && damageContext.instigator && GetState() != EDamageState.DESTROYED)
				holdout.OnZombieDamaged(GetOwner(), damageContext.instigator.GetInstigatorPlayerID(), damageContext.damageValue);

			damageContext.damageValue = damageContext.damageValue * holdout.GetZombieDamageScale(GetOwner());
		}

		return super.HijackDamageHandling(damageContext);
	}

	//------------------------------------------------------------------------------------------------
	//! Zombies don't go down: where one would fall unconscious (second chance, blood loss) it dies, to
	//! the last hit's instigator so the kill is still paid. Vanilla's "permit unconsciousness" can't do
	//! this - ACE's UpdateConsciousness never looks at it.
	override void UpdateConsciousness()
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout && Replication.IsServer() && holdout.IsZombie(GetOwner()) && ShouldBeUnconscious())
		{
			Kill(GetInstigator());
			return;
		}

		super.UpdateConsciousness();
	}
}
