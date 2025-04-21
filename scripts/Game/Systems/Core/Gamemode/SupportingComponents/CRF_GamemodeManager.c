class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	Widget m_wSavedHintWidget;
	
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GamemodeManager.Cast(gameMode.FindComponent(CRF_GamemodeManager));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	// Moderator Functions/Variables
	//------------------------------------------------------------------------------------------------
	override void OnPlayerAuditSuccess(int playerId)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_Gamemode.GetInstance().InitilizePlayer(playerId);
		
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		
		if (!playerIdentity.IsEmpty() && CRF_ModeratorConfig.IsModerator(playerIdentity))
			GetGame().GetCallqueue().CallLater(SetPlayerModerator, 5000, false, playerId);
	};
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		//GetGame().GetPlayerManager().GivePlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	};
}
