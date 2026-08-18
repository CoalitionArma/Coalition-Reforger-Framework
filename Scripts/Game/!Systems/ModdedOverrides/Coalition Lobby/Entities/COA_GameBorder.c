/*
modded class COA_GameBorder : COA_BorderBase
{
	//------------------------------------------------------------------------------------------------
	/** Edit: Trist
	 * Adds OOB effects to players currently outside IF reversed zone. Way the trigger works is it doesnt detect any players OUTSIDE.
	*  Reason: Polyline triggers even if reversed only detect changes 'if inside' so we have to manually check for outside players.
	 * Must manual call this. Is not called via the query/trigger itself. Currently only used in MapStagingComponent when activating reversed zones.
	void CheckPlayersOutside()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		
		foreach (int pid : players)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(pid));
			if (!character)
				continue;
			
			if (!m_aFactionKey.IsEmpty() && character.m_pFactionComponent)
			{
				Faction faction = character.m_pFactionComponent.GetAffiliatedFaction();
				if (faction && m_aFactionKey.Contains(faction.GetFactionKey()))
					continue;
			}
			
			if (m_bAliveOnly && character.GetDamageManager() && character.GetDamageManager().GetState() == EDamageState.DESTROYED)
				continue;

			if (!m_polyZone.IsInsidePolygon(character.GetOrigin()))
			{
				COA_PolyZoneEffectHandler handler = COA_PolyZoneEffectHandler.Cast(character.FindComponent(COA_PolyZoneEffectHandler));
				if (handler)
					handler.AddEffect(this, m_polyZoneEffect);
			}
		}
		
		players.Clear();
		players = null;
	}
}
*/