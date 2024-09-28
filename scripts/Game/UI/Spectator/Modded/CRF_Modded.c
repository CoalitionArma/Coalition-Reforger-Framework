modded class PS_SpectatorMenu
{
	protected SCR_NotificationSenderComponent m_Sender;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		m_Sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		m_Sender.SetKillFeedTypeDeadLocal();
	}
}

modded class PS_GameModeCoop
{
	protected SCR_NotificationSenderComponent m_Sender;
	
	override void SwitchToSpawnedEntity(int playerId, PS_RespawnData respawnData, IEntity entity, int frameCounter)
	{
		super.SwitchToSpawnedEntity(playerId, respawnData, entity, frameCounter);
		m_Sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		m_Sender.SetKillFeedTypeNoneLocal();
	}
}