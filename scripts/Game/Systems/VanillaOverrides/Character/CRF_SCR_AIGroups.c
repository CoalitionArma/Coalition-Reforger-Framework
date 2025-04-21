modded class SCR_AIGroup
{
	[Attribute("0", category: "Group")]
	bool m_bIsPlayable;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!GetGame().InPlayMode() || !CRF_Gamemode.GetInstance())
			return;
		
		GetGame().GetCallqueue().CallLater(CheckIfPlayableOnInit, 150, false);
	}
	
	protected void CheckIfPlayableOnInit()
	{
		CRF_PlayableCharacter playableChar = null;
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		array<AIAgent> OutAgents = {};
		GetAgents(OutAgents);
		
		if(!OutAgents.IsEmpty())
		{
			AIAgent agent = OutAgents.Get(0);
			
			if(agent)
				playableChar = CRF_PlayableCharacter.Cast(agent.GetControlledEntity().FindComponent(CRF_PlayableCharacter));
			
			if(gamemode && agent && playableChar && gamemode.m_GamemodeState == CRF_GamemodeState.GAME && gamemode.EnableAIInGameState && playableChar.IsPlayable())
				m_bIsPlayable = false;
		};
		
		#ifdef WORKBENCH
		if (m_bIsPlayable)
			GetGame().GetCallqueue().CallLater(SaveAIGRoup, 500, false);
		#else
		if (RplSession.Mode() == RplMode.Dedicated && m_bIsPlayable)
			GetGame().GetCallqueue().CallLater(SaveAIGRoup, 500, false);
		#endif
	}
	
	//Saves the group on the server
	void SaveAIGRoup()
	{
		CRF_Gamemode.GetInstance().AddGroup(this);
	}
	
	override void OnEmpty()
	{
		Event_OnEmpty.Invoke(this);

		//--- Delete after delay, doing it directly in this event would be unsafe
		if (m_bDeleteWhenEmpty && !m_bIsPlayable)
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1, false, this);		
	}
}