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
	
	//------------------------------------------------------------------------------------------------
	/** Edit: Trist
	 * Adds OOB effects to players currently outside IF reversed zone. Way the trigger works is it doesnt detect any players OUTSIDE.
	*  Reason: Polyline triggers even if reversed only detect changes 'if inside' so we have to manually check for outside players.
	 * Must manual call this. Is not called via the query/trigger itself. Currently only used in MapStagingComponent when activating reversed zones.
	 */
	void CheckPlayersOutside()
	{
		if (!m_bReversed || !m_polyZoneEffect || !m_polyZone)
			return;
		
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		
		foreach (int pid : players)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(pid));
			if (!character)
				continue;
			
			if (!m_aFactionKey.IsEmpty() && character.m_pFactionComponent)
			{
				Faction faction = character.m_pFactionComponent.GetDefaultAffiliatedFaction();
				if (faction && m_aFactionKey.Contains(faction.GetFactionKey()))
					continue;
			}
			
			if (m_bAliveOnly && character.GetDamageManager() && character.GetDamageManager().GetState() == EDamageState.DESTROYED)
				continue;

			if (!m_polyZone.IsInsidePolygon(character.GetOrigin()))
			{
				CRF_PolyZoneEffectHandler handler = CRF_PolyZoneEffectHandler.Cast(character.FindComponent(CRF_PolyZoneEffectHandler));
				if (handler)
					handler.AddEffect(this, m_polyZoneEffect);
			}
		}
		
		players.Clear();
		players = null;
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
			
			if (!m_aFactionKey.IsEmpty() && m_aFactionKey.Contains(factionAffiliation.GetAffiliatedFactionKey()))
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