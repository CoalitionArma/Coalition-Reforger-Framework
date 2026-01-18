//Disableds the night overlay for spectators
modded class SCR_MapLightUI
{
	
	override protected void UpdateTime()
	{
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionMan)
		{
			Faction playerFaction = factionMan.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
			if (playerFaction)
			{
				if (playerFaction.GetFactionKey() == "SPEC")
				{
					m_wLightCone.SetVisible(false);
					m_wLightOverlay.SetVisible(false);
					return;
				}
			}
		}
		super.UpdateTime();
	}
}