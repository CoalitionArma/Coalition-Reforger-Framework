modded class SCR_VoNComponent
{
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	
	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	// Constructor - Initializes component and gets required manager instances
	//------------------------------------------------------------------------------------------------
	void SCR_VoNComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		// Get singleton instances needed for voice functionality
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when the local player activates their microphone
	//------------------------------------------------------------------------------------------------
	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		// Call parent implementation to maintain core functionality
		super.OnCapture(transmitter);
		
		// Skip processing during active gameplay to prevent FPS impact
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return;
		
		// Register local player as currently talking
		AddPlayerTalking(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when receiving voice from another player
	//------------------------------------------------------------------------------------------------
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		// Call parent implementation to maintain core functionality
		super.OnReceive(playerId, receiver, frequency, quality);
		
		// Skip processing during active gameplay to prevent FPS impact
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return;
		
		// Register the remote player as currently talking
		AddPlayerTalking(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	// Adds a player to the list of currently talking players
	//------------------------------------------------------------------------------------------------
	protected void AddPlayerTalking(int playerId)
	{		
		// Only add if not already in the list (important as OnReceive runs every frame)
		if(!m_MenuManager.m_aPlayersTalking.Contains(playerId))
		{
			// Add player to the list to update UI indicators in various menus
			m_MenuManager.m_aPlayersTalking.Insert(playerId);
			
			// Schedule removal after a delay since there's no OnReceiveEnd event
			// 325ms timeout provides a buffer to handle voice transmission gaps
			GetGame().GetCallqueue().CallLater(RemovePlayerTalking, 325, false, playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Removes a player from the list of talking players after timeout
	//------------------------------------------------------------------------------------------------
	protected void RemovePlayerTalking(int playerId)
	{		
		// Find player's position in the array
		int playerIndex = m_MenuManager.m_aPlayersTalking.Find(playerId);
		
		// Only remove if player is actually in the list
		if(playerIndex != -1)
		{
			// Remove player to update UI indicators
			m_MenuManager.m_aPlayersTalking.Remove(playerIndex);
		}
	}
};