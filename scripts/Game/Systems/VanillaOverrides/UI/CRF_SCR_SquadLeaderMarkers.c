modded class SCR_MapMarkerSquadLeader
{
	override void OnPlayerIdUpdate()
	{
		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;
		
		SetLocalVisible(true);
	}
}