class CRF_SightArsenalAction: ScriptedUserAction
{
	CRF_GearscriptManager m_GearScriptManager;
	CRF_SafestartManager m_SafeStartManager;
	Faction m_PlayerFaction;
	ref CRF_GearScriptContainer m_GearScriptContainer
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_SafeStartManager = CRF_SafestartManager.GetInstance();
		m_GearScriptManager = CRF_GearscriptManager.GetInstance();
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CRF_SightArsenal sightMenu = CRF_SightArsenal.Cast(GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SightArsenal));
		sightMenu.m_Sight = pOwnerEntity;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if (m_PlayerFaction != SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()))
		{
			m_PlayerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
			m_GearScriptContainer = m_GearScriptManager.GetGearScriptSettings(m_PlayerFaction.GetFactionKey());
		}
		
		if (!m_GearScriptContainer.m_bEnableSightArsenal ||!m_SafeStartManager.GetSafestartStatus())
			return false;
		else	
			return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}