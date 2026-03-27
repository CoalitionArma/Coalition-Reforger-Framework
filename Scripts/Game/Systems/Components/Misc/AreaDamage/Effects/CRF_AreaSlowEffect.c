//! Persistent slow/movement-restriction effect (no damage). Pair with CRF_AreaDamageArea.
//! Inherits speed limiting + jump/climb blocking from SCR_SpecialCollisionDamageEffect.
//! Anti-stacking via HijackDamageEffect prevents duplicates from the same source.

[BaseContainerProps()]
class CRF_AreaSlowEffect : SCR_SpecialCollisionDamageEffect
{
	
	//! Anti-stacking: reject if same source already has an active effect on this character.
	override bool HijackDamageEffect(SCR_ExtendedDamageManagerComponent dmgManager)
	{
		// Collision effects require a responsible entity for mobility limits
		if (!m_ResponsibleEntity)
			return true;
		
		// Check for existing effects of this exact type from the same source
		array<ref SCR_PersistentDamageEffect> existingEffects = {};
		dmgManager.FindAllDamageEffectsOfType(Type(), existingEffects);
		
		foreach (SCR_PersistentDamageEffect existing : existingEffects)
		{
			SCR_SpecialCollisionDamageEffect collisionEffect = SCR_SpecialCollisionDamageEffect.Cast(existing);
			if (!collisionEffect)
				continue;
			
			if (collisionEffect.GetResponsibleEntity() == m_ResponsibleEntity)
				return true; // Already has an effect from this source — discard duplicate
		}
		
		return false;
	}
	
	override void OnEffectAdded(SCR_ExtendedDamageManagerComponent dmgManager)
	{
		super.OnEffectAdded(dmgManager);
	}
	
}
