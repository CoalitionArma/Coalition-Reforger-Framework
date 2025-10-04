class CRF_SightArsenalAction: ScriptedUserAction
{
	CRF_GearscriptManager m_GearScriptManager;
	Faction m_PlayerFaction;
	ref CRF_GearScriptContainer m_GearScriptContainer
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_GearScriptManager = CRF_GearscriptManager.GetInstance();
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SightArsenal);
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if (m_PlayerFaction != SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()))
		{
			m_PlayerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
			m_GearScriptContainer = m_GearScriptManager.GetGearScriptSettings(m_PlayerFaction.GetFactionKey());
		}
		
		if (!m_GearScriptContainer.m_bEnableSightArsenal)
			return false;
		else	
			return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}