class CRF_PlayerHelper
{
	//------------------------------------------------------------------------------------------------
	/**
	* Assign faction to player controller
	* @param playerController Player controller to assign faction to
	* @param faction Faction to assign
	*/
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
	/**
	* Assign character entity to player controller
	* @param playerController Player controller to assign character to
	* @param character Character to assign
	*/
	static void AssignCharacterToPlayer(SCR_PlayerController playerController, SCR_ChimeraCharacter character)
	{
		if (!character || !playerController)
			return;
		
		playerController.SetInitialMainEntity(character);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Remove player from their current group if any
	* @param playerId ID of the player to remove from group
	*/
	static void RemovePlayerFromCurrentGroup(int playerId)
	{
		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
	}
}