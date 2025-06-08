[BaseContainerProps()]
class CRF_PolyZoneEffect
{
	static int m_iLastId;
	int m_iId;
	
	void OnFrame(CRF_PolyZoneEffectHandler handler, IEntity ent, float timeSlice)
	{
		
	}
	
	void OnActivate(CRF_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	void OnDeactivate(CRF_PolyZoneEffectHandler handler, IEntity ent)
	{
		
	}
	
	CRF_PolyZoneEffect CreateCopyObject()
	{
		return new CRF_PolyZoneEffect();
	}
	
	void CopyFields(CRF_PolyZoneEffect effect)
	{
		
	}
	
	CRF_EffectContainer GetEffectContainer()
	{
		CRF_EffectContainer effect = new CRF_EffectContainer();
		return effect;
	}
	
	CRF_PolyZoneEffect Copy()
	{
		CRF_PolyZoneEffect copy = CreateCopyObject();
		CopyFields(copy);
		return copy;	
	}
	
	void CRF_PolyZoneEffect()
	{
		m_iLastId++;
		m_iId = m_iLastId;
	}
}
class CRF_EffectContainer
{
	int m_iId;
	CRF_EPolyZoneEffectHUDType m_iType;
	float m_fTime;
	string m_sString;
}