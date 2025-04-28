modded class SCR_NotificationsComponent
{
	//----------------------------------------------------------------
	// Disable killfeed on game masters too
	//----------------------------------------------------------------
	override static bool SendLocalUnlimitedEditor(ENotification notificationID, vector position, int param1 = 0, int param2 = 0, int param3 = 0, int param4 = 0, int param5 = 0, int param6 = 0)
	{
		return false;
	}
}