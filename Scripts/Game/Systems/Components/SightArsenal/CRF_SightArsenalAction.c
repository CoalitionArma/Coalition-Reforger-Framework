class CRF_SightArsenalAction: ScriptedUserAction
{
	CRF_Gamemode m_Gamemode;
	CRF_SafestartManager m_SafeStartManager;
	Faction m_PlayerFaction;
	ref CRF_GearScriptContainer m_GearScriptContainer
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_SafeStartManager = CRF_SafestartManager.GetInstance();
		m_Gamemode = CRF_Gamemode.GetInstance();
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
			m_GearScriptContainer = m_Gamemode.GetGearScriptSettings(m_PlayerFaction.GetFactionKey());
		}
		
		if ((!m_GearScriptContainer.m_bEnableSightArsenal ||!m_SafeStartManager.GetSafestartStatus()) && CRF_PlayerController.IsGracePeriodOver())
			return false;
		else	
			return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}