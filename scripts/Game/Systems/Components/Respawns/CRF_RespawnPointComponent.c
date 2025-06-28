class CRF_RespawnPointComponentClass: ScriptComponentClass {}
class CRF_RespawnPointComponent: ScriptComponent
{
	[Attribute("true", UIWidgets.CheckBox,""),RplProp(onRplName: "OnRespawnPointStateChanged")]
	bool m_bActiveRespawnPoint;
	
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")})]
	string m_sRespawnPointFaction;

	[Attribute("Base", "auto", "Nickname for the respawn point.")]
	string m_sRespawnPointName;

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);	
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Client) return;
		
		if(CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().RegisterRespawnPoint(owner);
	};
	
	override void OnDelete(IEntity owner)
	{		
		super.OnDelete(owner);
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Client) return;
		
		if (CRF_RespawnManager.GetInstance())
			CRF_RespawnManager.GetInstance().UnRegisterRespawnPoint(owner);
	};
	
	void SetRespawnActiveState(bool active)
	{
		m_bActiveRespawnPoint = active;
		Replication.BumpMe();
	}
	
	void OnRespawnPointStateChanged()
	{
		IEntity respawnPoint = GetOwner();
		
		RplComponent rplComp = RplComponent.Cast(respawnPoint.FindComponent(RplComponent));
		if (!rplComp)
			return;
		
		// Broadcast UI updates to clients
		CRF_RplBroadcastManager.GetInstance().SendRespawnScreenUpdate(rplComp.Id(), true);
	}
};