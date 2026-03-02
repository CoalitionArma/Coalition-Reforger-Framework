class CRF_SpectatorCameraClass : SCR_ManualCameraClass
{
}

class CRF_SpectatorCamera : SCR_ManualCamera
{
	//------------------------------------------------------------------------------------------------
	void AttatchSpectatorToCamera(IEntity camera)
	{
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();

		// Skip initialization if conditions aren't met
		if (!GetGame().InPlayMode() || !CRF_Gamemode.GetInstance() || !playerEntity || !CRF_EntityHelper.IsSpectator(playerEntity))
			return;

		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(camera.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;

		EntitySlotInfo cameraPoint = slotComp.GetSlotByName("SpectatorEntityHolder");
		if (!cameraPoint)
			return;
		
		vector mat[4];
		camera.GetTransform(mat);
		
		playerEntity.SetOrigin(camera.GetOrigin());
		playerEntity.SetTransform(mat);
		cameraPoint.AttachEntity(playerEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Determine if camera control is disabled by menu
	 * Modified to allow camera control in spectator menu
	 * @return True if camera should be disabled, false otherwise
	 */
	override protected bool IsDisabledByMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		
		if (!menuManager)
			return false;

		if (menuManager.IsAnyDialogOpen())
			return true;

		MenuBase topMenu = menuManager.GetTopMenu();
		
		// Allow camera control in editor and spectator menus
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(CRF_SpectatorMenu));
	}
}