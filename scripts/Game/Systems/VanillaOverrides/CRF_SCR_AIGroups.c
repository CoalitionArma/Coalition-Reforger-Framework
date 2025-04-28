modded class SCR_AIGroup
{
	//! Flag indicating if this AI group can be played by players
	[Attribute("0", category: "Group")]
	bool m_bIsPlayable;
	
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
				groupsManager.RegisterGroup(this);
				groupsManager.ClaimFrequency(GetRadioFrequency(), GetFaction());
				groupsManager.OnGroupCreated(this);
			}
		}
		
		// If in GAME state, check if this group is playable after a short delay
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode && gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			GetGame().GetCallqueue().CallLater(CheckIfPlayableOnInit, 100, false);
		}
	}
	
	//! Checks if this group should be playable after initialization
	protected void CheckIfPlayableOnInit()
	{
		// Initialize variables
		CRF_PlayableCharacter playableChar = null;
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		// Get all agents in this group
		array<AIAgent> outAgents = {};
		GetAgents(outAgents);
		
		// If we have at least one agent, check if it's playable
		if (!outAgents.IsEmpty())
		{
			// Get the first agent
			AIAgent agent = outAgents.Get(0);
			
			// If agent exists, try to find its playable character component
			if (agent)
			{
				IEntity controlledEntity = agent.GetControlledEntity();
				if (controlledEntity)
				{
					playableChar = CRF_PlayableCharacter.Cast(controlledEntity.FindComponent(CRF_PlayableCharacter));
				}
			}
			
			// Determine if this group should be playable
			if (gamemode && agent && playableChar)
			{
				if (gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
				{
					if (gamemode.EnableAIInGameState)
					{
						if (playableChar.IsPlayable())
						{
							m_bIsPlayable = false;
						}
					}
				}
			}
		}
	}
	
	//! Called when the group becomes empty (no agents left)
	override void OnEmpty()
	{
		// Invoke the empty event
		Event_OnEmpty.Invoke(this);

		// Delete the group after a short delay if set to delete when empty,
		// but only if it's not marked as playable
		if (m_bDeleteWhenEmpty && !m_bIsPlayable)
		{
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1, false, this);
		}
	}
}