class CRF_PlayerMenuManagerClass : ScriptComponentClass {}

class CRF_PlayerMenuManager : ScriptComponent
{		
	//------------------------------------------------------------------------------------------------
	/**
	 * Opens appropriate menu based on current gamemode state
	 */
	void OpenCurrentStateMenu()
	{	
		// Initialize references first
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		// Check if we should skip AAR
		if (gamemode && gamemode.m_GamemodeState == CRF_EGamemodeState.AAR)
			return;
		
		// Close any existing menus
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open appropriate menu based on gamemode state
		switch (gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
				break;
			}
			case CRF_EGamemodeState.SLOTTING:
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
				break;
			}
			case CRF_EGamemodeState.GAME: 
			{
				CRF_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
				break;
			}
			case CRF_EGamemodeState.AAR: 
			{
				break;
			}
		}
	}	

	//------------------------------------------------------------------------------------------------
	/**
	 * Opens the slotting menu for player assignment
	 */
	void OpenSlottingMenu()
	{
		// Check if appropriate menu is already open
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
		{
			if (topMenu.IsInherited(CRF_PreviewMenu) || topMenu.IsInherited(CRF_SlottingMenu))
				return;
			else if (topMenu.IsInherited(CRF_SpectatorMenu))
				GetGame().GetMenuManager().CloseMenu(topMenu);
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
	}	

	//------------------------------------------------------------------------------------------------
	void DisplayTitleCard()
	{
		Widget titleCard = GetGame().GetWorkspace().CreateWidgets("{4D2AE199F111C14A}UI/layouts/HUD/Intro/CRF_Intro.layout");
		TextWidget.Cast(titleCard.FindAnyWidget("TitleText")).SetText(CRF_MissionHelper.SanitizeMissionName(GetGame().GetMissionName()));
		AudioSystem.PlaySound("{932C08A5A988F96A}Sounds/Intro/cinematicBoom.wav");
		GetGame().GetCallqueue().CallLater(RemoveTitleCardWidget, 4000, false, titleCard);
	}
	
	//------------------------------------------------------------------------------------------------
	static void RemoveTitleCardWidget(Widget widget)
	{
		if (widget)
			widget.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerMenuManager m_sInstance;
	void CRF_PlayerMenuManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PlayerMenuManager GetInstance()
	{
		return m_sInstance;
	}
}