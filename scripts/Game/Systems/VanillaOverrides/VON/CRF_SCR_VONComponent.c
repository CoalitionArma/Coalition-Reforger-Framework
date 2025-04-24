modded class SCR_VoNComponent
{
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	
	//------------------------------------------------------------------------------------------------
	void SCR_VoNComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		// Super up so we dont break the component.
		super.OnCapture(transmitter);
		
		// If in game state, dont do this so it isnt a drag on clients fps.
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return;
		
		// Add player to the m_aPlayersTalking array the menu manager uses to tell when a player is talking.
		AddPlayerTalking(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		// Super up so we dont break the component.
		super.OnReceive(playerId, receiver, frequency, quality);
		
		// If in game state, dont do this so it isnt a drag on clients fps.
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return;
		
		// Add player to the m_aPlayersTalking array the menu manager uses to tell when a player is talking.
		AddPlayerTalking(playerId);
	}
	
	protected void AddPlayerTalking(int playerId)
	{		
		// Check if player exists in the array and can be added (very important we do this as OnReceive runs every frame for each client talking to this client).
		if(!m_MenuManager.m_aPlayersTalking.Contains(playerId))
		{
			// Insert player into the m_aPlayersTalking array on the Menu Manager, this makes it so menus (slotting, breifing, aar, etc) show this player as talking.
			m_MenuManager.m_aPlayersTalking.Insert(playerId);
			
			// Remove player as "talking" after a set period of time, need to do this since the SCR_VoNComponent doesn't have a OnReceiveEnd function.
			GetGame().GetCallqueue().CallLater(RemovePlayerTalking, 325, false, playerId);
		};
	}
	
	protected void RemovePlayerTalking(int playerId)
	{		
		// Get place the player is on the m_aPlayersTalking array so we can check then remove them.
		int place = m_MenuManager.m_aPlayersTalking.Find(playerId);
		
		// Check if player exists in the array.
		if(place != -1)
		{
			// Remove player from the m_aPlayersTalking array on the Menu Manager, this makes it so menus (slotting, breifing, aar, etc) no longer show this player as talking.
			m_MenuManager.m_aPlayersTalking.Remove(place);
		};
	}
};