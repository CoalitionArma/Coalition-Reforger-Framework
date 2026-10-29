//! Holdout: zombies get tougher each wave - they take only part of the damage dealt to them
//! (CRF_HoldoutGamemodeComponent.GetZombieDamageScale). Same hook and the same exclusions as the
//! Slasher's: the character damage manager's HijackDamageHandling runs before the hit is applied, and
//! the default (health) hit zone is skipped because damage passed on to it from a body part was
//! already scaled.
modded class SCR_CharacterDamageManagerComponent
{
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout && damageContext.damageValue > 0 && damageContext.struckHitZone != GetDefaultHitZone()
			&& damageContext.damageType != EDamageType.TRUE && damageContext.damageType != EDamageType.HEALING
			&& damageContext.damageType != EDamageType.REGENERATION)
		{
			damageContext.damageValue = damageContext.damageValue * holdout.GetZombieDamageScale(GetOwner());
		}

		return super.HijackDamageHandling(damageContext);
	}
}
