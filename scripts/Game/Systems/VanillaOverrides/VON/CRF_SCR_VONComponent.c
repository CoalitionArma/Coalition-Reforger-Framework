modded class SCR_VoNComponent
{
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	
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
		// Check if this is direct speech in spectator mode that should be restricted
		if (!m_Gamemode || m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			// Check if this is direct speech (no radio transmitter) in spectator mode
			if (!transmitter)
			{
				// Get local player's channel
				int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
				int localChannel;
				if (m_MenuManager)
					localChannel = m_MenuManager.GetChannel(localPlayerId);
				else
					localChannel = 0;
				
				// Only allow direct speech if in default channels (0 or 1)
				// For custom channels (>1), block direct speech to force radio usage
				if (localChannel > 1)
				{
					// Block direct speech by not calling parent
					// Only register for UI display purposes
					AddPlayerTalking(localPlayerId);
					return;
				}
			}
		}
		
		// Call parent implementation to maintain core functionality
		super.OnCapture(transmitter);
		
		// Skip processing during active gameplay to prevent FPS impact
		if (!m_Gamemode || m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return;
		
		// Register local player as currently talking
		AddPlayerTalking(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when receiving voice from another player
	//------------------------------------------------------------------------------------------------
	override protected event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		// Filter spectator direct speech based on channel membership
		if (!m_Gamemode || m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			// Check if this is direct speech (no radio receiver) in spectator mode
			if (!receiver && IsSpectatorDirectSpeechFiltered(playerId))
			{
				// Block direct speech reception by not calling parent
				// Only register for UI display purposes
				AddPlayerTalking(playerId);
				return;
			}
		}
		
		// Call parent implementation to maintain core functionality
		super.OnReceive(playerId, receiver, frequency, quality);
		
		// Skip processing during active gameplay to prevent FPS impact
		if (!m_Gamemode || m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
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
		if (!m_MenuManager.m_aPlayersTalking.Contains(playerId))
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
		if (playerIndex != -1)
		{
			// Remove player to update UI indicators
			m_MenuManager.m_aPlayersTalking.Remove(playerIndex);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Checks if direct speech from a player should be filtered in spectator mode
	// @param playerId - The ID of the player whose speech is being received
	// @return True if the speech should be blocked, false if it should be allowed
	//------------------------------------------------------------------------------------------------
	protected bool IsSpectatorDirectSpeechFiltered(int playerId)
	{
		// Only filter in spectator mode (non-game states)
		if (!m_Gamemode || m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			return false;
		
		// Don't filter if menu manager is not available
		if (!m_MenuManager)
			return false;
		
		// Get both players' channel assignments
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int localChannel = m_MenuManager.GetChannel(localPlayerId);
		int senderChannel = m_MenuManager.GetChannel(playerId);
		
		// Allow if either player is in the default channel (0) or unassigned
		// This ensures backward compatibility with existing behavior
		if (localChannel <= 1 || senderChannel <= 1)
			return false;
		
		// Filter (block) if players are in different custom channels
		// This isolates direct speech to only players in the same spectator channel
		return localChannel != senderChannel;
	}
};