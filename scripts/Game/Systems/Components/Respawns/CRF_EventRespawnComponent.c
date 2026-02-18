class CRF_EventRespawnComponentClass: ScriptComponentClass {}
class CRF_EventRespawnComponent: ScriptComponent
{
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")})]
	string m_sEventPoleFaction;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		#ifdef WORKBENCH
		//Only do error checking in the workbench itself
		//For the love of god BI please TRY CATCH
		//For in game in the WB
		if (GetGame().GetWorld())
		{
			CRF_RespawnManager respawnMan = CRF_RespawnManager.GetInstance();
			if (!respawnMan)
				return;
			
			RegisterEventPole(respawnMan);
			return;
		}
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;
		WorldEditorAPI api = worldEditor.GetApi();		
		if (!api)
			return;
		IEntitySource entitySource = api.FindEntityByName("CRF_Lobby");
		if (!entitySource)
			return;
		
		CRF_Gamemode gamemode = CRF_Gamemode.Cast(api.SourceToEntity(entitySource));
		if (!gamemode)
			return;
		
		CRF_RespawnManager respawnMan = CRF_RespawnManager.Cast(gamemode.FindComponent(CRF_RespawnManager));
		if (!respawnMan)
			return;
		
		//Setting it for checking in the gamemode
		RegisterEventPole(respawnMan);
		#else
		//Code that actually runs in the dedicated enviroment
		if (!Replication.IsServer())
			return;
		
		CRF_RespawnManager respawnMan = CRF_RespawnManager.GetInstance();
		if (!respawnMan)
			return;
		
		RegisterEventPole(respawnMan);
		#endif
	}
	
	void RegisterEventPole(CRF_RespawnManager respawnMan)
	{
		switch (m_sEventPoleFaction)
		{
			case "BLUFOR": 
			respawnMan.m_BLUFOREventPole = GetOwner();
			break;
			
			case "OPFOR": 
			respawnMan.m_OPFOREventPole = GetOwner();
			break;
			
			case "INDFOR": 
			respawnMan.m_INDFOREventPole = GetOwner();
			break;
			
			case "CIV": 
			respawnMan.m_CIVEventPole = GetOwner();
			break;
		}
	}
}