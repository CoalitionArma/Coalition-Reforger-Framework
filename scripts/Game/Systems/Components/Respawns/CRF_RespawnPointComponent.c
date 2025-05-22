class CRF_RespawnPointComponentClass: ScriptComponentClass {}

class CRF_RespawnPointComponent: ScriptComponent
{
	[Attribute("true", "auto", "")]
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
	
	string GetRespawnNickname()
	{
		return m_sRespawnPointName;
	}
	
	void SetRespawnActiveState(bool active)
	{
		m_bActiveRespawnPoint = active;
	}
};