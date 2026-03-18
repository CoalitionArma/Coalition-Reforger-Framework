class CRF_PlayerHelper
{
	//------------------------------------------------------------------------------------------------
	//! Remove player from their current group if any
	//! \param[in] playerId ID of the player to remove from group
	static void RemovePlayerFromCurrentGroup(int playerId)
	{
		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign faction to player controller
	//! \param[in] playerController Player controller to assign faction to
	//! \param[in] faction Faction to assign
	static void AssignFactionToPlayer(SCR_PlayerController playerController, Faction faction)
	{
		if (!faction || !playerController)
			return;
			
		SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
		);
		
		if (affiliationComponent)
			affiliationComponent.RequestFaction(faction);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign character entity to player controller
	//! \param[in] playerId ID of the player to assign the character to
	//! \param[in] character Character to assign
	static void AssignCharacterToPlayer(SCR_PlayerController playerController, CRF_PlayerCharacter character)
	{
		int playerId = playerController.GetPlayerId();
		if (playerId <= 0)
			return;
		
		// Route spectator assignment through the base game pipeline, same as playable characters
		SCR_RespawnComponent respawnComponent = SCR_RespawnComponent.Cast(
			GetGame().GetPlayerManager().GetPlayerRespawnComponent(playerId)
		);
		
		if (respawnComponent)
		{
			SCR_PossessSpawnData spawnData = SCR_PossessSpawnData.FromEntity(character);
			
			// Check if handler is available before using RequestSpawn
			// This prevents NULL pointer errors during early initialization
			bool canUseRequestSpawn = false;
			
			array<GenericComponent> components = {};
			respawnComponent.FindComponents(SCR_SpawnRequestComponent, components);
			
			foreach (GenericComponent comp : components)
			{
				SCR_SpawnRequestComponent requestComp = SCR_SpawnRequestComponent.Cast(comp);
				if (requestComp && requestComp.GetDataType() == SCR_PossessSpawnData && requestComp.GetHandlerComponent())
				{
					canUseRequestSpawn = true;
					break;
				}
			}
			
			if (canUseRequestSpawn)
			{
				if (!respawnComponent.RequestSpawn(spawnData))
					Print(string.Format("[CRF_GamemodeManager] WARNING: RequestSpawn failed for spectator, player %1", playerId), LogLevel.WARNING);
			} else
				playerController.SetInitialMainEntity(character);
		}
		else
		{
			// Fallback for very early init
			Print(string.Format("[CRF_GamemodeManager] No SCR_RespawnComponent for player %1 — using SetInitialMainEntity", playerId), LogLevel.WARNING);
			playerController.SetInitialMainEntity(character);
		}
	}
}