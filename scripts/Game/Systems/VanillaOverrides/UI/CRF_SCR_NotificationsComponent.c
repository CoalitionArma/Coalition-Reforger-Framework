modded class SCR_NotificationsComponent
{
	//----------------------------------------------------------------
	// Check if current player has unlimited editor access
	//----------------------------------------------------------------
	private static bool IsLocalPlayerUnlimitedEditor()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager)
			return false;
			
		return !editorManager.IsLimited() && !editorManager.IsOpened();
	}
	
	//----------------------------------------------------------------
	// Disable killfeeds for unlimited editor users
	//----------------------------------------------------------------
	override static bool SendLocalUnlimitedEditor(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		return false;
	}
	
	//----------------------------------------------------------------
	// Override SendLocal to block killfeeds for unlimited editor users
	//----------------------------------------------------------------
	override static bool SendLocal(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		// Block killfeed notifications for unlimited editor users
		if (IsKillfeedNotification(notificationID) && IsLocalPlayerUnlimitedEditor())
			return false;
			
		return super.SendLocal(notificationID, position, param1, param2, param3, param4, param5, param6);
	}
	
	//----------------------------------------------------------------
	// Override SendLocalLimitedEditor to block killfeeds for unlimited editor users  
	//----------------------------------------------------------------
	override static bool SendLocalLimitedEditor(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		// Block killfeed notifications for unlimited editor users
		if (IsKillfeedNotification(notificationID) && IsLocalPlayerUnlimitedEditor())
			return false;
			
		return super.SendLocalLimitedEditor(notificationID, position, param1, param2, param3, param4, param5, param6);
	}
	
	//----------------------------------------------------------------
	// Helper method to identify killfeed-related notifications
	//----------------------------------------------------------------
	private static bool IsKillfeedNotification(ENotification notificationID)
	{
		// Check for all killfeed-related notification types
		if (notificationID == ENotification.PLAYER_DIED ||
			notificationID == ENotification.PLAYER_KILLED_PLAYER ||
			notificationID == ENotification.AI_KILLED_PLAYER ||
			notificationID == ENotification.POSSESSED_AI_DIED ||
			notificationID == ENotification.POSSESSED_AI_KILLED_PLAYER ||
			notificationID == ENotification.POSSESSED_AI_KILLED_POSSESSED_AI ||
			notificationID == ENotification.AI_KILLED_POSSESSED_AI)
			return true;
		else
			return false;
	}
}