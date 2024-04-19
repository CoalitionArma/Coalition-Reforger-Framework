[ComponentEditorProps(category: "Game Mode Component", description: "")]
class COA_GameModePlayerComponentClass: ScriptComponentClass
{
	
}

class COA_GameModePlayerComponent: ScriptComponent
{	
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
	
	void Owner_ToggleBombPlanted(string stiePlanted, bool togglePlanted)
	{	
		Rpc(RpcAsk_ToggleBombPlanted, stiePlanted, togglePlanted);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleBombPlanted(string stiePlanted, bool togglePlanted)
	{
		CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent)).ToggleBombPlanted(stiePlanted, togglePlanted);
	}
}