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
		// Check if local player has unlimited editor access and suppress killfeeds
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsLimited() && !editorManager.IsOpened())
		{
			// Suppress all killfeeds for unlimited editor users when NOT in editor
			return;
		}
		
		// Call parent implementation for normal players and admins in editor
		super.OnControllableDestroyed(instigatorContextData);
	}
	
	//----------------------------------------------------------------
	// Sets the kill feed to display full information
	// Configures the local player to receive all kill notifications
	//----------------------------------------------------------------
	void SetKillFeedTypeDeadLocal()
	{
		// Check if local player has unlimited editor access
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && !editorManager.IsLimited() && !editorManager.IsOpened())
		{
			// Keep killfeeds disabled for unlimited editor users when NOT in editor
			m_iKillFeedType = EKillFeedType.DISABLED;
			return;
		}
		
		// Set kill feed to show complete information for spectators and admins in editor
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