class CRF_LibraryClass : ScriptComponentClass {}

class CRF_Library : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	/*!
	Check if given player is an moderator.
	\param playerId ID of queried player
	\return True when player with given ID is an moderator.
	*/
	static bool IsModerator(int playerId)
	{
		//return GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if local player is an moderator.
	\return True when local player is a moderator.
	*/
	static bool IsModerator()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		//return GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
		return false;
	}
};