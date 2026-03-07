class CRF_EntityHelper
{	
	// Spectator resource to use
	static const ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/!Spectator/CRF_Spectator.et";
	
	// NEVER EVER SPAWN AN ENT WITH A PURE 0 WORLD VECTOR OR ELSE I WILL CASTRATE YOU I STG - Njpatman
	static const vector ZERO_SPAWN_VECTOR[4] = { "1 0 0", "0 1 0", "0 0 1", "0 0 0" };

	//------------------------------------------------------------------------------------------------
	//! Get the spectator resource name
	//! \return ResourceName of the spectator entity
	static ResourceName GetSpectatorResource()
	{
		return SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Validate the proveied vector to ensure we arent spawning in a state that would cause errors or crashes
	//! \param[in] vectorToCheck vector to check
	//! \return true if the vector was valid
	static bool IsValidSpawnVector(vector vectorToCheck)
	{	
		bool finalcheck = false;
		bool zeroCheck = (vector.Distance(ZERO_SPAWN_VECTOR[3], vectorToCheck) > 5);
		bool tenCheck = (vector.Distance("0 0 0", vectorToCheck) > 5);
		bool negCheck = (vectorToCheck[1] >= 0);
		
		if (zeroCheck && tenCheck && negCheck)
			finalcheck = true;
		
		return finalcheck;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create spawn parameters for entity
	//! \param[in] entityPos Entity position to set parameters
	//! \return Spawn parameters
	static EntitySpawnParams CreateSpawnParams(vector entityPos[4])
	{	
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = entityPos;
		return spawnParams;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create spawn parameters for entity (This one only needs one vector in the matrix)
	//! \param[in] entityPos Entity position to set parameters
	//! \return Spawn parameters
	static EntitySpawnParams CreateSpawnParams(vector entityPos)
	{	
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = entityPos;
		return spawnParams;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if a given entity is a spectator
	//! \param[in] entity Entity to check
	//! \return True if entity is a spectator, false otherwise
	static bool IsSpectator(IEntity entity)
	{
		if (!entity)
			return false;
		
		return entity.GetPrefabData().GetPrefabName() == GetSpectatorResource();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if the local player is a spectator
	//! \return True if local player is a spectator, false otherwise
	static bool IsSpectator()
	{
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		if (mainEntity && mainEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;

		return false;
	}

	
	//------------------------------------------------------------------------------------------------
	//! Determine faction key from faction affiliation comp of the provided entity
	//! \param[in] entity Entity to pull the faction comp of
	//! \return Faction key or empty string if not found
	static FactionKey DetermineFactionKey(IEntity entity)
	{
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		
		if (!facComp)
			return "CIV";
		
		return facComp.GetAffiliatedFactionKey();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper method to get group from RplId
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
	//! Helper method to get character from RplId
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