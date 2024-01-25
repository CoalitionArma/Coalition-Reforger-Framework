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

	// Functions for ready up replication
	
	//------------------------------------------------------------------------------------------------
	
	void Owner_ToggleSideReady()
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
		Rpc(RpcAsk_ToggleSideReady, setReady);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleSideReady(string setReady)
	{
		CRF_TNK_SafestartComponent safestartComponent = CRF_TNK_SafestartComponent.GetInstance();
		safestartComponent.ToggleSideReady(setReady);
	}
}