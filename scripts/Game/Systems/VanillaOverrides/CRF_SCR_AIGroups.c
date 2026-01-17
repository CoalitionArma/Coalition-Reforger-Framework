modded class SCR_AIGroup
{
	[Attribute("0")]
	bool m_bIsGarrisonGroup;
	
	//------------------------------------------------------------------------------------------------
	//! Called when the entity is initialized
	override void EOnInit(IEntity owner)
	{
		// Call the parent implementation first
		super.EOnInit(owner);
		
		if (m_bIsGarrisonGroup)
			GetGame().GetCallqueue().CallLater(SetGarrison, 1000, false);
	}
	
	void SetGarrison()
	{
		array<AIAgent> agents = {};
		GetAgents(agents);
		foreach (AIAgent agent: agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			if (!SCR_ChimeraCharacter.Cast(entity))
				continue;
			
			SCR_ChimeraCharacter.Cast(entity).GetCharacterController().SetDisableMovementControls(true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void AddPlayer(int playerID)
	{
		// Super up so we dont break the vanilla side
		super.AddPlayer(playerID);

		// Get the current leader entity
		PlayerManager playerManager = GetGame().GetPlayerManager();
		IEntity currentLeaderEntity = playerManager.GetPlayerControlledEntity(GetLeaderID());

		// Check if leader entity exists
		if (!currentLeaderEntity)
			return;

		// If current leader is not a squad leader role
		if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
		{
			// Get joining player entity
			IEntity player = playerManager.GetPlayerControlledEntity(playerID);

			// Check if player entity exists
			if (!player)
				return;

			// If joining player has squad leader role, make them the new leader
			if (CRF_RoleHelper.IsSquadLeaderRole(player))
			{
				SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
				groupsManager.SetGroupLeader(GetGroupID(), playerID);
			}
		}
	}

	//------------------------------------------------------------------------------------------------	
	// Removes the "x left your group" upon death or anything else. Fucking stupid tbh.
	// This is a carbon copy of the method and may break if it changes in an update
	override void RemovePlayer(int playerID) 
	{
		if (!m_aPlayerIDs.Contains(playerID))
			return;

		//if player is last in group it doesnt matter as the group will get deleted
		if (playerID == m_iLeaderID && GetPlayerCount() > 1)
		{
			SetCustomName("", 0);
			SetCustomDescription("", 0);
		}

		RPC_DoRemovePlayer(playerID);
		Rpc(RPC_DoRemovePlayer, playerID);
		CheckForLeader(-1, false);
		RemovePlayerAgent(playerID);
		//SCR_NotificationsComponent.SendToGroup(m_iGroupID, ENotification.GROUPS_PLAYER_LEFT, playerID);
		
		// End of original method (aka super) ===========================================
		
		// Get player manager
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// Get the current group leader entity
		int leaderID = GetLeaderID();
		IEntity currentLeaderEntity = playerManager.GetPlayerControlledEntity(leaderID);

		// Check if leader entity exists
		if (!currentLeaderEntity)
			return;

		// If current leader is not a squad leader, find a team leader to promote
		if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
		{
			// Get all group members
			array<int> groupMembers = GetPlayerIDs();
			if (!groupMembers || groupMembers.IsEmpty())
				return;

			// Get groups manager component
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (!groupsManager)
				return;

			// Look for a team leader to promote
			for (int i = 0; i < groupMembers.Count(); i++)
			{
				int member = groupMembers[i];
				IEntity memberEntity = playerManager.GetPlayerControlledEntity(member);

				if (!memberEntity)
					continue;

				if (CRF_RoleHelper.IsTeamLeaderRole(memberEntity))
				{
					groupsManager.SetGroupLeader(GetGroupID(), member);
					break;
				}
			}
		}
	}
}