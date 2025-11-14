class CRF_RespawnPointComponentClass: ScriptComponentClass {}
class CRF_RespawnPointComponent: ScriptComponent
{
	[Attribute("true", UIWidgets.CheckBox,""),RplProp(onRplName: "OnRespawnPointStateChanged")]
	bool m_bActiveRespawnPoint;
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")})]
	string m_sRespawnPointFaction;

	[Attribute("Base", "auto", "Nickname for the respawn point.")]
	string m_sRespawnPointName;
	
	[Attribute("0", "auto", "Is this respawn point the default respawn point to be selected")]
	bool m_bIsDefaultRespawn;

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only server should register respawn points
		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;
		
		if(CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().RegisterRespawnPoint(owner);
	};
	
	override void OnDelete(IEntity owner)
	{		
		super.OnDelete(owner);
		
		// Only server should unregister respawn points
		if (!Replication.IsServer())
			return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(owner);
	};
	
	void SetRespawnActiveState(bool active)
	{
		// Only authority should modify replicated state
		if (!Replication.IsServer())
			return;
			
		m_bActiveRespawnPoint = active;
		Replication.BumpMe();
	}
	
	void OnRespawnPointStateChanged()
	{
		// Only server should broadcast updates when replicated value changes
		// This callback is invoked on proxies when they receive updates,
		// but we only want the authority to send the broadcast
		if (!Replication.IsServer())
			return;
			
		IEntity respawnPoint = GetOwner();
		
		RplComponent rplComp = RplComponent.Cast(respawnPoint.FindComponent(RplComponent));
		if (!rplComp)
			return;
		
		// Broadcast UI updates to clients
		CRF_RplBroadcastManager.GetInstance().SendRespawnScreenUpdate(rplComp.Id(), true);
	}
};