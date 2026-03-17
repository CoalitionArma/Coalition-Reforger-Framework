//! DOT damage area — mirrors SCR_DotDamageArea for CRF effects.
//! Damage type comes from the effect's GetDefaultDamageType(), not the area.
//! Exit modes: immediate removal, linger for duration, or persist indefinitely.

class CRF_DotDamageArea : SCR_DamageArea
{
	[Attribute(desc: "How much damage (or healing) per second while the character is in the area")]
	protected float m_fDotDamage;

	[Attribute(defvalue: "0", desc: "Effect duration in seconds.\n0 = infinite (until character exits and m_bRemoveEffectWhenLeavingTheArea is true, or until removed by code).", params: "0 inf 0.01")]
	protected float m_fEffectDuration;

	[Attribute(defvalue: "1", desc: "When true and m_bRemoveEffectWhenLeavingTheArea is false, the effect lingers for m_fEffectDuration seconds after the character leaves the area.\nNOTE: Same limitation as m_bRemoveEffectWhenLeavingTheArea — if the component has many areas with the same effect type, leaving one zone removes all matching instances from this owner.")]
	protected bool m_bAddDurationOnExit;

	//! Clone template DOT effect and configure DPS/type/hitzone/instigator/duration.
	override protected BaseDamageEffect GetDamageEffect(SCR_DamageManagerComponent dmgMgr = null, HitZone affectedHitZone = null)
	{
		// Return the raw template when called without context (e.g. for type checks)
		if (!dmgMgr && !affectedHitZone)
			return m_DamageEffect;

		SCR_DotDamageEffect output = SCR_DotDamageEffect.Cast(m_DamageEffect.Clone());
		if (!output)
			return m_DamageEffect;

		output.SetDPS(m_fDotDamage);
		output.SetDamageType(output.GetDefaultDamageType());
		output.SetAffectedHitZone(affectedHitZone);
		output.SetInstigator(Instigator.CreateInstigator(GetParent()));

		if (!m_bAddDurationOnExit)
			output.SetMaxDuration(m_fEffectDuration);

		return output;
	}

	//! Handle exit: remove immediately, start linger timer, or persist.
	override void OnAreaExit(IEntity entity)
	{
		// Mode (a) — immediate removal via base class
		if (m_bRemoveEffectWhenLeavingTheArea)
		{
			super.OnAreaExit(entity);
			return;
		}

		// Mode (c) — persist indefinitely
		if (!m_bAddDurationOnExit)
			return;

		// Mode (b) — linger for m_fEffectDuration then expire naturally
		if (!entity)
			return;

		SCR_ExtendedDamageManagerComponent dmgMgr = SCR_ExtendedDamageManagerComponent.Cast(SCR_DamageManagerComponent.GetDamageManager(entity));
		if (!dmgMgr)
			return;

		array<ref SCR_PersistentDamageEffect> damageEffects = {};
		if (dmgMgr.FindAllDamageEffectsOfType(GetDamageEffect().Type(), damageEffects) < 1)
			return;

		IEntity owner = GetParent();
		SCR_DotDamageEffect dotEffect;
		foreach (SCR_PersistentDamageEffect effect : damageEffects)
		{
			dotEffect = SCR_DotDamageEffect.Cast(effect);
			if (!dotEffect)
				continue;

			if (dotEffect.GetInstigator().GetInstigatorEntity() == owner)
				dotEffect.SetMaxDuration(dotEffect.GetCurrentDuration() + m_fEffectDuration);
		}
	}
}
