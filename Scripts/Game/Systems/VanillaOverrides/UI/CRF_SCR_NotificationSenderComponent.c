modded class SCR_NotificationSenderComponent
{
	//----------------------------------------------------------------
	// Don't send notification that someone became a GM
	//----------------------------------------------------------------
	override protected void OnEditorLimitedChanged(bool isLimited)
	{
		// Suppress the notification about editor state changes
	}
	
	//----------------------------------------------------------------
	// Override the main method that handles killfeed logic
	//----------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		// Suppress killfeed only when admin is alive/playing without zeus open
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsLimited() && !editorManager.IsOpened())
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && pc.GetControlledEntity())
				return;
		}
		
		super.OnControllableDestroyed(instigatorContextData);
	}
	
	//----------------------------------------------------------------
	// Sets the kill feed to display full information
	// Configures the local player to receive all kill notifications
	//----------------------------------------------------------------
	void SetKillFeedTypeDeadLocal()
	{
		// Suppress killfeed only when admin is alive/playing without zeus open
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsLimited() && !editorManager.IsOpened())
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && pc.GetControlledEntity())
			{
				m_iKillFeedType = EKillFeedType.DISABLED;
				return;
			}
		}
		
		// Spectating, zeus open, or normal player - show full killfeed
		m_iKillFeedType = EKillFeedType.FULL;
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