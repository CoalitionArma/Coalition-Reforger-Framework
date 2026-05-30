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
	//! Assign character entity to player controller.
	//! CRF pre-spawns all characters before calling InitilizePlayer, so SetInitialMainEntity is
	//! always used directly. The RequestSpawn path was previously attempted here but caused
	//! intermittent stats-tracking failures: the vanilla AssignEntity_S guard cancels the entire
	//! spawn finalization (and therefore never fires OnPlayerSpawnFinalize_S) whenever the player
	//! already controls the target entity — which happens on reconnects and re-initializations.
	//! Callers are always responsible for calling NotifyPlayerSpawned after this method returns.
	//! \param[in] playerController Player Controller to assign character to
	//! \param[in] character Character to assign to player
	static void AssignCharacterToPlayer(SCR_PlayerController playerController, CRF_PlayerCharacter character)
	{
		playerController.SetInitialMainEntity(character);
	}
}