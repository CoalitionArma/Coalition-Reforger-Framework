modded class SCR_NotificationSenderComponent
{
	void SetKillFeedTypeDeadLocal()
	{
		m_iKillFeedType = EKillFeedType.FULL;
		m_iReceiveKillFeedType = EKillFeedReceiveType.ALL;
	}
}