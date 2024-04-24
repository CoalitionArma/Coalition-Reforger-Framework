[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GameModePlayerComponentClass: ScriptComponentClass
{
	
}

class CRF_GameModePlayerComponent: ScriptComponent
{	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static CRF_GameModePlayerComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_GameModePlayerComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_GameModePlayerComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------

	// Functions for ready up replication
	
	//------------------------------------------------------------------------------------------------
	
	void Owner_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{	
		Rpc(RpcAsk_ToggleBombPlanted, sitePlanted, togglePlanted);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		CRF_SearchAndDestroyGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGameModeComponent)).ToggleBombPlanted(sitePlanted, togglePlanted);
	}
}