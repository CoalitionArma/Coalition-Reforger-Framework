class CRF_ReplicationComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_ReplicationComponent : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	static CRF_ReplicationComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_ReplicationComponent.Cast(gameMode.FindComponent(CRF_ReplicationComponent));
		else
			return null;
	}
	
	
};