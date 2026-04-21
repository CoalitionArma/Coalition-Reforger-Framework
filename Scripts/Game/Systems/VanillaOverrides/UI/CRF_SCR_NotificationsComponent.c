modded class SCR_NotificationsComponent
{
	//----------------------------------------------------------------
	// Returns true only when killfeed should be suppressed for this admin:
	// - Must be an unlimited editor user (admin)
	// - Must NOT have zeus open (zeus-open admins see killfeed)
	// - Must have a controlled entity (spectating admins see killfeed)
	//----------------------------------------------------------------
	private static bool IsLocalPlayerUnlimitedEditor()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager || editorManager.IsLimited())
			return false;
		
		// Zeus is open - admin should see killfeed
		if (editorManager.IsOpened())
			return false;
		
		// Admin is spectating (no controlled entity) - should see killfeed
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || !pc.GetControlledEntity())
			return false;
		
		// Admin is alive and playing without zeus open - suppress killfeed
		return true;
	}
	
	//----------------------------------------------------------------
	// Allow killfeed through for zeus-open admins; block everything else
	//----------------------------------------------------------------
	override static bool SendLocalUnlimitedEditor(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		if (IsKillfeedNotification(notificationID))
			return super.SendLocalUnlimitedEditor(notificationID, position, param1, param2, param3, param4, param5, param6);
		return false;
	}
	
	//----------------------------------------------------------------
	// Override SendLocal to block killfeed for admins who are alive
	// and playing without zeus open. Normal players are unaffected -
	// their killfeed is already disabled by base game config and
	// IsLocalPlayerUnlimitedEditor() returns false for non-admins.
	//----------------------------------------------------------------
	override static bool SendLocal(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		if (IsKillfeedNotification(notificationID) && IsLocalPlayerUnlimitedEditor())
			return false;
		
		return super.SendLocal(notificationID, position, param1, param2, param3, param4, param5, param6);
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