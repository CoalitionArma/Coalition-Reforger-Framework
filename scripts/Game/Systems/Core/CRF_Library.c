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
		return CRF_GamemodeManager.GetInstance().m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if local player is an moderator.
	\return True when local player is a moderator.
	*/
	static bool IsModerator()
	{
		return CRF_GamemodeManager.GetInstance().m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
};