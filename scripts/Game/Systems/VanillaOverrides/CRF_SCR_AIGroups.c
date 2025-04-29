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
		
		GetGame().GetCallqueue().CallLater(InitDelay, 500, false);
	} 
		
	void InitDelay()
	{
		// Check if we need to register this group as playable
		bool shouldRegisterGroup = false;
		
		// Not in GAME state or AI is disabled in GAME state
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
		
		// Register the group if needed
		if (shouldRegisterGroup)
		{
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (groupsManager)
			{
				m_bPlayable = true;
				
				groupsManager.RegisterGroup(this);
				groupsManager.AssignGroupIDUnprotected(this);
				groupsManager.ClaimFrequency(GetRadioFrequency(), GetFaction());
				groupsManager.OnGroupCreated(this);
				
				DeactivateAI();
				SetMaxMembers(12);
				SetCanDeleteIfNoPlayer(false);
				SetDeleteWhenEmpty(false);
			}
		} 
	}
}