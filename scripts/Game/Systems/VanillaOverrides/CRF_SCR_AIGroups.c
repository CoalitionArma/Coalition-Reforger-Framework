modded class SCR_AIGroup
{
	[Attribute("0", category: "Group")]
	protected bool m_bIsPlayable;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType), category: "Group")]
	protected CRF_EFlagType m_FlagType;
	
	[Attribute("1", category: "Group")]
	bool m_bBlueForceTrackerEnabled;
	
	protected bool m_bIsPlayableGroup;
	protected SCR_AIGroup m_NewGroup;
	
	[RplProp()] ref array<ResourceName> m_aGroupSlots = {};
	
	//------------------------------------------------------------------------------------------------
	//! Called when the entity is initialized
	override void EOnInit(IEntity owner)
	{
		// Call the parent implementation first
		super.EOnInit(owner);
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		
		// Skip processing if not in play mode or if gamemode doesn't exist
		if (!IsGroupPlayable() || !GetGame().InPlayMode() || !gamemode || !groupsManager || !Replication.IsServer())
			return;
		
		// In GAME state and AI is enabled in GAME state
		if (gamemode && groupsManager && gamemode.m_GamemodeState == CRF_EGamemodeState.GAME && gamemode.m_bCurrentEnableAIInGameState)
		{
			if (!IsAIActivated())
				ActivateAI();
			
			SetCanDeleteIfNoPlayer(true);
			SetDeleteWhenEmpty(true);
		} else {
			GetOnAllDelayedEntitySpawned().Insert(AllMembersSpawned);
			GetGame().GetCallqueue().CallLater(CreateNewGroup, 150, false); // DO NOT CHANGE. RPL JIP ERROR IF NOT INIT'd AFTER (LOL FUCK THIS ENGINE)
		};
		
		
	}
	
	void SetGroupSlots(array<ResourceName> slots)
	{
		m_aGroupSlots = slots;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsGroupPlayable()
	{
		return m_bIsPlayable;
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateNewGroup()
	{
		if(m_bIsPlayableGroup)
			return;
		
		SCR_Faction scrFaction = SCR_Faction.Cast(GetFaction());
		if(scrFaction && scrFaction.GetFlagName(0))
		{
			TStringArray flagArray = {};
			scrFaction.GetFlagNames(flagArray);
			if((flagArray.Count() - 1) < m_FlagType)
				m_FlagType = CRF_EFlagType.INFANTRY
		}
		
		m_NewGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(GetFaction());
		m_NewGroup.SetFaction(GetFaction());
		m_NewGroup.SetGroupFlag(m_FlagType, true);
		m_NewGroup.SetCanDeleteIfNoPlayer(false);
		m_NewGroup.SetDeleteWhenEmpty(false);
		m_NewGroup.SetMaxMembers(GetMaxMembers());
		m_NewGroup.SetIsPlayableGroup();
		
		m_NewGroup.SetGroupSlots(m_aUnitPrefabSlots);
	}
	
	//------------------------------------------------------------------------------------------------
	void AllMembersSpawned(SCR_AIGroup group)
	{
		GetGame().GetCallqueue().CallLater(ConvertSlotsToNewGroup, 350, false);
		GetOnAllDelayedEntitySpawned().Remove(AllMembersSpawned);
	};
	
	//------------------------------------------------------------------------------------------------
	void ConvertSlotsToNewGroup()
	{
		if (m_bIsPlayableGroup)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		RplId newRplId = RplComponent.Cast(m_NewGroup.FindComponent(RplComponent)).Id();
		RplId oldRplId = RplComponent.Cast(this.FindComponent(RplComponent)).Id();
		
		array<int> slotIdsForOldGroup = slottingManager.GetAllSlotIDsForGroup(oldRplId);
		
		foreach(int slotId : slotIdsForOldGroup)
			slottingManager.UpdateSlotGroup(slotId, newRplId);

		SCR_EntityHelper.DeleteEntityAndChildren(this);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsPlayableGroup()
	{
		m_bIsPlayableGroup = true;
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