modded class SCR_NotificationSenderComponent
{
	//----------------------------------------------------------------
	// Dont send notif that someone became a GM
	//----------------------------------------------------------------
	override protected void OnEditorLimitedChanged(bool isLimited)
	{
		
	}
	
	//----------------------------------------------------------------
	// Sets the kill feed to display full information
	// Configures the local player to receive all kill notifications
	//----------------------------------------------------------------
	void SetKillFeedTypeDeadLocal()
	{
		// Set kill feed to show complete information
		m_iKillFeedType = EKillFeedType.FULL;
		
		// Configure to receive all types of kill feed notifications
		m_iReceiveKillFeedType = EKillFeedReceiveType.ALL;
	}
	
	//----------------------------------------------------------------
	// Disables all kill feed notifications for the local player
	// Used when kill feed display should be turned off completely
	//----------------------------------------------------------------
	void SetKillFeedTypeNoneLocal()
	{
		// Disable kill feed display entirely
		m_iKillFeedType = EKillFeedType.DISABLED;
	}
}