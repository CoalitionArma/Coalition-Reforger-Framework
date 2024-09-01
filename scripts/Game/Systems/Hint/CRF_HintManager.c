[ComponentEditorProps(category: "GameScripted/GameMode/Components", description: "", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_HintManagerClass: ScriptComponentClass
{
	
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class CRF_HintManager : ScriptComponent
{
	VerticalLayoutWidget m_wBigMsgList;
	
	static CRF_HintManager GetInstance() 
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_HintManager.Cast(gameMode.FindComponent(CRF_HintManager));
		else
			return null;
	}
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddMsgAction, 0, false);
	}
	
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("hint");
		invoker.Insert(SendHint_Callback);
	}
	
	void SendHint_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "") return;
		Print("CRF SendHint_Callback: " + data);
		Rpc(RPC_SendHint, data)
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SendHint(string data)
	{
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout", m_wBigMsgList);
		
		if (!widget)
			return;
		
		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		float duration = 10;
		Print("CRF RPC_SendHint: " + data);
		hint.ShowHint(data, duration);
	}
}