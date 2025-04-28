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
		
		// Check if we need to register this group as playable
		bool shouldRegisterGroup = false;
		
		// Not on dedicated server and either not in GAME state or AI is disabled in GAME state
		if (RplSession.Mode() != RplMode.Dedicated)
		{
			CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
			if (gamemode)
			{
				if (gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
				{
					shouldRegisterGroup = true;
				}
				else if (!gamemode.EnableAIInGameState)
				{
					shouldRegisterGroup = true;
				}
			}
		}
		
		// Register the group if needed
		if (shouldRegisterGroup)
		{
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (groupsManager)
			{
				m_bPlayable = true;
				DeactivateAI();
				SetMaxMembers(12);
				SetCanDeleteIfNoPlayer(false);
				SetDeleteWhenEmpty(false);
				
				groupsManager.RegisterGroup(this);
				groupsManager.ClaimFrequency(GetRadioFrequency(), GetFaction());
				groupsManager.AssignGroupIDUnprotected(this);
				groupsManager.OnGroupCreated(this);
			}
		} 
	}
}

modded class SCR_GroupsManagerComponent
{
	//------------------------------------------------------------------------------------------------
	void AssignGroupIDUnprotected(SCR_AIGroup group)
	{
		group.SetGroupID(m_iLatestGroupID);
		m_iLatestGroupID++;
	}
}