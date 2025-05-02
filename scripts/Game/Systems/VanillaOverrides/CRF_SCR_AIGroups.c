modded class SCR_AIGroup
{
	//! Called when the entity is initialized
	override void EOnInit(IEntity owner)
	{
		// Call the parent implementation first
		super.EOnInit(owner);
		
		// Skip processing if not in play mode or if gamemode doesn't exist
		if (!GetGame().InPlayMode() || !CRF_Gamemode.GetInstance())
			return;
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		
		// In GAME state and AI is enabled in GAME state
		if (gamemode && groupsManager && gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && gamemode.EnableAIInGameState)
		{
			m_bPlayable = false;
			
			groupsManager.UnregisterGroup(this);
			groupsManager.GetOnPlayableGroupRemoved().Invoke(this);
			
			if (!IsAIActivated())
				ActivateAI();
			
			SetCanDeleteIfNoPlayer(true);
			SetDeleteWhenEmpty(true);
		} else {
			DeactivateAI();
			groupsManager.RegisterGroup(this);
			groupsManager.AssignGroupFrequencyUnprotected(this);
			groupsManager.AssignGroupIdUnprotected(this);
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
	override void RemovePlayer(int playerID)
	{
		// Super up so we dont break the vanilla side
		super.RemovePlayer(playerID);

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