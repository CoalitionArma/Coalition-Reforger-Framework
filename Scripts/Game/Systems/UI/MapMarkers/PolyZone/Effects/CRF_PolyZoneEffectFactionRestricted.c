[BaseContainerProps()]
class CRF_PolyZoneEffectRestricted : CRF_PolyZoneEffect
{
	[Attribute("10")]
	float m_fKillTime;
	
	bool m_bTriggerd = false;
	
	override void OnFrame(CRF_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		m_fKillTime -= timeSlice;
		if (m_fKillTime <= 0 && !m_bTriggerd)
		{
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
			if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
				return;
			damageManager.SetHealthScaled(0);
		}
	}
	
	override void OnActivate(CRF_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override void OnDeactivate(CRF_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	override CRF_EffectContainer GetEffectContainer()
	{
		CRF_EffectContainer effect = new CRF_EffectContainer();
		effect.m_iId = m_iId;
		effect.m_fTime = m_fKillTime;
		effect.m_iType = CRF_EPolyZoneEffectHUDType.RestrictedZone;
		return effect;
	}
	
	override CRF_PolyZoneEffect CreateCopyObject()
	{
		return new CRF_PolyZoneEffectRestricted();
	}
	
	override void CopyFields(CRF_PolyZoneEffect effect)
	{
		CRF_PolyZoneEffectRestricted effectCurrent = CRF_PolyZoneEffectRestricted.Cast(effect);
		effectCurrent.m_fKillTime = m_fKillTime;
	}
}