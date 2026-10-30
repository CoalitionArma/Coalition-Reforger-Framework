//! Predator special: the predator takes only part of the damage dealt to it (CRF_PredatorGamemodeComponent.GetDamageScale),
//! and a hit breaks its cloak for a while (OnPredatorHit). Works like the Slasher's override: HijackDamageHandling runs
//! before the damage is applied, and hits on the default (health) hit zone are left alone because they're copies of a
//! body-part hit that was already handled. Bleeding doesn't count as a hit, so a wounded predator can still cloak.
modded class SCR_CharacterDamageManagerComponent
{
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		CRF_PredatorGamemodeComponent predator = CRF_PredatorGamemodeComponent.GetInstance();
		if (predator && damageContext.damageValue > 0 && damageContext.struckHitZone != GetDefaultHitZone()
			&& damageContext.damageType != EDamageType.TRUE && damageContext.damageType != EDamageType.HEALING
			&& damageContext.damageType != EDamageType.REGENERATION && damageContext.damageType != EDamageType.BLEEDING)
		{
			damageContext.damageValue = damageContext.damageValue * predator.GetDamageScale(GetOwner());
			predator.OnPredatorHit(GetOwner());
		}

		return super.HijackDamageHandling(damageContext);
	}
}
