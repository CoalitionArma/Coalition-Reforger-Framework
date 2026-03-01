class CRF_EntityHelper
{	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Create spawn parameters for entity
	 * @param entity Entity to create parameters for
	 * @return Spawn parameters
	 */
	static EntitySpawnParams CreateSpawnParams(IEntity entity)
	{	
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entity.GetOrigin();
		return spawnParams;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Determine faction key from faction affiliation comp
	 * @param entity Entity to pull the faction comp of
	 * @return Faction key or empty string if not found
	 */
	static FactionKey DetermineFactionKey(IEntity entity)
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (!facComp)
			return "CIV";
		
		return facComp.GetAffiliatedFactionKey();
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to get group from RplId
	static SCR_AIGroup GetGroupFromRplId(RplId groupId)
	{
		if (groupId == RplId.Invalid())
			return null;

		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(groupId));
		if (!rplComp)
			return null;

		return SCR_AIGroup.Cast(rplComp.GetEntity());
	}

	//------------------------------------------------------------------------------------------------
	// Helper method to get character from RplId
	static SCR_ChimeraCharacter GetCharacterFromRplId(RplId charId)
	{
		if (charId == RplId.Invalid())
			return null;

		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(charId));
		if (!rplComp)
			return null;

		return SCR_ChimeraCharacter.Cast(rplComp.GetEntity());
	}
}