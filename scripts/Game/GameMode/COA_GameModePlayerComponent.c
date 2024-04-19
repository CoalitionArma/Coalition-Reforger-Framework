[ComponentEditorProps(category: "Game Mode Component", description: "")]
class COA_GameModePlayerComponentClass: ScriptComponentClass
{
	
}

class COA_GameModePlayerComponent: ScriptComponent
{
	CRF_GameMode_SearchAndDestroyComponent gameMode = CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent));
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static COA_GameModePlayerComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return COA_GameModePlayerComponent.Cast(GetGame().GetPlayerController().FindComponent(COA_GameModePlayerComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------

	// Functions for ready up replication
	
	//------------------------------------------------------------------------------------------------
	
	void Owner_ToggleBombPlanted(EntityID entityID, bool togglePlanted)
	{	
		Rpc(RpcAsk_ToggleBombPlanted, entityID, togglePlanted);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleBombPlanted(EntityID entityID, bool togglePlanted)
	{
		if (!togglePlanted) {
			GetGame().GetCallqueue().Remove(gameMode.startCountdown);
			gameMode.countDownActive = false;
			if (gameMode.aSitePlanted)
				gameMode.aSitePlanted = false;
			else	
				gameMode.bSitePlanted = false;
			return;
		};
		
		// Set which site is planted
		gameMode.countDownActive = true;
		if (!gameMode.aSite.IsDeleted() && entityID == gameMode.aSite.GetID())
			gameMode.aSitePlanted = true;
		else	
			gameMode.bSitePlanted = true;
		
		// Spawn countdown thread
		GetGame().GetCallqueue().CallLater(gameMode.startCountdown, 1000, true, GetGame().GetWorld().FindEntityByID(entityID));
	}
}