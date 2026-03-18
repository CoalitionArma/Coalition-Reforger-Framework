//! Damage area for persistent collision effects (slow/movement restriction).
//! Clones a SCR_SpecialCollisionDamageEffect template per-character, sets ResponsibleEntity,
//! and handles responsible-entity-aware cleanup on exit. Anti-stacking at both area and effect level.

class CRF_AreaDamageArea : SCR_DamageArea
{
	
	//! Clone template effect, set ResponsibleEntity/HitZone/Instigator.
	override protected BaseDamageEffect GetDamageEffect(SCR_DamageManagerComponent dmgMgr = null, HitZone affectedHitZone = null)
	{
		// Return raw template for type checks (e.g., OnAreaExit matching)
		if (!dmgMgr && !affectedHitZone)
			return m_DamageEffect;
		
		SCR_SpecialCollisionDamageEffect clone = SCR_SpecialCollisionDamageEffect.Cast(m_DamageEffect.Clone());
		if (!clone)
		{
			Print("[CRF_AreaDamageArea] Damage Effect must be a SCR_SpecialCollisionDamageEffect subclass (e.g., CRF_AreaSlowEffect). Check your prefab config.", LogLevel.WARNING);
			return m_DamageEffect;
		}
		
		clone.SetResponsibleEntity(GetParent());
		clone.SetAffectedHitZone(affectedHitZone);
		clone.SetInstigator(Instigator.CreateInstigator(GetParent()));
		
		return clone;
	}
	
	//! Server-only, characters-only, anti-stacking, then delegates to base.
	override void OnAreaEntered(notnull IEntity entity)
	{
		if (!Replication.IsServer())
			return;
		
		if (!ChimeraCharacter.Cast(entity))
			return;
		
		if (!m_DamageEffect)
		{
			Print("[CRF_AreaDamageArea] No Damage Effect configured on " + GetParent(), LogLevel.WARNING);
			return;
		}
		
		// Anti-stacking: skip if this source already has an active effect on the character
		SCR_ExtendedDamageManagerComponent dmgMgr = SCR_ExtendedDamageManagerComponent.Cast(SCR_DamageManagerComponent.GetDamageManager(entity));
		if (!dmgMgr)
			return;
		
		if (HasActiveEffectFrom(dmgMgr, GetParent()))
			return;
		
		// Base class selects hitzone, calls GetDamageEffect(), handles AddDamageEffect/HandleDamage
		super.OnAreaEntered(entity);
	}
	
	//! On exit, terminate all matching effects from this area's owner.
	override void OnAreaExit(IEntity entity)
	{
		if (!entity)
			return;
		
		if (!Replication.IsServer())
			return;
		
		SCR_ExtendedDamageManagerComponent dmgMgr = SCR_ExtendedDamageManagerComponent.Cast(SCR_DamageManagerComponent.GetDamageManager(entity));
		if (!dmgMgr)
			return;
		
		if (!m_DamageEffect)
			return;
		
		// Find effects matching this area's template type (same pattern as SCR_DamageArea.OnAreaExit)
		array<ref SCR_PersistentDamageEffect> effects = {};
		if (dmgMgr.FindAllDamageEffectsOfType(GetDamageEffect().Type(), effects) < 1)
			return;
		
		IEntity owner = GetParent();
		foreach (SCR_PersistentDamageEffect effect : effects)
		{
			SCR_SpecialCollisionDamageEffect collisionEffect = SCR_SpecialCollisionDamageEffect.Cast(effect);
			if (!collisionEffect)
				continue;
			
			if (collisionEffect.GetResponsibleEntity() == owner)
				collisionEffect.Terminate();
		}
	}
	
	//! True if an effect of this type from the given source is already active.
	protected bool HasActiveEffectFrom(SCR_ExtendedDamageManagerComponent dmgMgr, IEntity source)
	{
		array<ref SCR_PersistentDamageEffect> effects = {};
		if (dmgMgr.FindAllDamageEffectsOfType(GetDamageEffect().Type(), effects) < 1)
			return false;
		
		foreach (SCR_PersistentDamageEffect effect : effects)
		{
			SCR_SpecialCollisionDamageEffect collisionEffect = SCR_SpecialCollisionDamageEffect.Cast(effect);
			if (!collisionEffect)
				continue;
			
			if (collisionEffect.GetResponsibleEntity() == source)
				return true;
		}
		
		return false;
	}
}
