// Prevents map markers from being deleted when a player disconnects
modded class SCR_MapMarkerManagerComponent
{
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		// Override the Override that would delete markers on disconnect
		return;
	}
}