modded class PS_SpectatorMenu
{
	protected SCR_NotificationSenderComponent m_Sender;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		m_Sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		m_Sender.SetKillFeedTypeDeadLocal();
	}
	
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		m_Sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		m_Sender.SetKillFeedTypeNoneLocal();
	}
}