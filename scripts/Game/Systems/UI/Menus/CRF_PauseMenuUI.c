class CRF_PauseMenuUI: PauseMenuUI
{
	
	//--------------------------------------------------------------------------------------------------
	// Overridden method that's called when the pause menu is opened
	//--------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		// Call the parent implementation first
		super.OnMenuOpen();
		
		// Get the admin menu button and frame
		SCR_ButtonTextComponent adminButton = SCR_ButtonTextComponent.GetButtonText("CoalAdminMenuButton", GetRootWidget());
		FrameWidget adminFrame = FrameWidget.Cast(GetRootWidget().FindAnyWidget("CoalAdminMenu"));
		
		// Validate widget references
		if (!adminButton || !adminFrame)
		{
			Print("CRF_PauseMenuUI: Could not find admin menu UI elements", LogLevel.WARNING);
			return;
		}
		
		// Check if the current player has admin permissions
		bool hasAdminAccess = false;
		CRF_GamemodeManager gamemodeManager = CRF_GamemodeManager.GetInstance();
		
		// Check admin/moderator permissions and required game instances
		if (SCR_Global.IsAdmin() || (gamemodeManager && gamemodeManager.IsModerator() && CRF_Gamemode.GetInstance()))
			hasAdminAccess = true;
		
		// Set visibility based on permissions
		adminFrame.SetVisible(hasAdminAccess);
		
		// Only register click handler if the admin menu is visible
		if (hasAdminAccess)
			adminButton.m_OnClicked.Insert(OpenAdminMenu);
	}
	
	//--------------------------------------------------------------------------------------------------
	// Opens the admin menu when the admin button is clicked
	//--------------------------------------------------------------------------------------------------
	void OpenAdminMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoalAdminMenu);
	}
}