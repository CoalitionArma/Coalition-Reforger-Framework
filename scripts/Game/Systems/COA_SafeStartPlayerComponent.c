[ComponentEditorProps(category: "Safe Start Component", description: "")]
class COA_SafeStartPlayerComponentClass: ScriptComponentClass
{
	
}

class COA_SafeStartPlayerComponent: ScriptComponent
{
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static COA_SafeStartPlayerComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return COA_SafeStartPlayerComponent.Cast(GetGame().GetPlayerController().FindComponent(COA_SafeStartPlayerComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner) {
		super.EOnInit(owner);
		
		GetGame().GetInputManager().AddActionListener("ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
	};
	
	//------------------------------------------------------------------------------------------------

	// Functions for ready up replication
	
	//------------------------------------------------------------------------------------------------
	
	void ToggleSideReady() 
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if(!groupManager) return;
		
		SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		if(!playersGroup) return;
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
		
		if (!playerName || playerName == "") return;
		
		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId())) 
			Owner_ToggleSideReady(playerName);
	}
	
	//------------------------------------------------------------------------------------------------
	void Owner_ToggleSideReady(string playerName)
	{	
		string setReady = "";
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
			
		Color factionColor = faction.GetFactionColor();
		float rg = Math.Max(factionColor.R(), factionColor.G());
		float rgb = Math.Max(rg, factionColor.B());
			
		switch (true) {
			case(rgb == factionColor.R()) : {setReady = "Opfor";  break;};
			case(rgb == factionColor.G()) : {setReady = "Indfor"; break;};
			case(rgb == factionColor.B()) : {setReady = "Blufor"; break;};
		};
		
		if (setReady == "") return;
		Rpc(RpcAsk_ToggleSideReady, setReady, playerName);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleSideReady(string setReady, string playerName)
	{
		CRF_SafestartGameModeComponent safestartComponent = CRF_SafestartGameModeComponent.GetInstance();
		safestartComponent.ToggleSideReady(setReady, playerName);
	}
}