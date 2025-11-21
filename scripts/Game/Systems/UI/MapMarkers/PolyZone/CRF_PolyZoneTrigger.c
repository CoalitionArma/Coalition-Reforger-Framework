class CRF_PolyZoneTriggerClass: SCR_BaseTriggerEntityClass
{
	
}

class CRF_PolyZoneTrigger : SCR_BaseTriggerEntity
{
	CRF_PolyZone m_polyZone;
	
	[Attribute()]
	ref CRF_PolyZoneEffect m_polyZoneEffect;
	
	[Attribute("0")]
	bool m_bReversed;
	
	[Attribute("0")]
	bool m_bAliveOnly;
	
	[Attribute("1")]
	bool m_bHelisNotRestricted;
	
	[Attribute("")]
	ref array<FactionKey> m_aFactionKey;
	
	[Attribute("")]
	string m_sGroupKey;
	
	override void OnInit(IEntity owner)
	{
		m_polyZone = CRF_PolyZone.Cast(owner.GetParent().FindComponent(CRF_PolyZone));
	}
	
	override bool ScriptedEntityFilterForQuery(IEntity ent)
	{
		if (!m_polyZone)
			return true;
		
		if (!m_polyZone.IsInsidePolygon(ent.GetOrigin()))
			return false;
		
		if (m_bAliveOnly || !m_aFactionKey.IsEmpty() || m_sGroupKey != "")
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
			if (m_sGroupKey != "" && !character)
				return false;
			
			Vehicle vehicle = Vehicle.Cast(ent);
			SCR_DamageManagerComponent damageManager;
			FactionAffiliationComponent factionAffiliation;
			SCR_AIGroup aiGroup;
			
			if (vehicle)
			{
				damageManager = vehicle.GetDamageManager();
				factionAffiliation = vehicle.GetFactionAffiliation();
			}
			
			if (character)
			{
				damageManager = character.GetDamageManager();
				factionAffiliation = character.m_pFactionComponent;
				
				ChimeraAIControlComponent aiControlComp = ChimeraAIControlComponent.Cast(
					character.FindComponent(ChimeraAIControlComponent)
				);
				
				AIAgent aiAgent = aiControlComp.GetAIAgent();
				if (aiAgent)
					aiGroup = SCR_AIGroup.Cast(aiAgent.GetParentGroup());
			}
			
			if (m_bAliveOnly)
				damageManager = SCR_DamageManagerComponent.Cast(ent.FindComponent(SCR_DamageManagerComponent));
			
			if (damageManager)
				if (m_bAliveOnly && damageManager.GetState() == EDamageState.DESTROYED)
					return false;
			
			if (!factionAffiliation)
				return false;
			
			if (!m_aFactionKey.IsEmpty() && m_aFactionKey.Contains(factionAffiliation.GetDefaultAffiliatedFaction().GetFactionKey()))
				return false;
			
			if (m_sGroupKey != "")
			{
				if (!aiGroup)
					return false;
				
				if (!aiGroup.GetName().Contains(m_sGroupKey))
					return false;
			}
		}
		
		return true;
	}
	
	override protected void OnActivate(IEntity ent)
	{
		if (!m_polyZoneEffect)
			return;
		
		CRF_PolyZoneEffectHandler polyZoneEffectHandler = CRF_PolyZoneEffectHandler.Cast(ent.FindComponent(CRF_PolyZoneEffectHandler));
		if (!polyZoneEffectHandler)
			return;
		
		if (m_bReversed)
			polyZoneEffectHandler.RemoveEffect(this, m_polyZoneEffect);
		else
			polyZoneEffectHandler.AddEffect(this, m_polyZoneEffect);
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		if (!m_polyZoneEffect)
			return;
		
		if (CRF_GamemodeManager.IsSpectator(ent))
			return;
		
		// Allow Admins to teleport out of the game borders during safestart
		if (CRF_SafestartManager.GetInstance().GetSafestartStatus() && SCR_Global.IsAdmin(GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ent)))
			return;
		
		CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(ent.FindComponent(CompartmentAccessComponent)); // TODO nullcheck
		if (compAccess)
		{
			BaseCompartmentSlot compartment = compAccess.GetCompartment();
			if (compartment)
			{
				VehicleHelicopterSimulation heli = VehicleHelicopterSimulation.Cast(compartment.GetVehicle().FindComponent(VehicleHelicopterSimulation));
				
				if(heli && m_bHelisNotRestricted)
					return;
			}
		}
		
		CRF_PolyZoneEffectHandler polyZoneEffectHandler = CRF_PolyZoneEffectHandler.Cast(ent.FindComponent(CRF_PolyZoneEffectHandler));
		if (!polyZoneEffectHandler)
			return;
		
		if (m_bReversed)
			polyZoneEffectHandler.AddEffect(this, m_polyZoneEffect);
		else
			polyZoneEffectHandler.RemoveEffect(this, m_polyZoneEffect);
	}
}