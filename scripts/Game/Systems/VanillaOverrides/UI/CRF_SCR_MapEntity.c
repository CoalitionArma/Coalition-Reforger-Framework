modded class SCR_MapEntity
{
	string m_sFactionKey = "";
	
	override void OpenMap(MapConfiguration config)
	{
		super.OpenMap(config);
		//Whenver a player joins a new faction they can see other factions markers.
		//When you close the map and open again it seems to then actually load the right markers instead of all.
		//This just gives that effect of closing the map and opening it again.
		if (SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()))
			if (m_sFactionKey != SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey() && !CRF_GamemodeManager.IsSpectator(SCR_PlayerController.GetLocalControlledEntity()))
			{
				m_sFactionKey = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey();
				CloseMap();
				SCR_GadgetManagerComponent gadgetMgr = SCR_GadgetManagerComponent.GetGadgetManager(GetGame().GetPlayerController().GetControlledEntity());
				if (!gadgetMgr)
					return;
				
				IEntity mapGadget = gadgetMgr.GetGadgetByType(EGadgetType.MAP);
				if (!mapGadget)
					return;
				
				SCR_MapGadgetComponent mapComp = SCR_MapGadgetComponent.Cast(mapGadget.FindComponent(SCR_MapGadgetComponent));
				GetGame().GetCallqueue().CallLater(gadgetMgr.SetGadgetMode, 100, false, mapComp.GetOwner(), EGadgetMode.IN_STORAGE, false);
				GetGame().GetCallqueue().CallLater(gadgetMgr.SetGadgetMode, 600, false, mapComp.GetOwner(), EGadgetMode.IN_HAND, true);
				return;
			}
	}

}