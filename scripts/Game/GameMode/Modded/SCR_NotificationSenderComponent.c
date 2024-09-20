modded class SCR_NotificationSenderComponent
{
	void SetKillFeedTypeDeadLocal()
	{
		m_iKillFeedType = EKillFeedType.FULL;
		m_iReceiveKillFeedType = EKillFeedReceiveType.ALL;
	}
	
	void SetKillFeedTypeNoneLocal()
	{
		m_iKillFeedType = EKillFeedType.DISABLED;
	}
}