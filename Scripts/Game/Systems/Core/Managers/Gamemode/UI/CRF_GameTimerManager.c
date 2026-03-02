class CRF_GameTimerManagerClass : ScriptComponentClass {}

class CRF_GameTimerManager : ScriptComponent
{		
	[RplProp()]
	protected string m_sServerWorldTime;
	
	[RplProp()]
	int m_iTimeMissionEnds;
	int m_iTimeSafeStartBegan;
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get the current server world time string
	* @return Formatted server time string
	*/
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set the server world time
	* @param input Time string to set
	*/
	void SetServerWorldTime(string input)
	{
		m_sServerWorldTime = input;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Update the server world time based on safestart time
	*/
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTime(SCR_FormatHelper.FormatTime(totalSeconds));
		
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	/**
	* Update mission end timer and handle expiration
	*/
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTime(SCR_FormatHelper.FormatTime(totalSeconds));

		if (totalSeconds == 0) {
			SetServerWorldTime("Mission Time Expired!");
		}

		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_GameTimerManager m_sInstance;
	void CRF_GameTimerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_GameTimerManager GetInstance()
	{
		return m_sInstance;
	}
}