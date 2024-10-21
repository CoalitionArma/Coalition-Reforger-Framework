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
		SCR_PlayerControllerGroupComponent.Cast(SCR_PlayerController.Cast(owner).FindComponent(SCR_PlayerControllerGroupComponent)).GetOnGroupChanged().Insert(UpdateLocalPlayerGroup);
		GetGame().GetCallqueue().CallLater(WaitTillGameStart, 500, true);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void WaitTillGameStart()
	{
		if(!SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			return;
		
		GetGame().GetCallqueue().Remove(WaitTillGameStart);
		GetGame().GetCallqueue().CallLater(DelayUpdate, 650, false);
	}
	
	protected void DelayUpdate()
	{
		OnControlledEntityChanged(SCR_PlayerController.GetLocalMainEntity(), SCR_PlayerController.GetLocalMainEntity());
		UpdateLocalPlayerGroup(SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId()).GetGroupID())
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if(!to.GetPrefabData().GetPrefabName().Contains("CRF_GS_"))
			return;
		
		Rpc(RpcAsk_UpdatePlayerGearScriptMap, to.GetPrefabData().GetPrefabName(), SCR_PlayerController.GetLocalPlayerId(), "GSR"); // GSR = Gear Script Resource
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateLocalPlayerGroup(int groupId)
	{
		if(groupId <= 0)
			return;
		
		Rpc(RpcAsk_UpdatePlayerGearScriptMap, groupId.ToString(), SCR_PlayerController.GetLocalPlayerId(), "GID"); // GID = GROUP ID    
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_UpdatePlayerGearScriptMap(string value, int playerID, string key)
	{
		CRF_GearScriptGamemodeComponent.GetInstance().SetPlayerGearScriptsMapValue(value, playerID, key);
	}
};