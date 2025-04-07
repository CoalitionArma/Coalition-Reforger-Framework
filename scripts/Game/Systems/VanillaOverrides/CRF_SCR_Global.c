modded class SCR_Global
{
	//------------------------------------------------------------------------------------------------
	/*!
	Check if given player is an moderator.
	\param playerID ID of queried player
	\return True when player with given ID is an moderator.
	*/
	static bool IsModerator(int playerID)
	{
		return GetGame().GetPlayerManager().HasPlayerRole(playerID, EPlayerRole.COALITION_MODERATOR);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if local player is an moderator.
	\return True when local player is a moderator.
	*/
	static bool IsModerator()
	{
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		return GetGame().GetPlayerManager().HasPlayerRole(playerID, EPlayerRole.COALITION_MODERATOR);
	}
};