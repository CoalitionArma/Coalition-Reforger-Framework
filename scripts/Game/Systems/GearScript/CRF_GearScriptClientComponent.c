[ComponentEditorProps(category: "Client Component", description: "")]
class CRF_GearScriptClientComponentClass: ScriptComponentClass {}

class CRF_GearScriptClientComponent: ScriptComponent
{	
	static CRF_GearScriptClientComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_GearScriptClientComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_GearScriptClientComponent));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated) 
			return;

		SCR_PlayerController.Cast(owner).m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		Rpc(RpcAsk_UpdatePlayerGearScriptMap, to.GetPrefabData().GetPrefabName(), SCR_PlayerController.GetLocalPlayerId(), "GSR"); // GSR = Gear Script Resource
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdatePlayerGearScriptMap(string value, int playerID, string key)
	{
		CRF_GearScriptGamemodeComponent.GetInstance().SetPlayerGearScriptsMapValue(value, playerID, key);
	}
};