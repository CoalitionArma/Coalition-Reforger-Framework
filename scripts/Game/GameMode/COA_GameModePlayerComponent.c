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
	
	void Owner_ToggleBombPlanted(EntityID entityID, bool togglePlanted)
	{	
		Rpc(RpcAsk_ToggleBombPlanted, entityID, togglePlanted);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleBombPlanted(EntityID entityID, bool togglePlanted)
	{
		CRF_GameMode_SearchAndDestroyComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GameMode_SearchAndDestroyComponent)).ToggleBombPlanted(entityID, togglePlanted);
	}
}