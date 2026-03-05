
class CRF_PolyZoneFactionCheckTriggerClass: CRF_PolyZoneTriggerClass
{
	
}

class CRF_PolyZoneFactionCheckTrigger : CRF_PolyZoneTrigger
{
	[Attribute()]
	FactionKey m_FactionKey;
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!IsAlive(ent))
			return false;
		
		FactionAffiliationComponent factionAffiliationComponent = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!factionAffiliationComponent)
			return false;
		
		if (factionAffiliationComponent.GetDefaultAffiliatedFaction().GetFactionKey() != m_FactionKey)
			return false;
		
		if (!m_polyZone.IsInsidePolygon(ent.GetOrigin()))
			return false;
		
		return true;
	}
}