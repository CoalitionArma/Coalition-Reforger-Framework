[ComponentEditorProps(category: "GameScripted/Character", description: "Add label for observers", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_PolyZoneEffectHandlerClass: ScriptComponentClass
{
	
}

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class CRF_PolyZoneEffectHandler : ScriptComponent
{
	ref map<CRF_PolyZoneTrigger, ref CRF_PolyZoneEffect> m_mapPolyZoneEffects = new map<CRF_PolyZoneTrigger, ref CRF_PolyZoneEffect>();
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		foreach (CRF_PolyZoneTrigger zone, CRF_PolyZoneEffect effect : m_mapPolyZoneEffects)
		{
			effect.OnFrame(this, owner, timeSlice);
		}
	}
	
	void AddEffect(CRF_PolyZoneTrigger zone, CRF_PolyZoneEffect effect)
	{
		if (!Replication.IsServer()) return;
		CRF_PolyZoneEffect effectCopy = effect.Copy();
		effectCopy.OnActivate(this, GetOwner());
		m_mapPolyZoneEffects.Insert(zone, effectCopy);
	}
	
	void RemoveEffect(CRF_PolyZoneTrigger zone, CRF_PolyZoneEffect effect)
	{
		if (!Replication.IsServer()) return;
		if (!m_mapPolyZoneEffects.Contains(zone))
			return;
		m_mapPolyZoneEffects[zone].OnDeactivate(this, GetOwner());
		m_mapPolyZoneEffects.Remove(zone);
	}
	
	void RemoveByEffect(CRF_PolyZoneEffect effectForRemove)
	{
		if (!Replication.IsServer()) return;
		foreach (CRF_PolyZoneTrigger zone, CRF_PolyZoneEffect effect : m_mapPolyZoneEffects)
		{
			if (effectForRemove != effect)
				continue;
			RemoveEffect(zone, effect);
			return;
		}
	}
	
	void ClearAllEffects()
	{
		if (!Replication.IsServer()) return;
		m_mapPolyZoneEffects.Clear();
	}
}












