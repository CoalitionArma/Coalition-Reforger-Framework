[BaseContainerProps()]
class CRF_PolyZoneEffectScreenBlure : CRF_PolyZoneEffect
{
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
		effect.m_fTime = 10000;
		effect.m_iType = CRF_EPolyZoneEffectHUDType.ScreenBlure;
		return effect;
	}
	
	override CRF_PolyZoneEffect CreateCopyObject()
	{
		return new CRF_PolyZoneEffectScreenBlure();
	}
	
	override void CopyFields(CRF_PolyZoneEffect effect)
	{
		CRF_PolyZoneEffectScreenBlure effectCurrent = CRF_PolyZoneEffectScreenBlure.Cast(effect);
	}
}