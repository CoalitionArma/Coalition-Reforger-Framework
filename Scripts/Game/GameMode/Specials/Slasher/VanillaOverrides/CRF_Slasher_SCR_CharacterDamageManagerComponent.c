//! Slasher special: killers take only part of the damage dealt to them (CRF_SlasherGamemodeComponent.GetDamageScale).
//! HijackDamageHandling runs before the damage is applied, so scaling the value here scales the hit. It has to
//! be the character damage manager: its HijackDamageHandling doesn't call the base class's.
//! Hits on the default (health) hit zone are left alone: damage passed on from a body part arrives there as a
//! copy of a hit that was already scaled, and would otherwise be scaled twice.
modded class SCR_CharacterDamageManagerComponent
{
	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		CRF_SlasherGamemodeComponent slasher = CRF_SlasherGamemodeComponent.GetInstance();
		if (slasher && damageContext.damageValue > 0 && damageContext.struckHitZone != GetDefaultHitZone()
			&& damageContext.damageType != EDamageType.TRUE && damageContext.damageType != EDamageType.HEALING
			&& damageContext.damageType != EDamageType.REGENERATION)
		{
			damageContext.damageValue = damageContext.damageValue * slasher.GetDamageScale(GetOwner());
		}

		return super.HijackDamageHandling(damageContext);
	}
}
