class CRF_PermissionManagerClass : ScriptComponentClass {}

class CRF_PermissionManager : ScriptComponent
{	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	ref array<int> m_aDonators = {};
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if a given player is a moderator
	* @param playerId ID of the player to check
	* @return True if player is a moderator, false otherwise
	*/
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a moderator
	* @return True if local player is a moderator, false otherwise
	*/
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a donator
	* @param playerId ID of the player to check
	* @return True if local player is a donator, false otherwise
	*/
	bool IsDonator(int playerId)
	{
		return m_aDonators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a donator
	* @return True if local player is a donator, false otherwise
	*/
	bool IsDonator()
	{
		return m_aDonators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set a player status
	* @param playerId ID of the player to set as moderator or donator
	*/
	void SetPlayerStatus(int playerId, string role)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
			return;
		
		bool statusChanged = false;
		switch (role) {
			case "mod": {
				m_aModerators.Insert(playerId);
				statusChanged = true;
				break;
			}
			case "don": {
				m_aDonators.Insert(playerId);
				statusChanged = true;
				break;
			}
		}
			
		if (statusChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	static protected CRF_PermissionManager m_sInstance;
	void CRF_PermissionManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_PermissionManager GetInstance()
	{
		return m_sInstance;
	}
}